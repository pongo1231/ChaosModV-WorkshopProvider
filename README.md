# Chaos Mod Workshop Provider

The backend for the workshop functionality in the [Chaos Mod](https://github.com/gta-chaos-mod/ChaosModV) for Grand Theft Auto V.

This only runs on Linux. Windows support is not easily possible due to the libraries used.

## Building

The following external dependencies are required:

- zstd
- sqlite3
- ICU

Otherwise it's just the standard CMake affair:

```
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

Make sure to also copy the content of the `dist/` folder into your build folder.
