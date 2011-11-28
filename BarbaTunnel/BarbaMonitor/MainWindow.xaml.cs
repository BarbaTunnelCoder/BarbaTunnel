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

namespace BarbaMonitor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        BarbaComm BarbaComm = new BarbaComm();
        BarbaNotify BarbaNotify = new BarbaNotify();

        public String AppName { get { return "Barbatunnel Monitor"; } }

        bool ExitMode = false;
        public MainWindow()
        {
            this.Closed += new EventHandler(MainWindow_Closed);
            this.Loaded += new RoutedEventHandler(MainWindow_Loaded);
            BarbaComm.NotifyChanged += new EventHandler(BarbaComm_Notified);
            BarbaComm.LogChanged += new EventHandler(BarbaComm_LogAdded);
            BarbaComm.StatusChanged += new EventHandler(BarbaComm_StatusChanged);
            BarbaNotify.NotifyIcon.DoubleClick += new EventHandler(NotifyIcon_DoubleClick);
            BarbaNotify.NotifyIcon.Click += new EventHandler(NotifyIcon_DoubleClick);
            BarbaNotify.ExitMenu.Click += delegate(object sender, EventArgs args) { ExitMode = true; this.Close(); };
            BarbaNotify.StartMenu.Click += delegate(object sender, EventArgs args) { this.DoStart(); };
            BarbaNotify.RestartMenu.Click += delegate(object sender, EventArgs args) { this.DoRestart(); };
            BarbaNotify.StopMenu.Click += delegate(object sender, EventArgs args) { this.DoStop(); };
            InitializeComponent();
            BarbaComm.Initialize();
        }

        void UpdateStatus()
        {
            BarbaStatus status = BarbaComm.Status;
            if (status == BarbaStatus.Stopped)
            {
                BarbaNotify.NotifyIcon.Icon = Resource1.Status_Stopped;
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Red);
            }
            else if (status == BarbaStatus.Idle)
            {
                BarbaNotify.NotifyIcon.Icon = Resource1.Status_Idle;
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Orange);
            }
            else
            {
                statusTextBlock.Foreground = new SolidColorBrush(Colors.Green);
                BarbaNotify.NotifyIcon.Icon = Resource1.Status_Started;
            }

            BarbaNotify.StartMenu.Enabled = startButton.IsEnabled = status == BarbaStatus.Stopped;
            BarbaNotify.StopMenu.Enabled = stopButton.IsEnabled = status != BarbaStatus.Stopped;
            BarbaNotify.RestartMenu.Enabled = restartButton.IsEnabled = status != BarbaStatus.Stopped;
            statusTextBlock.Text = BarbaComm.Status.ToString();
            BarbaNotify.NotifyIcon.Text = "Barbatunnel is " + BarbaComm.Status.ToString();
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
                if (title.IndexOf("error:", StringComparison.InvariantCultureIgnoreCase) != -1)
                {
                    tipIcon = System.Windows.Forms.ToolTipIcon.Error;
                }
                BarbaNotify.NotifyIcon.ShowBalloonTip(10, title, text, tipIcon);
            }
        }


        void NotifyIcon_DoubleClick(object sender, EventArgs e)
        {
            this.Visibility = System.Windows.Visibility.Visible;
        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            BarbaComm.Dispose();
            BarbaNotify.Dispose();
        }

        void DoStart()
        {
            try
            {
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

    }
}
