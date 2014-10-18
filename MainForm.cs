using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace ConnectFour
{
    public partial class MainForm : Form
    {
        private Game game;
        private Brush computerColour, humanColour;
        private GameState gameState;
        private int recursionDepth;
        private int computerMove, computerScore;
        private int[,] board;

        public enum GameState
        {
            NotYetStarted,
            AwaitingHuman,
            AwaitingComputer,
            GameOverDraw,
            GameOverComputerWins,
            GameOverHumanWins
        }

        public MainForm()
        {
            gameState = GameState.NotYetStarted;
            game = new Game();
            board = game.GetBoard();
            InitializeComponent();
            new GameStartForm(this).ShowDialog();
            if (gameState == GameState.AwaitingComputer)
            {
                computerBackgroundWorker.RunWorkerAsync();
            }
        }

        public void SetColours(Brush computer, Brush human)
        {
            computerColour = computer;
            humanColour = human;
        }

        public void SetInitialState(GameState state)
        {
            gameState = state;
        }

        public void SetRecursionDepth(int depth)
        {
            recursionDepth = depth;
        }

        private void HandleEndGame()
        {
            DialogResult result = DialogResult.No;
            if (gameState == GameState.GameOverComputerWins)
            {
                result = MessageBox.Show("I won - would you like another game?", "Game over", MessageBoxButtons.YesNo);
            }
            if (gameState == GameState.GameOverHumanWins)
            {
                result = MessageBox.Show("You won - would you like another game?", "Game over", MessageBoxButtons.YesNo);
            }
            if (gameState == GameState.GameOverDraw)
            {
                result = MessageBox.Show("The game is a draw - would you like another game?", "Game over", MessageBoxButtons.YesNo);
            }
            if (result == DialogResult.No)
            {
                Application.Exit();
            }
            game.Reset();
            board = game.GetBoard();
            gamePictureBox.Invalidate();
            gameState = GameState.NotYetStarted;
            new GameStartForm(this).ShowDialog();
            if (gameState == GameState.AwaitingComputer)
            {
                computerBackgroundWorker.RunWorkerAsync();
            }
        }

        private void DrawBoard(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.FillRectangle(Brushes.LightGray, 0, 0, 350, 300);
            for (int x = 0; x != 7; x++)
            {
                for (int y = 0; y != 6; y++)
                {
                    switch (board[x, y])
                    {
                        case Game.EMPTY:
                            DrawDisc(g, Brushes.White, x, y);
                            break;
                        case Game.MAX:
                            DrawDisc(g, computerColour, x, y);
                            break;
                        case Game.MIN:
                            DrawDisc(g, humanColour, x, y);
                            break;
                    }
                }
            }
        }

        private void DrawDisc(Graphics g, Brush brush, int x, int y)
        {
            g.FillEllipse(brush, 50 * x + 5, 50 * (5 - y) + 5, 40, 40);
        }

        private void PictureBoxMouseClick(object sender, MouseEventArgs e)
        {
            if (gameState != GameState.AwaitingHuman)
            {
                return;
            }
            int x = e.X / 50;
            if (!game.IsMovePossible(x))
            {
                return;
            }
            game.DoMove(x, Game.MIN);
            board = game.GetBoard();
            gamePictureBox.Invalidate();
            if (game.IsWinningMove(x, Game.MIN))
            {
                gameState = GameState.GameOverHumanWins;
                HandleEndGame();
                return;
            }
            if (game.IsDraw(x, Game.MIN))
            {
                gameState = GameState.GameOverDraw;
                HandleEndGame();
                return;
            }
            gameState = GameState.AwaitingComputer; 
            computerBackgroundWorker.RunWorkerAsync();
        }

        private void DoComputerMove(object sender, DoWorkEventArgs e)
        {
            computerScore = game.GetNextMove(recursionDepth, Game.MAX, ref computerMove);
        }

        private void ComputerMoveFinished(object sender, RunWorkerCompletedEventArgs e)
        {
            game.DoMove(computerMove, Game.MAX);
            board = game.GetBoard();
            gamePictureBox.Invalidate();
            if (game.IsWinningMove(computerMove, Game.MAX))
            {
                gameState = GameState.GameOverComputerWins;
                HandleEndGame();
                return;
            }
            if (game.IsDraw(computerMove, Game.MAX))
            {
                gameState = GameState.GameOverDraw;
                HandleEndGame();
                return;
            }
            gameState = GameState.AwaitingHuman;
            
        }

    }
}