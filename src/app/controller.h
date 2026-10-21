#pragma once
/// @file controller.h The game being played: whose turn it is, the computer's search on a thread of its own, the held
/// puck's glide, the drop and its clicks, the game's end and the score - advanced once a frame and read by the views.

#include "board_picture.h"

#include "effects/click_sound.h"
#include "effects/drop_motion.h"
#include "game/game.h"

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace knect4::app
{

/** How hard the computer plays: how far it searches. */
enum class Level
{
    kEasy,   ///< Three plies beyond its move.
    kMedium, ///< Seven.
    kHard,   ///< Eleven.
};

/**
 * @param level A level.
 * @return The depth the computer searches at it: 3, 7 or 11, the original's.
 */
int DepthOf(Level level) noexcept;

/** What the person has chosen. */
struct Settings
{
    bool  is_two_player{false};    ///< Two people play each other; else the person plays the computer.
    Level level{Level::kMedium};   ///< How hard the computer plays.
    bool  is_person_red{true};     ///< The person's pucks are red, the computer's yellow; else the other way round.
    bool  does_person_start{true}; ///< The person (between two people, Red) drops the first puck of each new game.
    bool  has_sound{true};         ///< Each landing clicks.

    /**
     * @param other Other choices.
     * @return Whether they are the same.
     */
    bool operator==(const Settings &other) const = default;
};

/** The games so far, of one kind: against the computer, or between two people. */
struct Score
{
    int min_wins{0}; ///< Games kMin won: the person against the computer, Red between two people.
    int max_wins{0}; ///< Games kMax won: the computer, or Yellow.
    int drawn{0};    ///< Games drawn.
};

/** Where a game stands. */
enum class Phase
{
    kPersonToMove,     ///< A person chooses a column: the person, or between two people the one whose turn it is.
    kComputerThinking, ///< The computer searches.
    kComputerAiming,   ///< Its puck glides to the column it chose.
    kDropping,         ///< A puck falls.
    kWon,              ///< The game is over: one side made four (winner()).
    kDrawn,            ///< The board filled.
};

/** Where a landing's click goes. */
class ClickSink
{
  public:
    virtual ~ClickSink() = default;

    /**
     * A puck has started to fall and will land in a moment: the place to make ready to sound, which a fall gives
     * time for. Nothing by default.
     */
    virtual void Prepare()
    {
    }

    /**
     * A puck has landed.
     * @param landing What on.
     * @param gain How loud, 0 to 1, by how fast it hit.
     */
    virtual void Click(LandingOn landing, float gain) = 0;
};

/**
 * The game: the person kMin, the computer kMax, as in the original; between two people Red is kMin and Yellow kMax,
 * each dropping in turn with the same hand the person uses against the computer. Frame() moves it on - the search
 * polled, the held puck glided, the falling puck moved and its landings clicked - and the views read Picture() and the
 * rest.
 *
 * The computer's turn is paced for a person to follow: its puck appears above the board and sways while it thinks,
 * waits at least kLeastThinking whatever the search took, then glides to its column and drops.
 */
class Controller
{
  public:
    static constexpr double kLeastThinking = 0.45; ///< The least time the computer's turn takes before it aims, s.

    /**
     * A first game, begun at once.
     * @param clicks Where the landings' clicks go; may be null for none.
     * @param settings The person's choices.
     */
    explicit Controller(ClickSink *clicks, Settings settings = {});

    /** A search still running is stopped and waited for. */
    ~Controller();

    Controller(const Controller &)            = delete;
    Controller &operator=(const Controller &) = delete;

    /** A new game from an empty board, whoever starts by the settings; a game being played is abandoned unscored. */
    void NewGame();

    /**
     * The game moved on by `seconds`.
     * @param seconds The time since the last frame.
     */
    void Frame(double seconds);

    /**
     * The pointer is over a column: the person's held puck follows it.
     * @param column The column, or -1 off the board.
     */
    void Hover(int column);

    /**
     * The person drops a puck into `column`, if it is their turn and the column has room.
     * @param column The column.
     * @return Whether the puck dropped.
     */
    bool Drop(int column);

    /**
     * The person's held puck a column over, to the nearest column with room that way; where none that way has room it
     * stays where it is. It never wraps round the board's edge.
     * @param delta -1 for left, 1 for right.
     */
    void Step(int delta);

    /** @return Whether the person's held puck dropped where it hangs. */
    bool DropHeld();

    /**
     * A new game set up from moves already played, each puck put at rest with no fall and no click, the side that
     * starts by the settings first; then the game goes on as after the last one landed - won, drawn, or the other
     * side's turn. For the render tool and the suites.
     * @param columns The columns, in order; each must have room.
     */
    void PlaceMoves(const std::vector<int> &columns);

    /** @param level How hard the computer plays from its next search on. */
    void SetLevel(Level level);

    /** @param is_person_red Whether the person's pucks are red; the pucks on the board change at once. */
    void SetPersonRed(bool is_person_red);

    /** @param does_person_start Whether the person starts the next new game. */
    void SetPersonStarts(bool does_person_start);

    /** @param has_sound Whether landings click. */
    void SetSound(bool has_sound);

    /**
     * Against the computer or between two people; a change begins a new game.
     * @param is_two_player Whether two people play each other.
     */
    void SetTwoPlayer(bool is_two_player);

    /**
     * @param player kMin or kMax.
     * @return The colours that side's pucks are drawn in.
     */
    const theme::PuckColours &ColoursOf(int player) const noexcept;

    /** @return The board as it should be drawn now. */
    BoardPicture Picture() const;

    /** @return Where the game stands. */
    Phase phase() const noexcept
    {
        return phase_;
    }

    /** @return Whether the game is over. */
    bool IsOver() const noexcept
    {
        return phase_ == Phase::kWon || phase_ == Phase::kDrawn;
    }

    /** @return Who made four, kMin or kMax, once the game is won; kEmpty before then and in a draw. */
    int winner() const noexcept
    {
        return phase_ == Phase::kWon ? falling_player_ : kEmpty;
    }

    /** @return Whose puck a person holds now: kMin, or between two people whichever side's turn it is. */
    int mover() const noexcept
    {
        return mover_;
    }

    /**
     * @return The game's state in a few words: "Your move", "Thinking…", "You win!"; between two people "Red to
     *         move", "Yellow wins!".
     */
    std::string StatusText() const;

    /** @return The person's choices. */
    const Settings &settings() const noexcept
    {
        return settings_;
    }

    /** @return The games so far of the kind being played: against the computer, or between two people. */
    const Score &score() const noexcept
    {
        return settings_.is_two_player ? two_player_score_ : score_;
    }

    /** @return The board. */
    const Game &game() const noexcept
    {
        return game_;
    }

    /** @return How many pucks have been dropped this game, the falling one included. */
    int MovesMade() const noexcept
    {
        return moves_made_;
    }

  private:
    /** The computer's turn begun: its puck shown, its search started on a thread of its own. */
    void BeginComputerTurn();

    /**
     * A person's turn begun.
     * @param player Whose puck they hold: kMin, or between two people either.
     */
    void BeginPersonTurn(int player);

    /** @return The score of the kind of game being played, to count a result in. */
    Score &CurrentScore() noexcept;

    /** @return How far the thinking computer's puck sways from where it hangs now, in columns. */
    float Sway() const noexcept;

    /**
     * A puck of `player`'s dropped into `column` from where the held puck hangs.
     * @param column The column, with room.
     * @param player Whose.
     */
    void StartDrop(int column, int player);

    /** The falling puck at rest: the game won, drawn, or the other side's turn. */
    void Landed();

    /** A search running is stopped and waited for. */
    void StopSearch();

    /**
     * The first column with room from `from` on, `delta` at a time, up to the board's edge.
     * @param from Where to begin.
     * @param delta The step, -1 or 1.
     * @return The column, or -1 where none that way has room.
     */
    int OpenColumnFrom(int from, int delta) const;

    /**
     * The column with room nearest `column`, the right one first of two as near; `column` itself where it has room or
     * none has.
     * @param column A column on the board.
     * @return The column.
     */
    int NearestOpenColumn(int column) const;

    ClickSink *clicks_;                      ///< Where the clicks go; may be null.
    Settings   settings_;                    ///< The person's choices.
    Score      score_;                       ///< The games against the computer so far.
    Score      two_player_score_;            ///< The games between two people so far.
    Game       game_;                        ///< The board, with the falling puck already in it.
    Phase      phase_{Phase::kPersonToMove}; ///< Where the game stands.
    int        moves_made_{0};               ///< Pucks dropped this game.
    int        mover_{kMin};                 ///< Whose puck a person holds.

    float held_column_{3.0f};    ///< Where the held puck hangs, gliding toward its target.
    int   held_target_{3};       ///< The column it glides to.
    bool  is_held_shown_{false}; ///< The person's held puck shows: the pointer is over the board, or a key moved it.
    float held_opacity_{0.0f};   ///< The held puck fading in.
    int   computer_column_{3};   ///< Where the computer's puck appears, the column it last played.

    std::optional<DropMotion> motion_;               ///< The falling puck's fall.
    int                       falling_column_{0};    ///< Its column.
    int                       falling_player_{kMin}; ///< Whose.

    std::shared_ptr<std::atomic<bool>> stop_;           ///< The running search's stop.
    std::future<int>                   search_;         ///< The running search's column.
    int                                chosen_{-1};     ///< The column the computer chose.
    double                             turn_time_{0.0}; ///< Seconds since the computer's turn began.

    std::vector<Cell> winning_;        ///< The winning line, once there is one.
    double            glow_time_{0.0}; ///< Seconds since the game ended, for the glow's pulse.
};

} // namespace knect4::app
