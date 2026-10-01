# SCAFFOLDING (Code/Tests) - native-client step driver: the NATIVE
# run from the reference game dir like the CLion run configuration) with the freshly built native
# SwiffPlayer.dll staged beside it, windowed 800x600, then posted steps ("x,y" click, "t:text",
# "bs"), a client-area screenshot after each, kill, collect + scrub the log from the game dir.
param(
  [string]$Tag = 'nat',
  [string]$Steps = '291,414;383,414',
  [int]$WaitLogin = 30,
  [int]$Between = 4,
  [int]$AsLog = 1,                                                                        # 2 also forwards the movie's own trace() lines
  [string]$BaseArgs = '+modPath mods/bf2 +noSound 1 +fullscreen 0 +multi 1 +restart 1 +szx 800 +szy 600 +menu 1',
  [string]$ExtraArgs = '',                                                              # appended to the launch command line (e.g. '+playerName x +playerPassword y')
  [switch]$CaptureStderr,                                                                # the engine's [FRONTIER] stops go to stderr, which a windowed client otherwise drops
  [string]$EvidenceDir = (Join-Path $PSScriptRoot '..\..\Build\bin_swiffplayer\runs')   # SCAFFOLDING: screenshots + scrubbed log land here
)
$ErrorActionPreference = 'Stop'
$game = 'D:\B2BF Launcher\Battlefield2'
$bin  = 'D:\LC\BF2Rebuilt\cmake-build-debug\bin_client'
$src  = 'D:\LC\BF2Rebuilt\Code\Build\bin_swiffplayer_native'
New-Item -ItemType Directory -Force $EvidenceDir | Out-Null

if (Get-Process -Name BF2 -ErrorAction SilentlyContinue) { throw 'a BF2.exe is already running - not launching beside it' }

Copy-Item "$src\SwiffPlayer.dll" "$bin\SwiffPlayer.dll" -Force
if (Test-Path "$src\SwiffPlayer.pdb") { Copy-Item "$src\SwiffPlayer.pdb" "$bin\SwiffPlayer.pdb" -Force }
Remove-Item "$game\SwiffPlayerRebuilt.log" -ErrorAction SilentlyContinue
$h = (Get-FileHash "$bin\SwiffPlayer.dll" -Algorithm MD5).Hash
Write-Host "[$Tag] native SwiffPlayer.dll staged beside the diagnostic client ($h)"

$env:BF2_SWIFFPLAYER_ASLOG = "$AsLog"
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
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
  // SCAFFOLDING: the "real:x,y" step. The engine polls the MOUSE through its input device, so a
  // PostMessage'd WM_LBUTTONDOWN is invisible to it - the Flash menu consumes window messages and
  // reacts, the native HUD (Ref2System/meme, driven by io::MouseDevice) does not. mouse_event
  // enters the real input queue, which both see.
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, int dx, int dy, uint data, IntPtr extra);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@

$launch = @{ FilePath = "$bin\BF2.exe"
             ArgumentList = ($BaseArgs + ' ' + $ExtraArgs).Trim()
             WorkingDirectory = $game; PassThru = $true }
if ($CaptureStderr) {
  $launch.RedirectStandardError  = Join-Path $EvidenceDir "$Tag.stderr.txt"
  $launch.RedirectStandardOutput = Join-Path $EvidenceDir "$Tag.stdout.txt"
}
$p = Start-Process @launch
$t0 = Get-Date
Write-Host "[$Tag] native BF2 pid $($p.Id) launched; waiting $WaitLogin s for the login page"
Start-Sleep -Seconds $WaitLogin
if ($p.HasExited) { Write-Host "[$Tag] BF2 exited early, code $($p.ExitCode)" }

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 20 -and $hwnd -eq [IntPtr]::Zero -and -not $p.HasExited; $i++) {
  $p.Refresh(); $hwnd = $p.MainWindowHandle
  if ($hwnd -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 500 }
}
Write-Host "[$Tag] hwnd=$hwnd title='$($p.MainWindowTitle)'"

