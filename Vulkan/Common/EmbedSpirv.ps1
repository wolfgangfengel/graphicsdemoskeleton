param([string]$InputPath,[string]$OutputHeader,[string]$EntryPoint,[ValidateSet('cs','vs','ps')][string]$Stage,[ValidateSet(0,1)][int]$UseSmolv=0)
$ErrorActionPreference='Stop'
& "$PSScriptRoot\..\..\Tools\Vulkan\Setup.ps1"
$compiler="$PSScriptRoot\..\..\Tools\Vulkan\build\dxc\bin\x64\dxc.exe"
$inputFile=(Resolve-Path -LiteralPath $InputPath).Path
$directory=Split-Path $OutputHeader
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$source=[IO.File]::ReadAllText($inputFile)
# Resource declarations only: all rendering expressions and numeric constants
# remain in the original shared HLSL files.
$source=[regex]::Replace($source,'\[RootSignature\("[^"\r\n]*"\)\]','')
if($Stage -eq 'vs') {
    $source=[regex]::Replace($source,'(?s)cbuffer\s+\w+\s*:\s*register\(b0\)\s*\{(.*?)\};',{
        param($match)
        $body=$match.Groups[1].Value
        $result="struct VulkanConstants { $body };`n[[vk::push_constant]] ConstantBuffer<VulkanConstants> vkConstants;`n"
        foreach($field in [regex]::Matches($body,'\bfloat[1-4]?(?:x[1-4])?\s+([A-Za-z_][A-Za-z_0-9]*(?:\s*,\s*[A-Za-z_][A-Za-z_0-9]*)*)')) {
            foreach($name in ($field.Groups[1].Value -split '\s*,\s*')) { $result+="#define $name vkConstants.$name`n" }
        }
        return $result
    })
}
$source=[regex]::Replace($source,'\bRWTexture2D\s*<float4>','[[vk::image_format("rgba8")]] RWTexture2D<float4>')
$temporary="$OutputHeader.hlsl"
[IO.File]::WriteAllText($temporary,$source,[Text.Encoding]::ASCII)
$options=@('-spirv','-fspv-target-env=vulkan1.0','-fvk-use-dx-layout','-fvk-b-shift','2','0','-fvk-t-shift','1','0','-O3','-T',($Stage+'_6_0'),'-E',$EntryPoint,'-I',(Split-Path $inputFile),'-Fo',"$OutputHeader.spv")
if($Stage -eq 'vs') { $options+='-fvk-invert-y' }
& $compiler @options $temporary
if($LASTEXITCODE) { throw "SPIR-V compilation failed: $inputFile" }
$bytes=[IO.File]::ReadAllBytes("$OutputHeader.spv")
if($bytes.Length%4) { throw 'Unaligned SPIR-V module' }
# OpName/OpMemberName are optional debugging annotations. Keep entry-point
# names and every executable instruction; no arithmetic or control-flow edits.
[IO.File]::WriteAllBytes("$OutputHeader.unstripped.spv",$bytes)
$compact=New-Object 'System.Collections.Generic.List[byte]'
$compact.AddRange([byte[]]$bytes[0..19])
for($offset=20;$offset -lt $bytes.Length;) {
    $instruction=[BitConverter]::ToUInt32($bytes,$offset)
    $length=[int]($instruction -shr 16)*4; $opcode=$instruction -band 65535
    if(!$length -or $offset+$length -gt $bytes.Length) { throw 'Malformed SPIR-V instruction' }
    if($opcode -ne 5 -and $opcode -ne 6) { $compact.AddRange([byte[]]$bytes[$offset..($offset+$length-1)]) }
    $offset+=$length
}
$bytes=$compact.ToArray()
[IO.File]::WriteAllBytes("$OutputHeader.spv",$bytes)
if($UseSmolv) {
    & "$PSScriptRoot\..\..\Tools\smol-v\Build.ps1"
    & "$PSScriptRoot\..\..\Tools\smol-v\build\Encode.exe" "$OutputHeader.spv" "$OutputHeader.smolv"
    if($LASTEXITCODE) { throw "SMOL-V round-trip validation failed: $EntryPoint" }
    $bytes=[IO.File]::ReadAllBytes("$OutputHeader.smolv")
    $words=$bytes | ForEach-Object { '0x{0:x2}' -f $_ }
    $type='unsigned char'
} else {
    $words=for($i=0;$i -lt $bytes.Length;$i+=4){ '0x{0:x8}' -f [BitConverter]::ToUInt32($bytes,$i) }
    $type='unsigned int'
}
$text='// Generated shader; attribution remains in '+[IO.Path]::GetFileName($inputFile)+".`r`nstatic const $type spv_$EntryPoint"+'[] = {'+"`r`n"+($words -join ',')+"`r`n};`r`n"
[IO.File]::WriteAllText($OutputHeader,$text,[Text.Encoding]::ASCII)
Write-Host "$EntryPoint : $($bytes.Length) embedded bytes (SMOL-V=$UseSmolv)"
