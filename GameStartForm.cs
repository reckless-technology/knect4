using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace ConnectFour
{
    public partial class GameStartForm : Form
    {
        private bool hasChosenLevel, hasChosenColour;
        private MainForm parent;

        public GameStartForm(MainForm theParent)
        {
            parent = theParent;
            hasChosenColour = false;
            hasChosenLevel = false;
            InitializeComponent();
        }

        private void ChooseEasy(object sender, EventArgs e)
        {
            hasChosenLevel = true;
            parent.SetRecursionDepth(3);
            if (hasChosenColour)
            {
                yesButton.Enabled = true;
                noButton.Enabled = true;
            }
        }

        private void ChooseMedium(object sender, EventArgs e)
        {
            hasChosenLevel = true;
            parent.SetRecursionDepth(7);
            if (hasChosenColour)
            {
                yesButton.Enabled = true;
                noButton.Enabled = true;
            }
        }

        private void ChooseHard(object sender, EventArgs e)
        {
            hasChosenLevel = true;
            parent.SetRecursionDepth(11);
            if (hasChosenColour)
            {
                yesButton.Enabled = true;
                noButton.Enabled = true;
            }
        }

        private void ChooseRed(object sender, EventArgs e)
        {
            hasChosenColour = true;
            parent.SetColours(Brushes.Blue, Brushes.DarkRed);
            if (hasChosenLevel)
            {
                yesButton.Enabled = true;
                noButton.Enabled = true;
            }
        }

        private void ChooseBlue(object sender, EventArgs e)
        {
            hasChosenColour = true;
            parent.SetColours(Brushes.DarkRed, Brushes.Blue);
            if (hasChosenLevel)
            {
                yesButton.Enabled = true;
                noButton.Enabled = true;
            }
        }

        private void ChooseGoFirst(object sender, EventArgs e)
        {
            parent.SetInitialState(MainForm.GameState.AwaitingHuman);
            this.Close();
        }

        private void ChooseGoSecond(object sender, EventArgs e)
        {
            parent.SetInitialState(MainForm.GameState.AwaitingComputer);
            this.Close();
        }
    }
}