function Shot([string]$name) {
  # SCAFFOLDING: the capture is a screen copy, so the client must be in front of everything else
  [void][U32]::BringWindowToTop($hwnd); [void][U32]::SetForegroundWindow($hwnd); Start-Sleep -Milliseconds 400
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

if (-not $p.HasExited) { Shot "$Tag.0" }
$WM_MOUSEMOVE = 0x200; $WM_LBUTTONDOWN = 0x201; $WM_LBUTTONUP = 0x202; $WM_CHAR = 0x102
$n = 0
foreach ($s in $Steps.Split(';')) {
  if ($p.HasExited) { break }
  $n++
  # SCAFFOLDING: "step@seconds" waits that long after the step instead of -Between (a click on a
  # button that is only up for a moment, e.g. the browser modal's STOP UPDATE, needs a short gap)
  $delay = $Between
  if ($s -match '^(.*)@([0-9.]+)$') { $s = $matches[1]; $delay = [double]$matches[2] }
  if ($s -eq 'enter') {
    # SCAFFOLDING: Return as the client sees it from the keyboard - key down, the character, key up
    [void][U32]::PostMessage($hwnd, 0x100, [IntPtr]13, [IntPtr]0x001C0001); Start-Sleep -Milliseconds 30
    [void][U32]::PostMessage($hwnd, $WM_CHAR, [IntPtr]13, [IntPtr]0x001C0001); Start-Sleep -Milliseconds 30
    [void][U32]::PostMessage($hwnd, 0x101, [IntPtr]13, [IntPtr]0xC01C0001)
    Write-Host "[$Tag] step $n posted Return (WM_KEYDOWN/WM_CHAR/WM_KEYUP 13)"
  } elseif ($s.StartsWith('key:')) {
    # SCAFFOLDING: a real key press through the input queue (the engine polls keys; posted messages are not seen)
    Add-Type -AssemblyName System.Windows.Forms
    [void][U32]::BringWindowToTop($hwnd); [void][U32]::SetForegroundWindow($hwnd); Start-Sleep -Milliseconds 400
    [System.Windows.Forms.SendKeys]::SendWait('{' + $s.Substring(4) + '}')
    Write-Host "[$Tag] step $n sent key {$($s.Substring(4))} through SendKeys"
  } elseif ($s.StartsWith('real:')) {
    # SCAFFOLDING: a REAL left click at client (x,y) through the input queue, for the parts of the
    # client that poll the mouse instead of reading window messages - the in-game HUD above all.
    $xy = $s.Substring(5).Split(','); $cx = [int]$xy[0]; $cy = [int]$xy[1]
    [void][U32]::BringWindowToTop($hwnd); [void][U32]::SetForegroundWindow($hwnd); Start-Sleep -Milliseconds 400
    $sp = New-Object U32+POINT; $sp.X = $cx; $sp.Y = $cy; [void][U32]::ClientToScreen($hwnd, [ref]$sp)
    [void][U32]::SetCursorPos($sp.X, $sp.Y); Start-Sleep -Milliseconds 400
    [U32]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 120
    [U32]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
    Write-Host "[$Tag] step $n REAL click at client ($cx,$cy) -> screen ($($sp.X),$($sp.Y))"
  } elseif ($s.StartsWith('hud:')) {
    # SCAFFOLDING: move the IN-GAME HUD pointer by a RELATIVE mouse delta and (optionally) click.
    # ⚠ The HUD cursor is a VIRTUAL pointer: BF2HudManager::handleGameInput advances it by
    # `axis * sensitivity` from the mouse's DELTA, so SetCursorPos does not move it at all and a
    # "real:x,y" step clicks wherever the virtual pointer happens to be. Only relative motion works.
    # Syntax: hud:dx,dy       move only
    #         hud:dx,dy,click move then press and release
    $parts = $s.Substring(4).Split(',')
    $dx = [int]$parts[0]; $dy = [int]$parts[1]
    [void][U32]::BringWindowToTop($hwnd); [void][U32]::SetForegroundWindow($hwnd); Start-Sleep -Milliseconds 400
    # a few small steps, because one huge delta can be clamped by the engine's own per-frame limit
    $steps = 10
    for ($k = 0; $k -lt $steps; $k++) {
      [U32]::mouse_event(0x0001, [int]($dx / $steps), [int]($dy / $steps), 0, [IntPtr]::Zero)
      Start-Sleep -Milliseconds 30
    }
    Write-Host "[$Tag] step $n HUD pointer moved by ($dx,$dy) in $steps relative steps"
    if ($parts.Length -gt 2 -and $parts[2] -eq 'click') {
      Start-Sleep -Milliseconds 300
      [U32]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 120
      [U32]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
      Write-Host "[$Tag] step $n clicked where the HUD pointer now is"
    }
  } elseif ($s -eq 'wait') {
    # SCAFFOLDING: no input at all - a pause (normally with an @seconds suffix) between two steps
    Write-Host "[$Tag] step $n wait"
  } elseif ($s -eq 'bs') {
    [void][U32]::PostMessage($hwnd, $WM_CHAR, [IntPtr]8, [IntPtr]0)
    Write-Host "[$Tag] step $n posted backspace (WM_CHAR 8)"
  } elseif ($s.StartsWith('t:')) {
    foreach ($ch in $s.Substring(2).ToCharArray()) {
      [void][U32]::PostMessage($hwnd, $WM_CHAR, [IntPtr]([int][char]$ch), [IntPtr]0); Start-Sleep -Milliseconds 120
    }
    Write-Host "[$Tag] step $n typed '$($s.Substring(2))'"
  } else {
    $xy = $s.Split(','); $cx = [int]$xy[0]; $cy = [int]$xy[1]
    # SCAFFOLDING: the engine reads the REAL cursor while its window is in front, so park it on the target too
    $sp = New-Object U32+POINT; $sp.X = $cx; $sp.Y = $cy; [void][U32]::ClientToScreen($hwnd, [ref]$sp)
    [void][U32]::SetCursorPos($sp.X, $sp.Y); Start-Sleep -Milliseconds 150
    $l = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
    [void][U32]::PostMessage($hwnd, $WM_MOUSEMOVE, [IntPtr]0, $l); Start-Sleep -Milliseconds 300
    [void][U32]::PostMessage($hwnd, $WM_LBUTTONDOWN, [IntPtr]1, $l); Start-Sleep -Milliseconds 250
    [void][U32]::PostMessage($hwnd, $WM_LBUTTONUP, [IntPtr]0, $l)
    Write-Host "[$Tag] step $n clicked at client ($cx,$cy)"
  }
  Start-Sleep -Seconds $delay
  if ($p.HasExited) { Write-Host "[$Tag] BF2 EXITED after step $n, code $($p.ExitCode)"; break }
  Shot "$Tag.$n"
}

if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; Start-Sleep -Seconds 2 }
Write-Host "[$Tag] client stopped after $([int]((Get-Date) - $t0).TotalSeconds)s exited=$($p.HasExited) code=$($p.ExitCode)"

