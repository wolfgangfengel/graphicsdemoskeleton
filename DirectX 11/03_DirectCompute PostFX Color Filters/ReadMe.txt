Requirements:
- Visual Studio 2019 with MSVC v142 and a Windows 10/11 SDK
- Windows 10/11 (tested)
- DirectX 11 capable GPU
Select Debug / Win32 and press F5 in GraphicsDemo.sln.
Run BuildSmall.bat for build\Release\Intro.exe.
See the root README.md for the shared build and verification commands.

The shared Support/EmbeddedShaders.targets build step embeds compact HLSL from
qjulia4D.hlsl and ColorFilters.hlsl. Windows' D3DCompiler_47.dll compiles both
compute shaders once at startup. Temporary shader blobs are then released.
No external shader files are needed, and the 30-second playback clock starts
after initialization. The custom naked x86 winmain and CRT-free linking remain.

With VS2019/MSVC 14.29, SDK 10.0.22621.0 and default Crinkler 3.0b SLOW settings,
the packed EXE is 2,749 bytes, down from 2,994 bytes with embedded bytecode.
The ordinary Release EXE is 8,704 bytes, down from 20,480 bytes. The embedded
sources total 3,363 bytes before compression.

The build rejects compaction if stripped shader bytecode changes. Both
runtime-compiled stages also matched the previous bytecode exactly. Six GPU
readbacks through 29,900 ms matched the previous renderer pixel for pixel,
with no D3D11 validation warnings or errors. Debug, Release, and packed builds
completed their full timed runs and a separate window-close check passed.
The fractal, color filters, dispatch sizes, and animation are unchanged.
