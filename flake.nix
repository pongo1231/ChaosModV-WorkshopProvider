{
  description = "Chaos Mod V Workshop Provider implementation";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        libhttpserver = pkgs.stdenv.mkDerivation
          rec {
            name = "libhttpserver";
            version = "0.19.0";
            src = pkgs.fetchFromGitHub {
              owner = "etr";
              repo = name;
              rev = version;
              sha256 = "sha256-Pc3Fvd8D4Ymp7dG9YgU58mDceOqNfhWE1JtnpVaNx/Y=";
            };

            nativeBuildInputs = with pkgs; [ autoconf automake libtool ];

            buildInputs = with pkgs; [ gnutls libmicrohttpd sqlitecpp ];

            patchPhase = ''
              substituteInPlace configure.ac --replace "/bin/pwd" "${pkgs.coreutils}/bin/pwd"
            '';

            configurePhase = ''
              aclocal -I m4
              autoheader
              if [[ "$OSTYPE" == "darwin"* ]]; then
                glibtoolize --automake
              else
                libtoolize --automake
              fi
              automake --add-missing
              autoconf

              mkdir build
              cd build
              ../configure --prefix=$out
            '';
          };
        dpp = pkgs.stdenv.mkDerivation
          rec {
            name = "DPP";
            version = "10.0.26";
            src = pkgs.fetchFromGitHub {
              owner = "brainboxdotcc";
              repo = name;
              rev = "v${version}";
              sha256 = "sha256-o78ijctDqrONyi8A3+FXvnK9q97B4j1ZIQYNUgl6XJU=";
            };

            nativeBuildInputs = [ pkgs.cmake ];

            buildInputs = with pkgs; [ zlib openssl ];
          };
        elzip = pkgs.stdenv.mkDerivation
          rec {
            name = "elzip";
            version = "516e161d5c96aa8f2603fb30b10b7770a87332c2";
            src = pkgs.fetchFromGitHub {
              owner = "Sygmei";
              repo = "11Zip";
              rev = version;
              sha256 = "sha256-F4/ZI+mNWCyCfTjcr6AohlE/mJjYcblMZIkCp2hB6yY=";
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
        commonBuildInputs = with pkgs; [ openssl zstd sqlite icu sqlitecpp libmicrohttpd libhttpserver dpp elzip ];
      in
      {
        packages = {
          chaosmod-workshopprovider = pkgs.stdenv.mkDerivation {
            name = "chaosworkshop";
            version = "1.0";

            src = nixpkgs.lib.cleanSource ./.;

            nativeBuildInputs = [ pkgs.cmake ];

            cmakeFlags = [ "-DUSE_SYSTEM_LIBS=1" ];

            buildInputs = with pkgs; [ openssl zstd sqlite icu sqlitecpp libmicrohttpd libhttpserver dpp elzip ];

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

            buildInputs = commonBuildInputs ++ [ pkgs.nlohmann_json ];
          };
        };
      });
}