$srcLog = "$game\SwiffPlayerRebuilt.log"
$dst = "$EvidenceDir\$Tag.swiffplayer.log"
for ($try = 0; $try -lt 10; $try++) {
  try {
    $lines = Get-Content -LiteralPath $srcLog -ErrorAction Stop
    $outl = New-Object System.Collections.Generic.List[string]
    $scrub = 0
    $acct = 0
    foreach ($ln in $lines) {
      # The Profile account rows log their credentials as ARGUMENTS (loginOnlineAccount's arg1 is
      # the password), which the storage-key rule below never saw (2026-09-28): blank every argument
      # of these calls up to their leave line.
      if ($ln -match 'enter as_profile_(login|create)\w*Account') { $acct = 8 }
      elseif ($acct -gt 0) {
        $acct--
        $ln = $ln -replace '(arg\d+ = ")[^"]*(")', '$1<scrubbed>$2'
        if ($ln -match 'leave as_profile_') { $acct = 0 }
      }
      elseif ($ln -match 'loginPassword' -or $ln -match 'arg0 = "[^"]*[Pp]assword[^"]*"') { $scrub = 3 }
      elseif ($scrub -gt 0) {
        $scrub--
        $ln = $ln -replace '(-> ")[^"]*(")', '$1<scrubbed>$2'
        $ln = $ln -replace '(arg1 = ")[^"]*(")', '$1<scrubbed>$2'
      }
      $outl.Add($ln)
    }
    [System.IO.File]::WriteAllLines($dst, $outl)
    # The DLL's own log in the game directory holds the same lines UNSCRUBBED; overwrite it with the
    # scrubbed copy so no credential stays on disk there either.
    [System.IO.File]::WriteAllLines($srcLog, $outl)
    Write-Host "[$Tag] log collected + scrubbed -> $dst ($($outl.Count) lines)"
    break
  } catch { Start-Sleep -Seconds 1 }
}
