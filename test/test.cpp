#include "test.h"
#include <iostream>
#include "AyonCppApi.h"
#include "testBatchResolve.h"
#include "testSerialResolve.h"

int
main(int argc, char* argv[]) {
    backward::SignalHandling sh;
    //------------------ Init
    if (argc != 7) {
        std::cerr << "Usage: " << argc << argv[0] << " <string1> <string2> <int1> <int2> <int3> <int4>" << std::endl;
        return 1;
    }

    string1 = argv[1];
    string2 = argv[2];
    min_paths = std::atoi(argv[3]);
    runIterations = std::atoi(argv[4]);
    minRandomeNameInt = std::atoi(argv[5]);
    maxRandomeNameInt = std::atoi(argv[6]);

    //------- setup logging and progiling
    profile_ext = ".json";
    testStartDatation = getCurrentDateTimeAsString();
    profileJsonName = "profile_" + testStartDatation + profile_ext;

    loggingFileName = "Log_" + testStartDatation + ".txt";

    //-------- Start logging and profiling
    Instrumentor::Get().BeginSession("Profile", std::filesystem::current_path().string() + profileJsonName.c_str());
    TestLogger = new TestFileLogger(loggingFileName);

    PlotLogger = new TestFileLogger(plottingFileName);

    std::cout << "Testing started With\n min_paths: " << min_paths << "\n runIterations:" << runIterations << std::endl;
    //-------------- Testing

    Ayon = new AyonApi;
    *PlotLogger << std::string("Uri Paths") + "," + "Time in ms"
                << "\n";
    batchResolveTest();
    // serialResolveTest();

    delete Ayon;
    delete TestLogger;
    std::cout << "test end" << std::endl;
    Instrumentor::Get().EndSession();
}
