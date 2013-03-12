Set oShell = CreateObject("WScript.Shell")
Set oFso = CreateObject("Scripting.FileSystemObject")
Set oShellApp =  CreateObject("Shell.Application")
curDir = oFso.GetParentFolderName(Wscript.ScriptFullName)

' *** check is xp and 64-bit
DIM isxp
DIM is64
isxp = False
is64 = False
Set SystemSet = GetObject("winmgmts:").InstancesOf ("Win32_OperatingSystem") 
for each System in SystemSet 
	isxp = InStr(system.Version, "5.")=1
	if not(isxp) then is64 = LCase(system.OSArchitecture)="64-bit"
Next

' *** install
file = chr(34) & curDir & "\install.bat" & chr(34)
oShell.Run file, 1, 1

' *** enable test signing on windows 64
IF (is64 AND NOT isxp) THEN
	regValue = oShell.RegRead("HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SystemStartOptions")
	restartNeeded = InStr(UCase(regValue), "TESTSIGNING")=0

	file = chr(34) & curDir & "\TestSigning_ON.bat" & chr(34)
	oShell.Run file, 1, 1

	' *** prompt to restart for unsigned driver
	IF ( restartNeeded ) THEN
		msg = "BarbaTunnel use WinDivert driver that not signed by any company yet. Setup set TESTSINGING on and you need to restart your windows to take effect."
		msg = msg & Chr(13) & Chr(13) & "Do you want to restart your windows now?"
		if Msgbox(msg, vbYesNo or vbQuestion, "Reboot Machine") = vbYes then oShell.run "shutdown.exe /r /t 0", 0, 0
		Wscript.Quit
	End IF
End If


' *** run
file = chr(34) & curDir & "\..\run.vbs" & chr(34)
oShell.Run file, 1, 0