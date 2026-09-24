# Shared San Angeles scene

This directory contains the API-independent source used by the native DX11,
DX12 and Vulkan ports:

- `Scene.h`: procedural geometry, x87 math helpers, and camera animation.
- `shapes.h` and `cams.h`: the original shape parameters and camera tracks.
- `vertex.hlsl` and `pixel.hlsl`: shared lighting, projection, and flat shading.
- `EmbedShaders.ps1`, `Shaders.targets`, and `Shaders.h`: source embedding,
  build integration, and startup shader compilation for both renderers.
- `demo.c`, `README.txt`, `include/GLES/`, and license files: the supplied
  OpenGL ES reference implementation and its notices.

The Windows renderers and their Visual Studio 2019 solutions remain in
[`DirectX 11/04_San Angeles`](../DirectX%2011/04_San%20Angeles/README.md) and
[`DirectX 12/04_San Angeles`](../DirectX%2012/04_San%20Angeles/README.md).
The third renderer lives in [`Vulkan/04_San Angeles`](../Vulkan/04_San%20Angeles/README.md)
and embeds SPIR-V compiled from these same shaders. It generates geometry into
mapped host-coherent vertex memory and retains the reflection/floor/model passes.
The DirectX projects embed compact HLSL source in their EXEs. The shared build step uses
FXC to preprocess the readable shaders, shorten declared names, and remove
unnecessary whitespace. It combines the two stages into `ShaderSource.h` under
the project's intermediate directory. Before writing that header, it compiles
both entry points and requires byte-for-byte equality with the original
shaders' stripped bytecode. The original HLSL files remain readable.

At startup, `Shaders.h` calls `D3DCompile` once per stage with optimization level
3, then the renderer creates its shader objects or pipeline states and releases
the temporary blobs. The DX12 project defines `SAN_ANGELES_DX12` during source
preprocessing, preserving its 24-DWORD root signature in the compiled vertex
shader. Playback timing starts after initialization. This adds startup shader
compilation and a dependency on the Windows `D3DCompiler_47.dll`; no external
shader files or scene assets are required. Both custom entry points remain CRT-free.

With VS2019/MSVC 14.29, SDK 10.0.22621.0, and default Crinkler 3.0b SLOW settings,
source embedding reduces DX11 from 3,926 to **3,832 bytes** and DX12 from 4,391 to
**4,383 bytes**. These small shaders give more modest savings than the terrain
demo. Geometry, lighting, reflections, camera tracks, and precision are unchanged.

The scene contains 21 procedural supershapes, 21,522 vertices including the
ground, an 11-by-11 city, ten moving ships, and all 13 camera tracks. Vector
helpers take pointers to avoid structure copies; quad corners share a loop.
The supershape calculations retain their double intermediates and full float
constants.

DX11 uses the scene's static vertex array. DX12 defines `SA_EXTERNAL_VERTICES`
and sets `saVertices` to a mapped upload buffer with capacity for
`SA_VERTEX_LIMIT` vertices before calling `saCreateScene`. Generation finishes
before the first GPU submission. Neither renderer changes the geometry during
playback.

Derived from San Angeles Observation, copyright (c) 2004-2005 Jetro Lauha.
The ports use the supplied BSD license option. Retain source notices and
distribute `license-BSD.txt` with either demo binary. The original Android
launcher is not required by these Windows projects.
Both projects copy the BSD notice into their build output directory automatically.
