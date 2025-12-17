# ==============================================================================
# 1. OpenSSL Version Selector 
# ==============================================================================
option(USE_OPENSSL3 "Build against OpenSSL 3.x" OFF)

if(USE_OPENSSL3)
    message(STATUS " [AyonCppApi] OpenSSL Mode: v3 (Newer standards, Maya 2025+)")
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../ext/openssl3L")
else()
    message(STATUS " [AyonCppApi] OpenSSL Mode: v1 (VFX Platform 2023, Houdini 20/21)")
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../ext/openssl1L")
endif()

# ==============================================================================
# 2. Library Definition
# ==============================================================================

add_library(AyonCppApi STATIC AyonCppApi.cpp)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Include the header only library 
target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib)

# Precompile the header lib for better performance 
target_precompile_headers(AyonCppApi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib/httplib.h)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json)
target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json/include)

target_link_libraries(AyonCppApi PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(AyonCppApi PRIVATE Backward::Interface)

target_compile_definitions(AyonCppApi PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT) 

# Link against the specific OpenSSL version selected above
if(WIN32)
  target_link_libraries(AyonCppApi PRIVATE 
    ${OPENSSL_ROOT_DIR}/lib/libcrypto_static.lib
    ${OPENSSL_ROOT_DIR}/lib/libssl_static.lib
  )
else()
  # Standard Linux linking using the root dir we defined at the top
  target_link_libraries(AyonCppApi PRIVATE
    ${OPENSSL_ROOT_DIR}/lib/libssl.a
    ${OPENSSL_ROOT_DIR}/lib/libcrypto.a
    ${OPENSSL_ROOT_DIR}/lib/libssl.a   # Sometimes repeated for circular dep resolution
    ${OPENSSL_ROOT_DIR}/lib/libcrypto.a 
    dl # OpenSSL often needs dynamic loader on Linux
    pthread # OpenSSL often needs threads
  )
endif()

# Include the headers from that specific version
target_include_directories(AyonCppApi PUBLIC ${OPENSSL_ROOT_DIR}/include)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog)
target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog/include)

# Needed because of usd resolver 
set_target_properties(AyonCppApi PROPERTIES POSITION_INDEPENDENT_CODE ON)

install (
  TARGETS AyonCppApi
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install (
  FILES AyonCppApi.h appDataFolder.h devMacros.h Instrumentor.h
  DESTINATION include
)
