using System;
using System.Collections.Generic;
using System.Text;

namespace ConnectFour
{
    class Game
    {
        // Values to represent the MAX and MIN players pieces on the board
        public const int MAX = 1;
        public const int MIN = -1;

        // Value to represent an empty slot on the Board
        public const int EMPTY = 0;

        // Used to switch players during recursive search
        private const int XOR = MAX ^ MIN;

        // Default board size
        private const int DEFAULT_WIDTH = 7;
        private const int DEFAULT_HEIGHT = 6;

        // SCORES[n] holds the perceived value associated with  
        // obtaining n squares in a row in any direction.
        // The ratio of SCORES[3] : SCORES[2] affects the
        // playing style of the game, due to its effect
        // on the board evaluation heuristic.
        // These values have been empirically determined.
        private static readonly int[] SCORES = { 0, 1, 13, 37, 104729 };
        private static readonly int WINNING_SCORE = SCORES[4];

        // Initial values of alpha and beta.
        // NB: Do NOT set alpha to int.MinValue as in two's complement
        // arithmetic, -int.MinValue == int.MinValue, which would screw up
        // beta value during recursive search. This took me quite some time to debug.
        private const int ALPHA_INITIAL = int.MinValue + 1;
        private const int BETA_INITIAL = -ALPHA_INITIAL;

        // Member variables
        private int[,] theBoard;    // [0,0] is bottom left
        private int[] columnHeight;
        public readonly int width, height;

        public Game(int width, int height)
        {
            this.width = width;
            this.height = height;
            theBoard = new int[width, height];
            columnHeight = new int[width];
            for (int x = 0; x != width; x++)
            {
                columnHeight[x] = 0;
                for (int y = 0; y != height; y++)
                {
                    theBoard[x, y] = EMPTY;
                }
            }
        }

        public void Reset()
        {
            for (int x = 0; x != width; x++)
            {
                columnHeight[x] = 0;
                for (int y = 0; y != height; y++)
                {
                    theBoard[x, y] = EMPTY;
                }
            }
        }

        public Game()
            : this(DEFAULT_WIDTH, DEFAULT_HEIGHT)
        {
        }

        public bool IsMovePossible(int column)
        {
            return (columnHeight[column] != height);
        }

        public void DoMove(int column, int player)
        {
            theBoard[column, columnHeight[column]] = player;
            columnHeight[column]++;
        }

        public int[,] GetBoard()
        {
            int[,] board = new int[width, height];
            Array.Copy(theBoard, board, width * height);
            return board;
        }


        public bool IsWinningMove(int column, int player)
        {
            return (EvaluateMove(column, true, player) >= WINNING_SCORE);
        }

        public bool IsDraw(int column, int player)
        {
            for (int x = 0; x != width; x++)
            {
                if (IsMovePossible(x))
                {
                    return false;
                }
            }
            return (!IsWinningMove(column, player));
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();
            for (int y = height - 1; y >= 0; y--)
            {
                result.Append('|');
                for (int x = 0; x != width; x++)
                {
                    switch (theBoard[x, y])
                    {
                    case EMPTY:
                        result.Append(' ');
                        break;
                    case MAX:
                        result.Append('X');
                        break;
                    case MIN:
                        result.Append('O');
                        break;
                    default:
                        throw new ApplicationException(String.Format
                            ("Illegal board value {0} at {1},{2}", theBoard[x, y], x, y));
                    }
                    result.Append('|');
                }
                result.Append('\n');
            }
            for (int x = 0; x <= 2 * width; x++)
            {
                result.Append('_');
            }
            result.Append('\n');
            return result.ToString();
        }

        public int GetNextMove(int depth, int player, ref int bestMove)
        {
            lock (this)
            {
                return Negamax(depth, ALPHA_INITIAL, BETA_INITIAL, true, player, ref bestMove);
            }
        }

        private void UndoMove(int column)
        {
            columnHeight[column]--;
            theBoard[column, columnHeight[column]] = EMPTY;
        }

