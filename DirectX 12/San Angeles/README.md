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
FXC embeds the shaders and DX12 root signature during the build; the executable has no external assets
or DirectX 11 runtime dependency.

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
settings, the packed EXE is **4,391 bytes**, down from 4,568 bytes before this
optimization pass. The same shared scene changes reduced DX11 from 4,024 to
3,926 bytes. DX12 still has additional pipeline, descriptor, command-list, and
synchronization setup compared with DX11.

An extended SLOW search reached **4,390 bytes**, versus the previous best of
4,565 bytes. This is 175 bytes (3.8%) smaller, and 294 bytes above 4 KiB.
VERYSLOW with the same extended search budget produced 4,397 bytes, so a
slower compression mode is not automatically better.

To reproduce the default result from this directory:

```bat
BuildSmall.bat
```

To reproduce the smallest measured build:

```bat
BuildSmall.bat -CompressionMode SLOW -OrderTries 100000 -HashTries 1000
```

From the repository root:

```powershell
.\Build.ps1 -Project 'DirectX 12\San Angeles' -Configuration All -Compress
```

The size changes were checked against a snapshot taken before optimization:
all 21,522 vertices, 22 mesh ranges, and 91 camera samples matched byte for byte.
Seven offscreen DX12 WARP frames at 0, 1,024, 3,000, 40,000, 60,000, 100,000, and
108,839 milliseconds also matched exactly, with no D3D12 validation errors.
Both ports built in Debug and Release with `winmain` as the PE entry point and
no CRT imports. DX12 Debug and Release passed three-second window checks; the
final packed EXE passed a 15-second check. Each exited with code zero after
Escape. These short checks do not verify an uninterrupted 109-second run.

Derived from San Angeles Observation, copyright (c) 2004-2005 Jetro Lauha.
This adaptation uses the supplied BSD license option. Keep the shared `San Angeles/license-BSD.txt`
with redistributed binaries and retain source notices. The supplied original
source and additional notices remain in the root `San Angeles` directory.

The synchronous fence wait follows Microsoft's documented
[`SetEventOnCompletion` null-event behavior](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion).
The upload buffer follows the documented
[`Map` persistent-mapping rules](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map).
