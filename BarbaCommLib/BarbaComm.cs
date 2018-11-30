using System;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;

// ReSharper disable EmptyGeneralCatchClause
namespace BarbaTunnel.CommLib
{
	public enum BarbaStatus
	{
		Waiting,
		Stopped,
		Started,
		Idle,
	}

	public class BarbaComm : IDisposable
	{
		[DllImport("KERNEL32.DLL", EntryPoint = "GetPrivateProfileStringW", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
		private static extern int GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault, StringBuilder retVal, int nSize, string lpFilename);
		[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		private static extern bool WritePrivateProfileString(string lpAppName, string lpKeyName, string lpString, string lpFileName);
		[DllImport("kernel32.dll")]
		private static extern int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);

		public String WorkinFolderPath { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "Barbatunnel"); } }
		public String OptionsFilePath { get { return Path.Combine(WorkinFolderPath, "options.txt"); } }
		public String CommFilePath { get { return Path.Combine(WorkinFolderPath, "comm.txt"); } }
		public String LogFilePath { get { return Path.Combine(WorkinFolderPath, "report.txt"); } }
		public String NotifyFilePath { get { return Path.Combine(WorkinFolderPath, "notify.txt"); } }
		public String ModuleFolderPath { get { return Path.GetDirectoryName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName); } }
		public String BarbaTunnelFilePath { get { return Path.Combine(ModuleFolderPath, IntPtr.Size == 4 ? "x86\\barbatunnel.exe" : "x64\\barbatunnel.exe"); } }
		public String AppFolderPath { get { return Path.GetDirectoryName(SettingsFilePath); } }
		private String _settingsFilePath;
		public String SettingsFilePath
		{
			get
			{
				if (_settingsFilePath != null)
					return _settingsFilePath;

				var binFolder = ModuleFolderPath;
				var appFolder = Path.GetDirectoryName(binFolder);
				var settingFile = Path.Combine(appFolder, "BarbaTunnel.ini");
				if (File.Exists(settingFile))
				{
					_settingsFilePath = settingFile;
					return _settingsFilePath;
				}

				//maybe we are in configuration folder in development environment
				appFolder = Path.GetDirectoryName(appFolder);
				settingFile = Path.Combine(appFolder, "BarbaTunnel.ini");
				if (File.Exists(settingFile))
					_settingsFilePath = settingFile;

				return _settingsFilePath;
			}
		}

		public event EventHandler NotifyChanged;
		public event EventHandler LogChanged;
		public event EventHandler StatusChanged;
		public BarbaStatus Status { get; private set; }
		FileSystemWatcher _fileWatcher;

		public BarbaComm()
		{
			Status = BarbaStatus.Stopped;
			Directory.CreateDirectory(WorkinFolderPath);
		}

		void CreateFileWatcher()
		{
			// Create a new FileSystemWatcher for log
			_fileWatcher = new FileSystemWatcher
							   {
								   Path = Path.GetDirectoryName(LogFilePath),
								   Filter = "*.txt",
								   NotifyFilter = NotifyFilters.LastWrite
							   };

			// Add event handlers.
			_fileWatcher.Changed += LogWatcher_LogChanged;
			_fileWatcher.Created += LogWatcher_LogChanged;
			_fileWatcher.Deleted += LogWatcher_LogChanged;

			// Begin watching.
			_fileWatcher.EnableRaisingEvents = true;
		}

		void StatusChecker(Object stateInfo)
		{
			UpdateStatus();
		}

		void UpdateStatus()
		{
			try
			{
				//check running before read status
				if (!IsRunnig && Status != BarbaStatus.Stopped)
				{
					Status = BarbaStatus.Stopped;
					if (StatusChanged != null)
						StatusChanged(this, new EventArgs());
				}

				//read status
				StringBuilder st = new StringBuilder(100);
				GetPrivateProfileString("General", "Status", "", st, 100, CommFilePath);
				if (String.IsNullOrEmpty(st.ToString()))
					return;

				BarbaStatus status = IsRunnig ? (BarbaStatus)Enum.Parse(typeof(BarbaStatus), st.ToString(), false) : BarbaStatus.Stopped;
				//check idle state
				if (status == BarbaStatus.Started && IsIdle)
					status = BarbaStatus.Idle;

				//update status if it change
				if (Status != status)
				{
					Status = status;
					if (StatusChanged != null)
						StatusChanged(this, new EventArgs());
				}
			}
			catch { }
		}

		// ReSharper disable PossibleNullReferenceException
		void LogWatcher_LogChanged(object sender, FileSystemEventArgs e)
		{
			if (LogChanged != null && Path.GetFileName(LogFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
				LogChanged(this, new EventArgs());
			else if (NotifyChanged != null && Path.GetFileName(NotifyFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
				NotifyChanged(this, new EventArgs());
			else if (Path.GetFileName(CommFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
				UpdateStatus();
		}
		// ReSharper restore PossibleNullReferenceException

		private EventWaitHandle OpenCommandEvent()
		{
			return EventWaitHandle.OpenExisting("Global\\BarbaTunnel_CommandEvent");
		}

		private EventWaitHandle OpenServiceEvent()
		{
			return EventWaitHandle.OpenExisting("Global\\BarbaTunnel_ServiceEvent");
		}

		public void Start()
		{
			Start(false);
		}

		/// <summary>
		/// Tell BarbaServer to delay, ignore for barba client
		/// </summary>
		public void Start(bool delayMode)
		{
			try
			{
				System.Diagnostics.Process p = new System.Diagnostics.Process();
				p.StartInfo.FileName = BarbaTunnelFilePath;
				p.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
				if (delayMode) p.StartInfo.Arguments = "/delaystart";
				p.Start();
			}
			catch { }
		}

		public void Restart()
		{
			try
			{
				var res = OpenCommandEvent();
				if (res != null)
				{
					WritePrivateProfileString("General", "Command", "Restart", CommFilePath);
					res.Set();
					res.Close();
				}
			}
			catch { }
		}

		public Boolean IsAutoStartTunnel
		{
			get
			{
				return GetPrivateProfileInt("General", "AutoStartTunnel", 1, OptionsFilePath) != 0;
			}
			set
			{
				WritePrivateProfileString("General", "AutoStartTunnel", value ? "1" : "0", OptionsFilePath);
			}
			
		}

		public int LogLevel
		{
			get
			{
				return GetPrivateProfileInt("General", "LogLevel", 1, SettingsFilePath);
			}
			set
			{
				try
				{
					WritePrivateProfileString("General", "LogLevel", value.ToString(), SettingsFilePath);
					var res = OpenCommandEvent();
					if (res != null)
					{
						WritePrivateProfileString("General", "Command", "UpdateSettings", CommFilePath);
						res.Set();
						res.Close();
					}
				}
				catch { }
			}
		}

		public void Stop()
		{
			try
			{
				var res = OpenCommandEvent();
				if (res != null)
				{
					WritePrivateProfileString("General", "Command", "Stop", CommFilePath);
					res.Set();
					res.Close();
				}
			}
			catch { }
		}

		public void StartByService()
		{
			var res = OpenServiceEvent();
			if (res != null)
			{
				res.Set();
				res.Close();
			}
		}

		public bool IsServiceRunnig
		{
			get
			{
				try
				{
					var res = OpenServiceEvent();
					if (res != null)
						res.Close();
					return res != null;
				}
				catch
				{

					return false;
				}
			}
		}


		bool IsRunnig
		{
			get
			{
				try
				{
					var res = OpenCommandEvent();
					if (res != null)
						res.Close();
					return res != null;
				}
				catch
				{

					return false;
				}
			}
		}

		public void Initialize()
		{
			Initialize(true);
		}

		private Timer _statusCheckerTimer;
		public void Initialize(bool enableStatusChangeEvent)
		{
			_statusCheckerTimer = new Timer(StatusChecker, null, 0, 1000);
			if (enableStatusChangeEvent)
			{
				CreateFileWatcher();
				UpdateStatus();
			}
		}

		public void Dispose()
		{
			//if (_FileWatcher != null)
			//_FileWatcher.Dispose(); //do not dispose it here; it may wait much time to finish
		}

		public String ReadNotify(out String title)
		{
			try
			{
				using (var fs = File.Open(NotifyFilePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
				using (var logReader = new StreamReader(fs, Encoding.UTF8))
				{
					title = logReader.ReadLine();
					return logReader.ReadToEnd();
				}
			}
			catch
			{
				title = String.Empty;
				return "";
			}
		}

		public String ReadLog()
		{
			var log = "";
			try
			{
				const int maxSize = 60 * 1000;
				using (var fs = File.Open(LogFilePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
				{
					var pos = Math.Max(fs.Length - maxSize, 0);
					fs.Seek(pos, SeekOrigin.Begin);
					using (var logReader = new StreamReader(fs, Encoding.UTF8))
					{
						log = logReader.ReadToEnd();
						var nl = log.IndexOf("\r\n", StringComparison.Ordinal);
						if (pos > 0 && nl != -1)
						{
							log = log.Substring(nl + 2);
						}
					}
				}
			}
			catch
			{ }

			return log;

		}

		DateTime ReadLastWorkTime()
		{
			StringBuilder st = new StringBuilder(100);
			GetPrivateProfileString("General", "LastWorkTime", "", st, 100, CommFilePath);
			if (String.IsNullOrEmpty(st.ToString()))
				return new DateTime();
			Int64 ctime = Convert.ToInt64(st.ToString());
			return CTimeToDate(ctime);
		}

		bool IsIdle
		{
			get
			{
				DateTime lastWorkTime = ReadLastWorkTime();
				return DateTime.Now.Subtract(lastWorkTime).TotalSeconds > 5 * 60; //5min

			}
		}

		static DateTime CTimeToDate(Int64 ctime)
		{
			TimeSpan span = TimeSpan.FromTicks(ctime * TimeSpan.TicksPerSecond);
			DateTime t = new DateTime(1970, 1, 1).Add(span);
			return TimeZone.CurrentTimeZone.ToLocalTime(t);
		}


	}
}
