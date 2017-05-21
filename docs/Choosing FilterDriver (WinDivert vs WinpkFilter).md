## WinDivert vs WinpkFilter
BarbaTunnel need a Packet Filter to process and modify the outgoing and incoming packet from the whole device. BarbaTunnel initially use WinPkFilter but later I add WinDivert and make it as a default Filter Driver.

Actually WinpkFilter in much more than WinDivert itself because WinpkFilter created from draft, has own virtual adapter driver while WinpkDivert is powered by WFP ([Windows Filtering Platform](http://msdn.microsoft.com/en-us/library/windows/hardware/gg463267.aspx)) that developed by Microsft and embedded in Windows Vista and later. Capturing and modifying packet with WFP requites windows DDK programming and much stuff, So Thanks to WinDivert that wrote a wrapper over it and make it simple and usable in user mode.

Please note the actual in-depth comparison should between WFP and WinpkFilter, but as a point of BarbaTunnel view I compare WinDivert with WinpkFilter. I didn't also compare the performance and don't have a knowledge about potential bug and security hole.

### WinDivert Pros
# Powered by Microsoft built-in WFP
# Zero installation and does not need to restart.
# I couldn't run WinPkFilter on XEN virtual platforms but successfully run WinDivert over a virtualized XEN system.
# WinDivert is an open source tool with LGPL license while WinPkFilter is a commercial product and indicate that for open source application the runtime should to be installed from ntkernel site.
# With WinPkFilter, when my application freezes due my own software issue, it also freezes all of the network interfaces even whose that I don't filter. This really made me mad.

### WinDivert Cons
# WinDivert does not support Windows XP and 2003 because WFP does not exist there while WinPkFilter work. By the way Windows XP is going to be deprecated in [08/04/2014](http://support.microsoft.com/lifecycle/?ln=en-gb&c2=1173).

### Others
# WinDivert filter packets at Network Transport Layer while WinPKFilter filter packets at Network Data-Link Layer.
# WinPKFilter should be compared with WFP too because the main job of WinDivert exists in WFP while WinPKFilter do all stuff perhaps with NDIS driver mode.

## Filter Driver Compatibility Table
|| || Windows XP || Server 2003 || Windows Vista, 7, 8 || Server 2008 || 2008 R2, 2012  || XEN Virtualaiztion || HyperV Virtualization || VMware Virtualization || Parrarel Virtuozo Virtualization
| WinDivert   | No | No | Yes | Yes (1) | Yes | Yes | Yes | Yes | No
| WinpkFitler | Yes | Yes | Yes | Yes | Yes | No (2) | Yes | Yes | No
1. It requires to install update. [More information](https://barbatunnel.codeplex.com/wikipage?title=Error%3a%20Failed%20to%20open%20Divert%20device%20%28110%29%20In%20Windows%20Server%202008%20%28non%20R2%29).
2. It is not officially documented, just I couldn't install it.

## Can we use WinpkFilter on one side and WinDivert on the other side?
Maximum packet size of Network Transport Layer is 64 kilobyte while maximum packet size is usually 1450 bytes, so in theory it is impossible but practically if you re-tunnel **VPN** packets it should be possible, because VPN servers and clients already optimize packets in Network Transport Layer and make them less than 1450. So it should be possible to have one side WinDivert and other side WinpkFilter specially in TCP Tunnel or HTTP Tunnel. In the UDP tunnel it should be working too but it may have risk due MTU size on each side. 

_**Note**: Don't forget other software and proxies usually work over Transport Layer and may not optimize packet sizes to be less than 1450 bytes. _

## External Sites
For more information check their commercial sites.
* [WinDivert](http://reqrypt.org/divert.html), WinDivert is a free open source (LPGL) software package that provides user-mode packet capture, redirection, modification, and re-injection for Windows Vista/Windows 7/Windows 8/2008/2008R2. (LPGL License).
* [WinpkFilter](http://www.ntkernel.com/?Products:Development_Toolkits:Windows_Packet_Filter_Kit), high performance packet filtering framework for Windows 9x/ME/NT/2000/XP/2003/Vista/2008/Windows 7/Windows 8/2008R2/2012 that allows developers to transparently filter (view and modify) raw network packets with minimal impact on network activity without having to write low level TDI or NDIS driver code. (Commercial License).