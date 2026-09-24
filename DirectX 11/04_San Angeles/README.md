# San Angeles Observation — DirectX 11

Open `GraphicsDemo.sln` in Visual Studio 2019, select **Debug / Win32**, and
press **F5**. Run `BuildSmall.bat` for `build/Release/Intro.exe`.
See the [repository build guide](../../README.md) for requirements and options.

This port retains the supplied demo's 21 procedural supershapes, 11-by-11 city,
ten moving ships, flat shading with three directional lights, mirrored floor,
and all 13 camera tracks and fades. It runs for 108,840 milliseconds. The
supplied OpenGL ES version has no music, so this port has none either.

`Window.c` creates the Direct3D 11 pipeline and owns the custom naked `winmain`.
The shared `San Angeles/Scene.h` at the repository root generates 21,522 vertices
and computes the camera from elapsed time.
`vertex.hlsl` implements lighting and perspective; `pixel.hlsl` writes the flat
vertex color. The shared build step embeds 892 bytes of compact HLSL and checks
that both stages still produce identical stripped bytecode. At startup,
`D3DCompile` compiles the embedded source once per stage using the Windows
`D3DCompiler_47.dll`. Playback timing begins after shader and scene creation.
No meshes, textures, shader files, Android, EGL, OpenGL, or C runtime are needed
at runtime.

The [DirectX 12 port](../../DirectX%2012/04_San%20Angeles/README.md) references this
scene code and the same shaders, using its own native DX12 renderer.

The port uses float vertices instead of GLES 16.16 fixed-point arrays.
Supershape powers retain double intermediates through compact x87 helpers.
Release uses stdcall helpers, omits the window title and individual resource
releases before `ExitProcess`, and retains full float constants in compression.
Debug keeps the title, Direct3D debug layer, and explicit resource teardown.
`winmain` remains explicitly cdecl in both configurations.

With VS 2019/MSVC 14.29, SDK 10.0.22621.0, and the default Crinkler 3.0b SLOW
settings, the packed EXE is **3,832 bytes**, down from 3,926 bytes with embedded
shader bytecode. Geometry, rendering operations, and precision are unchanged.

The runtime-compiled vertex and pixel shaders matched the previous stripped
bytecode exactly. Debug, Release, and the final packed EXE each completed the
full sequence and exited with code zero. Escape and window-close checks also
passed. Both configurations retain `winmain` as the PE entry point and no CRT
imports.

The [shared San Angeles directory](../../San%20Angeles/README.md) holds the
scene, shaders, original `demo.c`, camera and shape tables, and license notices.
Its `README.txt` is the original OpenGL ES description. The Android launcher is
not needed by the Visual Studio project.

Build both configurations and compress Release from the repository root:

```powershell
.\Build.ps1 -Project 'DirectX 11\04_San Angeles' -Configuration All -Compress
```

Run `build/Release/Intro.exe` from this demo directory to check the packed build.
Escape exits early; uninterrupted playback exits automatically after the last
camera track.

Derived from San Angeles Observation, copyright (c) 2004-2005 Jetro Lauha.
This adaptation uses the supplied BSD license option. Keep the shared `San Angeles/license-BSD.txt`
with redistributed binaries and retain source notices.
