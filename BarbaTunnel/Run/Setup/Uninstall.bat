@echo *** It should run as administrator ***
@echo OFF

REM ---- Initialize path ----
@CALL "%~dp0vars.bat"

@echo.  
@echo ----------------------------------
@echo *** Uninstall BarbaService.
sc stop "BarbaTunnel"
"%installutil%"  "%barbaservice%" /u

@echo.  
@echo ----------------------------------
@echo *** uninstall divert service
sc delete divert
