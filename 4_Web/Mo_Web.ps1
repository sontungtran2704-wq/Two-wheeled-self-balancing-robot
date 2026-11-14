$ErrorActionPreference = 'Stop'
$webUrl = 'http://127.0.0.1:3000'
# Allow phones on the same private network; account login remains required.
$env:ROBOT_LAN = '1'

function Test-RobotWeb {
    try {
        $status = Invoke-RestMethod "$webUrl/api/health" -TimeoutSec 1
        return ($status.service -eq 'balancebot-web' -and $status.lan -eq $true)
    } catch { return $false }
}

try {
    if (-not (Test-RobotWeb)) {
        $nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
        $nodePath = if ($nodeCommand) { $nodeCommand.Source } else {
            Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe'
        }
        if (-not (Test-Path -LiteralPath $nodePath)) {
            throw 'Khong tim thay Node.js. Can cai Node.js hoac khoi phuc runtime.'
        }
        $serverPath = Join-Path $PSScriptRoot 'web_server.js'
        $serverProcess = Start-Process -FilePath $nodePath -ArgumentList ('"' + $serverPath + '"') -WorkingDirectory $PSScriptRoot -WindowStyle Hidden -PassThru
        for ($attempt = 0; $attempt -lt 20; $attempt++) {
            if (Test-RobotWeb) { break }
            if ($serverProcess.HasExited) { break }
            Start-Sleep -Milliseconds 250
        }
        if (-not (Test-RobotWeb)) {
            throw 'Khong mo duoc server web. Cong 3000 co the dang bi ung dung khac su dung. Khong tu dong dung tien trinh nao.'
        }
    }
    $webInfo = Invoke-RestMethod "$webUrl/api/health" -TimeoutSec 2
    foreach ($phoneUrl in $webInfo.urls) { Write-Host "Dien thoai cung mang: $phoneUrl" }
    Start-Process $webUrl
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
