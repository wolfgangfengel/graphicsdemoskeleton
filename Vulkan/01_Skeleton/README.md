# 01_Skeleton - Vulkan

Clears the swapchain to the same blue background as the DirectX skeleton. No shaders.

Open GraphicsDemo.sln in Visual Studio 2019, select Debug or Release / Win32,
and press F5. BuildSmall.bat builds Release and compresses it with the shared
Crinkler 3.0b. All builds retain the custom entry point and no CRT.
See [shared build, controls and validation notes](../README.md).

Outputs: build/Debug/GraphicsDemo.exe, build/Release/GraphicsDemo.exe, and
build/Release/Intro.exe. This example has no shaders and does not need DXC.
