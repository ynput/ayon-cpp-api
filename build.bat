cmake -S . -B build -DBUILD_TEST="OFF" -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build