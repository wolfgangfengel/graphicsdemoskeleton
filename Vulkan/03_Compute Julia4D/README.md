# 03_Compute Julia4D - Vulkan

Reuses the DX12 Julia HLSL, including its ray marcher, lighting, self shadows and animated quaternion/color. The original 4x64 workgroup layout is retained.

Open GraphicsDemo.sln in Visual Studio 2019, select Debug or Release / Win32,
and press F5. BuildSmall.bat builds Release and compresses it with the shared
Crinkler 3.0b. All builds retain the custom entry point, no CRT, and embedded
SPIR-V shaders. See [shared build, controls and validation notes](../README.md).

Outputs: build/Debug/GraphicsDemo.exe, build/Release/GraphicsDemo.exe, and
build/Release/Intro.exe. The first shader build downloads the pinned build-time
DXC compiler; it is not required to run the demo.
