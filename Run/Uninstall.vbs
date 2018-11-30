Set oShell = CreateObject("WScript.Shell")
Set oFso = CreateObject("Scripting.FileSystemObject")
Set oShellApp =  CreateObject("Shell.Application")
curDir = oFso.GetParentFolderName(Wscript.ScriptFullName)

' *** check is xp
DIM isxp
Set SystemSet = GetObject("winmgmts:").InstancesOf ("Win32_OperatingSystem") 
for each System in SystemSet 
	isxp = InStr(system.Version, "5.")=1
Next


' *** install
action = "runas"
if isxp then action = ""
file = chr(34) & curDir+"\setup\uninstall.bat" & chr(34)
oShellApp.ShellExecute file, "", "", action, 1

