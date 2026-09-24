param([string]$Api)
$ErrorActionPreference='Stop'
. "$PSScriptRoot\..\CompileCommon.ps1"
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class HimalayasWindowTest {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window,uint msg,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr window,int command);
}
'@
$env:VK_LAYER_ENABLES='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT'
$results=@()
foreach($mode in @('Debug','Release','Packed')) {
    $config=if($mode -eq 'Debug'){'Debug'}else{'Release'}
    $name=if($mode -eq 'Packed'){'Intro.exe'}else{'GraphicsDemo.exe'}
    $duration=if($mode -eq 'Packed'){35}else{3}
    $exe=Join-Path $repoRoot "$Api\07_Himalayas\build\$config\$name"
    $watch=[Diagnostics.Stopwatch]::StartNew()
    $p=Start-Process $exe -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
    $null=$p.Handle;$window=[IntPtr]::Zero;$sent=$false;$minimized=$false;$restored=$false
    try {
        while(-not $p.WaitForExit(50)) {
            $p.Refresh()
            if($window -eq [IntPtr]::Zero -and $p.MainWindowHandle -ne [IntPtr]::Zero) {
                $window=$p.MainWindowHandle
                [HimalayasWindowTest]::ShowWindow($window,0)|Out-Null
            }
            if($window -ne [IntPtr]::Zero) {
                if($mode -eq 'Packed' -and $watch.Elapsed.TotalSeconds -gt 4 -and -not $minimized) {
                    [HimalayasWindowTest]::ShowWindow($window,6)|Out-Null;$minimized=$true
                }
                if($minimized -and $watch.Elapsed.TotalSeconds -gt 5 -and -not $restored) {
                    [HimalayasWindowTest]::ShowWindow($window,9)|Out-Null
                    [HimalayasWindowTest]::ShowWindow($window,0)|Out-Null;$restored=$true
                }
                if(-not $sent -and $watch.Elapsed.TotalSeconds -gt $duration) {
                    $message=if($mode -eq 'Release'){0x10}else{0x100}
                    $key=if($mode -eq 'Release'){0}else{0x1b}
                    if(-not [HimalayasWindowTest]::PostMessage($window,$message,[IntPtr]$key,[IntPtr]::Zero)){throw 'Exit message failed'}
                    $sent=$true
                }
            }
            if($watch.Elapsed.TotalSeconds -gt $duration+20){throw 'Smoke timeout'}
        }
        if(-not $sent -or $p.ExitCode -ne 0){throw "Unexpected exit $($p.ExitCode)"}
        $results+=[pscustomobject]@{Api=$Api;Mode=$mode;Seconds=[Math]::Round($watch.Elapsed.TotalSeconds,2);ExitCode=$p.ExitCode;MinimizeRestore=$restored;Sha256=(Get-FileHash $exe).Hash}
    } finally {if(-not $p.HasExited){Stop-Process -Id $p.Id -Force};$p.Dispose()}
}
$results|ConvertTo-Json|Set-Content (Join-Path $probeRoot ($Api.Replace(' ','')+'-smoke.json'))
$results|Format-Table