        private int EvaluateMove(int column, bool hasMoveBeenMade, int player)
        {
            int length, x, y;
            int row = hasMoveBeenMade ? columnHeight[column] - 1 : columnHeight[column];
            int score = 0;

            // Compute length vertically - only need to consider down direction
            length = 1;
            y = row - 1;
            while (y >= 0 && theBoard[column, y] == player)
            {
                y--;
                length++;
            }
            length = Math.Min(length, 4);
            score += SCORES[length];

            // Compute length horizontally
            length = 1;
            x = column - 1;
            while (x >= 0 && theBoard[x, row] == player)
            {
                x--;
                length++;
            }
            x = column + 1;
            while (x < width && theBoard[x, row] == player)
            {
                x++;
                length++;
            }
            length = Math.Min(length, 4);
            score += SCORES[length];

            // Compute length of \ diagonal
            length = 1;
            x = column - 1;
            y = row + 1;
            while (x >= 0 && y < height && theBoard[x, y] == player)
            {
                x--;
                y++;
                length++;
            }
            x = column + 1;
            y = row - 1;
            while (x < width && y >= 0 && theBoard[x, y] == player)
            {
                x++;
                y--;
                length++;
            }
            length = Math.Min(length, 4);
            score += SCORES[length];

            // Compute length of / diagonal
            length = 1;
            x = column + 1;
            y = row + 1;
            while (x < width && y < height && theBoard[x, y] == player)
            {
                x++;
                y++;
                length++;
            }
            x = column - 1;
            y = row - 1;
            while (x >= 0 && y >= 0 && theBoard[x, y] == player)
            {
                x--;
                y--;
                length++;
            }
            length = Math.Min(length, 4);
            score += SCORES[length];
            return score;
        }

        // Evaluate the state of the board from the perspective of MAX player
        private int EvaluateBoard()
        {
            int player, score;
            int maxBestScore = 0;
            int minBestScore = 0;
            for (int column = 0; column != width; column++)
            {
                if (columnHeight[column] != 0)
                {
                    player = theBoard[column, columnHeight[column] - 1];
                    score = EvaluateMove(column, true, player);
                    if (player == MAX)
                    {
                        if (score > maxBestScore)
                        {
                            maxBestScore = score;
                        }
                    }
                    else
                    {
                        if (score > minBestScore)
                        {
                            minBestScore = score;
                        }
                    }
                }
            }
            return maxBestScore - minBestScore;
        }

        private int Negamax(int depth, int alpha, int beta, bool doAlphaBeta, int player, ref int bestMove)
        {
            List<Move> moves = new List<Move>();
            int dummy = 0;  // Pass dummy best move reference down to lower recursion levels
            for (int column = 0; column != width; column++)
            {
                if (IsMovePossible(column))
                {
                    int score = EvaluateMove(column, false, player);
                    if (score >= WINNING_SCORE)
                    {
                        // Terminal node - WIN
                        bestMove = column;
                        return score;
                    }
                    moves.Add(new Move(column, score));
                }
            }
            if (moves.Count == 0)
            {
                // Terminal node - DRAW
                return 0;
            }
            // Sort moves in descending score order at this depth,
            // to speed up beta cutoff efficiency during recursive search
            moves.Sort();
            foreach (Move move in moves)
            {
                DoMove(move.column, player);
                if (depth != 0)
                {
                    // Recurse further down the tree and return value
                    if (!doAlphaBeta)
                    {
                        move.score = -Negamax(depth - 1, ALPHA_INITIAL, BETA_INITIAL, false, player ^ XOR, ref dummy);
                    }
                    else
                    {
                        move.score = -Negamax(depth - 1, -beta, -alpha, true, player ^ XOR, ref dummy);
                    }
                }
                else
                {
                    // This is a maximum depth leaf node which is not a terminal 
                    // node (open game play). Evaluate node value from current player's perspective
                    if (player == MAX)
                    {
                        move.score = EvaluateBoard();
                    }
                    else
                    {
                        move.score = -EvaluateBoard();
                    }
                }
                UndoMove(move.column);
                if (move.score > alpha)
                {
                    alpha = move.score;
                    bestMove = move.column;
                }
                if (doAlphaBeta && move.score >= beta)
                {
                    // Beta cutoff - we can't achieve this score as further up the tree
                    // a player has identified a branch that would disallow us from 
                    // making this move, so return the ceiling value instead
                    return beta;
                }
            }
            return alpha;
        }

