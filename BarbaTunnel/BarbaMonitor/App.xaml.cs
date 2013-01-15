using System;
using System.Collections.Generic;
using System.Windows;
using Microsoft.Shell;

namespace BarbaTunnel.Monitor
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : ISingleInstanceApp
    {
        private const string Unique = @"Local\BarbaMonitor_SingleInstance";

        [STAThread]
        public static void Main()
        {
            if (SingleInstance<App>.InitializeAsFirstInstance(Unique))
            {
                var application = new App();

                application.InitializeComponent();
                application.Run();
                
                // Allow single instance code to perform cleanup operations
                SingleInstance<App>.Cleanup();
            }
        }

        #region ISingleInstanceApp Members

        public bool SignalExternalCommandLineArgs(IList<string> args)
        {
            // handle command line arguments of second instance
            // ...
            if (MainWindow.WindowState == System.Windows.WindowState.Minimized)
                MainWindow.WindowState = System.Windows.WindowState.Normal;
            MainWindow.Visibility = Visibility.Visible;
            MainWindow.Activate();
            return true;
        }

        #endregion
    }
}
