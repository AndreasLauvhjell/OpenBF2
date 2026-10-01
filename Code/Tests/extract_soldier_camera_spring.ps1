# [TEMP] Extract the production restoring function verbatim; no replacement implementation.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$path = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/SoldierAnimation.cpp'))
$source = [IO.File]::ReadAllText($path)
$start = $source.IndexOf('void applySpringFunction(')
$end = $source.IndexOf('// Soldier::updateFirstPersonAnimations -', $start)
if ($start -lt 0 -or $end -le $start) { throw 'Spring extraction boundaries changed.' }
$vars = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Utility/Vars.h')).Replace('\', '/')
$body = $source.Substring($start, $end - $start)
$result = @"
// [TEMP] Unchanged production function extracted from $path
#include "$vars"
namespace dice { namespace hfe { namespace world {
namespace { float* const anim_soldier_spring_clamp = Vars::getFloat("anim-soldier-spring-clamp", 0.001f); }
$body
} } }
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'Soldier_camera_spring_under_test.cpp'), $result)
