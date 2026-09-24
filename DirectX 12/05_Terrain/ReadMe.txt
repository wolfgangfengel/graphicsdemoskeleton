DirectX 12 - Mountains
======================

Native DX12 version of the DX11 Mountains port of David Hoskins's shader.
See LICENSE-Mountains.txt. The original shader is CC BY-NC-SA 3.0; retain
attribution and distribute the license with the executable.

Open GraphicsDemo.sln in Visual Studio 2019, select Debug or Release / Win32,
and press F5. Requires the v142 C++ tools, a Windows SDK containing FXC,
and a DX12 GPU/driver. Debug validation uses Windows Graphics Tools.
Both configurations use the custom naked winmain entry point with no CRT.
BuildSmall.bat builds and compresses Release using Tools/Crinkler3.0b.

Outputs:
  build/Debug/GraphicsDemo.exe
  build/Release/GraphicsDemo.exe
  build/Release/Intro.exe
The default Crinkler SLOW build is 3,721 bytes, retaining full float constants.

The demo renders at 1280x720 for 30 seconds. Close the window or press Escape
while it has focus to exit. Hold the left mouse button and drag horizontally
to shift the route; the selected offset persists after releasing the button.
The camera keeps the same lower, faster, steady altitude flight as DX11.

Terrain.hlsl adds a DX12 root signature to the shared, unchanged shader in
../../DirectX 11/05_Terrain/Terrain.hlsl. Compact HLSL is embedded and compiled
once by the system D3DCompiler_47.dll. No shader or asset files are loaded.
The wrapper's decorated prototype retains both numthreads and RootSignature;
the generated module contains four root constants and one UAV descriptor.
The output UAV is copied into the current swapchain image after each dispatch.
A fence protects command allocator and output reuse.

Verification: Debug, Release and packed versions completed playback; close
and Escape exited cleanly. Twenty captured frames cover four times and five
mouse offsets. D3D validation reported zero errors/warnings. Stripping the
DX12 root signature and debug/reflection metadata produces exactly the DX11
shader bytecode. GPU pixel output has small backend rounding differences;
the worst mean channel difference across those frames was 0.00295/255.
No terrain, material, lighting, ray-march, or camera simplifications were made.
