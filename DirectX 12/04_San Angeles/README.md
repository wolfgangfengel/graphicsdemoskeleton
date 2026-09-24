# San Angeles Observation — DirectX 12

Open `GraphicsDemo.sln` in Visual Studio 2019, select **Debug / Win32**, and
press **F5**. Run `BuildSmall.bat` to build and compress the Release executable
with the shared `Tools/Crinkler3.0b` distribution. The packed output is
`build/Release/Intro.exe`; no developer command prompt is required.
See the [repository build guide](../../README.md) for prerequisites and options.

This is the same San Angeles scene as the DirectX 11 port: 21 procedural
supershapes, an 11-by-11 city, ten moving ships, flat lighting, floor reflections,
and all 13 camera tracks with fades. Playback lasts 108,840 milliseconds.
Press Escape while the demo has focus to exit early.

`Window.c` implements a native Direct3D 12 renderer. The project references
`Scene.h`, `shapes.h`, `cams.h`, and both HLSL files in the root `San Angeles` directory
so geometry, camera, and shading changes stay consistent between the ports.
The shared build step embeds 1,034 bytes of compact HLSL, including the DX12 root
signature declaration. At startup, `D3DCompile` compiles both stages once using
the Windows `D3DCompiler_47.dll`. The root signature and both pipeline states
are created before the temporary shader blobs are released. The executable has
no external assets or DirectX 11 runtime dependency. Playback timing begins
after initialization.

The renderer draws the mirrored city, multiply-blended ground, and normal city
using two pipeline states and a depth buffer. A 24-DWORD root-constant block
holds each draw's camera, transform, and lighting data. A fence permits safe
reuse of the command allocator after each frame; `SetEventOnCompletion` with a
null event waits directly, avoiding separate Win32 event setup. Geometry is
generated directly into one persistently mapped upload vertex buffer, removing
the separate CPU vertex array and copy. All CPU writes finish before the first
GPU submission. One frame in flight keeps synchronization code small.

Both configurations use the custom naked x86 `winmain` and link without the
C runtime. Release uses stdcall helpers, embedded shaders, and process teardown
through `ExitProcess`. Debug enables the D3D12 validation layer when installed
and explicitly releases resources. Compression retains full float constants
for the supershape calculations. The shared geometry generator uses pointer
parameters instead of passing vectors by value, loops over quad corners, and
uses x87 `fabs`. The message loop handles Escape directly without
`TranslateMessage`.

With VS 2019/MSVC 14.29, SDK 10.0.22621.0, and the default Crinkler 3.0b SLOW
settings, the packed EXE is **4,383 bytes**, down from 4,391 bytes with embedded
shader bytecode. The DX11 port falls from 3,926 to 3,832 bytes with the same
approach. DX12's saving is only eight bytes because startup compilation and the
root-signature declaration largely offset the smaller shader representation.
It remains 287 bytes above 4 KiB. Geometry, rendering operations, and precision
are unchanged.

To reproduce the default result from this directory:

```bat
BuildSmall.bat
```

To explore a longer compression search (not guaranteed to produce a smaller EXE):

```bat
BuildSmall.bat -CompressionMode SLOW -OrderTries 100000 -HashTries 1000
```

From the repository root:

```powershell
.\Build.ps1 -Project 'DirectX 12\04_San Angeles' -Configuration All -Compress
```

The build rejects source compaction if either stage's stripped bytecode differs
from the readable shader. Runtime compilation with the installed Windows DLL
also matched the previous embedded bytecode exactly, including the DX12 root
signature. Seven offscreen DX12 WARP frames at 0, 1,024, 3,000, 40,000, 60,000,
100,000, and 108,839 milliseconds matched the previous build pixel for pixel,
with no D3D12 validation errors. Both ports built in Debug and Release with
`winmain` as the PE entry point and no CRT imports.
Debug, Release, and the final packed EXE each completed the full sequence and
exited with code zero. Escape and window-close checks also passed.

Derived from San Angeles Observation, copyright (c) 2004-2005 Jetro Lauha.
This adaptation uses the supplied BSD license option. Keep the shared `San Angeles/license-BSD.txt`
with redistributed binaries and retain source notices. The supplied original
source and additional notices remain in the root `San Angeles` directory.

The synchronous fence wait follows Microsoft's documented
[`SetEventOnCompletion` null-event behavior](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion).
The upload buffer follows the documented
[`Map` persistent-mapping rules](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map).
