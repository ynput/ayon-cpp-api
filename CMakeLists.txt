add_compile_definitions(JTRACE=${JTRACE})


#find_package(OpenSSL REQUIRED)


set(AR_PROJECT_NAME AyonCppApi)
option(BUILD_TEST "Build the test application" OFF)

set(CMAKE_CXX_STANDARD 20)


cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)


set(JSON_BuildTests OFF CACHE INTERNAL "")


# add the dependency directory but exclude the cmake because we don't need it

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/AyonCppApi")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/cpp-httplib")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/json" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/json/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog" EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog/include")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/backward-cpp")




if(BUILD_TEST)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")       # Dont add / Build test app 
endif()



