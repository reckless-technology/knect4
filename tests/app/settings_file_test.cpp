#include "app/settings_file.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace knect4::app
{
namespace
{

/** A folder of the test's own under the system's temporary folder, removed at the end. */
class SettingsFileTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        folder_ = std::filesystem::temp_directory_path() / std::filesystem::path(u8"knect4_settings_été") /
                  std::to_string(std::random_device{}());
        std::filesystem::create_directories(folder_);
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove_all(folder_, error);
    }

    /**
     * A file in the folder holding `text`.
     * @param text Its content.
     * @return Its path.
     */
    std::filesystem::path Holding(const std::string &text) const
    {
        const std::filesystem::path file = folder_ / "settings.json";
        std::ofstream(file, std::ios::binary) << text;
        return file;
    }

    std::filesystem::path folder_; ///< The test's folder, its name not ASCII.
};

TEST_F(SettingsFileTest, KeepsEveryChoiceAcrossAWriteAndARead)
{
    Settings settings;
    settings.is_two_player     = true;
    settings.level             = Level::kHard;
    settings.is_person_red     = false;
    settings.does_person_start = false;
    settings.has_sound         = false;
    settings.does_alternate    = false;
    settings.has_marks         = true;
    settings.board             = {16, 8};
    settings.red_name          = "\u00c9lodie";
    settings.yellow_name       = "Sam";
    settings.window_percent    = 125;
    Scores scores;
    scores.computer                  = {3, 4, 1};
    scores.two_player                = {7, 2, 0};
    const std::filesystem::path file = folder_ / "nested" / "settings.json";
    ASSERT_TRUE(WriteSaved(file, {settings, scores}));
    EXPECT_EQ(ReadSaved(file), (Saved{settings, scores}));

    settings.level = Level::kPerfect;
    ASSERT_TRUE(WriteSaved(file, {settings, scores}));
    EXPECT_EQ(ReadSaved(file).settings, settings);
    settings.level = Level::kEasy;
    ASSERT_TRUE(WriteSaved(file, {settings, scores}));
    EXPECT_EQ(ReadSaved(file).settings, settings);
}

TEST_F(SettingsFileTest, ReadsTheDefaultsWhereThereIsNothingToRead)
{
    EXPECT_EQ(ReadSaved(folder_ / "missing.json"), Saved{});
    EXPECT_EQ(ReadSaved({}), Saved{});
    EXPECT_EQ(ReadSaved(Holding("not json {")), Saved{});
    EXPECT_EQ(ReadSaved(Holding("[1, 2]")), Saved{});
    EXPECT_FALSE(WriteSaved({}, Saved{}));
}

TEST_F(SettingsFileTest, TakesTheDefaultForAChoiceThatMakesNoSense)
{
    const Settings read =
        ReadSaved(
            Holding(
                R"({"opponent": "robot", "level": "impossible", "your_colour": 7, "first_move": "computer", "sound": "loud",
            "board": "5x5", "window_percent": 300, "scores": {"computer": {"first": -4, "second": "x"}}})"))
            .settings;
    EXPECT_FALSE(read.is_two_player);
    EXPECT_EQ(read.level, Level::kMedium);
    EXPECT_TRUE(read.is_person_red);
    EXPECT_FALSE(read.does_person_start);
    EXPECT_EQ(read.board, kStandardBoard);
    EXPECT_EQ(read.window_percent, 100);
    EXPECT_TRUE(read.has_sound);
}

TEST(SettingsFile, LivesInTheSuitesFolder)
{
    const std::filesystem::path file = DefaultSettingsFile();
    if (!file.empty())
    {
        EXPECT_EQ(file.filename(), "settings.json");
        EXPECT_EQ(file.parent_path().filename(), "knect4");
        EXPECT_EQ(file.parent_path().parent_path().filename(), "reckless-technology");
    }
}

} // namespace
} // namespace knect4::app

namespace knect4::app
{
namespace
{

TEST_F(SettingsFileTest, KeepsTheChoicesAsTheyChangeAndOnlyThen)
{
    const std::filesystem::path file = folder_ / "kept.json";
    Settings                    earlier;
    earlier.level = Level::kHard;
    ASSERT_TRUE(WriteSaved(file, {earlier, {}}));

    SettingsKeeper keeper(file);
    Saved          now = keeper.Read();
    EXPECT_EQ(now.settings, earlier);
    // The first call reports a change, for the menu to be set, but the choices just read need no writing.
    EXPECT_TRUE(keeper.Changed(now));
    EXPECT_EQ(keeper.writes(), 0);
    EXPECT_FALSE(keeper.Changed(now));
    now.settings.has_sound = false;
    EXPECT_TRUE(keeper.Changed(now));
    EXPECT_EQ(keeper.writes(), 1);
    EXPECT_FALSE(ReadSaved(file).settings.has_sound);
    EXPECT_FALSE(keeper.Changed(now));
    EXPECT_EQ(keeper.writes(), 1);
    // A score changing is kept, but is no change of choices for the menu.
    now.scores.computer.min_wins = 1;
    EXPECT_FALSE(keeper.Changed(now));
    EXPECT_EQ(keeper.writes(), 2);
    EXPECT_EQ(ReadSaved(file).scores.computer.min_wins, 1);
}

TEST(SettingsKeeper, KeepsNothingWithNoFile)
{
    // A launch's check: the defaults, a change reported for the menu, nothing written anywhere.
    SettingsKeeper keeper({});
    Saved          now = keeper.Read();
    EXPECT_EQ(now, Saved{});
    EXPECT_TRUE(keeper.Changed(now));
    now.settings.level = Level::kEasy;
    EXPECT_TRUE(keeper.Changed(now));
    EXPECT_EQ(keeper.writes(), 0);

    // Never read: the first choices seen are reported and not written.
    SettingsKeeper unread({});
    EXPECT_TRUE(unread.Changed(Saved{}));
    EXPECT_FALSE(unread.Changed(Saved{}));
}

} // namespace
} // namespace knect4::app
