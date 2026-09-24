# graphicsdemoskeleton

Small, self-contained DirectX 11, DirectX 12, and Vulkan demos for demoscene experiments.
Every demo uses a custom naked Win32/x86 `winmain` entry point, embeds its shaders,
and links without the CRT. Crinkler replaces the linker for the packed build.
The DirectX demos embed compact HLSL source and compile it once at startup through
Windows' `D3DCompiler_47.dll`. Vulkan demos embed compiled SPIR-V, using lossless
SMOL-V packing where it reduces the complete EXE, and need no
runtime shader compiler. The Skeleton demos have no shaders to embed.

San Angeles is organized into one shared scene and three native renderers:

```text
San Angeles/                Scene, shaders, original source, and licenses
DirectX 11/04_San Angeles/   DX11 renderer, solution, and BuildSmall.bat
DirectX 12/04_San Angeles/   DX12 renderer, solution, and BuildSmall.bat
Vulkan/04_San Angeles/       Vulkan renderer, solution, and BuildSmall.bat
Tools/Crinkler3.0b/          Shared compressor
Tools/Vulkan/               Vulkan headers and pinned DXC setup script
Tools/smol-v/               Pinned shader encoder and round-trip verification
```

## macOS / iOS continuation

Read [HANDOVER_APPLE.md](HANDOVER_APPLE.md) for the source map, preserved Windows
artifacts, portable validation probes and the proposed Apple implementation.
Native macOS/iOS projects and runtime validation are still to be implemented.

## Visual Studio 2019

Install **Desktop development with C++**, **MSVC v142**, and a Windows 10/11 SDK
containing FXC. The verified toolchain is VS 2019 Professional, MSVC 14.29,
and Windows SDK 10.0.22621.0.

1. Open a demo's `GraphicsDemo.sln`, for example
   `DirectX 11/04_San Angeles/GraphicsDemo.sln` or
   `DirectX 12/04_San Angeles/GraphicsDemo.sln`.
2. Select **Debug / Win32** in the toolbar.
3. Press **Ctrl+Shift+B** to build, then **F5** to run with the debugger.
   Set a breakpoint in `Window.c` after the assembly prolog to inspect startup.
4. Select **Release / Win32**, then **Ctrl+F5** to run the optimized ordinary EXE.

Both configurations enter `winmain` directly. x64 is intentionally absent:
the entry point and tiny runtime helpers contain x86 assembly.
Debug Direct3D layers require Windows' **Graphics Tools** optional feature.
Release runs without those layers. DX11 demos need DirectX 11 hardware; DX12
demos need DirectX 12 hardware and drivers. Windows 10/11 is the tested environment.

If Visual Studio requests retargeting, choose an installed Windows SDK and keep
**Platform Toolset: Visual Studio 2019 (v142)**. Do not enable `/RTC`, `/GS`, or
default CRT libraries: these projects bypass CRT initialization.

## Build and compress

From PowerShell in the repository root:

```powershell
.\Build.ps1 -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 11\04_San Angeles' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 12\04_San Angeles' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 12\03_DirectCompute Julia4D' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 11\05_Terrain' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 12\05_Terrain' -Configuration All -Compress
.\Build.ps1 -Project 'Vulkan\05_Terrain' -Configuration All -Compress
```

