
add_library(AyonCppApi STATIC AyonCppApi.cpp)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#include the header only libary 
target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib)
# precompile the header lib for better perfocmance 
target_precompile_headers(AyonCppApi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/cpp-httplib/httplib.h)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json)
target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json/include)


target_link_libraries(AyonCppApi PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(AyonCppApi PRIVATE Backward::Interface)

target_compile_definitions(AyonCppApi PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT) 

if(WIN32)
  target_link_libraries(AyonCppApi PRIVATE 
    ${OPENSSL_ROOT_DIR}/lib/libcrypto_static.lib
    ${OPENSSL_ROOT_DIR}/lib/libssl_static.lib
  )
else()
  target_link_libraries(AyonCppApi PRIVATE
    ${OPENSSL_ROOT_DIR}/lib/libssl.a
    ${OPENSSL_ROOT_DIR}/lib/libcrypto.a
  )
endif()
target_include_directories(AyonCppApi PUBLIC ${OPENSSL_ROOT_DIR}/include)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog)

target_include_directories(AyonCppApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/spdlog/include)

# needed because off usd resovler 
set_target_properties(AyonCppApi PROPERTIES POSITION_INDEPENDENT_CODE ON)


install (
  TARGETS AyonCppApi
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install (
  FILES AyonCppApi.h appDataFoulder.h devMacros.h Instrumentor.h
  DESTINATION include
) 
