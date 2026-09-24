DirectX 11 - Mountains
======================

HLSL port of Mountains by David Hoskins (2013):
https://www.shadertoy.com/view/4slGD4

Terrain.hlsl retains the source's terrain formula, horizontal camera path, forests,
rock/grass/snow materials, animated water, clouds, lighting, fog, and post
curve. This is a port of that shader. The original author licenses it under
CC BY-NC-SA 3.0: attribution, noncommercial use, and share-alike are required.
See LICENSE-Mountains.txt and distribute that file with either executable.

The one external pebble texture used for small water glints is replaced by
procedural noise. The source's audio channel is not used by its graphics
code and is not included. This standalone version uses the default mono
view. All visuals are generated at runtime, with no external assets. Compact
HLSL source is embedded in the executable and compiled once at startup by
the system D3DCompiler_47.dll. No shader files are loaded at runtime.
Algorithm.txt is preserved as supplied notes and
is not used by this shader.

Build and run
-------------
Open GraphicsDemo.sln in Visual Studio 2019 with Desktop development with
C++, v142 x86 tools, and a Windows 10 SDK installed. Select Debug or Release,
Win32, and press F5. Debug requires the Direct3D debug layer (Graphics Tools).
Both configurations use the custom winmain entry point and no CRT startup.
Requires Direct3D 11 hardware at feature level 11_0 or later and
D3DCompiler_47.dll. See the D3DCompile API documentation:
https://learn.microsoft.com/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile

Run BuildSmall.bat to build Release and compress it with the repository's
shared Tools/Crinkler3.0b. Packed executables require Windows 10 or later and
a CPU with SSE4.2. Outputs:
  build/Debug/GraphicsDemo.exe
  build/Release/GraphicsDemo.exe
  build/Release/Intro.exe
The project copies LICENSE-Mountains.txt into both output directories.

The default Crinkler SLOW build is 3,147 bytes. VERYSLOW produced 3,144 bytes
in this test. Both meet the 4 KB target; SLOW is retained for quicker packing.

Or from the repository root:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\Build.ps1 -Project "DirectX 11\05_Terrain" -Configuration All -Compress

The demo plays at 1280x720 for 30 seconds. Escape while the window has focus,
or closing the window, ends playback early. Hold the left mouse button and
drag horizontally to shift the camera along its route, as on Shadertoy.
The selected offset remains after releasing the mouse button. Each frame
is rendered anew, with Present synchronized to vblank.

The camera holds one world-space cruise altitude throughout the selected
30-second route. It no longer rises and falls with the ground beneath it.
Changing the route with the mouse selects a new cruise altitude. The view
keeps a shallow, fixed downward pitch and the source's gentle banking.
Travel is 25% faster than the original camera. It starts earlier along the
same path to retain the previous endpoint without reaching a taller ridge.
The default cruise altitude is about 24.46 units, three units lower than
the previous steady-height camera.

Port details
------------
- EmbedShader.ps1 removes comments and whitespace, shortens identifiers,
  and shortens equivalent floating-point literals. It verifies byte-for-byte
  identical stripped FXC output for readable and compact source before
  generating build/<configuration>/obj/TerrainSource.h. A mismatch fails
  the build. This pass uses PowerShell and the existing Windows SDK only.
- Window.c compiles the embedded source with D3DCompile, creates the compute
  shader, releases the temporary compiler blob, then starts the flight timer.
  Terrain detail, noise precision, ray-march limits, camera, and controls
  are retained. Compilation runs once; every frame is rendered anew.
- The original five-octave height function alternates positive and negative
  contributions, rotating and stretching coordinates at each octave. A
  separate broad noise term creates the large mountain ranges.
  Height queries share one noise evaluator across these stages and the
  subsequent tree and normal-detail stages. Coarse, marching, and normal
  modes preserve their original sample order and tree-state updates.
- Ray marching preserves the source's 150-step budget and 240-unit limit,
  followed by five binary-subdivision steps. It retains the source's
  approximate intersections and tree silhouettes; it is not an exact SDF.
  Marching and subdivision use one loop to share the terrain evaluator in
  the compiled shader, with the same sample order and iteration limits.
