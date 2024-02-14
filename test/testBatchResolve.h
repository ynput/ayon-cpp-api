#include <ratio>
#include <string>
#include "test.h"

void
batchResolveTest() {
    *TestLogger << "batch";

    for (int x = 1; x <= runIterations; x++) {
        using Clock = std::chrono::high_resolution_clock;
        auto start = Clock::now();

        int pathsToResolve = min_paths * x;
        std::string timer_name = "Batch Paths : " + std::to_string(pathsToResolve);
        InstrumentationTimer timer(timer_name.data());
        *TestLogger << timer_name.data() << "\n";

        std::vector<std::string>* test = new std::vector<std::string>;

        for (int i = 0; i < pathsToResolve; i++) {
            test->emplace_back(string1 + std::to_string(generateRandomInt(i, minRandomeNameInt, maxRandomeNameInt))
                               + string2);
        }

        std::unordered_map<std::string, std::string> batchResolve = Ayon->batchResolvePath(*test);
        for (auto &b: batchResolve) {
            // std::cout << b.first << " / " << b.second << std::endl;
        }
        *TestLogger << "After Calling batchResolvePath() Returnd array len: " << batchResolve.size() << "\n";
        for (const std::pair<std::string, std::string> path: batchResolve) {
            *TestLogger << "Asset Identifier: " << path.first << "asset path: " << path.second << "\n";
        }

        auto end = Clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto duration = std::chrono::duration<double, std::milli>(end - start);

        *PlotLogger << std::to_string(pathsToResolve) + "," + std::to_string(duration.count()) << "\n";
        delete test;
    }
};
