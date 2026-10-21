#pragma once
/// @file settings_file.h The person's choices kept between launches: `reckless-technology/knect4/settings.json` in
/// the user's settings folder, a JSON object of the level, the colour, who starts and the sound.

#include "controller.h"

#include <filesystem>

namespace knect4::app
{

/** @return `reckless-technology/knect4/settings.json` in the user's settings folder; empty where there is none. */
std::filesystem::path DefaultSettingsFile();

/**
 * The choices kept in `file`: each one read where it is there and makes sense, the default where it is missing, of
 * another kind or unknown (a level other than "easy", "medium" or "hard"); all the defaults where the file is missing
 * or not JSON.
 * @param file The file.
 * @return The choices.
 */
Settings ReadSettings(const std::filesystem::path &file);

/**
 * The choices written to `file`, whole or not at all, its folder made first.
 * @param file The file; nothing is written where it is empty.
 * @param settings The choices.
 * @return Whether the file was written.
 */
bool WriteSettings(const std::filesystem::path &file, const Settings &settings);

} // namespace knect4::app
