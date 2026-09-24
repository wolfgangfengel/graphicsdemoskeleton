# Embed compact HLSL, preserving the readable sources and verifying identical
# stripped bytecode before the generated header can enter an executable.
param(
    [Parameter(Mandatory=$true)][string]$OutputHeader,
    [Parameter(Mandatory=$true)][string]$FxcPath,
    [switch]$DX12
)
$ErrorActionPreference = 'Stop'
$OutputHeader = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputHeader)
$outputDirectory = Split-Path -Parent $OutputHeader
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$stages = @(
    @{Name='vertex'; Entry='VShader'; Profile='vs_5_0'},
    @{Name='pixel'; Entry='PShader'; Profile='ps_5_0'}
)
$combined = ''
foreach ($stage in $stages) {
    $inputPath = Join-Path $PSScriptRoot ($stage.Name+'.hlsl')
    $prefix = Join-Path $outputDirectory $stage.Name
    $defines = @()
    if ($DX12 -and $stage.Name -eq 'vertex') { $defines = @('/D','SAN_ANGELES_DX12=1') }
    # Let FXC handle conditional root-signature declarations and remove comments.
    & $FxcPath /nologo /P "$prefix.preprocessed.hlsl" @defines $inputPath
    if ($LASTEXITCODE) { throw "Cannot preprocess $inputPath" }
    # FXC appends a NUL terminator; remove it before joining the two stages.
    $source = [IO.File]::ReadAllText("$prefix.preprocessed.hlsl").Replace([string][char]0,'')
    $source = [regex]::Replace($source,'(?m)^\s*#line[^\r\n]*','')
    if ($source -match '(?m)^\s*#') { throw 'Unexpected preprocessor directive' }
    $combined += $source+"`n"
}
$source = $combined
# Shorten declared names, including the comma-separated constant fields.
# Semantics, entry points, swizzles, and quoted root-signature text stay intact.
# This deliberately small declaration matcher is guarded by bytecode equality.
$used = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($match in [regex]::Matches($source,'[A-Za-z_][A-Za-z_0-9]*')) { [void]$used.Add($match.Value) }
$names = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
$types = 'void|bool|int|uint[1-4]?|float[1-4]?|cbuffer|struct'
foreach ($match in [regex]::Matches($source,'\bstruct\s+([A-Za-z_][A-Za-z_0-9]*)')) { $types += '|'+$match.Groups[1].Value }
foreach ($match in [regex]::Matches($source,'\b(?:'+$types+')\s+([A-Za-z_][A-Za-z_0-9]*(?:\s*,\s*[A-Za-z_][A-Za-z_0-9]*)*)')) {
    foreach ($name in ($match.Groups[1].Value -split '\s*,\s*')) {
        if ($name.Length -gt 1 -and $name -cnotin @('VShader','PShader') -and $name -cnotmatch '^[xyzwrgba]{1,4}$') { [void]$names.Add($name) }
    }
}
$mapping = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
$candidates = @((65..90 + 97..122) | ForEach-Object { [string][char]$_ })
$cursor = 0
foreach ($name in ($names | Sort-Object @{Expression={ -([regex]::Matches($source,'\b'+[regex]::Escape($_)+'\b').Count*$_.Length) }},@{Expression={$_}})) {
    while ($cursor -lt $candidates.Count -and $used.Contains($candidates[$cursor])) { ++$cursor }
    if ($cursor -eq $candidates.Count) { throw 'Too many names for this shader compactor' }
    $mapping.Add($name,$candidates[$cursor]); [void]$used.Add($candidates[$cursor]); ++$cursor
}
# Preserve complete numeric literals, operators, and strings while removing
# whitespace between tokens. Member names and their declarations rename alike.
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
    [void]$text.Append($token)
    $previous = $token
}
$shaderText = $text.ToString()
$compactPath = Join-Path $outputDirectory 'Shaders.compact.hlsl'
[IO.File]::WriteAllText($compactPath,$shaderText,[Text.Encoding]::ASCII)
foreach ($stage in $stages) {
    $inputPath = Join-Path $PSScriptRoot ($stage.Name+'.hlsl')
    $prefix = Join-Path $outputDirectory $stage.Name
    $defines = @()
    if ($DX12 -and $stage.Name -eq 'vertex') { $defines = @('/D','SAN_ANGELES_DX12=1') }
    & $FxcPath /nologo /O3 /T $stage.Profile /E $stage.Entry /Fo "$prefix.reference.cso" /Qstrip_reflect /Qstrip_debug @defines $inputPath
    if ($LASTEXITCODE) { throw 'Reference shader validation failed' }
    & $FxcPath /nologo /O3 /T $stage.Profile /E $stage.Entry /Fo "$prefix.compact.cso" /Qstrip_reflect /Qstrip_debug $compactPath
    if ($LASTEXITCODE) { throw 'Compact shader validation failed' }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes("$prefix.reference.cso")) -cne
        [Convert]::ToBase64String([IO.File]::ReadAllBytes("$prefix.compact.cso"))) {
        throw 'Compaction changed shader bytecode; the source will not be embedded.'
    }
    Write-Host "$($stage.Name): stripped bytecode is identical."
}
$header = "// Generated from San Angeles HLSL. See license-BSD.txt.`r`n"
$header += 'static const char shaderSource[] = '+"`r`n"
# Keep individual string tokens within VS2019's length limit.
for ($start=0; $start -lt $shaderText.Length; $start+=1000) {
    $chunk = $shaderText.Substring($start,[Math]::Min(1000,$shaderText.Length-$start))
    $header += '"'+$chunk.Replace('\','\\').Replace('"','\"')+'"'+"`r`n"
}
$header += ';'+"`r`n"
Write-Host "Embedded $($shaderText.Length) bytes of HLSL."
[IO.File]::WriteAllText($OutputHeader,$header,[Text.Encoding]::ASCII)
