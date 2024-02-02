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
main() {
    AyonApi Ayon;

    int min_paths = 480;
    int iterations = 1;
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
                test->emplace_back(
                    "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=*&representation=usd");
                // test->emplace_back(std::to_string(i));
            }
            std::vector<std::string> batchResolve = Ayon.batchResolvePath(*test);

            delete test;
        }
    }
    Instrumentor::Get().EndSession();
    std::cout << "End Test" << std::endl;
}
