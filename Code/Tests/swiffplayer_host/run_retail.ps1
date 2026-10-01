# SCAFFOLDING (Code/Tests) - retail smoke driver for the SwiffPlayer stream diff. Writes ONLY into the Dev-Swiff
# test install (never the reference dir), launches BF2 windowed, screenshots, kills, collects logs.
param(
  [ValidateSet('original','rebuilt')] [string]$Player = 'original',
  [string]$Tag = 'run',
  [int]$Seconds = 25,           # 0 = interactive: never kill, wait for the user to quit the client, then collect
  [int]$SwiffLog = 3,
  [int]$StreamLog = 3,
  [int]$AsLog = 0,
  [ValidateSet('original','rebuilt')] [string]$Rend = 'rebuilt',
  [int]$CrashLog = 1               # SCAFFOLDING: BF2_SWIFFPLAYER_CRASHLOG - the DLL logs its own first fault's call chain
)
$ErrorActionPreference = 'Stop'
$dev  = 'D:\B2BF Launcher\Battlefield2 - Dev - Swiff'
$ref  = 'D:\B2BF Launcher\Battlefield2'
$binR = 'D:\LC\BF2Rebuilt\Code\Build\bin_renddx9'
$binS = 'D:\LC\BF2Rebuilt\Code\Build\bin_swiffplayer'
$out  = (Join-Path $PSScriptRoot '..\..\Build\bin_swiffplayer\runs')   # SCAFFOLDING: logs + screenshots land here
New-Item -ItemType Directory -Force $out | Out-Null

# Back up the shipped RendDX9 in the dev dir once (it is md5-identical to the reference copy).
if (-not (Test-Path "$dev\RendDX9.original.dll")) { Copy-Item "$dev\RendDX9.dll" "$dev\RendDX9.original.dll" }
if ($Rend -eq 'rebuilt') {
  Copy-Item "$binR\RendDX9.dll" "$dev\RendDX9.dll" -Force
  Copy-Item "$binR\RendDX9.pdb" "$dev\RendDX9.pdb" -Force
} else {
  Copy-Item "$dev\RendDX9.original.dll" "$dev\RendDX9.dll" -Force
}
if ($Player -eq 'original') {
  Copy-Item "$dev\SwiffPlayer.original.dll" "$dev\SwiffPlayer.dll" -Force
} else {
  Copy-Item "$binS\SwiffPlayer.dll" "$dev\SwiffPlayer.dll" -Force
  Copy-Item "$binS\SwiffPlayer.pdb" "$dev\SwiffPlayer.pdb" -Force
}
Remove-Item "$dev\RendDX9Rebuilt.log" -ErrorAction SilentlyContinue
Remove-Item "$dev\SwiffPlayerRebuilt.log" -ErrorAction SilentlyContinue

$env:BF2_RENDDX9_SWIFFLOG = "$SwiffLog"
$env:BF2_SWIFFPLAYER_STREAMLOG = "$StreamLog"
if ($AsLog) { $env:BF2_SWIFFPLAYER_ASLOG = '1' } else { Remove-Item Env:BF2_SWIFFPLAYER_ASLOG -ErrorAction SilentlyContinue }
if ($CrashLog) { $env:BF2_SWIFFPLAYER_CRASHLOG = '1' } else { Remove-Item Env:BF2_SWIFFPLAYER_CRASHLOG -ErrorAction SilentlyContinue }

$h1 = (Get-FileHash "$dev\RendDX9.dll" -Algorithm MD5).Hash
$h2 = (Get-FileHash "$dev\SwiffPlayer.dll" -Algorithm MD5).Hash
Write-Host "[$Tag] RendDX9=$Rend ($h1)  SwiffPlayer=$Player ($h2)  SWIFFLOG=$SwiffLog STREAMLOG=$StreamLog ASLOG=$AsLog"

$p = Start-Process -FilePath "$dev\BF2.exe" -ArgumentList '+fullscreen 0 +szx 800 +szy 600 +menu 1 +multi 1' -WorkingDirectory $dev -PassThru
$t0 = Get-Date
if ($Seconds -le 0) {
  Write-Host "[$Tag] INTERACTIVE: BF2 pid $($p.Id) is yours to drive. Live logs: $dev\SwiffPlayerRebuilt.log and $dev\RendDX9Rebuilt.log. Quit the client to collect."
  while (-not $p.HasExited) { Start-Sleep -Milliseconds 500 }
} else {
  while (-not $p.HasExited -and ((Get-Date) - $t0).TotalSeconds -lt $Seconds) { Start-Sleep -Milliseconds 500 }
}

# Screenshot of the primary screen (the windowed 800x600 client is on it). Skipped in interactive mode (the client is gone).
if ($Seconds -gt 0) { try {
  Add-Type -AssemblyName System.Windows.Forms
  Add-Type -AssemblyName System.Drawing
  $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
  $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
  $bmp.Save("$out\$Tag.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
} catch { Write-Host "screenshot failed: $_" } }

$exited = $p.HasExited
$code = $null
if ($exited) { $code = $p.ExitCode } else { Stop-Process -Id $p.Id -Force; Start-Sleep -Seconds 2 }
Write-Host "[$Tag] BF2 exited on its own: $exited  exitcode: $code  ran $([int]((Get-Date) - $t0).TotalSeconds)s"

# The logs stay locked for a moment after an external kill: retry the copy a few times.
foreach ($pair in @(@("$dev\RendDX9Rebuilt.log", "$out\$Tag.renddx9.log"), @("$dev\SwiffPlayerRebuilt.log", "$out\$Tag.swiffplayer.log"))) {
  for ($try = 0; $try -lt 10; $try++) {
    try { Copy-Item $pair[0] $pair[1] -ErrorAction Stop; break } catch { Start-Sleep -Seconds 1 }
  }
}
Get-ChildItem $out | Where-Object { $_.Name -like "$Tag*" } | Format-Table Name, Length -AutoSize
