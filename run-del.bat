@echo off
@echo ServiceToRemove:%1
cd /d %~dp0
net stop %1
instsrv %1 remove
@echo ServiceRemoved:%1
