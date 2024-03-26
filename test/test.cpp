#include "test.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include "AyonCppApi.h"

std::string testStartDatation = getCurrentDateTimeAsString();
std::string loggingFileName = "Log_" + testStartDatation + ".txt";
std::string plottingFileName = "Plot_" + testStartDatation + ".txt";
std::string profileJsonName = "profile_" + testStartDatation + ".json";

TestFileLogger* TestLogger = new TestFileLogger(loggingFileName);
TestFileLogger* PlotLogger = new TestFileLogger(plottingFileName);
AyonApi* Ayon = new AyonApi;

int
main(int argc, char* argv[]) {
    backward::SignalHandling sh;
    //------------------ Init
    if (argc < 1) {
        std::cerr << "Usage: " << std::endl;
        return 1;
    }
    Instrumentor::Get().BeginSession("Profile",
                                     std::filesystem::current_path().string() + "/" + profileJsonName.c_str());

    // Test for single path reselution
    for (int i = 1; i < argc; i++) {
        std::string resolvedPath = Ayon->resolvePath(argv[i]).second;

        std::cout << "\nRequested Path: " << argv[i] << "\nResolved Path: " << resolvedPath << "\n" << std::endl;
    }

    // Test for Batch Resolve
    std::cout << "Batch Requested Paths: \n";
    std::vector<std::string> arguments(argv + 1, argv + argc);
    for (size_t i = 1; i < arguments.size(); i++) {
        std::cout << arguments.at(i) << "\n";
    }

    std::unordered_map<std::string, std::string> responseBatch = Ayon->batchResolvePath(arguments);
    std::cout << "Batch Resolved Paths: \n";
    for (const auto &entry: responseBatch) {
        std::cout << entry.second << "\n";
    }
    std::cout << std::endl;

    delete Ayon;
    delete TestLogger;
    delete PlotLogger;
    Instrumentor::Get().EndSession();
}
