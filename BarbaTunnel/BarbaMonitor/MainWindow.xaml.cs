using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Threading;
using System.Windows.Threading;
using BarbaTunnel.CommLib;

// ReSharper disable LocalizableElement
namespace BarbaTunnel.Monitor
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow
	{
		readonly BarbaComm _barbaComm = new BarbaComm();
		readonly BarbaNotify _barbaNotify = new BarbaNotify();

		public String AppName { get { return "BarbaTunnel Monitor"; } }
		public String ModuleFile { get { return System.Reflection.Assembly.GetExecutingAssembly().Location; } }
		private const String WindowsRunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
		private const String BarbaTunnelRegKeyPath = @"Software\BarbaTunnel";
		private String AutoStartRegValue { get { return String.Format("\"{0}\" /delaystart", ModuleFile); } }
		public Boolean LockReportUpdate = false;

		// ReSharper disable ValueParameterNotUsed
		public bool IsProfileCreated
		{
			get
			{
				try
				{
					return (int)Microsoft.Win32.Registry.GetValue("HKEY_CURRENT_USER\\" + BarbaTunnelRegKeyPath, "IsBarbaMonitorInitialized", 0) != 0;
				}
				catch
				{
					return false;
				}
			}
			set
			{
				Microsoft.Win32.Registry.SetValue("HKEY_CURRENT_USER\\" + BarbaTunnelRegKeyPath, "IsBarbaMonitorInitialized", 1);
			}
		}
		// ReSharper restore ValueParameterNotUsed

		public bool IsAutoStartTunnel
		{
			get { return _barbaComm.IsAutoStartTunnel; }
			set { _barbaComm.IsAutoStartTunnel = value; }
		}

		// ReSharper disable PossibleNullReferenceException
		public bool IsAutoStartMonitor
		{
			get
			{
				try
				{
					var val = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(WindowsRunKeyPath, true).GetValue("BarbaMonitor", null) as String;
					return AutoStartRegValue.Equals(val, StringComparison.InvariantCultureIgnoreCase);
				}
				catch
				{
					return false;
				}
			}
			set
			{
				if (value)
					Microsoft.Win32.Registry.CurrentUser.OpenSubKey(WindowsRunKeyPath, true).SetValue("BarbaMonitor", AutoStartRegValue);
				else
					Microsoft.Win32.Registry.CurrentUser.OpenSubKey(WindowsRunKeyPath, true).DeleteValue("BarbaMonitor");
			}
		}
		// ReSharper restore PossibleNullReferenceException


		bool _exitMode;
		public MainWindow()
		{
			//setup for first time
			if (!IsProfileCreated)
			{
				IsAutoStartMonitor = true;
				IsProfileCreated = true;
			}

			Visibility = Visibility.Hidden;
			Closed += MainWindow_Closed;
			Loaded += MainWindow_Loaded;
			_barbaComm.NotifyChanged += BarbaComm_Notified;
			_barbaComm.LogChanged += BarbaComm_LogAdded;
			_barbaComm.StatusChanged += BarbaComm_StatusChanged;
			_barbaNotify.NotifyIcon.MouseClick += NotifyIcon_MouseClick;
			_barbaNotify.NotifyIcon.MouseDoubleClick += NotifyIcon_MouseClick;
			_barbaNotify.MainWindowMenu.Click += (sender, args) => DoShowMainWindow();
			_barbaNotify.ExitMenu.Click += delegate { DoExit(false); };
			_barbaNotify.ExitAndStopMenu.Click += delegate { DoExit(true); };
			_barbaNotify.StartMenu.Click += delegate { DoStart(); };
			_barbaNotify.RestartMenu.Click += (sender, args) => DoRestart();
			_barbaNotify.StopMenu.Click += delegate { DoStop(); };
			_barbaNotify.AutoStartMonitorMenu.Click += delegate { IsAutoStartMonitor = !IsAutoStartMonitor; _barbaNotify.AutoStartMonitorMenu.Checked = IsAutoStartMonitor; };
			_barbaNotify.AutoStartMonitorMenu.Checked = IsAutoStartMonitor;
			_barbaNotify.AutoStartTunnelMenu.Click += delegate { IsAutoStartTunnel = !IsAutoStartTunnel; _barbaNotify.AutoStartTunnelMenu.Checked = IsAutoStartTunnel; };
			_barbaNotify.AutoStartTunnelMenu.Checked = IsAutoStartTunnel;
			InitializeComponent();
			_barbaComm.Initialize();
			LogLevelBox.SelectedIndex = _barbaComm.LogLevel - 1;

			//StartTunnel when monitor start
			if (_barbaComm.Status == BarbaStatus.Stopped && IsAutoStartTunnel)
			{
				ReportTextBox.Text = "BarbaTunnel is not started!";
				DoStart();
			}
			else
			{
				ReportTextBox.Text = _barbaComm.ReadLog();
			}
		}

		void UpdateStatus()
		{
			var status = _barbaComm.Status;
			if (status == BarbaStatus.Stopped)
			{
				StatusTextBlock.Foreground = new SolidColorBrush(Colors.Red);
				_barbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Stopped, new System.Drawing.Size(16, 16));
				Icon = StopIcon.Source;
			}
			else if (status == BarbaStatus.Idle || status == BarbaStatus.Waiting)
			{
				StatusTextBlock.Foreground = new SolidColorBrush(Colors.Orange);
				_barbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Idle, new System.Drawing.Size(16, 16));
				Icon = IdleIcon.Source;
			}
			else
			{
				StatusTextBlock.Foreground = new SolidColorBrush(Colors.Green);
				_barbaNotify.NotifyIcon.Icon = new System.Drawing.Icon(Resource1.Status_Started, new System.Drawing.Size(16, 16));
				Icon = StartIcon.Source;
			}

			_barbaNotify.StartMenu.Enabled = StartButton.IsEnabled = status == BarbaStatus.Stopped && status != BarbaStatus.Waiting;
			_barbaNotify.StopMenu.Enabled = StopButton.IsEnabled = status != BarbaStatus.Stopped && status != BarbaStatus.Waiting;
			_barbaNotify.RestartMenu.Enabled = RestartButton.IsEnabled = status != BarbaStatus.Stopped && status != BarbaStatus.Waiting;
			StatusTextBlock.Text = _barbaComm.Status.ToString();
			_barbaNotify.NotifyIcon.Text = "BarbaTunnel is " + _barbaComm.Status.ToString();
		}

		void BarbaComm_StatusChanged(object sender, EventArgs e)
		{
			Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)UpdateStatus);
		}

		void UpdateLog()
		{
			if (LockReportUpdate || WindowState==WindowState.Minimized || !IsVisible)
				return;
			
			ReportTextBox.Text = _barbaComm.ReadLog();
            ReportTextBox.Select(ReportTextBox.Text.Length, 0);
            ReportTextBox.ScrollToEnd();
		}

		void BarbaComm_LogAdded(object sender, EventArgs e)
		{
			Dispatcher.Invoke(DispatcherPriority.Normal, (ThreadStart)UpdateLog);
		}

		void MainWindow_Loaded(object sender, RoutedEventArgs e)
		{
			UpdateStatus();
            ReportTextBox.Select(ReportTextBox.Text.Length, 0);
            ReportTextBox.ScrollToEnd();
		}

		void BarbaComm_Notified(object sender, EventArgs e)
		{
			String title;
			var text = _barbaComm.ReadNotify(out title);
			if (!String.IsNullOrEmpty(title) && !String.IsNullOrEmpty(text))
			{
				var tipIcon = System.Windows.Forms.ToolTipIcon.Info;
				if (title.IndexOf("error:", StringComparison.InvariantCultureIgnoreCase) != -1 || title.IndexOf("stopped", StringComparison.InvariantCultureIgnoreCase) != -1)
				{
					tipIcon = System.Windows.Forms.ToolTipIcon.Error;
				}

				_barbaNotify.NotifyIcon.ShowBalloonTip(10, title, text, tipIcon);
			}
		}

		void NotifyIcon_MouseClick(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (e.Button == System.Windows.Forms.MouseButtons.Left)
				DoShowMainWindow();
		}

		void MainWindow_Closed(object sender, EventArgs e)
		{
			_barbaComm.Dispose();
			_barbaNotify.Dispose();
		}

		void ShowMainWindowTimer(object sender, EventArgs e)
		{
			var timer = (DispatcherTimer)sender;
			timer.Stop();
			Activate();
		}

		void DoShowMainWindow()
		{
			Visibility = Visibility.Visible;
			Activate();
			if (WindowState == WindowState.Minimized) WindowState = WindowState.Normal;
			var timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(0) };
			timer.Tick += ShowMainWindowTimer; // i don't know why WPF does not activate MainWindow at first-time
			timer.Start();
		}

		void DoExit(bool stop)
		{
			try
			{
				if (stop && _barbaComm.Status != BarbaStatus.Stopped)
					_barbaComm.Stop();

				_exitMode = true;
				Close();
			}
			// ReSharper disable EmptyGeneralCatchClause
			catch
			// ReSharper restore EmptyGeneralCatchClause
			{ }
		}

		void DoStart()
		{
			try
			{
				if (_barbaComm.IsServiceRunnig)
					_barbaComm.StartByService();
				else
					_barbaComm.Start();
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
				_barbaComm.Restart();
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
				_barbaComm.Stop();
			}
			catch (Exception err)
			{
				MessageBox.Show(err.ToString(), AppName, MessageBoxButton.OK, MessageBoxImage.Error);
			}

		}

		void StartButton_Click(object sender, RoutedEventArgs e)
		{
			DoStart();
		}

		private void RestartButton_Click(object sender, RoutedEventArgs e)
		{
			DoRestart();
		}

		private void StopButton_Click(object sender, RoutedEventArgs e)
		{
			DoStop();
		}

		private void Window_Closing(object sender, CancelEventArgs e)
		{
			if (!_exitMode)
			{
				Visibility = Visibility.Hidden;
				e.Cancel = true;
			}
		}

		private void LogLevelBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			_barbaComm.LogLevel = LogLevelBox.SelectedIndex + 1;
		}

		private void OpenLogFile_Click(object sender, RoutedEventArgs e)
		{
			Process.Start(new ProcessStartInfo("notepad.exe", _barbaComm.LogFilePath));
			e.Handled = true;

		}

		private void ReportTextBox_PreviewMouseDown(object sender, MouseButtonEventArgs e)
		{
			LockReportUpdate = true;
		}

		private void ReportTextBox_PreviewMouseUp(object sender, MouseButtonEventArgs e)
		{
			LockReportUpdate = false;
			UpdateLog();
		}

		private void Window_StateChanged(object sender, EventArgs e)
		{
			if (WindowState!=WindowState.Minimized)
				UpdateLog();
		}

		private void Window_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
		{
			if (IsVisible)
				UpdateLog();
		}
	}
}
