#include <array>
#include <iostream>
#include <string>
#include <vector>
#include "AyonCppApi.h"
int
main() {
    AyonApi Ayon;

    // ---------------- Testing individuale resolve
    constexpr int pathsToResolve = 3300;
    std::array<std::string, pathsToResolve> results;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < pathsToResolve; ++i) {
        results[i] = Ayon.resolvePath(
            "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=usd");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double seconds = duration.count() / 1000000.0;
    double milliseconds = duration.count() / 1000.0;

    std::cout << "Time taken to Resolve : " << pathsToResolve << " Paths  :: " << seconds << " seconds ("
              << milliseconds << " milliseconds)" << std::endl;

    // ------------------ Testing Batch resolve
    for (int x = 1; x <= 1; x++) {
        int pathsToResolve = 400 * x;

        std::vector<std::string>* test = new std::vector<std::string>;

        for (int i = 0; i < pathsToResolve; i++) {
            test->emplace_back(
                "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=usd");
        }

        std::cout << "tes start :" << pathsToResolve << " paths" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> batchResolve = Ayon.batchResolvePath(*test);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double seconds = duration.count() / 1000000.0;
        double milliseconds = duration.count() / 1000.0;

        delete test;

        std::cout << "Time taken to Resolve : " << pathsToResolve << " Paths  :: " << seconds << " seconds ("
                  << milliseconds << " milliseconds)" << std::endl;
    }
}
