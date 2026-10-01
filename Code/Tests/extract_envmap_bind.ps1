param([Parameter(Mandatory=$true)][string]$Output)
# [TEMP] Extract the unchanged production function into the narrow host-binding fixture.
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$source=Get-Content -LiteralPath (Join-Path $taskRoot 'BF2/Game/Objects/TemplateLeaves.cpp') -Raw
$start=$source.IndexOf('void EnvMapTemplate::templateInitialize()')
if($start -lt 0){throw 'EnvMapTemplate bind start missing'}
$open=$source.IndexOf('{',$start)
$depth=0;$finish=-1
for($index=$open;$index -lt $source.Length;$index++){
 if($source[$index] -eq '{'){$depth++}
 elseif($source[$index] -eq '}'){$depth--;if($depth -eq 0){$finish=$index+1;break}}
}
if($finish -lt 0){throw 'EnvMapTemplate bind end missing'}
[IO.File]::WriteAllText($Output,$source.Substring($start,$finish-$start),[Text.UTF8Encoding]::new($false))
