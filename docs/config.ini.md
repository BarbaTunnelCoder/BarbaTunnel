Configuration File
_Note: This document is just for latest version. Make sure that you have the latest version. _
_For version 6.4 or older see [config.ini (ver 6.4)](config.ini-(ver-6.4))_

{{
[General](General)
ServerAddress=x.x.x.x
Enabled=
Mode=HTTP-Tunnel | TCP-Tunnel | UDP-Tunnel | TCP-Redirect | UDP-Redirect | UDP-Simple-Tunnel
Key=1234567890ABCDEF
GrabProtocols=GRE:**,TCP:1723,ESP:**,UDP:1701,UDP:500,UDP:4500
TunnelPorts=9700,9800-9900,17000-17200
RealPort=
MaxUserConnections=
FakeFileTypes=jpg,zip
MaxTransferSize=
MinPacketSize=
MaxPacketSize=
KeepAliveInterval=
HttpRequestMode=

[Server](Server)
AllowHttpRequestNone=
AllowHttpRequestNormal=
AllowHttpRequestBombard=
}}

## ServerAddress
**Required**.
It can be IP4 address or domain name such as vpn.server.com. It is recommended to use IP address instead domain name, so if domain name could not resolve at start time, you should restart Barbatunnel.
Server Mode: If set 0 then the server will listen to all associated network IP.
Format: x.x.x.x

## Enabled
**Optional**, Default is "1".
It can be "0" or "1". If "0" the item will be disabled.

## Mode
**Required**
Can be "HTTP-Tunnel" or "TCP-Tunnel" or "UDP-Tunnel" or "UDP-Redirect" or "TCP-Redirect".

## Key
**Optional**, Default is empty key and will stop the encryption.
Your simple shared encryption key in Hex . BarbaTunnel XOR all packet data with this key. A VPN has already performed encryption so strong encryption does not require.
* Make sure server use same key.

## TunnelPorts
**Required**
Range of tunnel ports. BarbaClient will choose one of these ports by random.
Format: TunnelPorts=StartPornt-EndPort,StartPornt-EndPort,,StartPornt-EndPort,...
If you like to use only one port just set one port such as: TunnelPorts=80
* Make sure BarbaServer accept these ports.
* Make sure your network firewall does not block this port.
* System Local Firewall such as Windows Firewall does not effect on BarbaTunnel, you don't need any change to them.

## GrabProtocols
**Required**, Valid for UDP-Tunnel and TCP-Tunnel. Ignored in server mode.
BarbaClient grab packets of this protocol and put them to tunnel.
Format: Protocol:Port
Sample1 for PPTP: GRE:*,TCP:1723
Sample2 for L2TP: ESP:*,UDP:1701,UDP:500,UDP:4500
Sample3 for Custom: {"UDP:150,TCP:500,ICMP:**,IGMP**,GGP**,PUP:**,IDP:**,ND:**"}

## RealPort
**Required**, Valid for UDP-Redirect and TCP-Redirect.
BarbaClient will redirect this port to a tunnel port.

## MaxUserConnections
**Optional**, default is 5. Valid for HTTP-Tunnel and TCP-Tunnel.
Maximum TCP connection per user for each HTTP-Tunnel.

## FakeFileTypes
**Optional**, default is empty and will not send fake header file. Valid for HTTP-Tunnel.
List of Header file that should be send to simulate HTTP connection POST and GET. a type will be selected by random.
Header files should be exists in Templates folder with header extension. eg: jpg.header
Format: ext,ext,ext

## MaxTransferSize
**Optional**, default is 15000 (15MB). Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in server mode.
The maximum transfer size of each connection in kilobyte that going to GET or POST or TCP channel. a range between MaxTransferSize/2 and MaxTransferSize will be selected by random for each connection.

## MinPacketSize
**Optional**, default is 0. Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in server mode.
Indicate the minimum size in byte of each packet that going to send or receive. Barbatunnel add extra padding bytes to make packets look bigger.
_Note_: When used it will decrease the bandwidth.

## MaxPacketSize
**Optional**, default is 1500. Valid for Udp-Tunnel. Ignored in server mode.
Indicate the maximum size in byte of each packet that going to send or receive. BarbaTunnel will shrink packets if they exceed this maximum size.
_Note_: Small value will decrease the bandwidth.

## KeepAliveInterval
**Optional**, default is 30. Valid for HTTP-Tunnel, TCP-Tunnel and UDP-Tunnel. Ignored in server mode.
Time in second for keep alive mechanism that make sure all HTTP connections and UDP port is valid and up. 
Set 0 to disable it.

## HttpRequestMode
**Optional**, default is "None" for TCP-Tunnel and "Normal" for HTTP-Tunnel. Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in server mode.
Can be "None" or "Normal" or "Bombard".
More info coming soon...

## AllowHttpRequestNone
**Optional**, default is 1. Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in client mode.
Set 0 if you don't want server accept "None" http request. 
Set 1 if you want server accept "None" http request. 

## AllowHttpRequestNormal
**Optional**, default is 1. Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in client mode.
Set 0 if you don't want server accept "Normal" http request. 
Set 1 if you want server accept "Normal" http request. 

## AllowHttpRequestBombard
**Optional**, default is 1. Valid for HTTP-Tunnel and TCP-Tunnel. Ignored in client mode.
Set 0 if you don't want server accept "Bombard" http request. 
Set 1 if you want server accept "Bombard" http request. 

# Remarks
* You should put this file in a folder in BarbaTunnel Config folder. It is recommended to create a folder with your server domain name or IP address in BarbaTunnel config folder, then put this file in it. Such as: barbatunnel\config\yourserver.com\HTTP-Retunnel.ini