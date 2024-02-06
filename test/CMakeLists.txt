
if(BUILD_TEST)
    add_executable(test_app test.cpp testLib.cpp)

    target_link_libraries(test_app AyonCppApi)


    install (
      TARGETS test_app
      DESTINATION ${CMAKE_INSTALL_PREFIX}
    )

endif()
