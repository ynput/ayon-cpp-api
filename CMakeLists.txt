cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# Ensure CMAKE_EXPORT_COMPILE_COMMANDS is set before project() call
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(AR_PROJECT_NAME AyonCppApi)

# set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_definitions(JTRACE=${JTRACE})

option(BUILD_TEST "Build the test application" OFF)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(JSON_BuildTests OFF CACHE INTERNAL "")

if(WIN32)
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslW")
else()
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslL")
  # find_package(OpenSSL REQUIRED)
endif()
include_directories(${OPENSSL_ROOT_DIR}/include)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/backward-cpp")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/json" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/json/include")
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog" EXCLUDE_FROM_ALL)
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog/include")

set(AYON_CPPTOOLS_BUILD_LOGGER 1)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-dev-tools")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-dev-tools/src/")


project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

if(AYON_CPP_API_ENALBE_GTEST)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/googletest")
endif()
if(AYON_CPP_API_ENALBE_GBENCH)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/benchmark")
endif()

if(AYON_CPP_API_ENALBE_GTEST OR AYON_CPP_API_ENALBE_GBENCH)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")  
endif()


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")

