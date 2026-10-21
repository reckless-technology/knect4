# The game: its board and rules, and the computer's play on every board size, which nothing else is needed for.
add_library(knect4_game STATIC game/engine.cpp game/game.cpp game/opening_book.cpp)
target_include_directories(knect4_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(knect4_game PRIVATE knect4_warnings)

# What a drop feels like: the puck's fall and the click it lands with, written against no toolkit.
add_library(knect4_effects STATIC effects/click_sound.cpp effects/drop_motion.cpp)
target_include_directories(knect4_effects PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(knect4_effects PRIVATE knect4_warnings)

if(KNECT4_GUI)
    add_subdirectory(app)
endif()
