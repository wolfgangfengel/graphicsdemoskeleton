# Compact one readable shader without changing its stripped compiled bytecode.
# Original source files retain their attribution and remain the editing surface.
param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$OutputHeader,
    [Parameter(Mandatory=$true)][string]$EntryPoint,
    [Parameter(Mandatory=$true)][ValidateSet('Vertex','Pixel','Compute')][string]$ShaderType,
    [Parameter(Mandatory=$true)][string]$FxcPath,
    [ValidateSet(0,1)][int]$RootSignature10 = 0,
    [ValidateRange(0,3)][int]$OptimizationLevel = 3,
    [string]$AdditionalEntryPoints = ''
)
$ErrorActionPreference = 'Stop'
if ($EntryPoint -cnotmatch '^[A-Za-z_][A-Za-z_0-9]*$') { throw 'Invalid shader entry point' }
$entries = @($EntryPoint) + @($AdditionalEntryPoints.Split(';') | Where-Object { $_ })
foreach ($entry in $entries) {
    if ($entry -cnotmatch '^[A-Za-z_][A-Za-z_0-9]*$') { throw 'Invalid additional shader entry point' }
}
$InputPath = (Resolve-Path -LiteralPath $InputPath).Path
$OutputHeader = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputHeader)
$outputDirectory = Split-Path -Parent $OutputHeader
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$prefix = Join-Path $outputDirectory ([IO.Path]::GetFileNameWithoutExtension($InputPath))
$profile = @{Vertex='vs_5_0'; Pixel='ps_5_0'; Compute='cs_5_0'}[$ShaderType]
$options = @()
if ($RootSignature10) { $options = @('/force_rootsig_ver','rootsig_1_0') }
& $FxcPath /nologo /P "$prefix.preprocessed.hlsl" $InputPath
if ($LASTEXITCODE) { throw "Cannot preprocess $InputPath" }
# FXC emits line directives and a terminating NUL; neither belongs in the payload.
$source = [IO.File]::ReadAllText("$prefix.preprocessed.hlsl").Replace([string][char]0,'')
$source = [regex]::Replace($source,'(?m)^\s*#line[^\r\n]*','')
if ($source -match '(?m)^\s*#') { throw 'Unexpected preprocessor directive' }
$used = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($match in [regex]::Matches($source,'[A-Za-z_][A-Za-z_0-9]*')) { [void]$used.Add($match.Value) }
$names = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$types = 'void|bool|(?:int|uint|float|double)[1-4]?(?:x[1-4])?|cbuffer|struct|(?:RWTexture2D|RWStructuredBuffer|StructuredBuffer)\s*<[^>]+>'
foreach ($match in [regex]::Matches($source,'\bstruct\s+([A-Za-z_][A-Za-z_0-9]*)')) { $types += '|'+$match.Groups[1].Value }
# This is a conservative declaration matcher, not a general HLSL parser.
# Preserve entry points, semantics, swizzles, numeric literals and root strings.
foreach ($match in [regex]::Matches($source,'\b(?:'+$types+')\s+([A-Za-z_][A-Za-z_0-9]*(?:\s*,\s*[A-Za-z_][A-Za-z_0-9]*)*)')) {
    foreach ($name in ($match.Groups[1].Value -split '\s*,\s*')) {
        if ($name.Length -gt 1 -and $name -cnotin $entries -and
            $name -cnotmatch '^[xyzwrgba]{1,4}$' -and
            $name -cnotmatch '^(?:void|bool|int|uint|float|double)[1-4]?(?:x[1-4])?$' -and
            $name -cnotin @('in','out','inout','const','uniform','row_major','column_major','nointerpolation')) {
            [void]$names.Add($name)
        }
    }
}
$mapping = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
$candidates = @((65..90 + 97..122) | ForEach-Object { [string][char]$_ })
foreach ($first in 65..90) { foreach ($last in 65..90) { $candidates += ([string][char]$first)+([string][char]$last) } }
$cursor = 0
foreach ($name in ($names | Sort-Object @{Expression={ -([regex]::Matches($source,'\b'+[regex]::Escape($_)+'\b').Count*$_.Length) }},@{Expression={$_}})) {
    while ($cursor -lt $candidates.Count -and $used.Contains($candidates[$cursor])) { ++$cursor }
    if ($cursor -eq $candidates.Count) { throw 'Too many identifiers for the shader compactor' }
    $mapping.Add($name,$candidates[$cursor]); [void]$used.Add($candidates[$cursor]); ++$cursor
}
$tokens = '"(?:\\.|[^"\\])*"|(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?[fFuU]?|[A-Za-z_][A-Za-z_0-9]*|<<=|>>=|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|&=|\|=|\^=|[^\s]'
$text = New-Object Text.StringBuilder
$previous = ''
foreach ($match in [regex]::Matches($source,$tokens)) {
    $token = $match.Value
    if ($mapping.ContainsKey($token)) { $token = $mapping[$token] }
    if (($previous -match '[A-Za-z_0-9]$' -and $token -match '^[A-Za-z_0-9]') -or
        ($previous -eq '.' -and $token -match '^\d') -or
        ($previous -match '\d$' -and $token -eq '.') -or
        ($previous+$token -in @('++','--','<<','>>','<=','>=','==','!=','&&','||','+=','-=','*=','/=','%=','&=','|=','^=','<<=','>>=','//','/*'))) {
        [void]$text.Append(' ')
    }
    [void]$text.Append($token); $previous = $token
}
$shaderText = $text.ToString()
[IO.File]::WriteAllText("$prefix.compact.hlsl",$shaderText,[Text.Encoding]::ASCII)
foreach ($entry in $entries) {
    $validationPrefix = if ($entries.Count -eq 1) { $prefix } else { "$prefix.$entry" }
    & $FxcPath /nologo "/O$OptimizationLevel" /T $profile /E $entry /Fo "$validationPrefix.reference.cso" /Qstrip_reflect /Qstrip_debug @options $InputPath
    if ($LASTEXITCODE) { throw 'Reference shader validation failed' }
    & $FxcPath /nologo "/O$OptimizationLevel" /T $profile /E $entry /Fo "$validationPrefix.compact.cso" /Qstrip_reflect /Qstrip_debug @options "$prefix.compact.hlsl"
    if ($LASTEXITCODE) { throw 'Compact shader validation failed' }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes("$validationPrefix.reference.cso")) -cne
        [Convert]::ToBase64String([IO.File]::ReadAllBytes("$validationPrefix.compact.cso"))) {
        throw "Compaction changed shader bytecode for $entry; the source will not be embedded."
    }
}
$header = '// Generated from '+[IO.Path]::GetFileName($InputPath)+"; retain its original attribution.`r`n"
$header += 'static const char source_'+$EntryPoint+'[] = '+"`r`n"
for ($start=0; $start -lt $shaderText.Length; $start+=1000) {
    $chunk = $shaderText.Substring($start,[Math]::Min(1000,$shaderText.Length-$start))
    $header += '"'+$chunk.Replace('\','\\').Replace('"','\"')+'"'+"`r`n"
}
$header += ';'+"`r`n"
[IO.File]::WriteAllText($OutputHeader,$header,[Text.Encoding]::ASCII)
Write-Host "$EntryPoint`: embedded $($shaderText.Length) bytes of HLSL; stripped bytecode is identical."
