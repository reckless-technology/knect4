using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace ConnectFour
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Form mainForm = new MainForm();
            Application.Run(mainForm);
        }
    }
}