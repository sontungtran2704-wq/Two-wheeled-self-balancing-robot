param([string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildPath = Join-Path $projectRoot '.build\lqr60'
$sketchPath = Join-Path $projectRoot '03_Firmware_ESP32S3\balancebot_lqr_60_test'
New-Item -ItemType Directory -Force $buildPath | Out-Null
& $ArduinoCli compile --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi' --build-path $buildPath --jobs 1 --warnings all $sketchPath 2>&1 | Tee-Object (Join-Path $projectRoot '.build\lqr60-build.log')
if ($LASTEXITCODE -ne 0) { throw 'LQR 60 test build failed; see .build/lqr60-build.log' }
Get-FileHash (Join-Path $buildPath 'balancebot_lqr_60_test.ino.bin') -Algorithm SHA256

