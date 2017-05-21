# Creating Self Signed Certificate
There is many ways to create your own Self-Signed certificate. Here I show you two methods:

## Method1: Using IIS
See following document to Create a Self-Signed Server Certificate in IIS 7 using IIS:
[Create a Self-Signed Server Certificate in IIS 7](http://technet.microsoft.com/en-us/library/cc753127.aspx)

## Method2: Using command-line 
There is a batch file that uses Microsoft "makecert.exe" and "pvk2pfx.exe" Tools​.

# ​Download "[CreateRootCertificate.zip](How to create self-singed certificate_CreateRootCertificate.zip)" file and extract it.
# Run "_CreateRoot.bat".
# Select "None" when it prompts Create Private Key Password.
# Note: You can edit "_CreateRoot.bat" and change "CerFileName" and "CN" parameters to set your own name for the file and certificate subject.

**Note:** CN should contain your domain address such as machine.yourdomain.com

# Installing Self Signed Certificate 
The major difference between Self Signed Certificate and valid certificate is that Self Signed Certificate will not be accepted by client machine unless you install it manually in "Trusted Root Certification Authroties" of users' local machine. Here a batch file to do this:
# Download [InstallRootCertifcate.zip](How to create self-singed certificate_InstallRootCertifcate.zip).
# Extract it and put your certificate in extracted folder.
# Run install.vbs on client machine.

**Note**: You can install it manually from your system Certificate Manager.



