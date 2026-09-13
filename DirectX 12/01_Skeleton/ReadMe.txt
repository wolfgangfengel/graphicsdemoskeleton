Requirements:
- Visual Studio 2019 with MSVC v142 and a Windows 10/11 SDK
- Windows 10
- DirectX 12 capable GPU and drivers

Select Debug / Win32 and press F5 in GraphicsDemo.sln.
Run BuildSmall.bat for build\Release\Intro.exe.
See the root README.md for the shared build and verification commands.

Size and implementation:
- Crinkler 3.0b SLOW, 5,000 ordering attempts, 500 hash attempts: 1,119 bytes.
  Previously 1,332 bytes; saved 213 bytes (16.0%).
- Measured with VS 2019/MSVC 14.29.30133 and Windows SDK 10.0.22621.0.
- Both configurations use the custom naked x86 winmain, with no CRT imports.
  Release uses stdcall helpers and lets ExitProcess reclaim resources.
  Debug retains the D3D12 debug layer, WARP fallback, and explicit cleanup.
- Static descriptors, cached RTV handles, and one reused transition barrier
  reduce initialization and frame code. No shaders or PSO are needed to clear.
- A 32-bit counter is sufficient for the 3.3-second run. A synchronous fence
  wait completes each frame before its command allocator can be reused.
  SetEventOnCompletion with a null event avoids separate Win32 event APIs:
  https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion

The demo keeps its 800x600 blue window, hidden cursor, Escape polling, and
automatic exit after approximately 3.3 seconds. It queries DXGI for the current
back buffer instead of assuming that every Present advances the buffer index.

Validation for this optimization:
- Debug, Release, and packed builds completed their full timed runs with exit 0.
- Closing the demo window exited early with exit 0.
- 32 offscreen WARP frames: every pixel matched RGBA (0,51,102,255), every fence
  completed, and the D3D12 debug layer reported zero warnings or errors.
