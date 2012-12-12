using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;

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

        public String WorkinFolderPath { get { return System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "Barbatunnel"); } }
        public String CommFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "comm.txt"); } }
        public String LogFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "report.txt"); } }
        public String NotifyFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "notify.txt"); } }
        public String ModuleFolderPath { get { return System.IO.Path.GetDirectoryName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName); } }
        public String BarbaTunnelFilePath { get { return System.IO.Path.Combine(ModuleFolderPath, IntPtr.Size == 4 ? "x86\\barbatunnel.exe" : "x64\\barbatunnel.exe"); } }
        public String AppFolderPath { get { return System.IO.Path.GetDirectoryName(SettingsFilePath); } }
        private String _SettingsFilePath;
        public String SettingsFilePath
        {
            get
            {
                if (_SettingsFilePath!=null)
                    return _SettingsFilePath;

                String binFolder = ModuleFolderPath;
                String appFolder = System.IO.Path.GetDirectoryName(binFolder);
                String settingFile = System.IO.Path.Combine(appFolder, "BarbaTunnel.ini");
                if (System.IO.File.Exists(settingFile)) 
                {
                    _SettingsFilePath = settingFile;
                    return _SettingsFilePath;
                }

            	//maybe we are in configuration folder in development environment
                appFolder = System.IO.Path.GetDirectoryName(appFolder);
                settingFile = System.IO.Path.Combine(appFolder, "BarbaTunnel.ini");
                if (System.IO.File.Exists(settingFile))
                    _SettingsFilePath = settingFile;

                return _SettingsFilePath;
            }
        }

        public event EventHandler NotifyChanged;
        public event EventHandler LogChanged;
        public event EventHandler StatusChanged;
        public BarbaStatus Status { get; private set; }
        FileSystemWatcher _FileWatcher;
        Timer _StatusTimer;

        public BarbaComm()
        {
            Status = BarbaStatus.Stopped;
            System.IO.Directory.CreateDirectory(WorkinFolderPath);
        }

        void CreateFileWatcher()
        {
            // Create a new FileSystemWatcher for log
            _FileWatcher = new FileSystemWatcher();
            _FileWatcher.Path = System.IO.Path.GetDirectoryName(LogFilePath);
            _FileWatcher.Filter = "*.txt";
            _FileWatcher.NotifyFilter = NotifyFilters.LastWrite;

            // Add event handlers.
            _FileWatcher.Changed += new FileSystemEventHandler(LogWatcher_LogChanged);
            _FileWatcher.Created += new FileSystemEventHandler(LogWatcher_LogChanged);
            _FileWatcher.Deleted += new FileSystemEventHandler(LogWatcher_LogChanged);

            // Begin watching.
            _FileWatcher.EnableRaisingEvents = true;
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
                if (!this.IsRunnig && this.Status != BarbaStatus.Stopped)
                {
                    this.Status = BarbaStatus.Stopped;
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
                if (this.Status != status)
                {
                    this.Status = status;
                    if (StatusChanged != null)
                        StatusChanged(this, new EventArgs());
                }
            }
            catch { }
        }

        void LogWatcher_LogChanged(object sender, FileSystemEventArgs e)
        {
            if (LogChanged != null && System.IO.Path.GetFileName(LogFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
                LogChanged(this, new EventArgs());
            else if (NotifyChanged != null && System.IO.Path.GetFileName(NotifyFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
                NotifyChanged(this, new EventArgs());
            else if (System.IO.Path.GetFileName(CommFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
                UpdateStatus();
        }

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

        public Boolean VerboseMode
        {
            get
            {
                return GetPrivateProfileInt("General", "VerboseMode", 0, SettingsFilePath) != 0;
            }
            set
            {
                try
                {
                    WritePrivateProfileString("General", "VerboseMode", value ? "1" : "0", SettingsFilePath);
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

        public void Initialize(bool enableStatusChangeEvent)
        {
            if (enableStatusChangeEvent)
            {
                CreateFileWatcher();
                _StatusTimer = new Timer(StatusChecker, null, 0, 1000);
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
                const int MaxSize = 200*1000;
                using (var fs = File.Open(LogFilePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                {
                    var pos = Math.Max(fs.Length - MaxSize, 0);
                    fs.Seek(pos, SeekOrigin.Begin);
                    using (var logReader = new StreamReader(fs, Encoding.UTF8))
                    {
                        log = logReader.ReadToEnd();
                        var nl = log.IndexOf("\r\n");
                        if (pos > 0 && nl != -1)
                        {
                            log = log.Substring(nl + 2);
                        }
                    }
                }
            }
            catch { }

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
