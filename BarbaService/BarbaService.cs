using System.ServiceProcess;
using System.Threading;
using System.Security.AccessControl;

namespace BarbaTunnel.Service
{
    public partial class BarbaService : ServiceBase
    {
	    readonly CommLib.BarbaComm _barbaComm = new CommLib.BarbaComm();
        EventWaitHandle _serviceEvent;
        Thread _serviceEventThread;
        bool _stopping;
        public BarbaService()
        {
            InitializeComponent();
            _barbaComm.Initialize(false);
        }

        private void InitServiceEvent()
        {
            _stopping = false;
            var sec = new EventWaitHandleSecurity();
            sec.AddAccessRule(new EventWaitHandleAccessRule("Everyone", EventWaitHandleRights.Synchronize | EventWaitHandleRights.Modify, AccessControlType.Allow));
            bool createdNew;
            _serviceEvent = new EventWaitHandle(false, EventResetMode.AutoReset, "Global\\BarbaTunnel_ServiceEvent", out createdNew, sec);
            _serviceEventThread = new Thread(EventThreadMethod);
            _serviceEventThread.Start();
        }

        protected override void OnStart(string[] args)
        {
            //when BarbaService is running, BarbaTunnel should run by service not by user
            InitServiceEvent();
			if (_barbaComm.IsAutoStartTunnel)
				_barbaComm.Start(true);
        }

        protected override void OnStop()
        {
            _stopping = true;
            _serviceEvent.Set();
            _barbaComm.Stop();
            _serviceEvent.Close();
        }

        void EventThreadMethod()
        {
            try
            {
                while (_serviceEvent.WaitOne() && !_stopping)
                {
                    _barbaComm.Start();
                }
            }
// ReSharper disable EmptyGeneralCatchClause
            catch { }
// ReSharper restore EmptyGeneralCatchClause
        }

    }
}
