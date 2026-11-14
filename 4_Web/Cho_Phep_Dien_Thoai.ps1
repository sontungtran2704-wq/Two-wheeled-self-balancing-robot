$ErrorActionPreference = 'Stop'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell.exe -Verb RunAs -WindowStyle Hidden -ArgumentList ('-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '"')
    exit
}
$nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
$nodePath = if ($nodeCommand) { $nodeCommand.Source } else {
    Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe'
}
if (-not (Test-Path -LiteralPath $nodePath)) { throw 'Node.js not found.' }
$ruleName = 'BalanceBot-Web-LAN-3000'
$rule = Get-NetFirewallRule -Name $ruleName -ErrorAction SilentlyContinue
if ($rule) {
    $rule | Set-NetFirewallRule -Enabled True -Direction Inbound -Action Allow -Profile Private -Protocol TCP -LocalPort 3000 -RemoteAddress LocalSubnet -Program $nodePath
} else {
    New-NetFirewallRule -Name $ruleName -DisplayName 'BalanceBot Web - Private LAN TCP 3000' -Direction Inbound -Action Allow -Enabled True -Profile Private -Protocol TCP -LocalPort 3000 -RemoteAddress LocalSubnet -Program $nodePath | Out-Null
}
