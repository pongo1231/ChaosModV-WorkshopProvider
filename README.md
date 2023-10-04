# Chaos Mod Workshop Provider

The backend for the workshop functionality in the [Chaos Mod](https://github.com/gta-chaos-mod/ChaosModV) for Grand Theft Auto V.

This only runs on Linux. Windows support is not easily possible due to the libraries used.

## Building

### Nix
This project provides a flake.nix for dropping into a dev shell with all required dependencies installed. Make sure both the `nix-command` and `flakes` experimental features are enabled.

Run `nix develop` to drop into the dev shell. `nix build` or `nix run github:pongo1231/ChaosModV-WorkshopProvider` works for building the project as Nix derivation, however currently it will try to write data files in the binary's path (which will not work in the result dir, located inside the immutable Nix store).

See the build instructions below and make sure to add `-DUSE_SYSTEM_LIBS=1` to the cmake build command.

### Manually
The following external dependencies are required:

- cmake
- ninja
- gcc or clang
- pkg-config
- autoconf
- automake
- libtool
- zstd
- sqlite3
- ICU (> 61.0)

Once installed, it's just the standard CMake affair:

```
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

Make sure to also copy the content of the `dist/` folder into your build folder and to set the `domain` option in the `data/options.json` file correspondingly.

Currently you will also need a `cert.pem` and `key.pem` in the same directory as the binary (from e.g. letsencrypt).