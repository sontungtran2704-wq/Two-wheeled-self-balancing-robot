@echo off
cd /d "%~dp0"
if not exist "node_modules\express\package.json" (
  call npm ci
  if errorlevel 1 exit /b 1
)
call npm start
