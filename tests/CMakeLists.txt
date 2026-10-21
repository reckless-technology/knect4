# The game's suites: its rules and search, held to the original's own self-play (data/csharp_self_play.txt), and the
# drop's motion and click.
add_executable(knect4_game_test game_test.cpp effects_test.cpp)
target_link_libraries(knect4_game_test PRIVATE knect4_game knect4_effects GTest::gtest_main knect4_warnings)
target_compile_definitions(knect4_game_test PRIVATE KNECT4_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
gtest_discover_tests(knect4_game_test DISCOVERY_MODE PRE_TEST)

if(KNECT4_GUI)
    add_subdirectory(app)
endif()
