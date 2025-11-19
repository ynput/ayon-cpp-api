
# add_library(AyonCppApi STATIC AyonCppApi.cpp)

# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# #include the header only libary 
# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib)
# # precompile the header lib for better perfocmance 
# target_precompile_headers(AyonCppApi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib/httplib.h)

# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json)
# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json/include)


# target_link_libraries(AyonCppApi PRIVATE nlohmann_json::nlohmann_json)
# target_link_libraries(AyonCppApi PRIVATE Backward::Interface)

# target_compile_definitions(AyonCppApi PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT) 

# if(WIN32)
#   target_link_libraries(AyonCppApi PRIVATE 
#     ${OPENSSL_ROOT_DIR}/lib/libcrypto_static.lib
#     ${OPENSSL_ROOT_DIR}/lib/libssl_static.lib
#   )
# else()
#   target_link_libraries(AyonCppApi PRIVATE
#     ${OPENSSL_ROOT_DIR}/lib/libssl.a
#     ${OPENSSL_ROOT_DIR}/lib/libcrypto.a
#   )
# endif()
# target_include_directories(AyonCppApi PUBLIC ${OPENSSL_ROOT_DIR}/include)

# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog)

# target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog/include)

# # needed because off usd resovler 
# set_target_properties(AyonCppApi PROPERTIES POSITION_INDEPENDENT_CODE ON)


# install (
#   TARGETS AyonCppApi
#     DESTINATION ${CMAKE_INSTALL_PREFIX}
# )

# install (
#   FILES AyonCppApi.h appDataFoulder.h devMacros.h Instrumentor.h
#   DESTINATION include
# ) 

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# Ensure CMAKE_EXPORT_COMPILE_COMMANDS is set before project() call
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(AR_PROJECT_NAME AyonCppApi)

# ---------------------------------------------------------------------------
# Core settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Provide default when invoked from external projects
if(NOT DEFINED JTRACE)
    set(JTRACE 0)
endif()
add_compile_definitions(JTRACE=${JTRACE})

option(BUILD_TEST "Build the test application" OFF)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(JSON_BuildTests OFF CACHE INTERNAL "")

# ---------------------------------------------------------------------------
# OpenSSL selection (unused by the resolver, but required by AyonCppApi)
# ---------------------------------------------------------------------------
if(WIN32)
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslW")
else()
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslL")
endif()
include_directories(${OPENSSL_ROOT_DIR}/include)

# ---------------------------------------------------------------------------
# External dependencies (header-only or local)
# ---------------------------------------------------------------------------
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/backward-cpp")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/json" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/json/include")

set(AYON_CPPTOOLS_BUILD_LOGGER 1)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-dev-tools")

# ---------------------------------------------------------------------------
# Main project
# ---------------------------------------------------------------------------
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Optional tests and benchmarks
# ---------------------------------------------------------------------------
if(AYON_CPP_API_ENALBE_GTEST)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/googletest")
endif()

if(AYON_CPP_API_ENALBE_GBENCH)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/benchmark")
endif()

if(AYON_CPP_API_ENALBE_GTEST OR AYON_CPP_API_ENALBE_GBENCH)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()

# ---------------------------------------------------------------------------
# Main library
# ---------------------------------------------------------------------------
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")

# ---------------------------------------------------------------------------
# Fix: Houdini / USD / MayaUSD crash from bundled spdlog fmt
# This forces spdlog to use the SAME fmt version as the DCC.
# ---------------------------------------------------------------------------
add_definitions(-DSPDLOG_FMT_EXTERNAL)
find_package(fmt REQUIRED)
target_link_libraries(AyonCppApi PRIVATE fmt::fmt)

# ---------------------------------------------------------------------------
# Final link — AyonCppDevTools contains spdlog and helpers
# ---------------------------------------------------------------------------
target_link_libraries(AyonCppApi PUBLIC AyonCppDevToolsLib)