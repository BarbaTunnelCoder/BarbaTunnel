BarbaTunnel Configuration
_Note: This document is just for latest version. Make sure that you have the latest version. _
_For version 6.4 or older see [barbatunnel.ini (ver 6.4)](barbatunnel.ini-(ver-6.4))_

{{
[General](General)
ServerMode=
DebugMode=
LogLevel=
LogAnonymously=
TimeZone=
ConnectionTimeout=
AdapterIndex=
MTUDecrement=
FilterDriver=

[Server](Server)
VirtualIpRange=
AutoStartDelay=
}}

## ServerMode
**Optional**, Default is "0".
Set "0" to start BarbaTunnel as client and "1" to start BarbaTunnel as a server.

## LogLevel
**Optional**, Default is 1.
Can be 1, 2 or 3
Higher LogLevel will produce more reports in the log report.

## LogAnonymously
**Optional**, Default is 1.
Set "0" to log the user's identity such as the IP address in the log report, Set "1" to hide the user's identity such as the IP address in the log report.

## TimeZone
**Optional**, Default is machine time zone.
Any invalid format will be treated as local machine time zone.
Format: UTC+HH:MM
Sample1: UTC+6:00
Sample2: UTC-6:00

## ConnectionTimeout
**Optional**, Default is 15.
Timeout value for Barba Connections in minutes. It also used in server for TCP connection timeout if KeepAlive was not set.

## DebugMode
**Optional**, Default is "0".
Set "1" to enable debug mode, set "0" to disable debug mode.
In debug mode you can stop server by sending the following ping to your server:
ping serverip /l 1350

## AdapterIndex
**Optional**, Default is "0". Valid for data-link filters such as WinpkFilter. Ignored in WinDivert.
Index of main network Adapter that BarbaTunnel should monitor it. It should be a main network adapter. If "0" BarbaTunnel try to find it, otherwise you should set it manually.
To see the list of all available Adapters run "listadapters.exe" from WinpkFilter folder.

## MTUDecrement
**Optional**, Default is 60. Valid for data-link filters such as WinpkFilter. Ignored in WinDivert.
Set "-1" to prevent BarbaTunnel change the current system MTU Decrement. For UDP-Tunnel it should be at least to 60.

## FilterDriver
**Optional**, Default is "WinDivert".
Can be WinDivert or WinpkFilter.

## VirtualIpRange
**Optional**, Default is 10.207.0.0-10.207.255.255. Ignored in client mode.
BarbaServer assign a Virtual IP to each client connection.
* Make sure this range does not conflict with your other server network.
* Make sure the request for this IP should pass to your gateway.

## AutoStartDelay
**Optional**, default is 0. Ignored in client mode.
BarbaTunnel service will automatically start after this time in minutes. This helps you to stop the service from being started, in case of malfunction.