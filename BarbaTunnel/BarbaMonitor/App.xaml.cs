using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Windows;
using Microsoft.Shell;

namespace BarbaTunnel.Monitor
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application, ISingleInstanceApp
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
            if (this.MainWindow.WindowState == System.Windows.WindowState.Minimized)
                this.MainWindow.WindowState = System.Windows.WindowState.Normal;
            this.MainWindow.Visibility = Visibility.Visible;
            this.MainWindow.Activate();
            return true;
        }

        #endregion
    }
}
