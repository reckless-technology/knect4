#pragma once
/// @file controller.h The game being played: whose turn it is, the computer's search and the hint on a thread of their
/// own, the held puck's glide, the drop and its clicks, taking moves back, the game's end, the pucks let out for the
/// next, and the scores - advanced once a frame and read by the views.

#include "board_art.h"
#include "board_picture.h"

#include "effects/click_sound.h"
#include "effects/drop_motion.h"
#include "game/engine.h"
#include "game/game.h"

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace knect4::app
{

/** How well the computer plays. */
enum class Level
{
    kEasy,    ///< Looks two plies ahead and plays loosely: easy to beat.
    kMedium,  ///< Six plies on a judgement of threats.
    kHard,    ///< Up to twelve plies.
    kPerfect, ///< The exact solution wherever it can be found in time.
};

/**
 * @param level A level.
 * @return The engine's strength for it.
 */
Strength StrengthOf(Level level) noexcept;

/** What the person has chosen. */
struct Settings
{
    bool        is_two_player{false};    ///< Two people play each other; else the person plays the computer.
    Level       level{Level::kMedium};   ///< How well the computer plays.
    bool        is_person_red{true};     ///< The person's pucks are red, the computer's yellow; else the other way.
    bool        does_person_start{true}; ///< The person (between two people, Red) drops the next game's first puck.
    bool        does_alternate{true};    ///< Between two people, the first move passes to the other each new game.
    bool        has_sound{true};         ///< Each landing clicks.
    bool        has_marks{false};        ///< The pucks carry their side's mark, a cross or a star.
    BoardSize   board{kStandardBoard};   ///< The board's size, one of kBoardSizes.
    std::string red_name{"Red"};         ///< Red's name between two people.
    std::string yellow_name{"Yellow"};   ///< Yellow's.
    int         window_percent{100};     ///< The window's size, a percentage of its design size; 0 for none chosen.

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

    /** @param other Another score. @return The two the same. */
    bool operator==(const Score &other) const = default;
};

/** The games so far of both kinds, kept between launches. */
struct Scores
{
    Score computer;   ///< Against the computer.
    Score two_player; ///< Between two people.

    /** @param other Other scores. @return The two the same. */
    bool operator==(const Scores &other) const = default;
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
    kEmptying,         ///< The last game's pucks fall out through the slider before the next begins.
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
 * The game: the person kMin, the computer kMax; between two people Red is kMin and Yellow kMax, each dropping in turn
 * with the same hand. Frame() moves it on - a search polled, the held puck glided, the falling puck moved and its
 * landings clicked, the last game's pucks let out - and the views read Picture() and the rest.
 *
 * The computer's turn is paced for a person to follow: its puck appears above the board and sways while it thinks,
 * waits at least kLeastThinking whatever the search took, then glides to its column and drops. The engine is made the
 * first time a search needs it, for the board's size.
 */
class Controller
{
  public:
    static constexpr double kLeastThinking = 0.45; ///< The least time the computer's turn takes before it aims, s.
    static constexpr double kHintSeconds   = 2.0;  ///< The time a hint is searched for.

    /**
     * A first game, begun at once.
     * @param clicks Where the landings' clicks go; may be null for none.
     * @param settings The person's choices.
     * @param scores The games so far.
     */
    explicit Controller(ClickSink *clicks, Settings settings = {}, Scores scores = {});

    /** A search still running is stopped and waited for. */
    ~Controller();

    Controller(const Controller &)            = delete;
    Controller &operator=(const Controller &) = delete;

    /**
     * A new game, whoever starts by the settings (between two people with alternation, the other of last game); a
     * game being played is abandoned unscored. Pucks on the board first fall out through the slider (kEmptying).
     */
    void NewGame();

    /**
     * The game moved on by `seconds`.
     * @param seconds The time since the last frame.
     */
    void Frame(double seconds);

    /**
     * The pointer is over a column: the held puck follows it.
     * @param column The column, or -1 off the board.
     */
    void Hover(int column);

    /**
     * A person drops a puck into `column`, if it is a person's turn and the column has room.
     * @param column The column.
     * @return Whether the puck dropped.
     */
    bool Drop(int column);

    /**
     * The held puck a column over, to the nearest column with room that way; where none that way has room it stays
     * where it is. It never wraps round the board's edge.
     * @param delta -1 for left, 1 for right.
     */
    void Step(int delta);

    /** @return Whether the held puck dropped where it hangs. */
    bool DropHeld();

    /**
     * The last move taken back - against the computer, the person's last move and the computer's answer to it - and
     * a result it gave unscored. Not while a puck falls or the board empties.
     * @return Whether anything was taken back.
     */
    bool Undo();

    /** @return Whether Undo would take anything back now. */
    bool CanUndo() const noexcept;

    /**
     * A hint asked for: the best move for the side to move, searched for kHintSeconds on a thread of its own and shown
     * on the board once found (hint_column()), until a move is made or taken back.
     * @return Whether a search began.
     */
    bool AskHint();

    /** @return Whether a hint can be asked for now: a person's turn, none showing or being searched for. */
    bool CanHint() const noexcept;

    /** @return Whether a hint is being searched for. */
    bool IsHintPending() const noexcept;

    /** @return The column the hint suggests, or -1 for none. */
    int hint_column() const noexcept
    {
        return hint_column_;
    }

    /**
     * A new game set up from moves already played, each puck put at rest with no fall and no click, the side that
     * starts by the settings first; then the game goes on as after the last one landed - won, drawn, or the other
     * side's turn. For the render tool and the suites.
     * @param columns The columns, in order; a column with no room is left out.
     */
    void PlaceMoves(const std::vector<int> &columns);

    /** @param level How well the computer plays from its next search on. */
    void SetLevel(Level level);

    /** @param is_person_red Whether the person's pucks are red; the pucks on the board change at once. */
    void SetPersonRed(bool is_person_red);

    /** @param does_person_start Whether the person (between two people, Red) starts the next new game. */
    void SetPersonStarts(bool does_person_start);

    /** @param does_alternate Whether, between two people, the first move passes to the other each new game. */
    void SetAlternate(bool does_alternate);

    /** @param has_sound Whether landings click. */
    void SetSound(bool has_sound);

    /** @param has_marks Whether the pucks carry their side's mark. */
    void SetMarks(bool has_marks);

    /**
     * Against the computer or between two people; a change begins a new game.
     * @param is_two_player Whether two people play each other.
     */
    void SetTwoPlayer(bool is_two_player);

    /**
     * The board's size; a change begins a new game on the new board at once.
     * @param size One of kBoardSizes; another is ignored.
     */
    void SetBoardSize(BoardSize size);

    /**
     * The players' names between two people; an empty name is the colour's.
     * @param red_name Red's.
     * @param yellow_name Yellow's.
     */
    void SetNames(std::string red_name, std::string yellow_name);

    /** @param percent The window's size, a percentage of its design size. */
    void SetWindowPercent(int percent);

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
     *         move", "Yellow wins!" with the players' names.
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
        return settings_.is_two_player ? scores_.two_player : scores_.computer;
    }

    /** @return The games so far of both kinds. */
    const Scores &scores() const noexcept
    {
        return scores_;
    }

    /** @return The board. */
    const Game &game() const noexcept
    {
        return game_;
    }

    /** @return The board's layout. */
    const art::Layout &layout() const noexcept
    {
        return layout_;
    }

    /** @return The columns played this game, in order, the falling puck's included. */
    const std::vector<int> &moves() const noexcept
    {
        return moves_;
    }

    /** @return How many pucks have been dropped this game, the falling one included. */
    int MovesMade() const noexcept
    {
        return static_cast<int>(moves_.size());
    }

  private:
    /** A puck of the last game, falling out through the slider. */
    struct OutgoingPuck
    {
        int column{0};    ///< Its column.
        int row{0};       ///< Its slot's row.
        int player{kMin}; ///< Whose.
    };

    /** What the running search is for. */
    enum class SearchKind
    {
        kNone, ///< None runs.
        kMove, ///< The computer's move.
        kHint, ///< A hint for a person.
    };

    /** The game begun on an empty board: its first turn. */
    void StartGame();

    /** The computer's turn begun: its puck shown, its search started on a thread of its own. */
    void BeginComputerTurn();

    /**
     * A person's turn begun.
     * @param player Whose puck they hold: kMin, or between two people either.
     */
    void BeginPersonTurn(int player);

    /**
     * A puck of `player`'s dropped into `column` from where the held puck hangs.
     * @param column The column, with room.
     * @param player Whose.
     */
    void StartDrop(int column, int player);

    /** The falling puck at rest: the game won, drawn, or the other side's turn. */
    void Landed();

    /** A search running is stopped and waited for, its answer thrown away. */
    void StopSearch();

    /** @return The engine for the board's size, made the first time it is needed. */
    Engine &EngineForBoard();

    /** The board's game and layout made afresh at the settings' size. */
    void MakeBoard();

    /**
     * @param index A move's place in the game, from 0.
     * @return Whose move it was.
     */
    int PlayerOfMove(int index) const noexcept
    {
        return index % 2 == 0 ? first_player_ : -first_player_;
    }

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

    /** @return The score of the kind of game being played, to count a result in. */
    Score &CurrentScore() noexcept;

    /** @return How far the thinking computer's puck sways from where it hangs now, in columns. */
    float Sway() const noexcept;

    /**
     * @param player kMin or kMax.
     * @return The player's name between two people: the name given, or the colour's.
     */
    std::string NameOf(int player) const;

    ClickSink       *clicks_;                      ///< Where the clicks go; may be null.
    Settings         settings_;                    ///< The person's choices.
    Scores           scores_;                      ///< The games so far.
    Game             game_;                        ///< The board, with the falling puck already in it.
    art::Layout      layout_;                      ///< Where the board's slots stand.
    std::vector<int> moves_;                       ///< The columns played this game.
    int              first_player_{kMin};          ///< Who played this game's first puck.
    Phase            phase_{Phase::kPersonToMove}; ///< Where the game stands.
    int              mover_{kMin};                 ///< Whose puck a person holds.
    bool             does_next_alternate_{false};  ///< The next game follows one played: alternation applies.

    float held_column_{3.0f};    ///< Where the held puck hangs, gliding toward its target.
    int   held_target_{3};       ///< The column it glides to.
    bool  is_held_shown_{false}; ///< The held puck shows: the pointer is over the board, or a key moved it.
    bool  is_keyboard_{false};   ///< The keys moved it last, not the pointer.
    float held_opacity_{0.0f};   ///< The held puck fading in.
    int   computer_column_{3};   ///< Where the computer's puck appears, the column it last played.

    std::optional<DropMotion> motion_;               ///< The falling puck's fall.
    int                       falling_column_{0};    ///< Its column.
    int                       falling_player_{kMin}; ///< Whose.

    std::unique_ptr<Engine>            engine_; ///< The computer's play, made when first needed.
    std::shared_ptr<std::atomic<bool>> stop_;   ///< The running search's stop.
    std::future<int>                   search_; ///< The running search's column.
    SearchKind                         search_kind_{SearchKind::kNone}; ///< What it is for.
    int                                chosen_{-1};                     ///< The column the computer chose.
    double                             turn_time_{0.0};                 ///< Seconds since the computer's turn began.
    int                                hint_column_{-1};                ///< The hint's column; -1 for none.

    std::vector<Cell> winning_;        ///< The winning line, once there is one.
    double            glow_time_{0.0}; ///< Seconds since the game ended, for the glow's pulse.

    std::vector<OutgoingPuck> outgoing_;           ///< The last game's pucks, falling out.
    double                    emptying_time_{0.0}; ///< Seconds since they were let go.
    std::vector<bool>         has_clattered_;      ///< Each column's pucks have hit what is under the board.
};

} // namespace knect4::app
