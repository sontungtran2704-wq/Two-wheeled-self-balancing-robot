@echo off
title XE CAN BANG - LQR Server
color 0A
echo.
echo  =========================================
echo    XE CAN BANG DONG CO BUOC - ESP32-S3 - LQR
echo  =========================================
echo.
echo  Mo trinh duyet: http://localhost:3000
echo  (dien thoai cung mang WiFi: http://IP-may-tinh:3000)
echo.
echo  Giu cua so nay mo de duy tri server. Ctrl+C de dung.
echo  =========================================
echo.
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
  echo LOI: chua cai Node.js. Tai tai https://nodejs.org roi chay lai.
  pause
  exit /b 1
)
if not exist "node_modules\express\package.json" (
  echo Dang cai thu vien ^(npm install^)...
  if exist node_modules rmdir /s /q node_modules
  call npm install
  if errorlevel 1 (
    echo LOI: npm install that bai. Kiem tra ket noi mang.
    pause
    exit /b 1
  )
)
call npm start
pause
