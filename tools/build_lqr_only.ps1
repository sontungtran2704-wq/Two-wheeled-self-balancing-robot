param([string]$ArduinoCli = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildPath = Join-Path $projectRoot '.build\lqrbench'
$sketchPath = Join-Path $projectRoot '03_Firmware_ESP32S3\balancebot_lqr_only'
New-Item -ItemType Directory -Force $buildPath | Out-Null
& $ArduinoCli compile --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi' --build-path $buildPath --jobs 1 --warnings all $sketchPath 2>&1 | Tee-Object (Join-Path $projectRoot '.build\lqrbench-build.log')
if ($LASTEXITCODE -ne 0) { throw 'LQR-only build failed; see .build/lqrbench-build.log' }
Get-FileHash (Join-Path $buildPath 'balancebot_lqr_only.ino.bin') -Algorithm SHA256
