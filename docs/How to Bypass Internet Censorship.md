**Summary**
The importance of Internet Censorship Circumvention described, and common circumvention methods have been compared. Finally, I prove why "Commercial and Private Proxy Servers" are the best solution. 

**Table of Contents**
* How much Internet is being filtered?
	* Firewall Side Effect Disaster
* Internet Censorship Circumvention Methods
	* Anonymous P2P Software such Tor
	* Free Public Proxy Servers such as Freegate and Ultrasurf
	* Commercial and Private Proxy server
		* Why anti-filtering solution should have commercial plans?
		* How much is the profit for a seller?
		* Why the price is not important for users?
# Idea for Common Anti-Filtering Organization


# How much Internet is being filtered?
In many countries with a large population, people struggle to use basic Internet functionality. Read the following article to see how much Internet censorship spread across countries and companies.
* [Internet censorship](http://en.wikipedia.org/wiki/Internet_censorship)
* [Internet censorship by country](http://en.wikipedia.org/wiki/Internet_censorship_by_country)
Additionally other censorship and restriction applied to people in some countries via US and European policy such as:
* Commercial Sites
* Some Web Hosting providers, for example, godady.com blocks some countries IPs reach their network so millions of sites that exist in such hosting are being blocked even without knowing it. 
There is still much more firewall problems that I called "Firewall Side Effect Disaster". There is much Internet censorship circumvention software that helps people to bypass Internet censorship by a variety of methods. It leads government to create stricter and stronger Firewalls and there is a battle between Firewall and anti-censorship software,  but the real problem has been raised now. Regardless of who success there are many serious services, protocols and application sacrificed in this battle. It causes common people who even don't want and plan to bypass the censorship has been sacrificed and going to face trouble with the base Internet usage. 

## Firewall Side Effect Disaster
Usually people who run the Firewall don't want to disable or restrict or block all services, but many services became useless because of the nature of Firewall and implementation. To prevent tunneling application work, Firewalls need to block many critical technologies otherwise it is impossible to block many tunneling applications. 
* Limit or reduce speed HTTPS, SSH, VPN and many security protocols!
* Limit or reduce the speed of TCP (non HTTP) connections!
* Limit or reduce the speed of UDP connections!
* Limit or reduce the speed of VPN connections!
In theory, Firewall couldn't win against tunneling software without these restrictions but majority the Internet based on these protocols, just imagine how an application can work without these protocols! Thousands of applications encounter a serious problem and even miss-functionality such as Map just because firewall wants to block some rare tunneling application! 
It leads and forces people to find a censorship circumvention but not to reach government abandoned site, they use them in order to use just the Internet and regular service such as email. It is going black and white: 
# Black: They can't bypass censorship so they have not internet.
# White: They bypass censorship so they have internet.
The winning of Firewall in these battle lead to total disconnection of the Internet and it changes the duty of tunneling software from **Internet censorship circumvention** to **Internet Blocking circumvention**.
​
# Internet Censorship Circumvention Methods
There are a variety of methods for Internet censorship circumvention that you can find in following Wikipedia Article.
[Internet censorship circumvention](http://en.wikipedia.org/wiki/Internet_censorship_circumvention)

Each method has many pros and cons, but I look for a reliable and total solution. In this article, I don't plan to compare them with technical implementation such as TCP or UDP and other trick, instead, I am going to compare each method with its idea, philosophy and business model behind them.

## Public Anti-Filtering Organization such as Ultrasurf, Psiphon and Freegate
Let see how Ultrasurf work:
_ANSWER: When a user launches Ultrasurf, the tool automatically discovers the best available proxy servers from our global server pool* and connects the user to it via an encrypted tunnel... See [here](http://ultrasurf.us/faq.html)_

First weak point is starting point. Where is global server pool address? In theory you can simply extract it from the client software or simply sniff the network packet of software and just block it. There is why in most cases the software could not find any server to connect.
Second week point is that a robot can be created and start using such products themselves and when the software is connected, then the robot adds founded server IP to its block list and try again till all most common server IPs are blocked. In this case even if some server remains open, many people could not use it or at least too much load on limited servers. 

### Pros
# Free to public
# Uses can trust for non secure connection.
### Cons
# Too much expensive solution and need a big sponsor to support it.
# Major Server Pool Address that contains server proxy address can be found and blocked.
# Major server proxy address can be found by robots and be blocked.
# Performance may be low because there is huge traffic over sponsor servers and millions of people connected to limited server at the same time.
# Not reliable because a huge number of people depend to one company and system.

## Anonymous P2P Software such Tor
Wikipedia Article: [Anonymous P2P](http://en.wikipedia.org/wiki/Anonymous_P2P)
Actually this kind of software is being designed to keep the privacy of the publisher, not to bypass censorship. They are also being used for Anti-Filtering
The weak point is that each peer reveals other peers or networks and it can be simply added to the block list. Also finding the initial list of peers is a big problem, List servers can be blocked. So the network can be revealed easily by Firewall and blocked.

### Pros
# Free to public
# Don't need a big sponsor to support it.
# Don't need much cost for servers and maintenance.
# It keeps user privacy for everyone.
### Cons
# Designed to keep privacy not to bypass censorship.
# Initial peer or list server can be easily blocked.
# Robot can easily detect all peers by looking in list servers or query from one peer like a chain until all major peer revealed.
# Need many volunteers in a country where has freedom.
# Performance is depending to users and volunteers.

## Free Public Proxy Servers
This is one of the best ways to bypass the Internet censorship but the main issue is that how to find one. They are public so the government can easily find them too same as other people do. The government spent millions of dollars to create firewall software, additionally they hire mercenaries and ask them to try bypassing Internet the same way other people do, and when they find a public proxy address or VPN, they simply report it and add the server IP to the block list. 
In most cases it is impossible for regular people to find a working one.

### Pros
# Free for public user.
# There is not need for list servers.
# Servers are not connected so one server does not reveal other server.
# Not expensive for creating.
### Cons
# Very hard for People to find a working one
# Not reliable because good servers sooner or later will blocked.
# Need a sponsor or volunteers to support it.
# Performance may be not good because it's free so many people may connect to servers at the same time.

## Commercial and Private Proxy server
It is same as Free Public Proxy Servers but the advantage is that they should be non free! 
You many wonder at first that's why I say the advantage is that the service should not be free. There is a business model behind it that guarantees it working with high performance and reliability.

### Why anti-filtering solution should have commercial plans?

The key to this method is that it should not be free even if possible. Let me first tell the weakness of free public proxies and VPN services.
# Users will immediately publish and share the same IP address to others so the government will find them easily, and they will be blocked sooner or later.
# Even If a server does not block by the firewall, then the number of its users grew dramatically, until the server resources are not enough to support all of its users, and will be out of service sooner or later.
# Free services do not have any income or much encouragement for volunteers to maintain and support services.
# Most of the people do not have much knowledge and time to dig and search into the Internet to find temporary a public proxy server; most of them are out of service and may already be blocked.

The cost of service is not important, it can be even 0.5$ per month. The advantage is that commercial users will not publish and share Server IPs so firewall and its mercenaries couldn't find them by searching. Selling proxy account has much profit (I will describe later) and does not need much knowledge and investment so many smugglers will be raised. Such business should be illegal governments, it makes governments act as natural anti-trust organization and prevent a big seller appear. But it is good news because it makes thousands small seller appear with thousands isolated and scattered network. Competition in sellers will be raised due to high profit, and it leads to better service. Now sellers are finding people instead people look for them. Common people without computer knowledge should have access to the bypass Internet censorship.

### How much is the profit for a seller?
I found Virtual Windows Server (VPS) for 20$/m with 1GB network speed and 500GB transfer then I configured a VPN service on it. I created 30 user accounts only for my friends and family for 3$/m. I see it is near 350% profit just for 30 users; I thought that it was awesome! Actually, I didn't search for more users didn't create a bigger circle because it was not my business. 
I have estimated that the server can serve maximum 30 users, but when I got the service and monitor the server, I saw in the worst case. There was a maximum of 8 users connected to VPN simultaneously. They also used the bandwidth less than I expected, so I estimate each server can support approximately 80 users, and it will be near 900% profit for whoever sells anti-filter for 80 users without losing performance.

### Why the price is not important for users?
It depends on how much Internet is filtered in that region, If the Internet just filtered at the basic level, then the majority of people accept censorship and only some limited users may use Anti-Filtering services, but in a country such as China where the Internet strongly restricted specially when Firewall Side Effect raised, then it is mandatory for all users to look for a Firewall Circumvention. In this case, major public anti-filtering services aren't working or so slow too; it is nearly impossible for normal users to find a method. In this situation Internet usually not meaning without Firewall Circumvention because of Firewall Side Effect, so people who pay for carrier or ISP surely will pay for commercial proxy to get Internet access especially if the price is low enough. 

Pros
# It is impossible for Firewall to find proxy servers with robot and even mercenary. 
# Servers will not overload by huge requests because seller controls it.
# The service quality should be good due to compete.
# It does not require a big investment, so there is much interest from many small sellers to set up more servers.
Cons 
# Not Free and requires money transaction.
# Users should trust to unknown people for non secure connections. 

# Idea for Common Anti-Filtering Organization
Regardless of technology and implementation for connection, I think Common Anti-Filtering Organization such as Ultrasurf, Psiphon and Freegate can be used as a reliable service. The key is that they should offer commercial plans with private servers beside free services. Commercial plans should have been following features:

# IP addresses should be used just for a limited number of users and should not be changed.
# If Firewall finds IP in a region, the IP should not be used in that region again.
# If Firewall finds IP in a region, user should not get a new IP without any fee.
# Use variety of connection treat and technology such as what BarbaTunnel HTTP and UDP tunnel protocols. 

Lets people pay for their own freedom.


