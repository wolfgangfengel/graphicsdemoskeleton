# 05_Terrain - Vulkan

Reuses the unchanged DX11 Mountains HLSL, including all terrain, materials, water, clouds and camera settings. Left-button dragging shifts the route. Preserve LICENSE-Mountains.txt beside the executable (CC BY-NC-SA 3.0).

Open GraphicsDemo.sln in Visual Studio 2019, select Debug or Release / Win32,
and press F5. BuildSmall.bat builds Release and compresses it with the shared
Crinkler 3.0b. All builds retain the custom entry point, no CRT, and embedded
SPIR-V shaders. See [shared build, controls and validation notes](../README.md).

Outputs: build/Debug/GraphicsDemo.exe, build/Release/GraphicsDemo.exe, and
build/Release/Intro.exe. The first shader build downloads the pinned build-time
DXC compiler; it is not required to run the demo.
