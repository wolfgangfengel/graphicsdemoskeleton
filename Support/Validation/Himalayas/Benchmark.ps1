param([ValidateSet("DirectX 11","DirectX 12","Vulkan")][string]$Api="DirectX 11",[int]$Time=1000,[ValidateRange(0,5)][int]$Route=0)
$Define=@{"DirectX 11"="PROBE_DX11";"DirectX 12"="PROBE_DX12";"Vulkan"="PROBE_VULKAN"}[$Api]
. "$PSScriptRoot\..\CompileCommon.ps1"
$output=Join-Path $probeRoot ($Define+'-fps')
New-Item -ItemType Directory -Force $output | Out-Null
$project=Join-Path $repoRoot "$Api\07_Himalayas"
$encoding=@();if($Api -eq 'Vulkan'){$encoding+='/DDEMO_USE_SMOLV'}
& (Join-Path $vc 'bin\Hostx64\x86\cl.exe') @common @encoding /Gz ("/D$Define") ("/I$project") ("/I$project\build\Release\obj") "$PSScriptRoot\Benchmark.c" ("/Fo$output\Benchmark.obj") ("/Fe$output\Benchmark.exe") /link @link user32.lib d3d11.lib d3d12.lib dxgi.lib d3dcompiler.lib
if($LASTEXITCODE){throw 'Benchmark compile failed'}
Run-Probe "$output\Benchmark.exe" $output
