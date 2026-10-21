#include "app/settings_file.h"

#include "Json.h"
#include "JsonFile.h"
#include "PresetFiles.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace knect4::app
{

namespace
{

using reckless::core::json::Value;

/** The levels by their names in the file. */
constexpr std::array<std::pair<Level, const char *>, 4> kLevelNames{
    {{Level::kEasy, "easy"}, {Level::kMedium, "medium"}, {Level::kHard, "hard"}, {Level::kPerfect, "perfect"}}};

/**
 * @param level A level.
 * @return Its name in the file.
 */
const char *LevelName(Level level) noexcept
{
    for (const auto &[known, name] : kLevelNames)
    {
        if (known == level)
        {
            return name;
        }
    }
    return "medium";
}

/**
 * @param size A board's size.
 * @return Its name in the file: "7x6".
 */
std::string SizeName(BoardSize size)
{
    return std::to_string(size.columns) + "x" + std::to_string(size.rows);
}

/**
 * @param count A count read, perhaps missing or negative.
 * @return It held to 0 or more.
 */
int Count(std::int64_t count) noexcept
{
    return count < 0 ? 0 : (count > 1000000000 ? 1000000000 : static_cast<int>(count));
}

/**
 * @param object A score's object in the file.
 * @return The score.
 */
Score ReadScore(const Value &object)
{
    Score score;
    if (object.IsObject())
    {
        score.min_wins = Count(object.IntOr("first", 0));
        score.max_wins = Count(object.IntOr("second", 0));
        score.drawn    = Count(object.IntOr("drawn", 0));
    }
    return score;
}

/**
 * @param score A score.
 * @return Its object for the file: the first side's (the person's, or Red's) wins, the second's, and the draws.
 */
Value ScoreValue(const Score &score)
{
    Value object;
    object.Set("first", score.min_wins);
    object.Set("second", score.max_wins);
    object.Set("drawn", score.drawn);
    return object;
}

} // namespace

std::filesystem::path DefaultSettingsFile()
{
    const std::filesystem::path folder = reckless::core::UserSettingsFolder(reckless::core::kSuiteFolder);
    return folder.empty() ? folder : folder / "knect4" / "settings.json";
}

Saved ReadSaved(const std::filesystem::path &file)
{
    Saved saved;
    if (file.empty())
    {
        return saved;
    }
    const std::optional<Value> document = reckless::core::ReadJsonFile(file);
    if (!document || !document->IsObject())
    {
        return saved;
    }
    Settings &settings      = saved.settings;
    settings.is_two_player  = document->StringOr("opponent", "computer") == "friend";
    const std::string level = document->StringOr("level", LevelName(settings.level));
    for (const auto &[known, name] : kLevelNames)
    {
        if (level == name)
        {
            settings.level = known;
        }
    }
    settings.is_person_red     = document->StringOr("your_colour", "red") != "yellow";
    settings.does_person_start = document->StringOr("first_move", "you") != "computer";
    settings.does_alternate    = document->BoolOr("alternate", settings.does_alternate);
    settings.has_sound         = document->BoolOr("sound", settings.has_sound);
    settings.has_marks         = document->BoolOr("marks", settings.has_marks);
    const std::string board    = document->StringOr("board", SizeName(kStandardBoard));
    for (const BoardSize size : kBoardSizes)
    {
        if (board == SizeName(size))
        {
            settings.board = size;
        }
    }
    settings.red_name          = document->StringOr("red_name", settings.red_name);
    settings.yellow_name       = document->StringOr("yellow_name", settings.yellow_name);
    const std::int64_t percent = document->IntOr("window_percent", settings.window_percent);
    settings.window_percent    = percent >= 50 && percent <= 150 ? static_cast<int>(percent) : settings.window_percent;
    if (const Value *scores = document->Find("scores"); scores != nullptr && scores->IsObject())
    {
        saved.scores.computer   = ReadScore((*scores)["computer"]);
        saved.scores.two_player = ReadScore((*scores)["two_player"]);
    }
    return saved;
}

bool WriteSaved(const std::filesystem::path &file, const Saved &saved)
{
    if (file.empty())
    {
        return false;
    }
    const Settings &settings = saved.settings;
    Value           document;
    document.Set("opponent", std::string(settings.is_two_player ? "friend" : "computer"));
    document.Set("level", std::string(LevelName(settings.level)));
    document.Set("your_colour", std::string(settings.is_person_red ? "red" : "yellow"));
    document.Set("first_move", std::string(settings.does_person_start ? "you" : "computer"));
    document.Set("alternate", settings.does_alternate);
    document.Set("sound", settings.has_sound);
    document.Set("marks", settings.has_marks);
    document.Set("board", SizeName(settings.board));
    document.Set("red_name", settings.red_name);
    document.Set("yellow_name", settings.yellow_name);
    document.Set("window_percent", settings.window_percent);
    Value scores;
    scores.Set("computer", ScoreValue(saved.scores.computer));
    scores.Set("two_player", ScoreValue(saved.scores.two_player));
    document.Set("scores", std::move(scores));
    return reckless::core::WriteJsonFile(file, document);
}

Saved SettingsKeeper::Read()
{
    seen_ = ReadSaved(file_);
    return *seen_;
}

bool SettingsKeeper::Changed(const Saved &now)
{
    const bool does_differ        = !seen_ || !(*seen_ == now);
    const bool do_settings_differ = !seen_ || !(seen_->settings == now.settings);
    if (does_differ && seen_ && WriteSaved(file_, now))
    {
        ++writes_;
    }
    const bool is_first = !has_reported_;
    has_reported_       = true;
    seen_               = now;
    return do_settings_differ || is_first;
}

} // namespace knect4::app
