@REM ---- Initialize path ----
@SET installutil=%windir%\Microsoft.NET\Framework\v4.0.30319\InstallUtil.exe
@SET curdir=%~dp0
@SET bindir=%curdir%..\bin
@IF NOT EXIST "%bindir%\BarbaService.exe" @SET bindir=%curdir%..\bin\debug
@SET barbaservice=%bindir%\BarbaService.exe
@SET barbamonitor=%bindir%\BarbaMonitor.exe