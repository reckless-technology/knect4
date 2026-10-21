# The window's suite: the controller, the menus, the settings file and the views (painted into images, driven by
# events), with no window and no sound.
add_executable(knect4_app_test click_player_test.cpp controller_test.cpp settings_file_test.cpp views_test.cpp)
target_link_libraries(knect4_app_test PRIVATE knect4_gui GTest::gtest_main knect4_warnings)
gtest_discover_tests(knect4_app_test DISCOVERY_MODE PRE_TEST)
