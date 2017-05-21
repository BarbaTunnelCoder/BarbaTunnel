BarbaServer Configuration file.

{{
[General](General)
VirtualIpRange=10.207.0.0-10.208.0.0
Key=1234567890ABCDEF
AutoStartDelayMinutes=4

[Item1](Item1)
Enabled=1
Mode=UDP-Tunnel
TunnelPorts=9700,9800-9900,17000-17200

[Item2](Item2)
Enabled=1
Mode=TCP-Redirect
TunnelPorts=8700,8800-8900,18000-18200
RealPort=443

[.](.)
.
.
.
}}

## VirtualIpRange
**Required**
BarbaServer assign an Virtual IP to each client connection.
* Make sure this range does not conflict with your other server network.
* Make sure the request for this IP should pass to your gateway.

## Key
**Optional**, Default is empty key and will stop the encryption.
Your simple shared encryption key in Hex . BarbaTunnel XOR all packet data with this key. VPN already perform encryption so strong encryption does not required.
* Make sure client use same key.

## AutoStartDelayMinutes
**Optional**, Default is "0".
When you run BarbaTunnel as service in server, BarbaServer will run "BarbaTunnel.exe" with "/delaystart" command-line and BarbaTunnel wait till this time elapsed. It help your to recover your server if BarbaTunnel crash your network connection.
* This command will not effect on client mode.

## Enabled
**Optional**, Default is "1".
It can be "0" or "1". If "0" the item will be disabled.

## Mode
**Required**
Can be "UDP-Tunnel" | "UDP-Redirect" | "TCP-Redirect".

## TunnelPorts
**Required**
Range of accepting tunnel ports in server.
Format: StartPornt-EndPort,StartPornt-EndPort,,StartPornt-EndPort,...
* Make sure your network firewall does not block this port.
* System Local Firewall such as Windows Firewall does not effect on BarbaTunnel, you don't need any change to them.
* Make sure you don't put ports that used by services in your server such as Remote Desktop otherwise that services will be stopped.

## RealPort
**Required**, When mode is UDP-Redirect or TCP-Redirect.
BarbaServer will redirect tunnel port to this port.

# Remarks
* Make sure items be sequenced such as Item1, Item2, Item3, ....
