# SCAFFOLDING (Code/Tests) - retail step driver: retail
# WARNING (owner, 2026-09-13): this stages ONLY SwiffPlayer.dll. The dev directory keeps its OWN
# RendDX9.dll, which goes stale - a 2026-09-03 copy crashed four launches in a row on the first
# frame inside RendDX9+0x4D6A0. Use drive_native.ps1 (the CLion bin_client tree, kept current by
# CLion, and the NATIVE ABI) whenever the test needs a current engine or exercises a promoted row.
# SwiffPlayer.dll (retail flavour), windowed 800x600, then a SEQUENCE of posted steps separated by
# ';' - "x,y" posts a click at client coordinates, "t:text" posts one WM_CHAR per character,
# "bs" posts WM_CHAR 8 (backspace) - a client-area screenshot after each step, kill, collect + scrub.
param(
  [string]$Tag = 'seq',
  [string]$Steps = '291,414;500,168;302,168',
  [int]$WaitLogin = 25,
  [int]$Between = 4,
  [int]$Burst = 1,                                                                                 # extra screenshots per step (catches the blinking caret)
  [string]$Dll = 'D:\LC\BF2Rebuilt\Code\Build\bin_swiffplayer\SwiffPlayer.dll',               # stage a different DLL for an A/B (e.g. SwiffPlayer.original.dll)
  [string]$EvidenceDir = (Join-Path $PSScriptRoot '..\..\Build\bin_swiffplayer\runs')   # SCAFFOLDING: screenshots + scrubbed log land here
)
$ErrorActionPreference = 'Stop'
$dev  = 'D:\B2BF Launcher\Battlefield2 - Dev - Swiff'
$binS = 'D:\LC\BF2Rebuilt\Code\Build\bin_swiffplayer'
New-Item -ItemType Directory -Force $EvidenceDir | Out-Null

if (Get-Process -Name BF2 -ErrorAction SilentlyContinue) { throw 'a BF2.exe is already running - not launching beside it' }

Copy-Item $Dll "$dev\SwiffPlayer.dll" -Force
$pdb = [System.IO.Path]::ChangeExtension($Dll, '.pdb')
if (Test-Path $pdb) { Copy-Item $pdb "$dev\SwiffPlayer.pdb" -Force }
Remove-Item "$dev\SwiffPlayerRebuilt.log" -ErrorAction SilentlyContinue
$h = (Get-FileHash "$dev\SwiffPlayer.dll" -Algorithm MD5).Hash
Write-Host "[$Tag] SwiffPlayer.dll staged ($h)"

$env:BF2_SWIFFPLAYER_ASLOG = '1'
$env:BF2_SWIFFPLAYER_STREAMLOG = '0'
$env:BF2_SWIFFPLAYER_CRASHLOG = '1'
$env:BF2_RENDDX9_SWIFFLOG = '0'

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class U32 {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@

$p = Start-Process -FilePath "$dev\BF2.exe" -ArgumentList '+fullscreen 0 +szx 800 +szy 600 +menu 1 +multi 1' -WorkingDirectory $dev -PassThru
$t0 = Get-Date
Write-Host "[$Tag] BF2 pid $($p.Id) launched; waiting $WaitLogin s for the login page"
Start-Sleep -Seconds $WaitLogin
if ($p.HasExited) { Write-Host "[$Tag] BF2 exited early, code $($p.ExitCode)"; }

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 20 -and $hwnd -eq [IntPtr]::Zero; $i++) {
  $p.Refresh(); $hwnd = $p.MainWindowHandle
  if ($hwnd -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 500 }
}
Write-Host "[$Tag] hwnd=$hwnd title='$($p.MainWindowTitle)'"

function Shot([string]$name) {
  Add-Type -AssemblyName System.Windows.Forms
  Add-Type -AssemblyName System.Drawing
  $rc = New-Object U32+RECT
  [void][U32]::GetClientRect($hwnd, [ref]$rc)
  $pt = New-Object U32+POINT; $pt.X = 0; $pt.Y = 0
  [void][U32]::ClientToScreen($hwnd, [ref]$pt)
  $w = $rc.R - $rc.L; $hh = $rc.B - $rc.T
  if ($w -le 0 -or $hh -le 0) { $w = 800; $hh = 600 }
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen((New-Object System.Drawing.Point $pt.X, $pt.Y), [System.Drawing.Point]::Empty, (New-Object System.Drawing.Size $w, $hh))
  $bmp.Save("$EvidenceDir\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Host "[$Tag] screenshot $name.png"
}

Shot "$Tag.0"
$WM_MOUSEMOVE = 0x200; $WM_LBUTTONDOWN = 0x201; $WM_LBUTTONUP = 0x202; $WM_CHAR = 0x102
$n = 0
foreach ($s in $Steps.Split(';')) {
  $n++
  if ($s -eq 'bs') {
    [void][U32]::PostMessage($hwnd, $WM_CHAR, [IntPtr]8, [IntPtr]0)
    Write-Host "[$Tag] step $n posted backspace (WM_CHAR 8)"
  } elseif ($s.StartsWith('t:')) {
    foreach ($ch in $s.Substring(2).ToCharArray()) {
      [void][U32]::PostMessage($hwnd, $WM_CHAR, [IntPtr]([int][char]$ch), [IntPtr]0); Start-Sleep -Milliseconds 120
    }
    Write-Host "[$Tag] step $n typed '$($s.Substring(2))'"
  } else {
    $xy = $s.Split(','); $cx = [int]$xy[0]; $cy = [int]$xy[1]
    $l = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
    [void][U32]::PostMessage($hwnd, $WM_MOUSEMOVE, [IntPtr]0, $l); Start-Sleep -Milliseconds 300
    [void][U32]::PostMessage($hwnd, $WM_LBUTTONDOWN, [IntPtr]1, $l); Start-Sleep -Milliseconds 250
    [void][U32]::PostMessage($hwnd, $WM_LBUTTONUP, [IntPtr]0, $l)
    Write-Host "[$Tag] step $n clicked at client ($cx,$cy)"
  }
  Start-Sleep -Seconds $Between
  if ($p.HasExited) { Write-Host "[$Tag] BF2 EXITED after step $n, code $($p.ExitCode)"; break }
  Shot "$Tag.$n"
  for ($b = 1; $b -lt $Burst; $b++) { Start-Sleep -Milliseconds 170; Shot "$Tag.$n.b$b" }
}

if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; Start-Sleep -Seconds 2 }
Write-Host "[$Tag] client stopped after $([int]((Get-Date) - $t0).TotalSeconds)s"

$src = "$dev\SwiffPlayerRebuilt.log"
$dst = "$EvidenceDir\$Tag.swiffplayer.log"
for ($try = 0; $try -lt 10; $try++) {
  try {
    $lines = Get-Content -LiteralPath $src -ErrorAction Stop
    $outl = New-Object System.Collections.Generic.List[string]
    $scrub = 0
    foreach ($ln in $lines) {
      if ($ln -match 'loginPassword') { $scrub = 3 }
      elseif ($scrub -gt 0) {
        $scrub--
        $ln = $ln -replace '(-> ")[^"]*(")', '$1<scrubbed>$2'
        $ln = $ln -replace '(arg1 = ")[^"]*(")', '$1<scrubbed>$2'
      }
      $outl.Add($ln)
    }
    [System.IO.File]::WriteAllLines($dst, $outl)
    Write-Host "[$Tag] log collected + scrubbed -> $dst ($($outl.Count) lines)"
    break
  } catch { Start-Sleep -Seconds 1 }
}
