# SCAFFOLDING (Code/Tests) - waits for the interactive desktop to come back (no lock screen, the console
# session Active), then runs run_retail.ps1 in interactive mode. A D3D9 device cannot be created on a
# locked or disconnected session (RendDX9Init.cpp:517 "Failed to get desktop mode"), which is how
# interactive runs drive7 and drive8 died five seconds into boot with nothing wrong in the DLLs.
param(
  [string]$Tag = 'drive9',
  [int]$SettleSeconds = 20,     # give the desktop a moment after the unlock before creating the device
  [int]$PollSeconds = 10
)
$ErrorActionPreference = 'Continue'
$t0 = Get-Date
while ($true) {
  $locked = [bool](Get-Process LogonUI -ErrorAction SilentlyContinue)
  $active = $false
  try { $active = ((quser 2>$null | Select-String -Pattern '\sActive\s') -ne $null) } catch {}
  if (-not $locked -and $active) { break }
  Start-Sleep -Seconds $PollSeconds
}
Write-Host "[$Tag] desktop back after $([int]((Get-Date) - $t0).TotalSeconds)s - settling $SettleSeconds s, then launching"
Start-Sleep -Seconds $SettleSeconds
& powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\run_retail.ps1" -Player rebuilt -Rend rebuilt -Tag $Tag -Seconds 0 -AsLog 1 -SwiffLog 3 -StreamLog 3 -CrashLog 1
