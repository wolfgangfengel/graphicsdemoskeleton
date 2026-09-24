# Build-time DXC only. The demos load the GPU driver's Vulkan runtime.
$ErrorActionPreference='Stop'
$target=Join-Path $PSScriptRoot 'build\dxc'
if(Test-Path -LiteralPath "$target\bin\x64\dxc.exe") { return }
New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
$archive=Join-Path $PSScriptRoot 'build\dxc.zip'
Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2607/dxc_2026_07_29.zip' -OutFile $archive
$sha=[Security.Cryptography.SHA256]::Create()
try { $hash=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($archive))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
if($hash -ne 'a1dfb116ba3eeae6a1582291b53a8e7bf65ad760676bd3194685c8f7367cd241') { throw 'DXC archive checksum mismatch' }
Expand-Archive -LiteralPath $archive -DestinationPath $target -Force
