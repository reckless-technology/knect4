cmake_minimum_required(VERSION 3.25)
# A library a target reaches twice is kept on the link line only where the linker needs it (CMake 3.29 on).
if(POLICY CMP0156)
    cmake_policy(SET CMP0156 NEW)
endif()
# URL archives take the extraction time, so a changed dependency is newer than what was built from the one before.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Connect Four: four in a row against the computer. The game's rules, evaluation and search (src/game) and the drop's
# motion and click (src/effects) need nothing else; the window (src/app) is drawn with Skia on reckless-toolkit, a git
# submodule at external/reckless-toolkit, and plays its clicks through the toolkit's audio device layer.

# The oldest macOS the Mac binary runs on, set before project(): 13.3, the floor every Reckless product shares.
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "The oldest macOS the Mac binary runs on" FORCE)
endif()

project(Connect4 VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # the editor's code model reads build/<preset>/compile_commands.json

# cl and clang-cl both take MSVC's command line; every flag below is spelled for the front end, not the compiler.
set(CONNECT4_MSVC_FRONTEND OFF)
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CONNECT4_MSVC_FRONTEND ON)
endif()

if(MSVC)
    # The static release runtime in every configuration: nothing to install beside the program, and the toolkit's
    # prebuilt Skia is built for it alone.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    add_compile_options(/utf-8)
endif()

option(CONNECT4_BUILD_TESTS "Build the test suites" ON)
option(CONNECT4_GUI "Build the window on the toolkit submodule (external/reckless-toolkit)" ON)

# The strict warning set on every compiler, for the game's own targets. Not errors: a new compiler must not break the
# build; the code is kept warning-free on gcc, clang and MSVC, and CI's logs are the check.
add_library(connect4_warnings INTERFACE)
if(CONNECT4_MSVC_FRONTEND)
    target_compile_options(connect4_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(connect4_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# The version every binary reports, from project() above.
configure_file(cmake/version.h.in "${PROJECT_BINARY_DIR}/generated/version.h" @ONLY)

if(CONNECT4_BUILD_TESTS)
    enable_testing()
    # GoogleTest, pinned by its release tarball's hash.
    include(FetchContent)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        URL      https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

if(CONNECT4_GUI)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/reckless-toolkit/CMakeLists.txt")
        message(FATAL_ERROR "The window is built on the toolkit, a git submodule at external/reckless-toolkit, which this "
                            "checkout does not hold. Fetch it with 'git submodule update --init', or build the game and "
                            "its suites alone with -DCONNECT4_GUI=OFF.")
    endif()
    # The toolkit's options, as normal variables before it is added: its windowed parts and its audio device layer on
    # (the click), its own suites off, and no ASIO - a game's click goes to the system's own output.
    set(RECKLESS_TOOLKIT_BUILD_TESTS OFF)
    set(RECKLESS_TOOLKIT_GUI ON)
    set(RECKLESS_BUILD_AUDIO ON)
    set(RECKLESS_ASIO OFF)
    add_subdirectory(external/reckless-toolkit EXCLUDE_FROM_ALL)
endif()

add_subdirectory(src)
if(CONNECT4_BUILD_TESTS)
    add_subdirectory(tests)
endif()
