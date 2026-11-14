$ErrorActionPreference = 'Stop'
$webUrl = 'http://127.0.0.1:3000'
$env:ROBOT_LAN = '1'

function Test-RobotWeb {
    try {
        $status = Invoke-RestMethod "$webUrl/api/health" -TimeoutSec 1
        return ($status.service -eq 'balancebot-web' -and $status.lan -eq $true)
    } catch { return $false }
}

try {
    $nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
    $nodePath = if ($nodeCommand) { $nodeCommand.Source } else {
        Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe'
    }
    if (-not (Test-Path -LiteralPath $nodePath)) {
        throw 'Khong tim thay Node.js. Can cai Node.js hoac khoi phuc runtime.'
    }

    if (-not (Test-RobotWeb)) {
        $serverPath = Join-Path $PSScriptRoot 'web_server.js'
        $serverProcess = Start-Process -FilePath $nodePath -ArgumentList ('"' + $serverPath + '"') -WorkingDirectory $PSScriptRoot -WindowStyle Hidden -PassThru
        for ($attempt = 0; $attempt -lt 20; $attempt++) {
            if (Test-RobotWeb) { break }
            if ($serverProcess.HasExited) { break }
            Start-Sleep -Milliseconds 250
        }
        if (-not (Test-RobotWeb)) {
            throw 'Khong mo duoc server web. Cong 3000 co the dang bi ung dung khac su dung.'
        }
    }

    $webInfo = Invoke-RestMethod "$webUrl/api/health" -TimeoutSec 2
    
    Clear-Host
    Write-Host "=================================================================" -ForegroundColor Cyan
    Write-Host "       XE HAI BANH TU CAN BANG - ESP32-S3 (LQR + PID)            " -ForegroundColor Yellow
    Write-Host "=================================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host " [PC Browser]      : $webUrl" -ForegroundColor Green
    
    $qrScript = Join-Path $PSScriptRoot 'qr_console.js'
    $hasLan = $false
    foreach ($phoneUrl in $webInfo.urls) {
        $hasLan = $true
        Write-Host " [Dien thoai LAN]  : $phoneUrl" -ForegroundColor Magenta
        if (Test-Path -LiteralPath $qrScript) {
            Write-Host ""
            Write-Host "  >> Quet ma QR duoi day bang Camera dien thoai de vao nhanh:" -ForegroundColor Yellow
            & $nodePath $qrScript $phoneUrl
        }
    }
    
    if (-not $hasLan) {
        Write-Host " [Chu y] Chua tim thay dia chi IPv4 LAN. Kiem tra ket noi Wi-Fi." -ForegroundColor DarkYellow
    } else {
        Write-Host "  Luu y: Dien thoai va may tinh phai bat chung mot mang Wi-Fi." -ForegroundColor Gray
    }
    Write-Host "=================================================================" -ForegroundColor Cyan
    Write-Host ""

    # Open PC Browser
    Start-Process $webUrl

    Write-Host "Server dang chay. Ban co the thu nho cua so nay." -ForegroundColor DarkGray
    Write-Host "Nhan Enter de dong cua so console (Server van se tiep tuc chay ngam)..." -ForegroundColor DarkCyan
    [void](Read-Host)
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Nhan Enter de tiep tuc..."
    [void](Read-Host)
    exit 1
}
