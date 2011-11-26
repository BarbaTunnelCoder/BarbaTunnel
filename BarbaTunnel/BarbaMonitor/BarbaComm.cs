using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Security.AccessControl;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;

namespace BarbaMonitor
{
    class BarbaComm : IDisposable
    {
        [DllImport("KERNEL32.DLL", EntryPoint = "GetPrivateProfileStringW", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true, CallingConvention = CallingConvention.StdCall)]
        private static extern int GetPrivateProfileString(string lpAppName, string lpKeyName, string lpDefault, string lpReturnString, int nSize, string lpFilename);


        public String WorkinFolderPath { get { return System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "Barbatunnel"); } }
        public String CommFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "comm.txt"); } }
        public String LogFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "report.txt"); } }
        public String NotifyFilePath { get { return System.IO.Path.Combine(WorkinFolderPath, "notify.txt"); } }
        public event EventHandler NotifyChanged;
        public event EventHandler LogChanged;
        FileSystemWatcher _FileWatcher;

        private void CreateFileWatcher()
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

        void LogWatcher_LogChanged(object sender, FileSystemEventArgs e)
        {
            if (LogChanged != null && System.IO.Path.GetFileName(LogFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
                LogChanged(this, new EventArgs());
            else if (NotifyChanged != null && System.IO.Path.GetFileName(NotifyFilePath).Equals(e.Name, StringComparison.InvariantCultureIgnoreCase))
                NotifyChanged(this, new EventArgs());
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

        public bool IsStarted { get { return true; } }
    }
}
