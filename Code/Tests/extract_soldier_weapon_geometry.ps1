# [TEMP] Extract new production blocks verbatim. No gameplay implementation is substituted.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$path = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/SoldierAnimation.cpp'))
$source = [IO.File]::ReadAllText($path)
function Extract-Block([string]$marker, [int]$indent) {
    $start = $source.IndexOf($marker)
    if ($start -lt 0) { throw "Missing production boundary: $marker" }
    $tail = $source.Substring($start)
    $match = [regex]::Match($tail, ('(?m)^' + (' ' * $indent) + '\}\r?$'))
    if (!$match.Success) { throw 'Missing block end' }
    return $tail.Substring(0, $match.Index)
}
$weaponBlock = Extract-Block '                    // Weapon parts: ELF' 16
$boneBlock = Extract-Block '                        // ELF 0x81B326E / client PE' 20
$base = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2')).Replace('\', '/')
$generated = @"
// [TEMP] Unchanged blocks extracted from $path; independent receiver/storage fixture only.
#include "$base/Game/Objects/GeometryPartAccess.h"
#include "$base/Game/Objects/IWeaponObject.h"
#include "$base/Animation/BoneAnimation/Skeleton.h"
#include <vector>
using namespace dice::hfe;
using namespace dice::hfe::world;
void geometrySetGeomIndex(void* geometry, int index)
{
    typedef void (__thiscall* Function)(void*, int);
    geometry_part::slot<Function>(geometry, 0x6C, 0x70)(geometry, index);
}
void applyWeaponGeometryFixture(int& m_characterMode, void* weaponGeometry, IWeaponObject* weapon,
                                dice::anim::Skeleton* skeleton, const Mat4* soldierTransform)
{
    const int boneBase[2] = {54, 64};
    const int boneLimit[2] = {16, 8};
$weaponBlock
}
void gatherWeaponBonesFixture(void* weaponGeometry, std::vector<int>& usedBones)
{
$boneBlock
}
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'Soldier_weapon_geometry_under_test.cpp'), $generated)
