BarbaTunnel usually should be used conjunction with a VPN or an exiting tunnel or proxy software , it can also be used for port redirection too.

# Requirements
BarbaTunnel requires .NET Framework 4.5 for "Barba Monitor" and "Barba Service", but BarbaTunnel.exe does not need .NET Framework, so you can run it manually on both client and server side without .NET Framework.

# Configure Server
# Login to your Windows Server.
# Download BarbaTunnel and extract it.
# Open "[barbatunnel.ini](barbatunnel.ini)" in BarbaTunnel folder and set "ServerMode=1"
# Go to BarbaTunnel folder and open "config\servername" folder then open "HTTP-Retunnel.ini" file.
	* Set "ServerAddress" to your server ip address (required).
# Run "Install.vbs"
# Run "Run.Vbs"
	* Server already configured for specific ports, for custom configuration see "[config.ini](config.ini)".

# Configure Client Machine
# Login to your Windows Client.
# Download BarbaTunnel and extract it.
# Copy "config" folder and its config files that you have already created them in the server machine.
# Run "Install.vbs"
# Run "Run.Vbs"
# Try to establish a VPN connection to your server

# Remarks
* It is recommended to rename "servername" folder to your server name or server ip (optional).
* Ensure the major version of BarbaServer and BarbaClient is same. Such 1.0 and 1.1
* Make sure both server config file and client config file is same.
* Make sure the enterprise firewall does not block tunnel ports.
* Make sure your Local Firewall such as Windows Firewall does not block tunnel ports or BarbaTunnel.
* Make sure you have access to reboot your system if you lose the connection to your server, before run BarbaTunnel you can create a timer-job to restart your server if you have limited access to your server reboot.

# Support
Doesn't work! BarbaTunnel already tested behind most sophisticated firewalls, please don't hesitate to contact us at BarbaTunnel [Discussions](http://barbatunnel.codeplex.com/discussions).
We are very eager to know why it doesn't work.