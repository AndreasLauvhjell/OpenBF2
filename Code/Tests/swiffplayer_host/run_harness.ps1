# SCAFFOLDING - drives the swiffplayer_host harness on mainMenu.swf with the stream tracer, twice
# (3 frames, then N frames), and copies the DLL's log out under distinct names.
param(
  [string]$Tag = 'harness',
  [int]$LongFrames = 300,
  [int]$StreamLog = 3,
  [int]$AsLog = 1,
  [string]$FrameLabel = '',    # SCAFFOLDING: e.g. 'newbie' - what the retail host answers, so the harness navigates too
  [string]$Movie = 'mainMenu.swf'   # SCAFFOLDING: or menu.swf
)
$ErrorActionPreference = 'Continue'
$scratch = (Join-Path $PSScriptRoot '..\..\Build\bin_swiffplayer\runs')   # SCAFFOLDING: logs land here
$bin = 'D:\LC\BF2Rebuilt\Code\Build\bin_swiffplayer'
$log = Join-Path $bin 'SwiffPlayerRebuilt.log'
New-Item -ItemType Directory -Force $scratch | Out-Null
Set-Location 'D:\LC\BF2Rebuilt'

$env:BF2_SWIFFPLAYER_MOVIE = $Movie
$env:BF2_SWIFFPLAYER_STREAMLOG = "$StreamLog"
if ($AsLog) { $env:BF2_SWIFFPLAYER_ASLOG = "$AsLog" } else { $env:BF2_SWIFFPLAYER_ASLOG = $null }   # 2 = also gameswf's informational log_msg lines
if ($FrameLabel) { $env:BF2_SWIFFPLAYER_FRAMELABEL = $FrameLabel } else { $env:BF2_SWIFFPLAYER_FRAMELABEL = $null }

$env:BF2_SWIFFPLAYER_FRAMES = '3'
if (Test-Path $log) { Remove-Item -LiteralPath $log -Force }
cmd /c Code\Tests\swiffplayer_host\build.cmd > "$scratch\$Tag`3.console.txt"
Write-Host "[$Tag] 3-frame run rc=$LASTEXITCODE"
if (Test-Path $log) { Copy-Item -LiteralPath $log "$scratch\$Tag`3.swiffplayer.log" -Force }

$env:BF2_SWIFFPLAYER_FRAMES = "$LongFrames"
if (Test-Path $log) { Remove-Item -LiteralPath $log -Force }
cmd /c Code\Tests\swiffplayer_host\build.cmd nobuild > "$scratch\$Tag$LongFrames.console.txt"
Write-Host "[$Tag] $LongFrames-frame run rc=$LASTEXITCODE"
if (Test-Path $log) { Copy-Item -LiteralPath $log "$scratch\$Tag$LongFrames.swiffplayer.log" -Force }

Select-String -Path "$scratch\$Tag$LongFrames.console.txt" -Pattern 'PASSED|FAILED|CRASH' | ForEach-Object { $_.Line }
Get-ChildItem $scratch | Where-Object { $_.Name -like "$Tag*" } | Format-Table Name, Length -AutoSize
