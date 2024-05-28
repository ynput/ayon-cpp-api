#include <AyonCppApi.h>
#include <iostream>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"
#include <benchmark/benchmark.h>

nlohmann::json JsonFile;

void
AyonCppApiSerialResolve(benchmark::State &state) {
    AyonApi Api = AyonApi();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = true;
    bool printResult = false;

    for (auto _: state) {
        AyonCppApiTest::test_SimpleResolve(JsonFile, RunOnlyOneResolveIteration, printResult, Api);
    }
}

void
AyonCppApiBatchResolve(benchmark::State &state) {
    AyonApi Api = AyonApi();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool printResult = false;

    for (auto _: state) {
        AyonCppApiTest::test_BatchResolve(JsonFile, printResult, Api);
    }
}

BENCHMARK(AyonCppApiSerialResolve);

BENCHMARK(AyonCppApiBatchResolve);

int
main(int argc, char** argv) {
    std::ifstream file("test/testData.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
    }
    JsonFile = nlohmann::json::parse(file);
    file.close();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
