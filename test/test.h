#ifndef TESTHEADER
#define TESTHEADER

#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include "AyonCppApi.h"
#include "Instrumentor.h"
#include "testLib.h"
#include <backward.hpp>

std::string string1;
std::string string2;
int min_paths;
int runIterations;
int minRandomeNameInt;
int maxRandomeNameInt;

//------- setup logging and progiling
std::string profile_ext = ".json";
std::string testStartDatation = getCurrentDateTimeAsString();
std::string profileJsonName = "profile_" + testStartDatation + profile_ext;

std::string loggingFileName = "Log_" + testStartDatation + ".txt";

TestFileLogger* TestLogger;
AyonApi* Ayon;
#endif   // DEBUG
