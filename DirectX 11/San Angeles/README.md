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
vertex color. FXC embeds both shaders at build time. No meshes, textures,
shader files, Android, EGL, OpenGL, or C runtime are needed at runtime.

The [DirectX 12 port](../../DirectX%2012/San%20Angeles/README.md) references this
scene code and the same shaders, using its own native DX12 renderer.

The port uses float vertices instead of GLES 16.16 fixed-point arrays.
Supershape powers retain double intermediates through compact x87 helpers.
Release uses stdcall helpers, omits the window title and individual resource
releases before `ExitProcess`, and retains full float constants in compression.
Debug keeps the title, Direct3D debug layer, and explicit resource teardown.
`winmain` remains explicitly cdecl in both configurations.

With VS 2019/MSVC 14.29, SDK 10.0.22621.0, and the default Crinkler 3.0b SLOW
settings, the packed EXE is **3,926 bytes**, down from 4,024 bytes before the
shared geometry helper optimizations.

The [shared San Angeles directory](../../San%20Angeles/README.md) holds the
scene, shaders, original `demo.c`, camera and shape tables, and license notices.
Its `README.txt` is the original OpenGL ES description. The Android launcher is
not needed by the Visual Studio project.

Build both configurations and compress Release from the repository root:

```powershell
.\Build.ps1 -Project 'DirectX 11\San Angeles' -Configuration All -Compress
```

Run `build/Release/Intro.exe` from this demo directory to check the packed build.
Escape exits early; uninterrupted playback exits automatically after the last
camera track.

Derived from San Angeles Observation, copyright (c) 2004-2005 Jetro Lauha.
This adaptation uses the supplied BSD license option. Keep the shared `San Angeles/license-BSD.txt`
with redistributed binaries and retain source notices.
