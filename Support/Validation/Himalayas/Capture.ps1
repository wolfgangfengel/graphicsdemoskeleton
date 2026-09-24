param([ValidateSet("DirectX 11","DirectX 12","Vulkan")][string]$Api="DirectX 11",[int]$Time=1000,[ValidateRange(0,5)][int]$Route=0)
$Define=@{"DirectX 11"="PROBE_DX11";"DirectX 12"="PROBE_DX12";"Vulkan"="PROBE_VULKAN"}[$Api]
. "$PSScriptRoot\..\CompileCommon.ps1"
$output=Join-Path $probeRoot ($Define+"-t$Time-r$Route")
New-Item -ItemType Directory -Force $output | Out-Null
$project=Join-Path $repoRoot "$Api\07_Himalayas"
$encoding=@(); if($Api -eq 'Vulkan'){$encoding+='/DDEMO_USE_SMOLV'}
& (Join-Path $vc 'bin\Hostx64\x86\cl.exe') @common @encoding /Gz /D_DEBUG ("/D$Define") ("/I$project") ("/I$project\build\Debug\obj") "$PSScriptRoot\Capture.c" ("/Fo$output\Capture.obj") ("/Fe$output\Capture.exe") /link @link user32.lib d3d11.lib d3d12.lib dxgi.lib d3dcompiler.lib
if($LASTEXITCODE){throw 'Himalayas capture build failed'}
$env:VK_LAYER_ENABLES='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT'
$p=Start-Process "$output\Capture.exe" -ArgumentList @($Time,$Route) -WorkingDirectory $output -WindowStyle Hidden -RedirectStandardOutput "$output\probe.log" -PassThru
$null=$p.Handle
try {
    if(-not $p.WaitForExit(120000)){throw 'Capture timed out'}
    Get-Content "$output\probe.log"
    if($p.ExitCode){throw "Capture failed: $($p.ExitCode)"}
} finally {if(-not $p.HasExited){Stop-Process -Id $p.Id -Force};$p.Dispose()}
