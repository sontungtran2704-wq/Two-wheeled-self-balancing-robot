param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root '.build\standalone-host'
New-Item -ItemType Directory -Force $build | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'zig-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local'
$source=Join-Path $root '03_Firmware_ESP32S3\balancebot'
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$source" (Join-Path $source 'ControlCore.cpp') (Join-Path $root 'tests\cpp\standalone_tests.cpp') -o (Join-Path $build 'standalone_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Standalone compile failed'}
& (Join-Path $build 'standalone_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Standalone tests failed'}
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$source" (Join-Path $root 'tests\cpp\imu_calibration_tests.cpp') -o (Join-Path $build 'imu_calibration_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Calibration window compile failed'}
& (Join-Path $build 'imu_calibration_tests.exe')
if($LASTEXITCODE -ne 0){throw 'Calibration window tests failed'}
Write-Output 'PASS: existing IMU calibration acceptance tests'
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$(Join-Path $root 'tests\mocks')" "-I$source" (Join-Path $source 'ControlCore.cpp') (Join-Path $root 'tests\cpp\imu_storage_tests.cpp') -o (Join-Path $build 'imu_storage_tests.exe')
if($LASTEXITCODE -ne 0){throw 'IMU storage mock compile failed'}
& (Join-Path $build 'imu_storage_tests.exe')
if($LASTEXITCODE -ne 0){throw 'IMU storage mock tests failed'}
