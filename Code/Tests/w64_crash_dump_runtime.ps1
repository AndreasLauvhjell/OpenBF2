# [TEMP] [CHOSEN, not recovered] Extract the unchanged method text into the bounded fixture.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$source = Get-Content (Join-Path $PSScriptRoot '../BF2/Game/Main/BF2Log.cpp') -Raw
$start = $source.IndexOf('void BF2Log::addLogEntry(')
if ($start -lt 0) { throw 'Missing addLogEntry' }
$brace = $source.IndexOf('{', $start)
$depth = 1
$end = $brace + 1
while ($depth -gt 0 -and $end -lt $source.Length) {
    if ($source[$end] -eq '{') { ++$depth }
    if ($source[$end] -eq '}') { --$depth }
    ++$end
}
if ($depth) { throw 'Incomplete addLogEntry body' }
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'crash_log_body.inc'), $source.Substring($start, $end - $start))
