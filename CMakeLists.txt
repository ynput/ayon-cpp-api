cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

add_compile_definitions(JTRACE=${JTRACE})
#we need to define this varible in order for httplib to suport https clients 
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)


set(AR_PROJECT_NAME AyonCppApi)
option(BUILD_TEST "Build the test application" OFF)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)


set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(JSON_BuildTests OFF CACHE INTERNAL "")

if(WIN32)
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslW")
else()
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/opensslL")
  # find_package(OpenSSL REQUIRED)
endif()
include_directories(${OPENSSL_ROOT_DIR}/include)



project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/json" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/json/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/backward-cpp")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")

if(BUILD_TEST)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/googletest")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/benchmark")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")       # Dont add / Build test app 
endif()



