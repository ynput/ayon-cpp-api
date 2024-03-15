# AYON CPP API

An API Wrapper for [AYON server](https://ayon.ynput.io/) written in cpp

## Requirements: 
- C++ Compiler
- Cmake

## Tested Platforms: 
- AlmaLinux9
- Ubunto 22.04.3LTS
- Windows 11

## Build instructions
Coling the repository:

```sh
git clone --recurse-submodules https://github.com/ynput/ayon-cpp-api.git
git submodule update --init --recursive
```

### Using Script

**Linux**
```sh
./build_scripts/build.sh 
```

**Windows**

Run it from your Developer console for Visual Studio
```sh
.\build_scripts\build.bat
```

### Manual
```
cmake -S . -B build -DBUILD_TEST="OFF" -DJTRACE=0 DCMAKE_BUILD_TYPE=Release
```

## Environment Varibles. 
The `AyonLogger` can be controlled with environment variables:

| variable | value |
| -------- | ----- |
| `AYONLOGGERLOGLVL` | `INFO` `ERROR` `WARN` `CRITICAL` `OFF` |
| `AYONLOGGERFILELOGGING` | `OFF` / `ON` |
| `AYONLOGGERFILEPOS` | `/path/to or ./relative/path` |
