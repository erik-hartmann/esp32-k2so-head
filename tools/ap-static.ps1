# Bypasses DHCP by assigning a static address on the head's subnet, tests
# whether the ESP32 is reachable at all, then puts the adapter back on DHCP.
#
# MUST be run from an elevated (Administrator) PowerShell - assigning a static
# IP is a privileged operation. The script checks and refuses rather than
# failing halfway.
#
# Run WHILE joined to the head's AP:
#   powershell -ExecutionPolicy Bypass -File C:\Users\ehart\esp32-k2so-head\tools\ap-static.ps1
#
# The revert runs in a finally block, so the adapter goes back to DHCP even if
# something in the middle throws. ASCII only - PowerShell 5.1 reads .ps1 as
# ANSI without a BOM.

$alias = "Wi-Fi"
$out = Join-Path $env:TEMP "k2so-ap-static.txt"
$stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# Changing IP configuration needs elevation. Check up front and say so
# plainly: without this the script runs, fails every step with a bare
# "Access is denied", and still reports that it reverted to DHCP - which
# looks like a diagnosis rather than a script that never got started.
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host ""
    Write-Host "This script must run as Administrator." -ForegroundColor Yellow
    Write-Host "Assigning a static IP is a privileged operation; without elevation"
    Write-Host "every step fails with 'Access is denied' and nothing is tested."
    Write-Host ""
    Write-Host "Open PowerShell with 'Run as administrator', then:"
    Write-Host "  powershell -ExecutionPolicy Bypass -File $PSCommandPath"
    Write-Host ""
    "K-2SO static IP test - $stamp" | Out-File $out
    "ABORTED: not running as Administrator - nothing was tested." | Out-File $out -Append
    exit 1
}

"K-2SO static IP test - $stamp" | Out-File $out

try {
    "" | Out-File $out -Append
    "=== assigning 192.168.4.2/24 to $alias ===" | Out-File $out -Append

    Get-NetIPAddress -InterfaceAlias $alias -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Remove-NetIPAddress -Confirm:$false -ErrorAction SilentlyContinue
    Set-NetIPInterface -InterfaceAlias $alias -Dhcp Disabled -ErrorAction SilentlyContinue
    New-NetIPAddress -InterfaceAlias $alias -IPAddress 192.168.4.2 -PrefixLength 24 -ErrorAction Stop |
        Out-Null
    Start-Sleep -Seconds 3

    Get-NetIPAddress -InterfaceAlias $alias -AddressFamily IPv4 |
        Select-Object InterfaceAlias, IPAddress, PrefixLength |
        Format-Table -AutoSize | Out-String | Out-File $out -Append

    "" | Out-File $out -Append
    "=== ping 192.168.4.1 ===" | Out-File $out -Append
    ping -n 4 192.168.4.1 | Out-File $out -Append

    "" | Out-File $out -Append
    "=== ARP (a MAC here means layer 2 works) ===" | Out-File $out -Append
    arp -a 192.168.4.1 | Out-File $out -Append

    "" | Out-File $out -Append
    "=== HTTP GET http://192.168.4.1/ ===" | Out-File $out -Append
    try {
        $r = Invoke-WebRequest -Uri "http://192.168.4.1/" -TimeoutSec 10 -UseBasicParsing -ErrorAction Stop
        "OK - status $($r.StatusCode), $($r.RawContentLength) bytes" | Out-File $out -Append
    } catch {
        "FAILED: $($_.Exception.Message)" | Out-File $out -Append
    }
}
catch {
    "SETUP FAILED: $($_.Exception.Message)" | Out-File $out -Append
}
finally {
    "" | Out-File $out -Append
    "=== reverting to DHCP ===" | Out-File $out -Append
    Get-NetIPAddress -InterfaceAlias $alias -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Remove-NetIPAddress -Confirm:$false -ErrorAction SilentlyContinue
    Set-NetIPInterface -InterfaceAlias $alias -Dhcp Enabled -ErrorAction SilentlyContinue
    "adapter returned to DHCP" | Out-File $out -Append
}

Write-Host ""
Write-Host "Saved to $out"
Write-Host "Adapter is back on DHCP. Rejoin your normal WiFi and tell Claude."
