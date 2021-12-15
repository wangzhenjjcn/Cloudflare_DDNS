@echo off
@echo ServiceName:%1
@echo AppName:%2
set curExe=%~dp0%2
set regpath=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CloudflareDDNS\Parameters\
set sourcePath=%~dp0WinServiceTool/srvany.exe
cd /d "%~dp0"
instsrv %1  "%sourcePath%"
@echo ServiceAddSuccess
reg add %regpath% /v AppDirectory /t REG_SZ /d "%~dp0\" /f
reg add %regpath% /v Application /t REG_SZ /d "%curExe%" /f 
reg add %regpath% /v AppParameters /t REG_SZ /f
@echo regAddFinished