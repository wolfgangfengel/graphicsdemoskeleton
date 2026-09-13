[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'All')][string]$Configuration = 'Release',
    [string]$Project,
    [switch]$Compress,
    [ValidateSet('INSTANT', 'FAST', 'SLOW', 'VERYSLOW')][string]$CompressionMode = 'SLOW',
    [string]$CrinklerPath,
    [ValidateRange(1, 64)][int]$FloatBits,
    [ValidateRange(0, 100000)][int]$OrderTries = 5000,
    [ValidateRange(1, 100000)][int]$HashTries = 500,
    [string]$Toolset = 'v142',
    [string]$SdkVersion
)
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ build tools and a Windows 10/11 SDK.' }
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { throw 'No Visual Studio installation with the C++ build tools was found.' }
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not $SdkVersion) {
    $SdkVersion = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Lib') -Directory |
        Where-Object { (Test-Path -LiteralPath (Join-Path $_.FullName 'um\x86\d3d12.lib')) -and
            (Test-Path -LiteralPath (Join-Path $sdkRoot ('bin\' + $_.Name + '\x86\fxc.exe'))) } |
        Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
}
if (-not $SdkVersion) { throw 'No complete Windows SDK with FXC and x86 DirectX libraries was found.' }
$hostArchitecture = if ([Environment]::Is64BitOperatingSystem) { 'Win64' } else { 'Win32' }
$crinkler = if ($CrinklerPath) { (Resolve-Path -LiteralPath $CrinklerPath).Path } else { Join-Path $repoRoot "Tools\Crinkler3.0b\$hostArchitecture\Crinkler.exe" }
if ($Compress -and -not (Test-Path -LiteralPath $crinkler -PathType Leaf)) { throw "Crinkler 3.0b was not found: $crinkler" }

function Invoke-BuildTool([string]$File, [string[]]$Arguments, [string]$WorkingDirectory, [string]$Log) {
    # Windows PowerShell/.NET Framework cannot initialize ProcessStartInfo's
    # environment dictionary when cmd.exe inherits both Path and PATH.
    # Normalize duplicates in this build process before accessing that property.
    $entries = @([Environment]::GetEnvironmentVariables().GetEnumerator())
    $environmentCopy = @{}
    foreach ($entry in $entries) {
        if (-not [string]::IsNullOrEmpty($entry.Key)) { $environmentCopy[$entry.Key.ToUpperInvariant()] = $entry.Value }
    }
    $duplicates = $entries | Group-Object { $_.Key.ToUpperInvariant() } | Where-Object Count -gt 1
    foreach ($duplicate in $duplicates) {
        foreach ($entry in $duplicate.Group) { [Environment]::SetEnvironmentVariable($entry.Key, $null) }
        [Environment]::SetEnvironmentVariable($duplicate.Name, $environmentCopy[$duplicate.Name])
    }
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = $File
    # Arguments here are paths or switches, never shell commands. No shell is invoked.
    $start.Arguments = ($Arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }) -join ' '
    $start.WorkingDirectory = $WorkingDirectory
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.EnvironmentVariables.Clear()
    foreach ($key in $environmentCopy.Keys) { $start.EnvironmentVariables[$key] = $environmentCopy[$key] }
    $process = [System.Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText($Log, $output)
    $exitCode = $process.ExitCode
    $process.Dispose()
    if ($exitCode -ne 0) { throw "Tool exited with code $exitCode. See $Log`n$output" }
}

if ($Project) {
    $projectPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Project)
    if (Test-Path -LiteralPath $projectPath -PathType Container) { $projectPath = Join-Path $projectPath 'GraphicsDemo.vcxproj' }
    $projects = @(Get-Item -LiteralPath $projectPath)
} else {
    $projects = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'DirectX 11'), (Join-Path $repoRoot 'DirectX 12') -Recurse -Filter GraphicsDemo.vcxproj | Where-Object { $_.FullName -notmatch '(?i)[\\/]build[\\/]|[\\/]\.vs[\\/]' } | Sort-Object FullName)
}
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$results = @()
$failures = 0
foreach ($demo in $projects) {
    [xml]$demoXml = Get-Content -LiteralPath $demo.FullName
    $floatProperty = $demoXml.SelectSingleNode('//*[local-name()="CrinklerFloatBits"]')
    $packedFloatBits = if ($FloatBits) { $FloatBits } elseif ($floatProperty) { [int]$floatProperty.InnerText } else { 16 }
    foreach ($config in $configurations) {
        $label = $demo.DirectoryName.Substring($repoRoot.Length + 1)
        $outDir = Join-Path $demo.DirectoryName "build\$config"
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
        $record = [ordered]@{ Demo=$label; Configuration=$config; Toolset=$Toolset; SDK=$SdkVersion; Built=$false; ExeBytes=$null; PackedBytes=$null; Crinkler=$crinkler; CompressionMode=$CompressionMode; FloatBits=$packedFloatBits; Error=$null }
        try {
            Write-Host "Building $label ($config, Win32, $Toolset, SDK $SdkVersion)"
            Invoke-BuildTool $msbuild @($demo.FullName, '/t:Rebuild', "/p:Configuration=$config", '/p:Platform=Win32', "/p:PlatformToolset=$Toolset", "/p:WindowsTargetPlatformVersion=$SdkVersion", '/nologo', '/v:minimal') $demo.DirectoryName (Join-Path $outDir 'build.log')
            $record.Built = $true
            $record.ExeBytes = (Get-Item -LiteralPath (Join-Path $outDir 'GraphicsDemo.exe')).Length
            if ($Compress -and $config -eq 'Release') {
                $libraries = if ($label.StartsWith('DirectX 12')) { @('d3d12.lib', 'dxgi.lib') } else { @('d3d11.lib') }
                $packArguments = @('/OUT:Intro.exe', '/SUBSYSTEM:WINDOWS', '/ENTRY:winmain', '/NODEFAULTLIB', "/COMPMODE:$CompressionMode", "/ORDERTRIES:$OrderTries", "/HASHTRIES:$HashTries", '/HASHSIZE:500', "/TRUNCATEFLOATS:$packedFloatBits", '/PRINT:IMPORTS', '/REPORT:report.html', ('/LIBPATH:' + (Join-Path $sdkRoot "Lib\$SdkVersion\um\x86")), 'kernel32.lib', 'user32.lib') + $libraries + @('obj\Window.obj')
                Invoke-BuildTool $crinkler $packArguments $outDir (Join-Path $outDir 'Compression.log')
                $record.PackedBytes = (Get-Item -LiteralPath (Join-Path $outDir 'Intro.exe')).Length
                Write-Host "  Packed: $($record.PackedBytes) bytes"
            }
        } catch {
            $record.Error = $_.Exception.Message + "`n" + $_.ScriptStackTrace
            $failures++
            Write-Warning $record.Error
        }
        $results += [pscustomobject]$record
    }
}
$resultDirectory = Join-Path $repoRoot 'build'
New-Item -ItemType Directory -Force -Path $resultDirectory | Out-Null
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $resultDirectory 'build-results.json')
$results | Format-Table Demo, Configuration, Built, ExeBytes, PackedBytes -AutoSize
if ($failures) { throw "$failures build or compression operation(s) failed." }
