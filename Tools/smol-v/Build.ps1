$ErrorActionPreference='Stop'
$exe=Join-Path $PSScriptRoot 'build\Encode.exe'
$inputs=@('Encode.cpp','smolv.cpp','smolv.h','Encoder.vcxproj','..\..\Vulkan\Common\Smolv.h','..\..\Vulkan\Common\SmolvOps.h') | ForEach-Object {Get-Item -LiteralPath (Join-Path $PSScriptRoot $_)}
if((Test-Path -LiteralPath $exe) -and (Get-Item -LiteralPath $exe).LastWriteTimeUtc -gt ($inputs | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc) { return }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs) {throw 'Visual Studio C++ build tools are required for the SMOL-V encoder'}
& "$vs\MSBuild\Current\Bin\MSBuild.exe" "$PSScriptRoot\Encoder.vcxproj" /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if($LASTEXITCODE) {throw 'SMOL-V encoder build failed'}
