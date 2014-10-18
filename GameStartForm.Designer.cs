namespace ConnectFour
{
    partial class GameStartForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.difficultyGroupBox = new System.Windows.Forms.GroupBox();
            this.colourGroupBox = new System.Windows.Forms.GroupBox();
            this.goFirstLabel = new System.Windows.Forms.Label();
            this.yesButton = new System.Windows.Forms.Button();
            this.noButton = new System.Windows.Forms.Button();
            this.easyButton = new System.Windows.Forms.RadioButton();
            this.mediumButton = new System.Windows.Forms.RadioButton();
            this.hardButton = new System.Windows.Forms.RadioButton();
            this.redButton = new System.Windows.Forms.RadioButton();
            this.blueButton = new System.Windows.Forms.RadioButton();
            this.difficultyGroupBox.SuspendLayout();
            this.colourGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // difficultyGroupBox
            // 
            this.difficultyGroupBox.Controls.Add(this.hardButton);
            this.difficultyGroupBox.Controls.Add(this.mediumButton);
            this.difficultyGroupBox.Controls.Add(this.easyButton);
            this.difficultyGroupBox.Location = new System.Drawing.Point(13, 13);
            this.difficultyGroupBox.Name = "difficultyGroupBox";
            this.difficultyGroupBox.Size = new System.Drawing.Size(140, 100);
            this.difficultyGroupBox.TabIndex = 0;
            this.difficultyGroupBox.TabStop = false;
            this.difficultyGroupBox.Text = "Select the difficulty level:";
            // 
            // colourGroupBox
            // 
            this.colourGroupBox.Controls.Add(this.blueButton);
            this.colourGroupBox.Controls.Add(this.redButton);
            this.colourGroupBox.Location = new System.Drawing.Point(198, 13);
            this.colourGroupBox.Name = "colourGroupBox";
            this.colourGroupBox.Size = new System.Drawing.Size(140, 100);
            this.colourGroupBox.TabIndex = 1;
            this.colourGroupBox.TabStop = false;
            this.colourGroupBox.Text = "Select your colour:";
            // 
            // goFirstLabel
            // 
            this.goFirstLabel.AutoSize = true;
            this.goFirstLabel.Location = new System.Drawing.Point(109, 130);
            this.goFirstLabel.Name = "goFirstLabel";
            this.goFirstLabel.Size = new System.Drawing.Size(129, 13);
            this.goFirstLabel.TabIndex = 2;
            this.goFirstLabel.Text = "Would you like to go first?";
            // 
            // yesButton
            // 
            this.yesButton.Enabled = false;
            this.yesButton.Location = new System.Drawing.Point(78, 161);
            this.yesButton.Name = "yesButton";
            this.yesButton.Size = new System.Drawing.Size(75, 23);
            this.yesButton.TabIndex = 3;
            this.yesButton.Text = "Yes";
            this.yesButton.UseVisualStyleBackColor = true;
            this.yesButton.Click += new System.EventHandler(this.ChooseGoFirst);
            // 
            // noButton
            // 
            this.noButton.Enabled = false;
            this.noButton.Location = new System.Drawing.Point(185, 161);
            this.noButton.Name = "noButton";
            this.noButton.Size = new System.Drawing.Size(75, 23);
            this.noButton.TabIndex = 4;
            this.noButton.Text = "No";
            this.noButton.UseVisualStyleBackColor = true;
            this.noButton.Click += new System.EventHandler(this.ChooseGoSecond);
            // 
            // easyButton
            // 
            this.easyButton.AutoSize = true;
            this.easyButton.Location = new System.Drawing.Point(7, 20);
            this.easyButton.Name = "easyButton";
            this.easyButton.Size = new System.Drawing.Size(48, 17);
            this.easyButton.TabIndex = 0;
            this.easyButton.TabStop = true;
            this.easyButton.Text = "Easy";
            this.easyButton.UseVisualStyleBackColor = true;
            this.easyButton.Click += new System.EventHandler(this.ChooseEasy);
            // 
            // mediumButton
            // 
            this.mediumButton.AutoSize = true;
            this.mediumButton.Location = new System.Drawing.Point(7, 44);
            this.mediumButton.Name = "mediumButton";
            this.mediumButton.Size = new System.Drawing.Size(62, 17);
            this.mediumButton.TabIndex = 1;
            this.mediumButton.TabStop = true;
            this.mediumButton.Text = "Medium";
            this.mediumButton.UseVisualStyleBackColor = true;
            this.mediumButton.Click += new System.EventHandler(this.ChooseMedium);
            // 
            // hardButton
            // 
            this.hardButton.AutoSize = true;
            this.hardButton.Location = new System.Drawing.Point(7, 68);
            this.hardButton.Name = "hardButton";
            this.hardButton.Size = new System.Drawing.Size(48, 17);
            this.hardButton.TabIndex = 2;
            this.hardButton.TabStop = true;
            this.hardButton.Text = "Hard";
            this.hardButton.UseVisualStyleBackColor = true;
            this.hardButton.Click += new System.EventHandler(this.ChooseHard);
            // 
            // redButton
            // 
            this.redButton.AutoSize = true;
            this.redButton.Location = new System.Drawing.Point(7, 20);
            this.redButton.Name = "redButton";
            this.redButton.Size = new System.Drawing.Size(45, 17);
            this.redButton.TabIndex = 0;
            this.redButton.TabStop = true;
            this.redButton.Text = "Red";
            this.redButton.UseVisualStyleBackColor = true;
            this.redButton.Click += new System.EventHandler(this.ChooseRed);
            // 
            // blueButton
            // 
            this.blueButton.AutoSize = true;
            this.blueButton.Location = new System.Drawing.Point(7, 44);
            this.blueButton.Name = "blueButton";
            this.blueButton.Size = new System.Drawing.Size(46, 17);
            this.blueButton.TabIndex = 1;
            this.blueButton.TabStop = true;
            this.blueButton.Text = "Blue";
            this.blueButton.UseVisualStyleBackColor = true;
            this.blueButton.Click += new System.EventHandler(this.ChooseBlue);
            // 
            // GameStartForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(352, 216);
            this.Controls.Add(this.noButton);
            this.Controls.Add(this.yesButton);
            this.Controls.Add(this.goFirstLabel);
            this.Controls.Add(this.colourGroupBox);
            this.Controls.Add(this.difficultyGroupBox);
            this.Name = "GameStartForm";
            this.Text = "Choose your game";
            this.difficultyGroupBox.ResumeLayout(false);
            this.difficultyGroupBox.PerformLayout();
            this.colourGroupBox.ResumeLayout(false);
            this.colourGroupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox difficultyGroupBox;
        private System.Windows.Forms.GroupBox colourGroupBox;
        private System.Windows.Forms.RadioButton hardButton;
        private System.Windows.Forms.RadioButton mediumButton;
        private System.Windows.Forms.RadioButton easyButton;
        private System.Windows.Forms.RadioButton blueButton;
        private System.Windows.Forms.RadioButton redButton;
        private System.Windows.Forms.Label goFirstLabel;
        private System.Windows.Forms.Button yesButton;
        private System.Windows.Forms.Button noButton;
    }
}