# The window's suite: the controller and the menus, with no window and no sound.
add_executable(connect4_app_test controller_test.cpp)
target_link_libraries(connect4_app_test PRIVATE connect4_gui GTest::gtest_main connect4_warnings)
gtest_discover_tests(connect4_app_test DISCOVERY_MODE PRE_TEST)
