set(AR_PROJECT_NAME AyonCppApi)
option(BUILD_TEST "Build the test application" OFF)

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)


set(JSON_BuildTests OFF CACHE INTERNAL "")


# add the dependency directory but exclude the cmake because we don't need it


add_subdirectory("src/AyonCppApi")
include_directories("src/AyonCppApi")

add_subdirectory("ext/cpp-httplib" EXCLUDE_FROM_ALL)
include_directories("ext/cpp-httplib")

add_subdirectory("ext/json" EXCLUDE_FROM_ALL)
include_directories("ext/json/include")

add_subdirectory("ext/spdlog" EXCLUDE_FROM_ALL)
include_directories("ext/spdlog/include")


if(BUILD_TEST)
    add_subdirectory("test")       # Executable
endif()



