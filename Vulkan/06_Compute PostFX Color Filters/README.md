# 06_Compute PostFX Color Filters - Vulkan

Reuses both original PostFX HLSL stages. Julia writes a full-precision float4 storage buffer; the second compute pass applies contrast, saturation, color correction and addition. A storage-buffer memory barrier orders the two passes.

Open GraphicsDemo.sln in Visual Studio 2019, select Debug or Release / Win32,
and press F5. BuildSmall.bat builds Release and compresses it with the shared
Crinkler 3.0b. All builds retain the custom entry point, no CRT, and embedded
SPIR-V shaders. See [shared build, controls and validation notes](../README.md).

Outputs: build/Debug/GraphicsDemo.exe, build/Release/GraphicsDemo.exe, and
build/Release/Intro.exe. The first shader build downloads the pinned build-time
DXC compiler; it is not required to run the demo.
