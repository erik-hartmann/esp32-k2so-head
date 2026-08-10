# Captures everything needed to diagnose a connection to the head's soft AP.
# Run this WHILE joined to the ESP32-LEDs network, then rejoin your normal
# WiFi. Results go to a file rather than the screen, so nothing is lost when
# the network changes.
#
#   powershell -ExecutionPolicy Bypass -File C:\Users\ehart\esp32-k2so-head\tools\ap-diag.ps1
#
# ASCII only, deliberately. Windows PowerShell 5.1 reads .ps1 files as ANSI
# unless they carry a BOM, so any non-ASCII character here becomes mojibake
# and breaks string parsing.

$out = Join-Path $env:TEMP "k2so-ap-diag.txt"
$stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
"K-2SO soft AP diagnostics - $stamp" | Out-File $out

"" | Out-File $out -Append
"=== IPv4 addresses (want 192.168.4.x on Wi-Fi) ===" | Out-File $out -Append
Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike "127.*" } |
    Select-Object InterfaceAlias, IPAddress, PrefixLength, AddressState |
    Format-Table -AutoSize | Out-String | Out-File $out -Append

"" | Out-File $out -Append
"=== Wi-Fi connection ===" | Out-File $out -Append
netsh wlan show interfaces | Out-File $out -Append

"" | Out-File $out -Append
"=== Network profile (Public blocks inbound by default) ===" | Out-File $out -Append
Get-NetConnectionProfile |
    Select-Object Name, InterfaceAlias, NetworkCategory |
    Format-Table -AutoSize | Out-String | Out-File $out -Append

"" | Out-File $out -Append
"=== Which interface would reach 192.168.4.1 ===" | Out-File $out -Append
try {
    Find-NetRoute -RemoteIPAddress 192.168.4.1 -ErrorAction Stop |
        Select-Object InterfaceAlias, IPAddress, NextHop |
        Format-Table -AutoSize | Out-String | Out-File $out -Append
} catch {
    "Find-NetRoute failed: $($_.Exception.Message)" | Out-File $out -Append
}

"" | Out-File $out -Append
"=== ping 192.168.4.1 ===" | Out-File $out -Append
ping -n 4 192.168.4.1 | Out-File $out -Append

"" | Out-File $out -Append
"=== ARP table (did we hear from the ESP at layer 2) ===" | Out-File $out -Append
arp -a | Out-File $out -Append

"" | Out-File $out -Append
"=== HTTP GET http://192.168.4.1/ ===" | Out-File $out -Append
try {
    $r = Invoke-WebRequest -Uri "http://192.168.4.1/" -TimeoutSec 8 -UseBasicParsing -ErrorAction Stop
    "OK - status $($r.StatusCode), $($r.RawContentLength) bytes" | Out-File $out -Append
} catch {
    "FAILED: $($_.Exception.Message)" | Out-File $out -Append
}

Write-Host ""
Write-Host "Saved to $out"
Write-Host "Rejoin your normal WiFi, then tell Claude it is ready."
