set(AR_PROJECT_NAME AyonCppApi)
option(BUILD_TEST "Build the test application" OFF)

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)


# add_subdirectory("ext/cpp-httplib")
add_subdirectory("ext/cpp-httplib" EXCLUDE_FROM_ALL)
add_subdirectory("src/AyonCppApi")



if(BUILD_TEST)
    add_subdirectory("test")       # Executable
endif()



include_directories("ext/cpp-httplib")
include_directories("src/AyonCppApi")
