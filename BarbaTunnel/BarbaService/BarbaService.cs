using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.Threading;
using System.Security.AccessControl;

namespace BarbaTunnel.Service
{
    public partial class BarbaService : ServiceBase
    {
        BarbaTunnel.CommLib.BarbaComm BarbaComm = new BarbaTunnel.CommLib.BarbaComm();
        EventWaitHandle ServiceEvent;
        Thread ServiceEventThread;
        public BarbaService()
        {
            InitializeComponent();
            BarbaComm.Initialize(false);
        }

        private void InitServiceEvent()
        {
            EventWaitHandleSecurity sec = new EventWaitHandleSecurity();
            sec.AddAccessRule(new EventWaitHandleAccessRule("Everyone", EventWaitHandleRights.Synchronize | EventWaitHandleRights.Modify, AccessControlType.Allow));
            bool createdNew;
            ServiceEvent = new EventWaitHandle(false, EventResetMode.AutoReset, "Global\\BarbaTunnel_ServiceEvent", out createdNew, sec);
            ServiceEventThread = new Thread(new ThreadStart(EventThreadMethod));
            ServiceEventThread.Start();
        }

        protected override void OnStart(string[] args)
        {
            //when BarbaService is running, BarbaTunnel should run by service not by user monitor
            InitServiceEvent();

            BarbaComm.Start();
        }

        protected override void OnStop()
        {
            ServiceEventThread.Abort();
            BarbaComm.Stop();
            ServiceEvent.Close();
        }

        void EventThreadMethod()
        {
            try
            {
                if (ServiceEvent.WaitOne())
                {
                    BarbaComm.Start();
                }
            }
            catch { }
        }

    }
}
