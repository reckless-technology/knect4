# The game's suites: its rules and search, held to the original's own self-play (data/csharp_self_play.txt), and the
# drop's motion and click.
add_executable(connect4_game_test game_test.cpp effects_test.cpp)
target_link_libraries(connect4_game_test PRIVATE connect4_game connect4_effects GTest::gtest_main connect4_warnings)
target_compile_definitions(connect4_game_test PRIVATE CONNECT4_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
gtest_discover_tests(connect4_game_test DISCOVERY_MODE PRE_TEST)

if(CONNECT4_GUI)
    add_subdirectory(app)
endif()
