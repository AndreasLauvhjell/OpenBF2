# [TEMP] [CHOSEN, not recovered] Extract the actual startup bracket without launching BF2.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$source = Get-Content (Join-Path $repo 'Code/BF2/Game/Main/BF2.cpp') -Raw
$start = $source.IndexOf('static int bf2WinMain(')
$end = $source.IndexOf('} // namespace hfe', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Startup bracket not found' }
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$source.Substring($start, $end - $start) | Set-Content (Join-Path $OutputDirectory 'startup_body.inl')
