DirectX 12 - DirectCompute Julia4D
================================

Native DirectX 12 port of DirectX 11/02_DirectCompute Julia4D, based on
Jan Vlietnick's demo and Keenan Crane's quaternion Julia-set renderer,
ported by Wolfgang Engel. See qjulia4D.hlsl for the original attribution.

The 1280x720 demo morphs the quaternion and diffuse color, shades the
surface with the corrected eye/reflection vectors, and casts self-shadows.
It runs for 30 seconds. Escape or closing the window ends it early.

Build and run with Visual Studio 2019
-----------------------------------
Install Desktop development with C++, the v142 x86 tools, and a Windows 10
SDK. Open GraphicsDemo.sln, select Debug or Release and Win32, then press
F5 to build/run. Both configurations use the custom winmain entry point
without CRT startup. Keep the settings imported from Support/TinyDemo.props.
Debug enables the D3D12 debug layer when installed and can fall back to WARP.
Release requires a DirectX 12 hardware device at feature level 11_0 or later.

Run BuildSmall.bat to build Release and compress it with the shared
Tools/Crinkler3.0b distribution. Outputs:
  build/Debug/GraphicsDemo.exe   - debug executable and symbols
  build/Release/GraphicsDemo.exe - ordinary optimized executable
  build/Release/Intro.exe        - packed standalone executable

Or, from the repository root:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\Build.ps1 -Project "DirectX 12\03_DirectCompute Julia4D" -Configuration All -Compress

Packed builds use the shared default compression settings, including
16-bit float truncation. Crinkler 3.x output requires Windows 10+ and SSE4.2.
All shader bytecode is embedded: no external shaders or runtime compiler DLL.

Implementation
--------------
The HLSL matches the corrected compact DX11 shader, with an embedded root
signature 1.0 added. Its 10-iteration normal-estimation loop remains rolled.
The 128-byte constants occupy 32 root DWORDs at b0; a descriptor table binds
one RGBA8 UAV at u0. No constant-buffer upload allocation is needed.

Each frame dispatches 4x64 threads per group, including bounds checks for
the partial bottom row of groups. The UAV texture transitions to copy source,
is copied into DXGI's current back buffer, then returns to UAV state. The
two-buffer flip-discard swap chain uses explicit present/copy transitions.
A fence completes each frame before reusing the allocator and output texture.
Every frame recomputes the fractal. There is no cached frame or DX11 wrapper.

HRESULT failures terminate with the failing result. Debug releases completed
GPU resources explicitly; Release uses process teardown to keep the EXE small.

Measured build and verification
-------------------------------
VS2019 v142 (MSVC 14.29.30133), Windows SDK 10.0.22621.0, default Crinkler
3.0b settings: Debug 13,824 bytes, Release 11,264 bytes, Intro.exe 2,607 bytes.
The embedded Release compute shader, including its root signature, is 6,676
bytes before compression. Sizes can vary with compiler/SDK versions.

Debug, Release, and packed binaries each completed the full 30-second run
with exit code 0; a separate window-close run exited cleanly. The PE entry
points match winmain, and imports contain only d3d12, dxgi, kernel32, user32.

An instrumented Release run compared 11 GPU readbacks against the actual
DX11 shader at 1280x720: six fixed quaternion cases and five animation samples
through the first transition. All RGBA8 pixels matched exactly on the tested
hardware. Both swap-chain buffers and hide/show were exercised, with zero
D3D12 debug-layer warnings/errors. This pixel comparison covers the ordinary
Release renderer; the packed executable was separately smoke-tested.
