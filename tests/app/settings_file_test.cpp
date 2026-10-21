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
    settings.level                   = Level::kHard;
    settings.is_person_red           = false;
    settings.does_person_start       = false;
    settings.has_sound               = false;
    const std::filesystem::path file = folder_ / "nested" / "settings.json";
    ASSERT_TRUE(WriteSettings(file, settings));
    EXPECT_EQ(ReadSettings(file), settings);

    settings.level = Level::kEasy;
    ASSERT_TRUE(WriteSettings(file, settings));
    EXPECT_EQ(ReadSettings(file), settings);
}

TEST_F(SettingsFileTest, ReadsTheDefaultsWhereThereIsNothingToRead)
{
    EXPECT_EQ(ReadSettings(folder_ / "missing.json"), Settings{});
    EXPECT_EQ(ReadSettings({}), Settings{});
    EXPECT_EQ(ReadSettings(Holding("not json {")), Settings{});
    EXPECT_EQ(ReadSettings(Holding("[1, 2]")), Settings{});
    EXPECT_FALSE(WriteSettings({}, Settings{}));
}

TEST_F(SettingsFileTest, TakesTheDefaultForAChoiceThatMakesNoSense)
{
    const Settings read = ReadSettings(
        Holding(R"({"level": "impossible", "your_colour": 7, "first_move": "computer", "sound": "loud"})"));
    EXPECT_EQ(read.level, Level::kMedium);
    EXPECT_TRUE(read.is_person_red);
    EXPECT_FALSE(read.does_person_start);
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
