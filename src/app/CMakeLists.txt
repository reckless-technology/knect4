# The window, on the toolkit: the game (Connect4) - a Windows executable with its version resource and icon, a macOS
# .app, a Linux binary - the render tool, and the library both are made from, which the window's suite links.

# The files the binary carries (the toolkit's reckless_module_assets): the series' serif, the company mark, and the
# app's icon at 256 pixels for the window's title bar on Linux.
reckless_module_assets(connect4_assets "icon_256.png=${PROJECT_SOURCE_DIR}/assets/icon/icon_256.png")

add_library(connect4_gui STATIC
    board_art.cpp
    board_view.cpp
    click_player.cpp
    controller.cpp
    main_view.cpp
    menus.cpp
    side_panel.cpp)
target_include_directories(connect4_gui PUBLIC "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(connect4_gui
    PUBLIC  connect4_game connect4_effects connect4_assets reckless_app_shell reckless_common_panel reckless_gfx_skia
            reckless_audio_devices
    PRIVATE connect4_warnings)

add_executable(Connect4 main.cpp)
target_link_libraries(Connect4 PRIVATE connect4_gui reckless_app_platform connect4_warnings)
# On Linux the program is named as Linux programs are, in lower case.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(Connect4 PROPERTIES OUTPUT_NAME connect4)
endif()

# The render tool: the window drawn to a PNG in a chosen state, and the app's icon at any size.
add_executable(connect4_render render_main.cpp)
target_link_libraries(connect4_render PRIVATE connect4_gui connect4_warnings)

# The identity the executable carries, as the toolkit's bundle helpers read it.
set(CONNECT4_ICONS "${PROJECT_SOURCE_DIR}/assets/icon")
set(CONNECT4_COMPANY "Reckless Technology")
set(CONNECT4_DISPLAY "Connect Four")
set(CONNECT4_COPYRIGHT "Copyright (C) 2014-2026 Jonny Reckless")
set(RECKLESS_VERSION "${PROJECT_VERSION}")
set(RECKLESS_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(RECKLESS_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(RECKLESS_VERSION_BUILD "${PROJECT_VERSION_PATCH}")

if(APPLE)
    set(BUNDLE_EXECUTABLE Connect4)
    set(BUNDLE_IDENTIFIER com.recklesstechnology.connect4)
    set(DISPLAY "${CONNECT4_DISPLAY}")
    set(VERSION "${PROJECT_VERSION}")
    set(COPYRIGHT "${CONNECT4_COPYRIGHT}")
    set(plist "${CMAKE_CURRENT_BINARY_DIR}/connect4_info.plist")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/info.plist.in" "${plist}" @ONLY)
    set_target_properties(Connect4 PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_INFO_PLIST "${plist}")
    target_sources(Connect4 PRIVATE "${CONNECT4_ICONS}/icon.icns")
    set_source_files_properties("${CONNECT4_ICONS}/icon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    reckless_sign_bundle(Connect4 "$<TARGET_BUNDLE_DIR:Connect4>")
elseif(WIN32)
    # A window of its own, no console; main() as its entry, as the toolkit's platform expects.
    set_target_properties(Connect4 PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(Connect4 PRIVATE "/ENTRY:mainCRTStartup")
    endif()
    reckless_windows_version_resource(Connect4 CONNECT4 APP Connect4.exe "${CONNECT4_ICONS}/icon.ico")
endif()

if(CONNECT4_BUILD_TESTS)
    # The app's own launch: its window opened, a few frames drawn, closed, with no sound.
    add_test(NAME app_smoke COMMAND Connect4 --smoke)
    set_tests_properties(app_smoke PROPERTIES LABELS window RESOURCE_LOCK display TIMEOUT 120)
endif()
