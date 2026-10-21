# The game's suites: its rules, the engine on every board size (its solver held to exhaustive search), and the
# drop's motion and click.
add_executable(knect4_game_test game_test.cpp engine_test.cpp effects_test.cpp)
target_link_libraries(knect4_game_test PRIVATE knect4_game knect4_effects GTest::gtest_main knect4_warnings)
gtest_discover_tests(knect4_game_test DISCOVERY_MODE PRE_TEST)

if(KNECT4_GUI)
    add_subdirectory(app)
endif()
