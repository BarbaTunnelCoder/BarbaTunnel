BarbaClient configuration file.

{{
[General](General)
ServerName=Your Server Friendly Name
ServerAddress=0.0.0.0
Key=1234567890ABCDEF

[Item1](Item1)
Name=VPN-PPTP Retunnel
Enabled=1
Mode=UDP-Tunnel
TunnelPorts=9700,9800-9900,17000-17200
GrabProtocols=GRE:*,TCP:1723

[Item2](Item2)
Name=VPN-SSTP Redirect
Enabled=1
Mode=TCP-Redirect
TunnelPorts=8700,8800-8900,18000-18200
RealPort=443

[.](.)
.
.
.
}}

## ServerName
**Optional**, Default is ServerAddress.
You can name this configuration file, usually the your server friendly name such as "VpnServer". BarbaMonitor display this name when use this configuration.

## ServerAddress
**Required**
The IP4 address of your server in this format: 0.0.0.0

## Key
**Optional**, Default is empty key and will stop the encryption.
Your simple shared encryption key in Hex . BarbaTunnel XOR all packet data with this key. VPN already perform encryption so strong encryption does not required.
* Make sure server use same key.

## Name
**Optional**, Default is Item Mode.
Your optional friendly name for the item. BarbaMonitor display this name when use this item configuration.

## Enabled
**Optional**, Default is "1".
It can be "0" or "1". If "0" the item will be disabled.

## Mode
**Required**
Can be "UDP-Tunnel" | "UDP-Redirect" | "TCP-Redirect".

## TunnelPorts
**Required**
Range of tunnel ports. BarbaClient will choose one of this port by random.
Format: TunnelPorts=StartPornt-EndPort,StartPornt-EndPort,,StartPornt-EndPort,...
If you like to use only one port just set one port such as: TunnelPorts=80
* Make sure BarbaServer accept this port.
* Make sure your network firewall does not block this port.
* System Local Firewall such as Windows Firewall does not effect on BarbaTunnel, you don't need any change to them.

## RealPort
**Required**, When mode is UDP-Redirect or TCP-Redirect.
BarbaClient will redirect this port to a tunnel port.

## GrabProtocols
**Required**, When mode is UDP-Tunnel or TCP-Tunnel.
BarbaClient grab sending packet from this protocols and put them to tunnel.
Format: Protocol:Port
Sample1 for PPTP: GRE:*,TCP:1723
Sample2: {"UDP:150,TCP:500,ICMP:**,IGMP**,GGP**,PUP:**,IDP:**,ND:**"}

# Remarks
* Make sure items be sequenced such as Item1, Item2, Item3, ....
