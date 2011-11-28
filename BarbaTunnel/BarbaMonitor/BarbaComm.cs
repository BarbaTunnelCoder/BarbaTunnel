using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Security.AccessControl;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;
using System.Windows;

namespace BarbaMonitor
{
    public enum BarbaStatus
    {
        Waiting,
        Stopped,
        Started,
        Idle,
    }

    class BarbaComm : IDisposable
    {
        [DllImport("KERNEL32.DLL", EntryPoint = "GetPrivateProfileStringW", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
        private static extern int GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault, StringBuilder retVal, int nSize, string lpFilename);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool WritePrivateProfileString(string lpAppName, string lpKeyName, string lpString, string lpFileName);


        public String WorkinFolderPath { get { return System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "Barbatunnel"); } }
        public String CommFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "comm.txt"); } }
        public String LogFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "report.txt"); } }
        public String NotifyFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "notify.txt"); } }
        public String ModuleFolderPath { get { return System.IO.Path.GetDirectoryName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName); } }
        public String BarbaTunnelFilePath { get { return System.IO.Path.Combine(ModuleFolderPath, "barbatunnel.exe"); } }
        public event EventHandler NotifyChanged;
        public event EventHandler LogChanged;
        public event EventHandler StatusChanged;
        public BarbaStatus Status { get; private set; }
        FileSystemWatcher _FileWatcher;
        Timer _StatusTimer;

        public BarbaComm()
        {
            Status = BarbaStatus.Stopped;
            _StatusTimer = new Timer(StatusChecker, null, 0, 1000);
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
            if (!this.IsRunnig && this.Status != BarbaStatus.Stopped)
            {
                this.Status = BarbaStatus.Stopped;
                if (StatusChanged != null)
                    StatusChanged(this, new EventArgs());
            }
        }

        void UpdateStatus()
        {
            StringBuilder st = new StringBuilder(100);
            GetPrivateProfileString("General", "Status", "", st, 100, CommFilePath);
            if (String.IsNullOrEmpty(st.ToString()))
                return;

            try
            {
                BarbaStatus status = IsRunnig ? (BarbaStatus)Enum.Parse(typeof(BarbaStatus), st.ToString(), false) : BarbaStatus.Stopped;
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

        public void Start()
        {
            System.Diagnostics.Process p = new System.Diagnostics.Process();
            p.StartInfo.FileName = BarbaTunnelFilePath;
            p.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
            p.Start();
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

        bool IsRunnig
        {
            get
            {
                try
                {
                    var res = EventWaitHandle.OpenExisting("Global\\BarbaTunnel_CommandEvent");
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
            //bool create;
            //var sec = new EventWaitHandleSecurity();
            //sec.SetAccessRule(new EventWaitHandleAccessRule("everyone", EventWaitHandleRights.FullControl, AccessControlType.Allow));
            //NotifyEvent = new EventWaitHandle(false, EventResetMode.ManualReset, "Global\\BarbaTunnel_NotifyEvent", out create, sec);
            //LogEvent = new EventWaitHandle(false, EventResetMode.ManualReset, "Global\\BarbaTunnel_LogEvent", out create, sec);
            //_WaitForLogTread = new Thread(new ThreadStart(WaitForLogTreadMethod));
            //_WaitForLogTread.Start();
            CreateFileWatcher();
            UpdateStatus();
        }

        public void Dispose()
        {
            _FileWatcher.Dispose();
        }

        public String ReadNotify(out String title)
        {
            try
            {
                var fs = File.Open(NotifyFilePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                var logReader = new StreamReader(fs, Encoding.UTF8);
                title = logReader.ReadLine();
                return logReader.ReadToEnd();
            }
            catch
            {
                title = String.Empty;
                return "";
            }
        }

        public String ReadLog()
        {
            try
            {
                var fs = File.Open(LogFilePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                var logReader = new StreamReader(fs, Encoding.UTF8);
                return logReader.ReadToEnd();
            }
            catch
            {
                return "";
            }
        }

    }
}
