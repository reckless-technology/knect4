# The window's suite: the controller, the menus and the settings file, with no window and no sound.
add_executable(knect4_app_test controller_test.cpp settings_file_test.cpp)
target_link_libraries(knect4_app_test PRIVATE knect4_gui GTest::gtest_main knect4_warnings)
gtest_discover_tests(knect4_app_test DISCOVERY_MODE PRE_TEST)
