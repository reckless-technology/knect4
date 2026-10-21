# The game: its board, evaluation and search, which nothing else is needed for.
add_library(connect4_game STATIC game/game.cpp)
target_include_directories(connect4_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(connect4_game PRIVATE connect4_warnings)

# What a drop feels like: the puck's fall and the click it lands with, written against no toolkit.
add_library(connect4_effects STATIC effects/click_sound.cpp effects/drop_motion.cpp)
target_include_directories(connect4_effects PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(connect4_effects PRIVATE connect4_warnings)

if(CONNECT4_GUI)
    add_subdirectory(app)
endif()
