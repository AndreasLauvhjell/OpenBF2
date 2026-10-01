# [TEMP] Copy one complete production body unchanged into the flow fixture.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$sourcePath = Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/SoldierAnimation.cpp'
$source = [IO.File]::ReadAllText($sourcePath)
$start = $source.IndexOf('void Soldier::updateAnimationSystems(float dt)')
if ($start -lt 0) { throw 'Missing updateAnimationSystems' }
$open = $source.IndexOf('{', $start)
$depth = 1
$end = $open + 1
while ($depth -gt 0 -and $end -lt $source.Length) {
    if ($source[$end] -eq '{') { ++$depth }
    if ($source[$end] -eq '}') { --$depth }
    ++$end
}
if ($depth -ne 0) { throw 'Missing body end' }
$header = (Join-Path $PSScriptRoot 'soldier_animation_feed_fixture.h').Replace('\','/')
$generated = '// [TEMP] Unchanged source method, isolated collaborator fixture.' + "`r`n#include `"$header`"`r`n" + $source.Substring($start, $end-$start)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'Soldier_animation_feed_under_test.cpp'), $generated)