The first command builds all DirectX and Vulkan demos; the others select an individual demo.
Each demo's `BuildSmall.bat` invokes the same Release/compression build and
works from a normal command prompt. The script discovers MSBuild and the SDK.
`BuildAll.bat` builds both configurations and packs every Release executable.
If PowerShell script execution is restricted, invoke it explicitly:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build.ps1 -Configuration All -Compress
```

| Output beneath each demo | Purpose |
| --- | --- |
| `build/Debug/GraphicsDemo.exe` | Custom-entry executable with debug symbols |
| `build/Release/GraphicsDemo.exe` | Optimized ordinary executable |
| `build/Release/Intro.exe` | Crinkler-packed release |
| `build/Release/Compression.log`, `report.html` | Compression results and size breakdown |
| `build/<configuration>/obj/` | Objects and generated shader headers |

The default is shared Crinkler 3.0b with `SLOW`, 5,000 ordering attempts and
500 hash-size attempts. Use `-CompressionMode INSTANT` for a quick packed-build
check; its size is not representative of final compression. `-Toolset` and
`-SdkVersion` allow explicit toolchain selection.

## Shared Crinkler 3.0b

The official distribution is included once under `Tools/Crinkler3.0b`, with
its manual and license. All example batch files use this copy through the
shared build script. Older per-example compressor copies have been removed.
See [tool provenance and requirements](Tools/README.md).

```powershell
.\Build.ps1 -Project 'DirectX 11\04_San Angeles' -Compress -CompressionMode VERYSLOW
```

The script chooses the Win64 or Win32 compressor host to match Windows;
both produce a **32-bit intro**. Crinkler 3.x and its output require SSE4.2;
the tool requires Windows 10 or later. Measure the final file: `VERYSLOW`
and more optimization attempts do not necessarily improve every input.
`-CrinklerPath` permits an explicit override for future experiments.

San Angeles, terrain, and all Vulkan demos set `CrinklerFloatBits=32` to retain
their procedural constants. Older DirectX demos retain their original 16-bit
float truncation. `-FloatBits` overrides this explicitly.

## Behavior and verification

The skeletons and triangle run about 3.3 seconds, Julia/terrain/compute demos about
30 seconds, and San Angeles about 109 seconds. Escape or closing the window
ends a run early. San Angeles and terrain handle Escape only from their own window.

After building, run each configuration's `GraphicsDemo.exe` and the packed
`build/Release/Intro.exe`. In Debug, inspect Visual Studio's Output window for
Direct3D validation messages. Test Escape and allow a separate run to reach its
automatic exit. The generated `GraphicsDemo.map` identifies `winmain`; compare
its address with the PE entry point when checking custom-entry builds.

Shared settings live in `Support/TinyDemo.props`. The small `memcpy`, `memset`,
and `_fltused` definitions in `Support/TinyRuntime.h` satisfy compiler-generated
references without CRT startup or a runtime DLL. Unused helpers are discarded.
The DX12 C examples retain the explicit ABI workaround for descriptor-handle
struct returns from the SDK's C COM interfaces.

`DirectX 12/03_DirectCompute Julia4D` ports the corrected DX11 Julia demo to
native DX12 compute, with embedded shaders/root signature, root constants,
an output UAV, and a copy into a two-buffer flip-discard swap chain. It retains
the 1280x720 fractal, animated quaternion/color, lighting, and self-shadows.
Open its `GraphicsDemo.sln` in VS2019 or run its `BuildSmall.bat`.

`DirectX 11/05_Terrain` ports David Hoskins's
[Mountains](https://www.shadertoy.com/view/4slGD4) to HLSL, including its terrain,
forests, water, clouds, and horizontal camera path. The camera holds a steady
world-space altitude for each route. Hold the left mouse button and drag
horizontally to shift the camera along its route. It compiles embedded HLSL
once at startup using `D3DCompiler_47.dll`; a procedural mask replaces the
water-glint image texture. The build verifies that source compaction preserves
the compiled shader bytecode.
The adapted shader is **CC BY-NC-SA 3.0**: see its
[attribution and license](DirectX%2011/05_Terrain/LICENSE-Mountains.txt), which
must accompany the demo executables. The supplied `Algorithm.txt` is preserved
as reference notes; the active shader is `Terrain.hlsl`.

[`DirectX 12/05_Terrain`](DirectX%2012/05_Terrain/ReadMe.txt) uses the same
unchanged terrain shader through a DX12 wrapper, with root constants, a compute
UAV, and a copy to the current swapchain buffer. Its default SLOW packed EXE is
**3,721 bytes**. All camera controls, materials and rendering detail are retained.

## Himalayas

[`Himalayas/README.md`](Himalayas/README.md) documents the shared HLSL and
`07_Himalayas` projects for DirectX 11, DirectX 12 and Vulkan. These port
Reinder Nijhoff's ShaderToy MdGfzh with mountains, volumetric clouds, temporal
reprojection, moving clouds and a free-fly camera. All three target 120 FPS.
Packed sizes are 6,660 / 7,404 / 16,308 bytes respectively, with all shader
detail retained. Use WASD, Q/E, right-button mouse look, Shift to boost, and
R to reset the view; Escape exits.
The DirectX versions share one embedded shader source bundle. These Himalayas
builds remain above 4 KiB; see the shared README for measured compression modes
and rendering/performance validation.
The original source and adapted shaders use CC BY-NC-SA 4.0; preserve the
included attribution and license alongside the executables.

## Vulkan demos

[`Vulkan/README.md`](Vulkan/README.md) covers seven native ports: Skeleton,
rotating Triangle, Julia4D, San Angeles, Terrain, PostFX Color Filters and Himalayas.
Each has a VS2019 solution and `BuildSmall.bat`; use Debug/Release and Win32.
They reuse the existing readable HLSL and San Angeles scene data. Shared native
Vulkan code handles buffers, images, pipelines, synchronization and presentation.

The first shader build downloads a pinned DXC compiler into `Tools/Vulkan/build`;
later builds work offline. See [tool setup](Tools/Vulkan/README.md).
Run-time requirements are Windows and a GPU driver with 32-bit Vulkan support.
No shader compiler DLL, shader files or scene assets accompany the EXE.
Retain the terrain and San Angeles license files copied into their build outputs.
Debug enables a compatible Khronos validation layer when available.

## Embedded shader source

The Julia4D demos, PostFX, and triangle use `Support/EmbeddedShaders.targets`,
`Support/EmbedShader.ps1`, and `Support/TinyShader.h`. Source generation is part
of both Visual Studio and batch builds. The script preprocesses the readable
HLSL, shortens identifiers, and removes unnecessary whitespace. Each generated
shader must compile to exactly the same stripped bytecode as the original;
otherwise the build fails. Runtime compilation uses the same optimization
level and preserves the DX12 triangle/Julia root-signature 1.0 requirement.
Shader blobs are released after creating the shader objects and pipeline states.
The playback clock starts after initialization, so compilation does not shorten
the animation. Shader files are not needed alongside the EXE.

For these four conversions, using VS2019/MSVC 14.29, SDK 10.0.22621.0 and the
same default Crinkler 3.0b SLOW settings before and after:

| Demo | Embedded bytecode | Embedded source |
| --- | ---: | ---: |
| DX11 Julia4D | 2,095 bytes | 2,067 bytes |
| DX11 PostFX Color Filters | 2,994 bytes | 2,749 bytes |
| DX12 Triangle | 1,794 bytes | 1,832 bytes |
| DX12 Julia4D | 2,607 bytes | 2,596 bytes |

The triangle grows by 38 bytes: its tiny original shaders do not save enough
to offset the compiler import and startup calls. Its world/view/projection
rotation and rendering quality are unchanged. All six runtime-compiled shader
stages matched the previous stripped bytecode, and 24 GPU readbacks (six per
demo) matched pixel for pixel with no Direct3D validation warnings or errors.
Debug, Release, and packed builds all completed their timed runs successfully.

## References and attribution

San Angeles is adapted from **San Angeles Observation**, copyright 2004-2005
Jetro Lauha. Its supplied source, camera/shape data, and license notices are
in the root `San Angeles` directory. Both ports share the scene and HLSL source;
each project embeds compact HLSL and compiles it at startup through the Windows
`D3DCompiler_47.dll`, using its native Direct3D API. The build verifies identical
stripped shader bytecode before embedding the source.
Distribute the included `license-BSD.txt` with either binary.

Original project references:

- [Tiny C Runtime Library](https://www.codeproject.com/Articles/15156/Tiny-C-Runtime-Library)
- [Antoine de Saint-Exupery](https://en.wikiquote.org/wiki/Antoine_de_Saint_Exup%C3%A9ry):
  Il semble que la perfection soit atteinte non quand il n'y a plus rien Ã 
  ajouter, mais quand il n'y a plus rien Ã  retrancher.
