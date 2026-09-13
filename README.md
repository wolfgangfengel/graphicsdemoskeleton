# graphicsdemoskeleton

Small, self-contained DirectX 11 and DirectX 12 demos for demoscene experiments.
Every demo uses a custom naked Win32/x86 `winmain` entry point, embeds its shaders,
and links without the CRT. Crinkler replaces the linker for the packed build.

San Angeles is organized into one shared scene and two native renderers:

```text
San Angeles/              Scene, shaders, original source, and licenses
DirectX 11/San Angeles/   DX11 renderer, solution, and BuildSmall.bat
DirectX 12/San Angeles/   DX12 renderer, solution, and BuildSmall.bat
Tools/Crinkler3.0b/       Shared compressor
```

## Visual Studio 2019

Install **Desktop development with C++**, **MSVC v142**, and a Windows 10/11 SDK
containing FXC. The verified toolchain is VS 2019 Professional, MSVC 14.29,
and Windows SDK 10.0.22621.0.

1. Open a demo's `GraphicsDemo.sln`, for example
   `DirectX 11/San Angeles/GraphicsDemo.sln` or
   `DirectX 12/San Angeles/GraphicsDemo.sln`.
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
.\Build.ps1 -Project 'DirectX 11\San Angeles' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 12\San Angeles' -Configuration All -Compress
.\Build.ps1 -Project 'DirectX 12\03_DirectCompute Julia4D' -Configuration All -Compress
```

The first command builds all demos; the others select an individual demo.
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
.\Build.ps1 -Project 'DirectX 11\San Angeles' -Compress -CompressionMode VERYSLOW
```

The script chooses the Win64 or Win32 compressor host to match Windows;
both produce a **32-bit intro**. Crinkler 3.x and its output require SSE4.2;
the tool requires Windows 10 or later. Measure the final file: `VERYSLOW`
and more optimization attempts do not necessarily improve every input.
`-CrinklerPath` permits an explicit override for future experiments.

San Angeles sets `CrinklerFloatBits=32` to retain constants used by its
sensitive supershape calculations. Older demos retain their original 16-bit
float truncation. `-FloatBits` overrides this explicitly.

## Behavior and verification

The skeletons and triangle run about 3.3 seconds, Julia/compute demos about
30 seconds, and San Angeles about 109 seconds. Escape or closing the window
ends a run early. San Angeles handles Escape only from its own window.

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

## References and attribution

San Angeles is adapted from **San Angeles Observation**, copyright 2004-2005
Jetro Lauha. Its supplied source, camera/shape data, and license notices are
in the root `San Angeles` directory. Both ports share the scene and HLSL source;
each project embeds its own shaders and uses its native Direct3D API.
Distribute the included `license-BSD.txt` with either binary.

Original project references:

- [Tiny C Runtime Library](https://www.codeproject.com/Articles/15156/Tiny-C-Runtime-Library)
- [Antoine de Saint-Exupery](https://en.wikiquote.org/wiki/Antoine_de_Saint_Exup%C3%A9ry):
  Il semble que la perfection soit atteinte non quand il n'y a plus rien à
  ajouter, mais quand il n'y a plus rien à retrancher.
