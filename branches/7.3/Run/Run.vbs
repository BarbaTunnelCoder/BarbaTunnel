Set oShell = CreateObject("WScript.Shell")
Set oFso = CreateObject("Scripting.FileSystemObject")
Set oShellApp =  CreateObject("Shell.Application")
curDir = oFso.GetParentFolderName(Wscript.ScriptFullName)

' *** install
barbaMonitor = curDir & "\bin\BarbaMonitor.exe"
If NOT oFso.FileExists(barbaMonitor) Then barbaMonitor = curDir & "\bin\debug\BarbaMonitor.exe"

file = chr(34) & barbaMonitor & chr(34)
oShellApp.ShellExecute file, "", "", "", 1