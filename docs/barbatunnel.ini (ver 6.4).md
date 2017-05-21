BarbaTunnel Configuration (**Legacy Version 6.4 or Older**). 
**Note: This document is just for version 6.4 or older.**

{{
[General](General)
AdapterIndex=
ServerMode=
DebugMode=
VerboseMode=
MTUDecrement=
ConnectionTimeout=
FilterDriver=

[Server](Server)
VirtualIpRange=
AutoStartDelay=
}}

## ServerMode
**Optional**, Default is "0".
Set "0" to start BarbaTunnel as client and "1" to start BarbaTunnel as a server.

## VerboseMode
**Optional**, Default is 0.
0 to disable, 1 for enable.
Enabled it to get more reports. 

## DebugMode
**Optional**, Default is "0".
Set "1" to enable debug mode, set "0" to disable debug mode.
In debug mode you can stop server by sending the following ping to your server:
ping serverip /l 1350

## FilterDriver
**Optional**, Default is "WinDivert".
Can be WinDivert or WinpkFilter.

## AdapterIndex
**Optional**, Default is "0". Valid for data-link filters such as WinpkFilter.
Index of main network Adapter that BarbaTunnel should monitor it. It should be a main network adapter. If "0" BarbaTunnel try to find it, otherwise you should set it manually.
To see the list of all available Adapters run "listadapters.exe" from WinpkFilter folder.

## MTUDecrement
**Required** for UDP-Tunnel.
Set Empty or -1 to don't change current system MTU Decrement. For UDP-Tunnel it should set to 60.

## ConnectionTimeout
**Optional**, Default is 15.
Timeout value for Barba Connections in minutes. It also used in server for TCP connection timeout if KeepAlive was not set.

## VirtualIpRange
**Optional**, Default is 10.207.0.0-10.207.255.255. Ignored in client mode.
BarbaServer assign a Virtual IP to each client connection.
* Make sure this range does not conflict with your other server network.
* Make sure the request for this IP should pass to your gateway.

## AutoStartDelay
**Optional**, default is 0. Ignored in client mode.
BarbaTunnel service will automatically start after this time in minutes. This helps you to stop the service from being started, in case of malfunction.