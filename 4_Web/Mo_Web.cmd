@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Mo_Web.ps1"
if errorlevel 1 pause
