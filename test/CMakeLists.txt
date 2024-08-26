
if(AYON_CPP_API_ENALBE_GTEST)
  add_executable(AyonCppApiGtestMain GTestMain.cpp AyonCppApiTestsMain.cpp) 

    target_link_libraries(AyonCppApiGtestMain PRIVATE AyonCppApi)
    target_link_libraries(AyonCppApiGtestMain PRIVATE GTest::gtest_main)


    install (
      TARGETS AyonCppApiGtestMain
      DESTINATION ${CMAKE_INSTALL_PREFIX}
    )

endif()

if(AYON_CPP_API_ENALBE_GBENCH)
  add_executable(AyonCppApiGBenchMain GBenchMain.cpp AyonCppApiTestsMain.cpp) 

    target_link_libraries(AyonCppApiGBenchMain PRIVATE AyonCppApi)
    target_link_libraries(AyonCppApiGBenchMain PRIVATE benchmark::benchmark)


    install (
      TARGETS AyonCppApiGBenchMain
      DESTINATION ${CMAKE_INSTALL_PREFIX}
    )


endif()
