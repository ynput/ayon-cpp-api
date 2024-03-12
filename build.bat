@echo off
rmdir /s /q build
rmdir /s /q bin
cmake -S . -B build -DBUILD_TEST="ON" -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build
