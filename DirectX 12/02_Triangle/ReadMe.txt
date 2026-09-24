Requirements:
- Visual Studio 2019 with MSVC v142 and a Windows 10/11 SDK
- Windows 10
- DirectX 12 capable GPU and drivers

Select Debug / Win32 and press F5 in GraphicsDemo.sln.
Run BuildSmall.bat for build\Release\Intro.exe.
See the root README.md for the shared build and verification commands.

Size with VS 2019/MSVC 14.29.30133, SDK 10.0.22621.0, and Crinkler 3.0b:
- Default BuildSmall.bat: 1,832 bytes with embedded HLSL source.
- The previous world/view/projection triangle with embedded bytecode was
  1,794 bytes with the same settings. Source embedding adds 38 bytes here;
  these tiny shaders do not offset the compiler import and startup calls.

Support/EmbeddedShaders.targets generates the compact source automatically.
The EXE embeds 337 bytes of vertex HLSL and 47 bytes of pixel HLSL, compiled once
at startup through Windows' D3DCompiler_47.dll. No external shader files are
required. The playback clock starts after shader and pipeline initialization.

The vertex shader generates the triangle and its RGB corner colors from
SV_VertexID. This removes the upload buffer, input layout, buffer view, and
associated setup. D3DCompile embeds the root signature in the vertex shader.
Root-signature version 1.0 is retained through its Flags2 option. Both temporary
blobs are released after the root signature and pipeline state are created.

Window.c builds the conventional transforms on the CPU:
- World: rotate the model around its centroid on the Y axis, at two radians
  per second (one turn in approximately 3.14 seconds).
- View: a left-handed camera at (0, 0, -2), looking toward +Z with +Y up.
- Projection: perspective, 60-degree vertical FOV, 800/600 aspect ratio,
  near plane 0.1, and far plane 10; Direct3D depth is in [0, 1].

UpdateTransform calls MultiplyMatrices twice each frame:
  worldView = World * View
  worldViewProjection = worldView * Projection

The C matrices use row-major storage and row-vector multiplication. All 16
floats of worldViewProjection are uploaded as root constants. vertex.hlsl
declares row_major float4x4 and transforms each model-space vertex with
mul(position, worldViewProjection), so no transpose is required. The GPU then
performs the perspective divide and perspective-correct color interpolation.

The CPU uses x87 FSINCOS to build the world rotation without a CRT math import.
GetTickCount supplies elapsed time, making rotation independent of frame rate.
Back-face culling is disabled to show both faces during the full turn. The
triangle naturally becomes edge-on twice per turn. A single triangle needs no
depth buffer, so depth testing remains disabled.

Static pipeline/swap-chain descriptors and one reused transition barrier reduce
initialization code. The shaders omit the unused pixel-position input and use
a short matching color semantic to reduce their embedded bytecode.

Every frame clears and redraws the current FLIP_DISCARD swap-chain buffer.
Both Debug and Release wait on a fence after Present before resetting the
command allocator and list for the next frame. A null SetEventOnCompletion
event waits synchronously, avoiding separate Win32 event management. The final
frame's wait also makes Debug resource cleanup safe.

Both configurations retain the custom naked x86 winmain and have no CRT imports.
Release uses stdcall helpers and ExitProcess for resource reclamation. Debug
retains the D3D12 validation layer, WARP fallback, and explicit resource cleanup.
The 800x600 window, hidden cursor, Escape polling, and approximately 3.3-second
playback remain. DXGI supplies the current back-buffer index each frame.

Validation for this animation:
- The compact sources and runtime compiler produce exactly the previous
  stripped shader bytecode, including root signature 1.0. Six frames at 0,
  300, 650, 1,200, 2,500, and 3,250 ms matched pixel for pixel with no D3D12
  validation warnings or errors. Debug, Release, and packed source builds
  completed full playback, and the separate window-close check passed.
- The instrumented Release loop completed 377 frames across both buffers,
  including hide/show, with zero D3D12 warnings or errors.
- Nine GPU readbacks spanning 0 to 3,250 ms matched independently calculated
  projection and perspective-correct RGB interpolation, allowing two color
  levels of rounding and excluding the narrow rasterization boundary. Clip
  coordinates (including W and depth), background, projected triangle area,
  and centroid checks passed, covering both faces. A captured frame was also
  visually inspected.
- PE entry points resolve to winmain; imports contain no CRT or root serializer.
- Debug, Release, and the final packed EXE completed their full timed runs with
  exit code zero. Closing a separate test window also exited cleanly.

Microsoft references:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/specifying-root-signatures-in-hlsl
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics
https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
https://learn.microsoft.com/en-us/windows/win32/direct3d9/transforms
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math
https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-constants-directly-in-the-root-signature
