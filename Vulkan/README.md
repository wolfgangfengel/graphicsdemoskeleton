# Vulkan demos

Seven native Windows Vulkan ports use the same x86 custom `winmain` entry point,
no CRT, and Crinkler 3.0b as the DirectX examples.

| Example | Rendering | Playback |
| --- | --- | --- |
| [01_Skeleton](01_Skeleton/README.md) | Clear a swapchain image | 3.3 seconds |
| [02_Triangle](02_Triangle/README.md) | Rotating triangle, World * View * Perspective | 3.3 seconds |
| [03_Compute Julia4D](03_Compute%20Julia4D/README.md) | Quaternion Julia ray marching and self shadows | 30 seconds |
| [04_San Angeles](04_San%20Angeles/README.md) | Procedural city, ships, reflections and camera tracks | 109 seconds |
| [05_Terrain](05_Terrain/README.md) | Mountains, forests, water, clouds and flight camera | 30 seconds |
| [06_Compute PostFX Color Filters](06_Compute%20PostFX%20Color%20Filters/README.md) | Julia into a float buffer, then a color-filter compute pass | 30 seconds |
| [07_Himalayas](07_Himalayas/README.md) | Mountains, two moving cloud layers, temporal reprojection, free flight | Until closed; 120 FPS target |

## Build and run

Open any `GraphicsDemo.sln` in Visual Studio 2019, select **Debug / Win32**
or **Release / Win32**, and press F5. Install Desktop development with C++,
MSVC v142, and a Windows SDK. The first shader build downloads a pinned x64 DXC
compiler; see [tool setup and provenance](../Tools/Vulkan/README.md).
Subsequent builds work offline. No separately installed Vulkan SDK is required.

Each `BuildSmall.bat` produces `build/Release/Intro.exe` using the shared
Crinkler. From PowerShell in the repository root:

```powershell
.\Build.ps1 -Project 'Vulkan\05_Terrain' -Configuration All -Compress
```

Ordinary executables are `build/Debug/GraphicsDemo.exe` and
`build/Release/GraphicsDemo.exe`. All are Win32/x86. They dynamically load
`vulkan-1.dll` from the GPU driver's runtime and target Vulkan 1.0. A 32-bit
Vulkan driver is required. Packed EXEs also require the SSE4.2 CPU support
required by Crinkler 3.0b. No shader files, runtime shader compiler DLLs,
or scene assets accompany the executable. Keep the license files shipped
beside Terrain, San Angeles, Himalayas and demos using SMOL-V.

Escape while the demo has focus, or closing its window, ends playback early.
Terrain also accepts left-button dragging to choose a different camera route.
Himalayas uses WASD, Q/E, right-button mouse look, Shift boost, R reset and
Escape to exit. See the shared Himalayas README for timing and camera details.
Minimized windows pause rendering; the playback timer continues. Swapchain
recreation handles out-of-date/suboptimal surfaces, retaining the demo's
original rendering resolution.

## Source organization

`Common/Vulkan.h` contains Win32 surface/device setup, small allocation helpers,
and swapchain management. `Common/Graphics.h` and `Common/Compute.h` contain
the native pipelines; `Common/Entry.h` handles presentation and playback.
Only one frame is submitted at a time. A fence protects command and uniform
memory reuse, and each swapchain image has its own presentation semaphore.
The acquire wait and image layout transition both use the transfer stage.

All shader expressions come from the existing DirectX HLSL. `EmbedSpirv.ps1`
adapts resource declarations, compiles SPIR-V, strips optional OpName and
OpMemberName annotations, and embeds the remaining words. Terrain and Himalayas
use [SMOL-V](../Tools/smol-v/README.md) to encode them losslessly; a compact C
decoder restores SPIR-V once at startup. Every packed shader build verifies
byte-for-byte round trips with both upstream and runtime decoders. Entry-point names,
arithmetic, control flow, numeric precision and quality settings are retained.
Vertex shaders invert clip-space Y to match the DirectX image orientation.
Graphics use push constants; compute shaders use uniform buffers so PostFX
can retain its full 160-byte layout even on devices with 128-byte push limits.

San Angeles uses the shared `../San Angeles/Scene.h` geometry and camera data.
It prefers D24 depth when supported and otherwise uses D32 floating point.
This matters on Vulkan drivers that do not expose D24 depth attachments.
Compute and graphics render into RGBA8 images, then blit to the supported
UNORM swapchain format without an additional color-space conversion.

## Validation

Tested with VS2019 14.29, Windows SDK 10.0.22621.0, DXC v1.9.2607,
and an AMD Radeon RX 9070 XT. Debug enables the Khronos validation layer
when a compatible 32-bit layer is available. Messages appear in the debugger's
Output window; a validation error also produces a nonzero exit code.

Deterministic GPU readbacks cover six triangle frames, six Julia frames,
six PostFX frames, seven San Angeles camera points and twenty terrain frames
(four times across five routes). Khronos validation and synchronization
validation report zero errors and warnings for these captures. Cross-API
comparisons preserve the rendered scene, with normal compiler/driver rounding
differences. They are not claimed to be byte-identical across APIs.

Debug/Release/packed playback and window lifecycle checks are recorded in the
local ignored `build/vulkan-validation/` artifacts. For fresh verification,
run each ordinary and packed EXE through its full duration, and check close,
Escape, minimize/restore and window-size changes separately with validation.

## Size work

All projects retain full float constants (`CrinklerFloatBits=32`). Packing
uses the repository's default SLOW settings, except Himalayas, which selects FAST. Read `build/Release/report.html`
for the per-symbol costs; `Compression.log` records the actual final size.

Direct imports let the linker retain only used Vulkan functions. Reused static
create-info structures reduce generated initialization code; the hosts are
single-threaded. SMOL-V includes a decoder cost, so the four smaller shader
demos remain raw SPIR-V after measuring both complete EXEs. PostFX also remained
smaller raw after the initialization changes (6,227 versus 6,253 bytes).

Measured with the same default SLOW settings, retaining all 32 float bits:

| Original six demos | Before | Optimized | Saved |
| --- | ---: | ---: | ---: |
| Skeleton | 2,549 B | 2,049 B | 500 B |
| Triangle | 4,719 B | 4,000 B | 719 B |
| Julia4D | 6,200 B | 5,537 B | 663 B |
| San Angeles | 7,390 B | 6,700 B | 690 B |
| Terrain | 12,949 B | 10,170 B | 2,779 B |
| PostFX | 6,923 B | 6,227 B | 696 B |

The original six demos' 45 deterministic GPU captures are byte-identical before
and after these optimizations. Debug, Release and packed full playback, close,
Escape, minimize/restore and resize checks passed. Their ordinary PE entry
addresses match `winmain`, with only Vulkan, kernel32 and user32 DLL imports.
No scene simplifications, precision truncation or cached rendered frames were
introduced. Himalayas is a new addition; see its shared README for measurements.
