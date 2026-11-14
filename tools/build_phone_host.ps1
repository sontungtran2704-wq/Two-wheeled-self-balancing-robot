param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root '.build\phone-host'
New-Item -ItemType Directory -Force $build | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'zig-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local'
$source=Join-Path $root '03_Firmware_ESP32_S3\balancebot'
foreach($name in @('phone_auth_tests','phone_web_tests','phone_stop_tests','phone_lqr_tests','standalone_tests','imu_calibration_tests','imu_storage_tests')) {
    $argsList=@('c++','-std=c++17','-O2','-Wall','-Wextra','-Werror',"-I$source","-I$(Join-Path $root 'tests\mocks')")
    if($name -in @('phone_stop_tests','phone_lqr_tests','standalone_tests','imu_storage_tests')){$argsList+=Join-Path $source 'ControlCore.cpp'}
    $argsList+=@((Join-Path $root "tests\cpp\$name.cpp"),'-o',(Join-Path $build "$name.exe"))
    if($name -in @('phone_auth_tests','phone_web_tests')){$argsList+='-lbcrypt'}
    & $Compiler @argsList
    if($LASTEXITCODE -ne 0){throw "$name compile failed"}
    & (Join-Path $build "$name.exe")
    if($LASTEXITCODE -ne 0){throw "$name failed"}
    Write-Output "PASS: $name"
}
