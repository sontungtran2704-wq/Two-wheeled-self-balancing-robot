param([string]$Compiler='D:\TLHT\HTDKTD_Xe_Can_Banh\.runtime\ziglang\zig.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root '.build\host'
New-Item -ItemType Directory -Force $build | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'zig-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local'
$source=Join-Path $root '03_Firmware_ESP32S3\balancebot'
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$source" (Join-Path $root 'tests\cpp\imu_calibration_tests.cpp') -o (Join-Path $build 'imu_calibration_tests.exe')
if($LASTEXITCODE -ne 0){throw 'IMU test compile failed'}
& (Join-Path $build 'imu_calibration_tests.exe')
if($LASTEXITCODE -ne 0){throw 'IMU calibration tests failed'}
Write-Output 'PASS: IMU window stability, offset, motion, invalid samples'
& $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$source" (Join-Path $source 'ControlCore.cpp') (Join-Path $root 'tests\cpp\control_tests.cpp') -o (Join-Path $build 'control_tests.exe')
if($LASTEXITCODE -ne 0){throw 'C++ host test build failed'}
& (Join-Path $build 'control_tests.exe')
if($LASTEXITCODE -ne 0){throw 'C++ host tests failed'}
if(Test-Path (Join-Path $root 'tests\cpp\controller_cli.cpp')) {
 & $Compiler c++ -std=c++17 -O2 -Wall -Wextra -Werror "-I$source" (Join-Path $source 'ControlCore.cpp') (Join-Path $root 'tests\cpp\controller_cli.cpp') -o (Join-Path $build 'controller_cli.exe')
 if($LASTEXITCODE -ne 0){throw 'C++ host CLI build failed'}
}
