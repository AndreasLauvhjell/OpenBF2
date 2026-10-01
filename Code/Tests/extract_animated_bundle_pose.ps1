# [TEMP] Extract the actual method and its helpers unchanged; never rewrite the production TU.
param([Parameter(Mandatory = $true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$sourcePath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../BF2/Game/Objects/Game/AnimatedBundle.cpp'))
$source = [IO.File]::ReadAllText($sourcePath)
$prefixEndMarker = '} // namespace'
$prefixEnd = $source.IndexOf($prefixEndMarker)
$methodStart = $source.IndexOf('void AnimatedBundle::handleVisualUpdate(')
$methodEnd = $source.IndexOf('// 0x81678F0', $methodStart)
if ($prefixEnd -lt 0 -or $methodStart -lt $prefixEnd -or $methodEnd -lt $methodStart) {
    throw 'AnimatedBundle extraction boundaries changed; review the fixture extractor.'
}
$prefix = $source.Substring(0, $prefixEnd + $prefixEndMarker.Length)
# The generated TU lives in scratch storage, so only include paths need relocation.
$sourceDirectory = [IO.Path]::GetDirectoryName($sourcePath)
$prefix = [regex]::Replace($prefix, '(?m)^#include "([^"]+)"', {
    param($match)
    '#include "' + [IO.Path]::GetFullPath((Join-Path $sourceDirectory $match.Groups[1].Value)).Replace('\', '/') + '"'
})
$method = $source.Substring($methodStart, $methodEnd - $methodStart)
$generated = "// [TEMP] Mechanically extracted, unchanged method from $sourcePath`r`n" +
    $prefix + "`r`n" + $method + "`r`n} } }`r`n"
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'AnimatedBundle_pose_under_test.cpp'), $generated)
$hasher = [Security.Cryptography.SHA256]::Create()
Write-Output ('Production source SHA256: ' + [BitConverter]::ToString(
    $hasher.ComputeHash([IO.File]::ReadAllBytes($sourcePath))).Replace('-', ''))
$hasher.Dispose()
