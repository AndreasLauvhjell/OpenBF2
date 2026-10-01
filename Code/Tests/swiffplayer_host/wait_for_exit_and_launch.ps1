# SCAFFOLDING (Code/Tests) - waits for a running client (by pid) to exit, lets its driver collect the
# logs, then launches the next interactive drive on whatever is in bin_swiffplayer. Lets a rebuilt DLL
# reach the user without a hand-over in the middle of a drive.
param(
  [int]$WaitPid,
  [string]$Tag = 'drive12',
  [int]$SettleSeconds = 25,
  [int]$SwiffLog = 0,       # the renderer's per-display lines are 100 MB per half hour; off by default here
  [int]$StreamLog = 1
)
$ErrorActionPreference = 'Continue'
$t0 = Get-Date
while (Get-Process -Id $WaitPid -ErrorAction SilentlyContinue) { Start-Sleep -Seconds 5 }
Write-Host "[$Tag] pid $WaitPid gone after $([int]((Get-Date) - $t0).TotalSeconds)s - settling $SettleSeconds s for the log copy, then launching"
Start-Sleep -Seconds $SettleSeconds
& powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\run_retail.ps1" -Player rebuilt -Rend rebuilt -Tag $Tag -Seconds 0 -AsLog 1 -SwiffLog $SwiffLog -StreamLog $StreamLog -CrashLog 1
