param([string]$ArduinoCli = 'C:\Users\USER\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root '.build\esp32s3-n16r8'
New-Item -ItemType Directory -Force $build | Out-Null
# ESP32-S3-N16R8: 16 MB flash, 8 MB octal PSRAM.
& $ArduinoCli compile --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi' --build-path $build --warnings all (Join-Path $root '03_Firmware_ESP32S3\balancebot') 2>&1 | Tee-Object (Join-Path $root '.build\esp32s3-n16r8.log')
if($LASTEXITCODE -ne 0){throw 'ESP32-S3 reference compile failed'}
Get-FileHash (Join-Path $build 'balancebot.ino.bin') -Algorithm SHA256 | Format-List

