param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root '.build\wifi-host'
New-Item -ItemType Directory -Force $build | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $root '.build\standalone-host\zig-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local'
$source=Join-Path $root '03_Firmware_ESP32S3\balancebot'
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror -DCONFIG_IDF_TARGET_ESP32S3=1 "-I$(Join-Path $root 'tests\mocks\runtime')" "-I$(Join-Path $root 'tests\mocks')" "-I$source" (Join-Path $source 'ControlCore.cpp') (Join-Path $root 'tests\cpp\wifi_runtime_tests.cpp') -o (Join-Path $build 'wifi_runtime_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Wi-Fi Runtime mock compile failed'}
& (Join-Path $build 'wifi_runtime_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Wi-Fi Runtime mock tests failed'}
