#pragma once
/// @file settings_file.h The person's choices and the scores kept between launches: `reckless-technology/knect4/
/// settings.json` in the user's settings folder, a JSON object of the opponent, the level, the colour, who starts and
/// whether that alternates, the sound, the marks, the board's size, the players' names, the window's size and the
/// scores of both kinds of game.

#include "controller.h"

#include <filesystem>
#include <optional>
#include <utility>

namespace knect4::app
{

/** What the file keeps. */
struct Saved
{
    Settings settings; ///< The person's choices.
    Scores   scores;   ///< The games so far.

    /** @param other Other contents. @return The two the same. */
    bool operator==(const Saved &other) const = default;
};

/** @return `reckless-technology/knect4/settings.json` in the user's settings folder; empty where there is none. */
std::filesystem::path DefaultSettingsFile();

/**
 * What `file` keeps: each choice read where it is there and makes sense, the default where it is missing, of another
 * kind or unknown (a level other than "easy", "medium", "hard" or "perfect", a board size the game does not offer, a
 * window size outside 50 to 150); a count that is missing or below 0 is 0; all the defaults where the file is missing
 * or not JSON.
 * @param file The file.
 * @return The contents.
 */
Saved ReadSaved(const std::filesystem::path &file);

/**
 * The contents written to `file`, whole or not at all, its folder made first.
 * @param file The file; nothing is written where it is empty.
 * @param saved The contents.
 * @return Whether the file was written.
 */
bool WriteSaved(const std::filesystem::path &file, const Saved &saved);

/**
 * The choices and scores kept in one file across a session: read once as the app starts, then told the controller's
 * each frame, writing the file whenever they differ from those it last saw - never for what it read.
 */
class SettingsKeeper
{
  public:
    /** @param file The file; empty to read the defaults and keep nothing (a launch's check). */
    explicit SettingsKeeper(std::filesystem::path file) : file_(std::move(file))
    {
    }

    /** @return What the file keeps (ReadSaved), which is then what it has seen. */
    Saved Read();

    /**
     * The contents now: written to the file when they differ from those last seen.
     * @param now The contents now.
     * @return Whether the choices differ - the first call always - so the menu's ticks can follow them.
     */
    bool Changed(const Saved &now);

    /** @return How many times the file has been written. */
    int writes() const noexcept
    {
        return writes_;
    }

  private:
    std::filesystem::path file_;                ///< The file.
    std::optional<Saved>  seen_;                ///< The contents last read or seen; none before either.
    bool                  has_reported_{false}; ///< Changed has been called.
    int                   writes_{0};           ///< The file's writes.
};

} // namespace knect4::app
