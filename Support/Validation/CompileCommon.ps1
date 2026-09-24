$ErrorActionPreference='Stop'
$repoRoot=Split-Path (Split-Path $PSScriptRoot)
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vs){throw 'Install Visual Studio C++ build tools'}
$vc=(Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC') -Directory | Where-Object Name -Like '14.29.*' | Sort-Object Name -Descending | Select-Object -First 1).FullName
if(-not $vc){throw 'Install the VS2019 v142 toolset (14.29) for these reference probes'}
$sdk=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdkVersion=(Get-ChildItem (Join-Path $sdk 'Lib') -Directory | Where-Object {Test-Path (Join-Path $_.FullName 'um\x86\d3d12.lib')} | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1).Name
if(-not $sdkVersion){throw 'Install a Windows SDK'}
$common=@('/nologo','/TC','/O1','/MT','/Gy','/W4','/wd4201','/fp:fast','/arch:IA32','/GS-','/DUNICODE','/D_UNICODE','/DWIN32','/D_WINDOWS',('/I'+(Join-Path $vc 'include')),('/I'+$PSScriptRoot))
foreach ($part in @('ucrt','shared','um')) { $common+='/I'+(Join-Path $sdk "Include\$sdkVersion\$part") }
$link=@('/OPT:REF',('/LIBPATH:'+(Join-Path $vc 'lib\x86')),('/LIBPATH:'+(Join-Path $sdk "Lib\$sdkVersion\ucrt\x86")),('/LIBPATH:'+(Join-Path $sdk "Lib\$sdkVersion\um\x86")))
$link+=@(("/LIBPATH:$repoRoot\Tools\Vulkan\lib\x86"),"vulkan-1.lib")
function Run-Probe($exe,$directory) {
    $process=Start-Process -FilePath $exe -WorkingDirectory $directory -WindowStyle Hidden -RedirectStandardOutput (Join-Path $directory 'probe.log') -PassThru
    $handle=$process.Handle
    try {
        if(-not $process.WaitForExit(135000)) { throw "Timed out: $exe" }
        Get-Content -LiteralPath (Join-Path $directory 'probe.log')
        if($process.ExitCode) { throw "Failed: $exe ($($process.ExitCode))" }
    } finally { if(-not $process.HasExited) {Stop-Process -Id $process.Id -Force}; $process.Dispose() }
}

$probeRoot=Join-Path $repoRoot 'build\himalayas-validation'
New-Item -ItemType Directory -Force $probeRoot | Out-Null
# Prefer the caller's compatible x86 validation layer, then the old local cache.
$localLayer=Join-Path $repoRoot 'build\vulkan-tools\validation\Bin32'
if(-not $env:VK_LAYER_PATH -and (Test-Path $localLayer)){$env:VK_LAYER_PATH=$localLayer}
$env:VK_LAYER_ENABLES='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT'
