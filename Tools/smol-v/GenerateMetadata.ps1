$ErrorActionPreference='Stop'
$source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'smolv.cpp'))
$table=[regex]::Match($source,'(?s)static const OpData kSpirvOpData\[\] =\s*\{(.*?)\};').Groups[1].Value
$values=foreach($row in [regex]::Matches($table,'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}')) {
    $result=[int]$row.Groups[1].Value+2*[int]$row.Groups[2].Value+8*[int]$row.Groups[3].Value+4*[int]$row.Groups[4].Value
    if($result -gt 255) { throw 'Opcode metadata does not fit the compact table' }
    $result
}
if($values.Count -ne 367) { throw "Unexpected upstream table length: $($values.Count)" }
$header="/* Generated from pinned smolv.cpp by GenerateMetadata.ps1. Aras Pranckevicius, MIT. */`nstatic const unsigned char smolOps[] = {`n"
for($i=0;$i -lt $values.Count;$i+=24){$header+=($values[$i..([Math]::Min($i+23,$values.Count-1))] -join ',')+",`n"}
$header+="};`n"
[IO.File]::WriteAllText((Join-Path $PSScriptRoot '..\..\Vulkan\Common\SmolvOps.h'),$header,[Text.Encoding]::ASCII)
