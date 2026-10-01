# [TEMP] Extract the production method unchanged; no replacement implementation.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$sourcePath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/SoldierAnimation.cpp'))
$source = [IO.File]::ReadAllText($sourcePath)
$start = $source.IndexOf('bool Soldier::applyCameraAnimation(')
$end = $source.IndexOf('// =================================================================================================', $start)
if ($start -lt 0 -or $end -le $start) { throw 'Camera method extraction boundaries changed.' }
$soldier = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/Soldier.h')).Replace('\', '/')
$template = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/SoldierTemplate.h')).Replace('\', '/')
$skeleton = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Animation/BoneAnimation/Skeleton.h')).Replace('\', '/')
$body = $source.Substring($start, $end - $start)
$generated = @"
// [TEMP] Unchanged production method extracted from $sourcePath
#include "$soldier"
#include "$template"
#include "$skeleton"
extern int cameraAnimationsFixture;
namespace dice { namespace hfe { namespace world {
namespace { int* const anim_playCameraAnimations = &cameraAnimationsFixture; }
$body
} } }
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'Soldier_camera_apply_under_test.cpp'), $generated)
