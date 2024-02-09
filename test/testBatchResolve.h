#include "test.h"

void
batchResolveTest() {
    *TestLogger << "batch";

    for (int x = 1; x <= runIterations; x++) {
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

        *TestLogger << "After Calling batchResolvePath() Returnd array len: " << batchResolve.size() << "\n";
        for (const std::pair<std::string, std::string> path: batchResolve) {
            *TestLogger << "Asset Identifier: " << path.first << "asset path: " << path.second << "\n";
        }

        delete test;
    }
};
