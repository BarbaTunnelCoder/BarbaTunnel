using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace BarbaTunnel.Monitor
{
    class BarbaNotify : IDisposable
    {
        NotifyIcon _NotifyIcon = new NotifyIcon();
        public NotifyIcon NotifyIcon { get { return _NotifyIcon; } }
        public MenuItem ExitAndStopMenu { get; private set; }
        public MenuItem ExitMenu { get; private set; }
        public MenuItem StartMenu { get; private set; }
        public MenuItem RestartMenu { get; private set; }
        public MenuItem StopMenu { get; private set; }
        public MenuItem MainWindowMenu { get; private set; }
		public MenuItem AutoStartMonitorMenu { get; private set; }
		public MenuItem AutoStartTunnelMenu { get; private set; }
        public BarbaNotify()
        {
            var menu = new System.Windows.Forms.ContextMenu();
            MainWindowMenu = menu.MenuItems.Add("Barba Monitor Window");
            MainWindowMenu.DefaultItem = true;
            menu.MenuItems.Add("-");
			AutoStartTunnelMenu = menu.MenuItems.Add("Auto Start Barba Tunnel");
			AutoStartMonitorMenu = menu.MenuItems.Add("Auto Start Barba Monitor");
            menu.MenuItems.Add("-");
            StartMenu = menu.MenuItems.Add("Start");
            RestartMenu = menu.MenuItems.Add("Restart");
            StopMenu = menu.MenuItems.Add("Stop");
            menu.MenuItems.Add("-");
            ExitAndStopMenu = menu.MenuItems.Add("Exit && Stop");
            ExitMenu = menu.MenuItems.Add("Exit");

            NotifyIcon.Icon = Resource1.Status_Stopped;
            NotifyIcon.Visible = true;
            NotifyIcon.ContextMenu = menu;
        }


        public void Dispose()
        {
            NotifyIcon.Dispose();
        }
    }
}
