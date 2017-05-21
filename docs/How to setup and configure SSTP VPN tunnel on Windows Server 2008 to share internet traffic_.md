# How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?
SSTP VPN is supported in Windows Vista, 7 and Windows server 2008. It uses HTTPS so it does not blocked by many firewalls.

## Requirements
"Windows Server 2008 Standard" or "Enterprise" or "Data Center edition". Not "Web edition"!
Your server should have at-least one valid IP.
You should open TCP port 443 (HTTPS port) on your network firewall to able to connect to your server in the normal way, but if you don't have access to your network firewall you can use BarbaTunnel.

## Installing certificate
SSTP VPN requires valid certificate, but if you don't have valid certificate you can create self-signed certificate, in this case your users should install your self-signed certificate on their machine. To create self-signed certificate sees [How to create self-singed certificate](How-to-create-self-singed-certificate).

# Open start menu then in search box type "mmc.exe" and run it.
# In MMC window go to "File" menu and select "Add/Remove Snap-In" item.
	* ![Microsoft Management Console - Add/Remove Snap-In](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_MMC-AddSnapin.png)
# In "Add or Remove Snap-ins" window select "Certificates" from "Available snap-ins" and press "Add".
	* ![Add or Remove Snap-ins](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_MMC-AddCertificates.png)
# In "Certificates snap-in" page select "Computer account" and click "Next".
	* ![Certificates snap-in](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_CertificatesSnap-in.png)
# In "Select Computer" page select "Local computer" and click "Finish".
	* ![Certificates snap-in - Select Computer](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_SelectComputer.png)
# In "Add or Remove Snap-ins" window make sure that "Certificates (Local Computer)" is added to "selected snap-ins" and press "OK".
	* ![Add or Remove Snap-ins](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_MMC-AddCertificates2.png)
# In MMC window expand "Certificates (Local Computer)" node and select "Personal" node. In "Personal" item context menu select "All Tasks" and choose "Import".
	* ![MMC - Import Certificate for Personal store](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_SelectImportCertificate.png)
# In "Certificate Import Wizard" step to "File to Import" page and select "Browse".
	* ![Certificate Import Wizard - File Import](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_CertImportWizard.png)
# In "Open" file window select "Personal Information Exchange" file type and select your certificate PFX file and press "Open". Make sure you select your PFX file not CER file.
	* ![Certificate Import Wizard - PFX Import](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_SelectPFXFile.png)
# Step to "Certificate Store" page and ensure "Personal Store" is selected then press "Next".
	* ![Certificate Import Wizard - Certificate Store](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_CertificateStore.png)
# Step to the final page and check your settings then press "Finish" button.
	* ![Certificate Import Wizard - Finish](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_CertificateImportFinish.png)

## Configure SSTP VPN for Windows Server 2008 R2
# Do all steps in [How to setup and configure PPTP VPN tunnel on Windows 2008 to share internet traffic?](How-to-setup-and-configure-PPTP-VPN-tunnel-on-Windows-2008-R2-to-share-internet-traffic)
# From start menu find and open "Routing and Remote Access". It is usually under "Administrative Tools" menu.
# Select your Server Name and open its context menu by pressing right click on the mouse then select "Properties".
	* ![Launching Routing and Remote Access Properties of your server](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_RRAS-Properties.png)
# Go to "Security" page and in SSL certificate binding is selected your certificate then press "OK".
	* ![RRAS Properties - Security](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_RRAS-Security.png)
# Restart “Routing as Remote Access services” by going to context menu of your Server Name then open "All Tasks" sub-menu and select "Restart".
	* ![Restarting Routing and Remote Access Properties of your server](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_RRAS-Restart.png)

## Configure SSTP VPN for Windows Server 2008
If your Windows Server 2008 is not R2 version you should run following command to bind your certificate to SSTP VPN Server.

# Do all steps in [How to setup and configure PPTP VPN tunnel on Windows 2008 to share internet traffic?](How-to-setup-and-configure-PPTP-VPN-tunnel-on-Windows-2008-R2-to-share-internet-traffic)
# Run the following commands with administrator privilege in command prompt to delete the old bound certificate.
	* netsh http delete sslcert ipport=0.0.0.0:443
	* netsh http delete sslcert ipport=[::](__):443
	* reg delete HKLM\SYSTEM\CurrentControlSet\Services\SstpSvc\Parameters /v SHA256CertificateHash /f
# Restart “Routing as Remote Access services” by going to context menu of your Server Name then open "All Tasks" sub-menu and select "Restart".
	* ![Restarting Routing and Remote Access Properties of your server](How to setup and configure SSTP VPN tunnel on Windows Server 2008 to share internet traffic?_RRAS-Restart.png)

_More Info: [http://support.microsoft.com/kb/947027](http___support.microsoft.com_kb_947027)_


Your server is now ready to accept VPN-SSTP connection
* Make sure your certificate is valid or installed on your client computer.
* Make sure your user connects to your server via domain name not IP its address.