- Noise visits its four corners in a rolled loop, retaining one copy of the
  hash calculation per noise call. Corner order and interpolation are unchanged.
- Normals add six finer terrain octaves outside forest areas. Three samples
  run in their original order through one rolled loop, preserving the
  source's tree state while keeping the embedded shader smaller.
- GLSL matrix constructors specify columns; the HLSL matrix is transposed
  at construction so multiplication rotates the terrain in the same way.
- The reversed-edge tree-line smoothstep is written as one minus the
  equivalent ascending transition. Fractional hashes use precise HLSL
  arithmetic to reduce differences from fused floating-point operations.
- Compute threads use bottom-up pixel-center coordinates and guard image
  bounds. The source's final post curve is clamped to match an UNORM display.
- One 16-byte constant buffer holds elapsed seconds, resolution, and mouse X.
  CrinklerFloatBits=32 preserves the noise and camera constants.
- Cruise altitude is the maximum of 21 units and the highest of 129 terrain
  samples along the complete 30-second route plus a 9-unit margin. This
  calculation is independent of elapsed time and shared within each compute
  thread group. Camera movement and the survey use the same phase speed.
  Keep both shader 30-second intervals in sync with Window.c's cutoff.
  The original local height averaging, vertical bob, and altitude-dependent
  pitch are removed. The new pitch is approximately 4.6 degrees downward.

Verification
------------
Validated with MSVC 14.29.30133 / Windows SDK 10.0.22621.0, Win32:
  Debug GraphicsDemo.exe:    10,240 bytes
  Release GraphicsDemo.exe:   9,728 bytes
  Crinkler 3.0b Intro.exe:     3,147 bytes (SLOW)
Build.ps1 and build/Release/Compression.log report current sizes after a
rebuild. Both ordinary PE entry points match winmain; the only imported
DLLs are d3d11, d3dcompiler_47, kernel32, and user32. The output license copies
were verified. There are no CRT imports or CRT startup.

The final Debug and packed executables created their windows, completed the
full flight, and exited with code 0 (31.14 and 31.74 seconds respectively,
including startup). Release window-close and Escape checks exited with code
0 within 1.1 seconds. Rendering was also checked independently below.

Twenty floating-point GPU readbacks at 0, 10, 20, and 29.9 seconds across
mouse-X offsets 0, 320, 640, 960, and 1280 contained finite, in-range RGBA
with opaque alpha and no Direct3D debug-layer warnings or errors. Every
rendered 8-bit image was byte-identical to the previous precompiled-shader
version. The runtime-compiled shader, after reflection/debug stripping,
also matches the original shader byte for byte (SHA-256:
450DF466A023E373C45177746294023E1D72093FCFED010157B7C748680BF01C).
The tested system D3DCompiler_47.dll version is 10.0.26100.9168. Default-route
dispatches took approximately 7.0-8.2 ms; these are spot checks, not a
hardware-independent performance guarantee. Validation artifacts are under
the ignored build/terrain-preview, build/terrain-size2, and build/terrain-4k
directories.

The shader remains 34,924 bytes after compilation, while its embedded
compact source is 5,855 bytes. Packing source instead of bytecode reduces
the executable from 6,066 to 3,147 bytes, 949 bytes below the 4,096-byte target.
Resolution, terrain detail, camera behavior, and full-precision constants
are retained. The tradeoff is one shader compilation at startup.

A dense GPU route survey checked 1,280 times during each of all 1,281 integer
mouse-X route selections. Every route held a constant altitude and sampled
clearance above the terrain plus trees was at least 5.77 units. This is a
sampled clearance check, not a mathematical bound between samples. The
default route stays at approximately 24.46 units, versus the previous fixed
27.46-unit cruise altitude.

Related native renderers:
  ../../DirectX 12/05_Terrain (embedded HLSL, shared terrain source)
  ../../Vulkan/05_Terrain (embedded SPIR-V, same terrain source)
