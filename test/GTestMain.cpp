#include "gtest/gtest.h"
#include <AyonCppApi.h>
#include <iostream>
#include <string>
#include "Instrumentor.h"
#include "nlohmann/json.hpp"
#include "AyonCppApiTestsMain.h"

nlohmann::json JsonFile;

AyonApi getApiInstance() {
    std::string AYON_API_KEY;
    std::string AYON_SERVER_URL;
    std::string AYON_SITE_ID;
    std::string AYON_PROJECT_NAME;
    std::string AYONLOGGERLOGLVL;
    std::string AYONLOGGERFILELOGGING;
    #ifdef _WIN32
    std::string envFilePath("test\\.env_http");
    #else
    std::string envFilePath("test/.env_http");
    #endif
    if (!AyonCppApiTest::load_EnvVariables(envFilePath, AYON_API_KEY, AYON_SERVER_URL, AYON_SITE_ID, AYON_PROJECT_NAME, AYONLOGGERLOGLVL, AYONLOGGERFILELOGGING)) {
        std::cerr << "Failed to load environment variables!" << std::endl;
    }

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

TEST(AyonCppApi, AyonCppApiCreation) {
    std::cout << "Running AyonCppApiCreation test..." << std::endl;
    AyonApi Test = getApiInstance();
    std::cout << "AyonCppApiCreation test completed." << std::endl;
}

TEST(AyonCppApi, AyonCppApiSerialResolveRootReplace) {
    std::cout << "Running AyonCppApiSerialResolveRootReplace test..." << std::endl;
    Instrumentor::Get().BeginSession("Profile", "bin/profSerial.json");
    AyonApi Api = getApiInstance();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = false;
    bool printResult = true;

    if (!AyonCppApiTest::test_SimpleResolve(JsonFile, RunOnlyOneResolveIteration, printResult, Api)) {
        FAIL();
    }

    Instrumentor::Get().EndSession();
    std::cout << "AyonCppApiSerialResolveRootReplace test completed." << std::endl;
}

TEST(AyonCppApi, AyonCppApiBatchResolveRootReplace) {
    std::cout << "Running AyonCppApiBatchResolveRootReplace test..." << std::endl;
    Instrumentor::Get().BeginSession("Profile", "bin/profBatch.json");
    AyonApi Api = getApiInstance();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = false;
    bool printResult = true;

    if (!AyonCppApiTest::test_BatchResolve(JsonFile, printResult, Api)) {
        FAIL();
    }

    Instrumentor::Get().EndSession();
    std::cout << "AyonCppApiBatchResolveRootReplace test completed." << std::endl;
}

AyonApi getApiInstanceSSL() {
    std::string AYON_API_KEY;
    std::string AYON_SERVER_URL;
    std::string AYON_SITE_ID;
    std::string AYON_PROJECT_NAME;
    std::string AYONLOGGERLOGLVL;
    std::string AYONLOGGERFILELOGGING;
    #ifdef _WIN32
    std::string envFilePath("test\\.env_https");
    #else
    std::string envFilePath("test/.env_https");
    #endif
    if (!AyonCppApiTest::load_EnvVariables(envFilePath, AYON_API_KEY, AYON_SERVER_URL, AYON_SITE_ID, AYON_PROJECT_NAME, AYONLOGGERLOGLVL, AYONLOGGERFILELOGGING)) {
        std::cerr << "Failed to load environment variables!" << std::endl;
    }

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

TEST(AyonCppApi, AyonCppApiCreationSSL) {
    std::cout << "Running AyonCppApiCreationSSL test..." << std::endl;
    AyonApi Test = getApiInstanceSSL();
    std::cout << "AyonCppApiCreationSSL test completed." << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Running tests..." << std::endl;
    std::ifstream file("test/testData.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
    }
    JsonFile = nlohmann::json::parse(file);
    file.close();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
