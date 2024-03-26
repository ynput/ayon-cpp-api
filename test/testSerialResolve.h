#include <future>
#include "test.h"

void
serialResolveTest() {
    for (int x = 1; x <= runIterations; x++) {
        int pathsToResolve = min_paths * x;
        std::string timer_name = "Serial Paths : " + std::to_string(pathsToResolve);
        InstrumentationTimer timer(timer_name.data());
        *TestLogger << timer_name.data() << "\n";

        std::vector<std::string>* test = new std::vector<std::string>;

        for (int i = 0; i < pathsToResolve; i++) {
            test->emplace_back(string1 + std::to_string(generateRandomInt(i, minRandomeNameInt, maxRandomeNameInt))
                               + string2);
        }

        std::vector<std::future<std::pair<std::string, std::string>>> future;
        std::vector<std::string> resolved;
        for (std::string uri: *test) {
            future.push_back(std::async(std::launch::async, &AyonApi::resolvePath, &*Ayon, uri));
        }

        for (auto &path: future) {
            auto data = path.get();
            *TestLogger << "serial resolve: " << data.first << "asset path: " << data.second << "\n";
        }
        delete test;
    }
}
