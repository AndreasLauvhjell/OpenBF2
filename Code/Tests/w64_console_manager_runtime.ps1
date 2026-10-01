# [TEMP] [CHOSEN, not recovered] Extract only the two real module classes'
# definitions from the shared registry TU; no copied bodies or fake collaborators.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$source = Get-Content (Join-Path $repo 'Code/BF2/Core/ModuleRegistry.cpp') -Raw
function ExactMatch([string]$pattern) {
    $found = [regex]::Matches($source, $pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if ($found.Count -ne 1) { throw "Expected one source match for $pattern, found $($found.Count)" }
    return $found[0].Value
}
function Member([string]$name) {
    $escaped = [regex]::Escape($name)
    $signature = [regex]::Match($source, "(?m)^(?:void|bool) $escaped\([^\r\n]*\)[^\r\n{]*\{")
    if (!$signature.Success) {
        $signature = [regex]::Match($source, "(?m)^(?:void|bool) $escaped\([^\r\n]*\)\s*\{")
    }
    if (!$signature.Success) { throw "Member $name not found" }
    $depth = 1
    $end = $signature.Index + $signature.Length
    while ($depth -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        if ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth) { throw "Unbalanced source member $name" }
    return $source.Substring($signature.Index, $end - $signature.Index)
}
$parts = @(
    '// Generated test-only selection of actual ModuleRegistry.cpp definitions.',
    '#include "Core/Modules.h"', '#include "Core/Ids.h"',
    '#include "IO/Console/Manager.h"', '#include "Math/Math.h"',
    'namespace dice { namespace hfe {',
    (ExactMatch '^#define BF2_MODULE_IBASE[^\r\n]*\\\r?\n(?:[^\r\n]*\\\r?\n)*[^\r\n]*'),
    (ExactMatch '^#define BF2_MODULE_NOOP_CLOSE[^\r\n]*'),
    'BF2_MODULE_IBASE(MathModule)',
    (Member 'MathModule::getClassList'), (Member 'MathModule::getSingletonList'),
    (Member 'MathModule::init'), (Member 'MathModule::close'),
    'namespace console {', 'BF2_MODULE_IBASE(ModuleConsole)',
    (Member 'ModuleConsole::getClassList'), (Member 'ModuleConsole::getSingletonList'),
    (Member 'ModuleConsole::init'), 'BF2_MODULE_NOOP_CLOSE(ModuleConsole)',
    '} } }'
)
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$parts -join "`r`n" | Set-Content (Join-Path $OutputDirectory 'module_bodies.cpp')
