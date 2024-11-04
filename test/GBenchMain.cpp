#include <AyonCppApi.h>
#include <iostream>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"
#include <benchmark/benchmark.h>

nlohmann::json JsonFile;

AyonApi
getApiInstance() {
    std::string AYON_API_KEY("SuperSaveTestKey");
    std::string AYON_SERVER_URL("http://localhost:8003");
    std::string AYON_SITE_ID("TestId");
    std::string AYON_PROJECT_NAME("TestPrjName");
    std::string AYONLOGGERLOGLVL("CRITICAL");
    std::string AYONLOGGERFILELOGGING("OFF");

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

void
AyonCppApiSerialResolve(benchmark::State &state) {
    AyonApi Api = getApiInstance();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = true;
    bool printResult = false;

    for (auto _: state) {
        AyonCppApiTest::test_SimpleResolve(JsonFile, RunOnlyOneResolveIteration, printResult, Api);
    }
}

void
AyonCppApiBatchResolve(benchmark::State &state) {
    AyonApi Api = getApiInstance();
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