        class Move : IComparable<Move>
        {
            public readonly int column;
            public int score;

            public Move(int column, int score)
            {
                this.column = column;
                this.score = score;
            }
            // Predicate used when sorting move list 
            // during alpha beta search
            public int CompareTo(Move other)
            {
                return (other.score - this.score);  // Backwards sort - best first
            }
        }

        public static void DoSelfTests()
        {
            Game board = new Game(7, 6);
            int moveNegamax = 0, scoreNegamax = 0, moveAlphaBeta = 0, scoreAlphaBeta = 0, move = 0;
            DateTime start, end;
            TimeSpan negamaxTime, alphaBetaTime;
            while (true)
            {
                start = DateTime.Now;
                scoreNegamax = board.Negamax(7, ALPHA_INITIAL, BETA_INITIAL, false, MAX, ref moveNegamax);
                end = DateTime.Now;
                negamaxTime = end - start;
                start = DateTime.Now;
                scoreAlphaBeta = board.Negamax(7, ALPHA_INITIAL, BETA_INITIAL, true, MAX, ref moveAlphaBeta);
                end = DateTime.Now;
                alphaBetaTime = end - start;
                move = moveNegamax;
                System.Diagnostics.Debug.Assert(moveAlphaBeta == moveNegamax && scoreAlphaBeta == scoreNegamax);
                Console.WriteLine("X to play:");
                Console.WriteLine("Negamax:   X chooses move {0} with score {1}", moveNegamax, scoreNegamax);
                Console.WriteLine("Negamax:   Milliseconds {0}", negamaxTime.TotalMilliseconds);
                Console.WriteLine("AlphaBeta: X chooses move {0} with score {1}", moveAlphaBeta, scoreAlphaBeta);
                Console.WriteLine("AlphaBeta: Milliseconds {0}", alphaBetaTime.TotalMilliseconds);
                board.DoMove(move, MAX);
                Console.WriteLine(board.ToString());
                if (board.IsWinningMove(move, MAX) || board.IsDraw(move, MAX))
                {
                    Console.WriteLine("Game over - X");
                    Console.ReadLine();
                    return;
                }
                start = DateTime.Now;
                scoreNegamax = board.Negamax(7, ALPHA_INITIAL, BETA_INITIAL, false, MIN, ref moveNegamax);
                end = DateTime.Now;
                negamaxTime = end - start;
                start = DateTime.Now;
                scoreAlphaBeta = board.Negamax(7, ALPHA_INITIAL, BETA_INITIAL, true, MIN, ref moveAlphaBeta);
                end = DateTime.Now;
                alphaBetaTime = end - start;
                move = moveNegamax;
                System.Diagnostics.Debug.Assert(moveAlphaBeta == moveNegamax && scoreAlphaBeta == scoreNegamax);
                Console.WriteLine("O to play:");
                Console.WriteLine("Negamax:   O chooses move {0} with score {1}", moveNegamax, scoreNegamax);
                Console.WriteLine("Negamax:   Milliseconds {0}", negamaxTime.TotalMilliseconds);
                Console.WriteLine("AlphaBeta: O chooses move {0} with score {1}", moveAlphaBeta, scoreAlphaBeta);
                Console.WriteLine("AlphaBeta: Milliseconds {0}", alphaBetaTime.TotalMilliseconds);
                board.DoMove(move, MIN);
                Console.WriteLine(board.ToString());
                if (board.IsWinningMove(move, MIN) || board.IsDraw(move, MIN))
                {
                    Console.WriteLine("Game over - O");
                    Console.ReadLine();
                    return;
                }
            }
        }
    }
}
