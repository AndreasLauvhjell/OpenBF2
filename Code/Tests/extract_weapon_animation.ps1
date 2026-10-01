# [TEMP] Extract selected production methods verbatim; no gameplay body is substituted.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$base = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2')).Replace('\', '/')
$source = [IO.File]::ReadAllText((Join-Path $base 'Game/Objects/Weapons/GenericFireArm.cpp'))
$methods = @('void GenericFireArm::updateGeometry()', 'void GenericFireArm::setActiveAnimationSystem(int index)', 'void* GenericFireArm::getActiveAnimationSystem()', 'void GenericFireArm::doReload()')
$bodies = foreach ($signature in $methods) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production method: $signature" }
    $tail = $source.Substring($start)
    $end = [regex]::Match($tail, '(?m)^\}\r?$')
    if (!$end.Success) { throw "Missing method end: $signature" }
    $tail.Substring(0, $end.Index + 1)
}
$generated = @"
// [TEMP] Exact production methods extracted at build time under their real class declarations.
#include "$base/Game/Objects/Weapons/GenericFireArm.h"
#include "$base/Game/Objects/Weapons/AnimationComponents/AnimationComp.h"
#include "$base/Game/Objects/Weapons/AmmoComponents/AmmoComp.h"
#include "$base/Game/Objects/Weapons/ZoomComponents/ZoomComp.h"
#include "$base/Animation/BoneAnimation/AnimationSystem.h"
#include "$base/Math/Mat4.h"
#include "$base/Core/Ids.h"
namespace dice { namespace hfe { namespace world {
$($bodies -join "`r`n")
} } }
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'GenericFireArm_animation_under_test.cpp'), $generated)
