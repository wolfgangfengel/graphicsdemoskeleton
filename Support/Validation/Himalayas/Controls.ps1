. "$PSScriptRoot\..\CompileCommon.ps1"
& (Join-Path $vc 'bin\Hostx64\x86\cl.exe') @common /Gz "$PSScriptRoot\Controls.c" "/Fo$probeRoot\Controls.obj" "/Fe$probeRoot\Controls.exe" /link @link user32.lib
if($LASTEXITCODE){throw 'Controls build failed'}
Run-Probe "$probeRoot\Controls.exe" $probeRoot
