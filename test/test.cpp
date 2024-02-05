#include <iostream>
#include <string>
#include <vector>
#include "AyonCppApi.h"
#include "Instrumentor.h"

void
test() {
    InstrumentationTimer timer("Batch Test");
};

int
generateRandomInt(int seed, int min_val, int max_val) {
    // Create a random number generator with the given seed
    std::mt19937 gen(seed);

    // Define the range of integers
    std::uniform_int_distribution<int> distribution(min_val, max_val);

    // Generate a random integer
    return distribution(gen);
}

int
main() {
    AyonApi Ayon;

    int min_paths = 30;
    int iterations = 20;
    std::cout << "Start Test" << std::endl;
    Instrumentor::Get().BeginSession("Profile", "/home/workh/Ynput/dev/ayon-cpp-api/profile.json");

    // ---------------- Testing individuale resolve
    // {
    //     InstrumentationTimer timer("Single Test");
    //
    //     for (int x = 1; x <= iterations; x++) {
    //         int pathsToResolve = min_paths * x;
    //         std::vector<std::string>* test = new std::vector<std::string>;
    //
    //         std::string timer_name = "Paths : " + std::to_string(pathsToResolve);
    //         InstrumentationTimer timer(timer_name.data());
    //
    //         for (int i = 0; i < pathsToResolve; ++i) {
    //             test->emplace_back(Ayon.resolvePath(
    //                 "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=usd"));
    //         }
    //     }
    // }
    // TODO AYON server dies when sending to manny requests
    //  ------------------ Testing Batch resolve
    {
        InstrumentationTimer timer("Batch Test");
        for (int x = 1; x <= iterations; x++) {
            int pathsToResolve = min_paths * x;
            std::string timer_name = "Paths : " + std::to_string(pathsToResolve);
            InstrumentationTimer timer(timer_name.data());

            std::vector<std::string>* test = new std::vector<std::string>;

            for (int i = 0; i < pathsToResolve; i++) {
                test->emplace_back("ayon://Usd_Base/UsdTesting?product=usdUsdTest_"
                                   + std::to_string(generateRandomInt(i, 0, 300)) + "&version=v001&representation=usd");

                // test->emplace_back("ayon://Usd_Base/UsdTesting?product=usdUsdTest_214&version=v001&representation=usd");
            }
            std::unordered_map<std::string, std::string> batchResolve = Ayon.batchResolvePath(*test);
            // for (std::string &path: batchResolve) {
            //     std::cout << path << std::endl;
            // }
            std::cout << timer_name << std::endl;
            delete test;
        }
    }
    Instrumentor::Get().EndSession();
    std::cout << "End Test" << std::endl;
}
