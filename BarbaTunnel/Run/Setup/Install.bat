@echo *** It should run as administrator ***

@REM *** Initialize path
@CALL "%~dp0vars.bat"

@REM *** Uninstall old serives
CALL "%curdir%\uninstall.bat"

@echo.  
@echo ----------------------------------
@echo *** Install BarbaService.
"%installutil%" "%barbaservice%"
sc start "BarbaTunnel"

