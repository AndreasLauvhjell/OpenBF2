# SCAFFOLDING (Code/Tests) - on-demand screenshot of the primary screen while an interactive retail run is
# up (the driver only screenshots timed runs). Saves runs/<Tag>.png.
param([string]$Tag = 'shot')
$ErrorActionPreference = 'Stop'
$out = (Join-Path $PSScriptRoot '..\..\Build\bin_swiffplayer\runs')
New-Item -ItemType Directory -Force $out | Out-Null
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
$path = Join-Path $out "$Tag.png"
$bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "[$Tag] screenshot: $path"
