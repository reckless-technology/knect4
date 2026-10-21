// The standard board's opening book, worked out by the solver: for each side the computer may play, every position it
// can meet up to a depth while it plays the book's moves and the person plays anything, and the best column there -
// written as src/game/opening_book_data.inc, sorted by key, mirror images folded together. The empty board's move is
// the centre, which Victor Allis (1988) and James Allen (1988) proved the first player's only winning start; solving it
// here would take hours, so it is written in rather than worked out.
//
//   knect4_make_book <out.inc> [first-depth] [second-depth] [threads]
//
// first-depth: the last ply the computer moves at when it starts (even; 6 by default). second-depth: when the person
// starts (odd; 5 by default). Beyond them the solver finds the answer in the time a move is allowed.

#include "game/opening_book.h"
#include "game/position.h"
#include "game/search.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{

using knect4::OpeningBook;
using knect4::Position;
using knect4::Search;

/** A position to answer, as the moves to it. */
using Moves = std::vector<int>;

/**
 * The best column in the position after `moves`, by the solver: the position's score, then the first move, centre
 * first, whose position scores the same from the mover's side.
 * @param search The solver.
 * @param moves The moves; the game is not over and the side to move has a move.
 * @return The column.
 */
int BestColumn(Search<Position> &search, const Moves &moves)
{
    const Position position = Position::FromMoves(moves);
    const int      score    = *search.Solve(position, 1.0e9);
    for (int index = 0; index != Position::kWidth; ++index)
    {
        const int column = Position::kWidth / 2 + (1 - 2 * (index % 2)) * (index + 1) / 2;
        if (!position.CanPlay(column))
        {
            continue;
        }
        if (position.IsWinningMove(column))
        {
            if (score == (Position::kSlots + 1 - position.moves()) / 2)
            {
                return column;
            }
            continue;
        }
        Position child = position;
        child.Play(column);
        if (-*search.Solve(child, 1.0e9) == score)
        {
            return column;
        }
    }
    return -1; // never: some move keeps the position's score
}

} // namespace

/**
 * Work the book out and write it.
 * @param argc The argument count.
 * @param argv The arguments.
 * @return 0 when it was written, 1 otherwise, 2 for a usage error.
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: knect4_make_book <out.inc> [first-depth] [second-depth] [threads]\n");
        return 2;
    }
    const int first_depth  = argc > 2 ? std::atoi(argv[2]) : 6;
    const int second_depth = argc > 3 ? std::atoi(argv[3]) : 5;
    const int threads =
        argc > 4 ? std::atoi(argv[4]) : static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));

    std::map<std::uint64_t, int> book; // canonical key -> column in the canonical position
    std::mutex                   guard;
    const auto                   started = std::chrono::steady_clock::now();
    const auto                   record  = [&](const Moves &moves, int column) {
        const auto [key, is_mirrored] = OpeningBook::CanonicalKey(moves);
        const std::lock_guard lock(guard);
        book[key] = is_mirrored ? Position::kWidth - 1 - column : column;
    };

    // The computer starting: the centre, then each person's answer, level by level.
    record({}, Position::kWidth / 2);
    for (const bool does_computer_start : {true, false})
    {
        std::vector<Moves> level;
        int                ply = does_computer_start ? 2 : 1;
        if (does_computer_start)
        {
            for (int reply = 0; reply != Position::kWidth; ++reply)
            {
                level.push_back({Position::kWidth / 2, reply});
            }
        }
        else
        {
            for (int opening = 0; opening != Position::kWidth; ++opening)
            {
                level.push_back({opening});
            }
        }
        const int last = does_computer_start ? first_depth : second_depth;
        for (; ply <= last && !level.empty(); ply += 2)
        {
            // One position of each mirror pair, and of each order the moves came in.
            std::map<std::uint64_t, Moves> distinct;
            for (const Moves &moves : level)
            {
                distinct.emplace(OpeningBook::CanonicalKey(moves).first, moves);
            }
            std::vector<Moves> work;
            for (auto &[key, moves] : distinct)
            {
                const Position position = Position::FromMoves(moves);
                if (!position.CanWinNext() && knect4::Any(position.PossibleNonLosingMoves()))
                {
                    work.push_back(moves);
                }
            }
            std::printf("%s ply %d: %zu positions\n", does_computer_start ? "computer first," : "person first,", ply,
                        work.size());
            std::fflush(stdout);
            std::atomic<std::size_t> next{0};
            std::vector<Moves>       answered(work.size());
            std::vector<int>         columns(work.size(), -1);
            std::vector<std::thread> pool;
            for (int index = 0; index != threads; ++index)
            {
                pool.emplace_back([&] {
                    Search<Position> search(1);
                    for (std::size_t item; (item = next.fetch_add(1)) < work.size();)
                    {
                        columns[item] = BestColumn(search, work[item]);
                        record(work[item], columns[item]);
                    }
                });
            }
            for (std::thread &thread : pool)
            {
                thread.join();
            }
            // The next level: the book's move, then every reply that leaves the game open.
            std::vector<Moves> next_level;
            for (std::size_t item = 0; item != work.size(); ++item)
            {
                Moves after = work[item];
                after.push_back(columns[item]);
                const Position position = Position::FromMoves(after);
                if (Position::HasFour(position.current() ^ position.mask()))
                {
                    continue; // the book's move won
                }
                for (int reply = 0; reply != Position::kWidth; ++reply)
                {
                    if (position.CanPlay(reply) && !position.IsWinningMove(reply))
                    {
                        Moves next_moves = after;
                        next_moves.push_back(reply);
                        next_level.push_back(next_moves);
                    }
                }
            }
            level                = std::move(next_level);
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            std::printf("  done, %zu in the book, %.0f s so far\n", book.size(), seconds);
            std::fflush(stdout);
        }
    }

    std::ofstream file(argv[1], std::ios::binary);
    file << "// The standard board's opening book: {key, column}, sorted by key. Written by tools/make_book.cpp ("
         << "knect4_make_book, depths " << first_depth << " and " << second_depth << "); not edited by hand.\n";
    for (const auto &[key, column] : book)
    {
        char line[64];
        std::snprintf(line, sizeof line, "{0x%013llxu, %d},\n", static_cast<unsigned long long>(key), column);
        file << line;
    }
    if (!file)
    {
        std::fprintf(stderr, "knect4_make_book: could not write %s\n", argv[1]);
        return 1;
    }
    std::printf("%s: %zu positions\n", argv[1], book.size());
    return 0;
}
