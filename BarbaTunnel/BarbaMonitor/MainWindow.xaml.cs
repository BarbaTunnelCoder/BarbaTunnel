using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Threading;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Runtime.InteropServices;
using System.IO;
using System.Windows.Threading;
using BarbaTunnel.CommLib;

namespace BarbaTunnel.Monitor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        BarbaComm BarbaComm = new BarbaComm();
        BarbaNotify BarbaNotify = new BarbaNotify();

        public String AppName { get { return "BarbaTunnel Monitor"; } }

        [DllImport("kernel32.dll")]
        private static extern int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);

        bool ExitMode = false;
        public MainWindow()
        {
            this.Closed += new EventHandler(MainWindow_Closed);
            this.Loaded += new RoutedEventHandler(MainWindow_Loaded);
            BarbaComm.NotifyChanged += new EventHandler(BarbaComm_Notified);
            BarbaComm.LogChanged += new EventHandler(BarbaComm_LogAdded);
            BarbaComm.StatusChanged += new EventHandler(BarbaComm_StatusChanged);
            BarbaNotify.NotifyIcon.MouseClick += new System.Windows.Forms.MouseEventHandler(NotifyIcon_MouseClick);
            BarbaNotify.NotifyIcon.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(NotifyIcon_MouseClick);
            BarbaNotify.MainWindowMenu.Click += delegate(object sender, EventArgs args) { this.DoShowMainWindow(); };
            BarbaNotify.ExitMenu.Click += delegate(object sender, EventArgs args) { this.DoStopAndExit(); };
            BarbaNotify.StartMenu.Click += delegate(object sender, EventArgs args) { this.DoStart(); };
            BarbaNotify.RestartMenu.Click += delegate(object sender, EventArgs args) { this.DoRestart(); };
            BarbaNotify.StopMenu.Click += delegate(object sender, EventArgs args) { this.DoStop(); };
            InitializeComponent();
            BarbaComm.Initialize();
            this.verboseCheckBox.IsChecked = BarbaComm.VerboseMode;
            if (BarbaComm.Status == BarbaStatus.Stopped)
                DoStart();

        }

        void UpdateStatus()
        {
            BarbaStatus status = BarbaComm.Status;
            if (status == BarbaStatus.Stopped)
            {
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Red);
                BarbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Stopped, new System.Drawing.Size(16, 16));
                Icon = StopIcon.Source;
            }
            else if (status == BarbaStatus.Idle || status == BarbaStatus.Waiting)
            {
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Orange);
                BarbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Idle, new System.Drawing.Size(16, 16));
                Icon = IdleIcon.Source;
            }
            else
            {
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Green);
                BarbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Started, new System.Drawing.Size(16, 16));
                Icon = StartIcon.Source;
            }

            BarbaNotify.StartMenu.Enabled = startButton.IsEnabled = status == BarbaStatus.Stopped && status != BarbaStatus.Waiting;
            BarbaNotify.StopMenu.Enabled = stopButton.IsEnabled = status != BarbaStatus.Stopped && status != BarbaStatus.Waiting;
            BarbaNotify.RestartMenu.Enabled = restartButton.IsEnabled = status != BarbaStatus.Stopped && status != BarbaStatus.Waiting;
            statusTextBlock.Text = BarbaComm.Status.ToString();
            BarbaNotify.NotifyIcon.Text = "BarbaTunnel is " + BarbaComm.Status.ToString();
        }

        void BarbaComm_StatusChanged(object sender, EventArgs e)
        {
            this.Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)delegate
            {
                UpdateStatus();
            });
        }

        void BarbaComm_LogAdded(object sender, EventArgs e)
        {
            this.Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)delegate
            {
                reportTextBox.Text = BarbaComm.ReadLog();
                reportTextBox.ScrollToEnd();
            });

        }

        void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateStatus();
            if (BarbaComm.Status == BarbaStatus.Stopped)
            {
                reportTextBox.Text = "Barbatunnel is not started!";
            }
            else
            {
                reportTextBox.Text = BarbaComm.ReadLog();
                reportTextBox.ScrollToEnd();
            }
        }

        void BarbaComm_Notified(object sender, EventArgs e)
        {
            String title;
            String text = BarbaComm.ReadNotify(out title);
            if (!String.IsNullOrEmpty(title) && !String.IsNullOrEmpty(text))
            {
                var tipIcon = System.Windows.Forms.ToolTipIcon.Info;
                if (title.IndexOf("error:", StringComparison.InvariantCultureIgnoreCase) != -1 || title.IndexOf("stopped", StringComparison.InvariantCultureIgnoreCase) != -1)
                {
                    tipIcon = System.Windows.Forms.ToolTipIcon.Error;
                }

                BarbaNotify.NotifyIcon.ShowBalloonTip(10, title, text, tipIcon);
            }
        }

        void NotifyIcon_MouseClick(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == System.Windows.Forms.MouseButtons.Left)
                DoShowMainWindow();
        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            BarbaComm.Dispose();
            BarbaNotify.Dispose();
        }

        void ShowMainWindowTimer(object sender, EventArgs e)
        {
            DispatcherTimer timer = (DispatcherTimer)sender;
            timer.Stop();
            this.Activate();
        }

        void DoShowMainWindow()
        {
            this.Visibility = System.Windows.Visibility.Visible;
            DispatcherTimer timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromMilliseconds(0);
            timer.Tick += new EventHandler(ShowMainWindowTimer); // i don't know why WPF does not activate MainWindow at first-time
            timer.Start();
        }

        void DoStopAndExit()
        {
            try
            {
                if (BarbaComm.Status != BarbaStatus.Stopped)
                    BarbaComm.Stop();

                this.ExitMode = true;
                this.Close();
            }
            catch { }
        }

        void DoStart()
        {
            try
            {
                if (BarbaComm.IsServiceRunnig)
                    BarbaComm.StartByService();
                else
                    BarbaComm.Start();
            }
            catch (Exception err)
            {
                MessageBox.Show(err.ToString(), AppName, MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        void DoRestart()
        {
            try
            {
                BarbaComm.Restart();
            }
            catch (Exception err)
            {
                MessageBox.Show(err.ToString(), AppName, MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        void DoStop()
        {
            try
            {
                BarbaComm.Stop();
            }
            catch (Exception err)
            {
                MessageBox.Show(err.ToString(), AppName, MessageBoxButton.OK, MessageBoxImage.Error);
            }

        }

        void startButton_Click(object sender, RoutedEventArgs e)
        {
            DoStart();
        }

        private void restartButton_Click(object sender, RoutedEventArgs e)
        {
            DoRestart();
        }

        private void stopButton_Click(object sender, RoutedEventArgs e)
        {
            DoStop();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (!ExitMode)
            {
                this.Visibility = System.Windows.Visibility.Hidden;
                e.Cancel = true;
            }
        }

        private void verboseCheckBox_Click(object sender, RoutedEventArgs e)
        {
            BarbaComm.VerboseMode = verboseCheckBox.IsChecked.Value;
        }

    }
}
