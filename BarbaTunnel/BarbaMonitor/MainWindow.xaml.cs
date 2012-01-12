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
        [DllImport("kernel32.dll")]
        private static extern int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);

        BarbaComm BarbaComm = new BarbaComm();
        BarbaNotify BarbaNotify = new BarbaNotify();

        public String AppName { get { return "BarbaTunnel Monitor"; } }
        public String ModuleFile { get { return System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName; } }
        private const String CurrentUserRunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private String AutoStartRegValue { get { return String.Format("\"{0}\" /delaystart", ModuleFile); } }

        public bool IsAutoStart
        {
            get
            {
                String val = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(CurrentUserRunKeyPath, true).GetValue("BarbaMonitor", null) as String;
                return AutoStartRegValue.Equals(val, StringComparison.InvariantCultureIgnoreCase);
            }
            set
            {
                if (value)
                    Microsoft.Win32.Registry.CurrentUser.OpenSubKey(CurrentUserRunKeyPath, true).SetValue("BarbaMonitor", AutoStartRegValue);
                else
                    Microsoft.Win32.Registry.CurrentUser.OpenSubKey(CurrentUserRunKeyPath, true).DeleteValue("BarbaMonitor");
            }
        }


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
            BarbaNotify.AutoStartMenu.Click += delegate(object sender, EventArgs args) { IsAutoStart = !IsAutoStart; BarbaNotify.AutoStartMenu.Checked = IsAutoStart; };
            BarbaNotify.AutoStartMenu.Checked = IsAutoStart;
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

        void UpdateLog()
        {
            if (reportCheckBox.IsChecked.Value)
            {
                if (reportTextBox.IsFocused)
                    reportCheckBox.Focus(); // TextBox jump to start if have focus
                reportTextBox.Text = BarbaComm.ReadLog();
                reportTextBox.ScrollToEnd();
            }
        }

        void BarbaComm_LogAdded(object sender, EventArgs e)
        {
            this.Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)delegate
            {
                UpdateLog();
            });
        }

        private void reportCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            UpdateLog();
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
            this.Activate();
            if (this.WindowState == System.Windows.WindowState.Minimized) this.WindowState = System.Windows.WindowState.Normal;
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
