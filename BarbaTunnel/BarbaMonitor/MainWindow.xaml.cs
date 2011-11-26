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
        System.Windows.Forms.NotifyIcon ni;

        BarbaComm bm = new BarbaComm();
        public MainWindow()
        {
            this.Closed += new EventHandler(MainWindow_Closed);
            this.Loaded += new RoutedEventHandler(MainWindow_Loaded);
            bm.NotifyChanged += new EventHandler(bm_Notified);
            bm.LogChanged += new EventHandler(bm_LogAdded);
            InitializeNotifyIcon();
            InitializeComponent();
            bm.Initialize();
        }

        void bm_LogAdded(object sender, EventArgs e)
        {
            reportTextBox.Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)delegate
            {
                reportTextBox.Text = bm.ReadLog();
                reportTextBox.ScrollToEnd();
            });

        }

        void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            if (bm.IsStarted)
            {
                reportTextBox.Text = bm.ReadLog();
                reportTextBox.ScrollToEnd();
            }
        }

        void bm_Notified(object sender, EventArgs e)
        {
            String title;
            String text = bm.ReadNotify(out title);
            if (!String.IsNullOrEmpty(title))
                ni.ShowBalloonTip(10, title, text, System.Windows.Forms.ToolTipIcon.Info);
        }

        void InitializeNotifyIcon()
        {

            var menu = new System.Windows.Forms.ContextMenu();
            menu.MenuItems.Add("aaa");

            ni = new System.Windows.Forms.NotifyIcon();
            ni.Icon = Resource1.NotifyIcon;
            ni.Visible = true;
            ni.ContextMenu = menu;
            ni.DoubleClick += delegate(object sender, EventArgs args)
                {
                    //this.Show();
                    //this.WindowState = WindowState.Normal;
                };

        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            bm.Dispose();
            ni.Visible = false;
        }

        private void button1_Click(object sender, RoutedEventArgs e)
        {
            reportTextBox.Text = bm.ReadLog();
            reportTextBox.ScrollToEnd();

        }

    }
}
