#include "app/settings_file.h"

#include "Json.h"
#include "JsonFile.h"
#include "PresetFiles.h"

#include <string>

namespace knect4::app
{

namespace
{

/**
 * @param level A level.
 * @return Its name in the file.
 */
const char *LevelName(Level level) noexcept
{
    switch (level)
    {
    case Level::kEasy:
        return "easy";
    case Level::kHard:
        return "hard";
    case Level::kMedium:
    default:
        return "medium";
    }
}

} // namespace

std::filesystem::path DefaultSettingsFile()
{
    const std::filesystem::path folder = reckless::core::UserSettingsFolder(reckless::core::kSuiteFolder);
    return folder.empty() ? folder : folder / "knect4" / "settings.json";
}

Settings ReadSettings(const std::filesystem::path &file)
{
    Settings settings;
    if (file.empty())
    {
        return settings;
    }
    const std::optional<reckless::core::json::Value> document = reckless::core::ReadJsonFile(file);
    if (!document || !document->IsObject())
    {
        return settings;
    }
    settings.is_two_player  = document->StringOr("opponent", "computer") == "friend";
    const std::string level = document->StringOr("level", LevelName(settings.level));
    for (const Level known : {Level::kEasy, Level::kMedium, Level::kHard})
    {
        if (level == LevelName(known))
        {
            settings.level = known;
        }
    }
    settings.is_person_red     = document->StringOr("your_colour", "red") != "yellow";
    settings.does_person_start = document->StringOr("first_move", "you") != "computer";
    settings.has_sound         = document->BoolOr("sound", settings.has_sound);
    return settings;
}

bool WriteSettings(const std::filesystem::path &file, const Settings &settings)
{
    if (file.empty())
    {
        return false;
    }
    reckless::core::json::Value document;
    document.Set("opponent", std::string(settings.is_two_player ? "friend" : "computer"));
    document.Set("level", std::string(LevelName(settings.level)));
    document.Set("your_colour", std::string(settings.is_person_red ? "red" : "yellow"));
    document.Set("first_move", std::string(settings.does_person_start ? "you" : "computer"));
    document.Set("sound", settings.has_sound);
    return reckless::core::WriteJsonFile(file, document);
}

} // namespace knect4::app
