# [TEMP] Copy selected production bodies verbatim for the strict focused fixture.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$base = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2')).Replace('\', '/')
$factoryPath = Join-Path $base 'Game/Objects/Base/SimpleObjectTemplate.cpp'
if ($env:BF2_TEST_FACTORY_SOURCE) { $factoryPath = $env:BF2_TEST_FACTORY_SOURCE }
function Read-Body([string]$path, [string]$signature) {
    $source = [IO.File]::ReadAllText($path)
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing production method: $signature" }
    $tail = $source.Substring($start)
    $end = [regex]::Match($tail, '(?m)^\}\r?$')
    if (!$end.Success) { throw "Missing method end: $signature" }
    $tail.Substring(0, $end.Index + 1)
}
$nameBody = Read-Body $factoryPath 'std::string SimpleObjectTemplate::makeSoundTemplateName('
$factoryBody = Read-Body $factoryPath 'void* SimpleObjectTemplate::createSoundFromTemplate('
$interfaceSource = [IO.File]::ReadAllText((Join-Path $base 'Game/Objects/Weapons/FireComponents/FireComp.cpp'))
$destructor = [regex]::Match($interfaceSource, '(?m)^IWeaponSoundComp::~IWeaponSoundComp\(\) \{\}\r?$')
if (!$destructor.Success) { throw 'Missing IWeaponSoundComp destructor' }
$generated = @"
// [TEMP] Exact production bodies under real headers; no behavior is substituted.
#include "$base/Game/Objects/Base/SimpleObjectTemplate.h"
#include "$base/Game/Objects/Weapons/FireComponents/FireComp.h"
#include "$base/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "$base/Game/Main/Game.h"
#include "$base/Sound/BfSound.h"
#include "$base/Sound/SoundObjectTemplate.h"
namespace dice { namespace hfe { namespace world {
$nameBody
$factoryBody
$($destructor.Value)
} } }
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'weapon_sound_under_test.cpp'), $generated)
