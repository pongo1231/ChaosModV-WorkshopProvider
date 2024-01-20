{
  description = "Chaos Mod V Workshop Provider implementation";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      readData = file:
        let
          data = nixpkgs.lib.splitString "\n" (builtins.readFile file);
        in
        {
          version = builtins.elemAt data 0;
          hash = builtins.elemAt data 1;
        };
    in
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        libhttpserver =
          let
            data = readData ./vendor/libhttpserver.txt;
          in
          pkgs.stdenv.mkDerivation (finalAttrs: {
            pname = "libhttpserver";
            version = data.version;
            src = pkgs.fetchFromGitHub {
              owner = "etr";
              repo = finalAttrs.pname;
              rev = data.version;
              hash = data.hash;
            };

            nativeBuildInputs = with pkgs; [ autoconf automake libtool ];

            buildInputs = with pkgs; [ gnutls libmicrohttpd ];

            enableParallelBuilding = true;

            patchPhase = ''
              patchShebangs ./bootstrap
            '';

            configurePhase = ''
              ./bootstrap
              ./configure --prefix=$out --enable-same-directory-build
            '';
          });
        dpp =
          let
            data = readData ./vendor/dpp.txt;
          in
          pkgs.stdenv.mkDerivation (finalAttrs: {
            pname = "DPP";
            version = builtins.substring 0 6 data.version;
            src = pkgs.fetchFromGitHub {
              owner = "brainboxdotcc";
              repo = finalAttrs.pname;
              rev = data.version;
              hash = data.hash;
            };

            nativeBuildInputs = [ pkgs.cmake ];

            buildInputs = with pkgs; [ zlib openssl ];
          });
        elzip =
          let
            data = readData ./vendor/elzip.txt;
          in
          pkgs.stdenv.mkDerivation {
            pname = "elzip";
            version = builtins.substring 0 6 data.version;
            src = pkgs.fetchFromGitHub {
              owner = "Sygmei";
              repo = "11Zip";
              rev = data.version;
              hash = data.hash;
            };

            nativeBuildInputs = [ pkgs.cmake ];

            cmakeFlags = [ "-DELZIP_EXCLUDE_MINIZIP=1" ];

            buildInputs = [ pkgs.minizip-ng ];

            patchPhase = ''
              substituteInPlace CMakeLists.txt --replace "add_library(elzip" "add_library(elzip SHARED"
              substituteInPlace CMakeLists.txt --replace "target_link_libraries(elzip minizip)" "target_link_libraries(elzip minizip-ng)"

              substituteInPlace src/elzip.cpp --replace "minizip/" "minizip-ng/"
              substituteInPlace src/unzipper.cpp --replace "minizip/" "minizip-ng/"
              substituteInPlace src/zipper.cpp --replace "minizip/" "minizip-ng/"

              substituteInPlace include/elzip/elzip.hpp --replace "#include <fswrapper.hpp>" "#include \"fswrapper.hpp\""
              substituteInPlace include/elzip/unzipper.hpp --replace "minizip/" "minizip-ng/"
              substituteInPlace include/elzip/zipper.hpp --replace "minizip/" "minizip-ng/"
            '';

            installPhase = ''
              mkdir -p $out/lib/
              cp -r libelzip.so $out/lib/

              cp -r ../include $out/
            '';
          };
        buildInputs = with pkgs; [ openssl zstd sqlite icu sqlitecpp libmicrohttpd libhttpserver dpp elzip ];
      in
      {
        packages = {
          chaosmod-workshopprovider = pkgs.stdenv.mkDerivation {
            pname = "chaosworkshop";
            version = "1.0";

            src = nixpkgs.lib.cleanSource ./.;

            nativeBuildInputs = with pkgs; [ cmake ];

            inherit buildInputs;

            cmakeFlags = [ "-DUSE_SYSTEM_LIBS=1" ];

            installPhase = ''
              mkdir -p $out/bin/
              cp -r chaosworkshop $out/bin/
            '';
          };

          default = self.packages.x86_64-linux.chaosmod-workshopprovider;
        };

        devShells = {
          default = pkgs.mkShell {
            packages = with pkgs; [ cmake ninja gcc pkg-config autoconf automake libtool ];

            inherit buildInputs;
          };
        };
      });
}
