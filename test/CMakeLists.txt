
if(BUILD_TEST)
  add_executable(AyonCppApiBenchMain benchMain.cpp) 

    target_link_libraries(AyonCppApiBenchMain PRIVATE AyonCppApi)
    target_link_libraries(AyonCppApiBenchMain PRIVATE benchmark::benchmark)
    target_link_libraries(AyonCppApiBenchMain PRIVATE GTest::gtest_main)


    install (
      TARGETS AyonCppApiBenchMain
      DESTINATION ${CMAKE_INSTALL_PREFIX}
    )

endif()
