# How to disable multi-login of a same account on Windows Server 2008?

Windows server accept any user with same username and password connect from several places. There are many third party application that help you to prevent and manage VPN accounts, but if you don't want to install any third party application you can use this trick that help you to disable multi-login of same user on windows server.

## Requirements
* "Windows Server 2008 Standard" or "Enterprise" or "Data Center edition". Not "Web edition"!
* This document wrote on Windows Server 2008 R2 but it almost same for Windows Server 2008.

## Instructions
# From start menu find and open "Routing and Remote Access". It is usually under "Administrative Tools" menu.
# Select your Server Name and open its context menu by pressing right click on mouse then select "Properties".
	* ![Launching Routing and Remote Access Properties of your server](How to disable multi-login of a same account on Windows Server?_RAS-Properties.png)
# In properties page go to "IPv4" page and in "IPv4 address assignment section" select "Static address pool" then press "Add" button.
	* ![RAS Properties - Add IPv4 Range](How to disable multi-login of a same account on Windows Server?_RAS-AddRangeIP.png)
# In "New IPv4 Address Range" window assign 192.168.100.1 and 192.168.100.2. It can be any other range but it should be just 2 number of address. Press "OK".
	* ![New IPv4 Address Range](How to disable multi-login of a same account on Windows Server?_IPv4AddressRange.png)
# Make sure item added to "IPv4 address assignment section" and press "OK" to close.
	* ![IPv4 address assignment section](How to disable multi-login of a same account on Windows Server?_RAS-AddRangeIP2.png)
# From start menu find and open "Server Manager", go to users node, select a user and open its "Properties" from its context menu.
	* ![Lanuching Windows User's Properties](How to disable multi-login of a same account on Windows Server?_LaunchUsersProperties.png)
# In "User Properties" window go to "Dial-In" page then select "Assign Static IP addresses" section.
	* ![User Dial-in Properties](How to disable multi-login of a same account on Windows Server?_UserPropertiesDialin.png)
# Assing an IP more than what you assign in RAS Properties. In our case it should be 192.168.100.3 to be 192.168.100.255. Ensure that each user have unique IP. Press "OK" and close user's properties page.
	* ![User Static IP Address](How to disable multi-login of a same account on Windows Server?_UserStaticIPAddress.png)

Now users will got error 720 if they already connected from another place.

**Exception:** Just one user at time can sign-in from two places not even three places, because IP 192.168.100.2 is still free and RAS does not accept 1 number IP range. All other attempt will throw error 720 on user's client.