# macOS / iOS handover — 2026-09-23

## Start here

This checkpoint preserves the complete Windows work: six DirectX 11 demos,
six DirectX 12 demos, seven Vulkan demos, shared scenes/shaders, build tools,
license notices, and Windows validation evidence. **There is no Apple backend,
Xcode project, native Apple build or Apple runtime validation yet.**

```sh
git clone --branch codex/apple-handover https://github.com/wolfgangfengel/graphicsdemoskeleton.git
cd graphicsdemoskeleton
git status --short
git rev-parse HEAD
cat HANDOVER_APPLE.md
xcodebuild -version
xcrun --find metal
```

Read [README.md](README.md), [Vulkan/README.md](Vulkan/README.md), and
[Himalayas/README.md](Himalayas/README.md). The publication includes a
[Windows reference download](https://github.com/wolfgangfengel/graphicsdemoskeleton/releases/tag/windows-handover-2026-09-23)
with ordinary and packed EXEs, licenses, compiled SPIR-V, deterministic Himalayas
RGBA captures, and a SHA-256 manifest. EXEs are Windows/x86 reference artifacts;
they do not run natively on macOS/iOS. The archive records its exact source commit.
After extracting it, verify every file with:

```sh
python3 Support/VerifyReference.py /path/to/extracted/windows-reference
```

The source tree retains reproducible dependencies and setup scripts, rather than
compiler download caches, object files, IDE databases, or abandoned experiments.
Crinkler 3.0b is vendored once. The old compressor and reduction demos must remain
removed. The early [repository evaluation](docs/history/EVALUATION-2026-09-12.md)
is historical; its old failures and reduction examples do not describe this checkpoint.

## Requirements that carry over

- Small self-contained executables are the purpose of this collection. Preserve
  the compact organization; avoid a large engine/framework rewrite.
- Retain procedural detail, iteration counts, texture precision, moving clouds,
  lighting and animation. No cached rendered scenes or visual simplifications
  to meet a size target. If full quality cannot fit 4 KiB, report the real size.
- Windows keeps its custom x86 `winmain`, no CRT startup/imports, and Crinkler.
  Those mechanisms target Win32 PE and x86, not Apple ARM64 Mach-O. Use the
  required Apple app lifecycle/signing first; report executable, metallib,
  app bundle and distribution sizes separately before size optimization.
- Keep one shared San Angeles scene and one shared Himalayas shader directory.
  Preserve `04_San Angeles`, `05_Terrain`, `07_Himalayas` numbering. DX11 PostFX
  is example 03; Vulkan PostFX is example 06. There is no DX11 triangle example.
- Triangle uses real World * View * Perspective matrices and rotates continuously.
- Himalayas has free flight, not the old automatic camera: WASD, Q/E, right mouse
  look, Shift boost, R reset, Escape exit. It runs until closed, targets 120 FPS,
  and keeps the 1280x720 reference rendering resolution. Design equivalent touch
  controls for iOS; do not silently reinstate the old camera path.

## Recommended Apple implementation

Start with a small native Metal backend using Objective-C/Objective-C++ and an
MTKView host for macOS and iOS. This is a recommendation based on the tiny-binary
goal, not an existing implementation. Reuse scene/math algorithms and keep shader
sources authoritative. Offline SPIR-V-to-MSL conversion with SPIRV-Cross is an
initial route for retaining HLSL; inspect generated binding layouts and validate
images before treating it as equivalent. Compile Metal libraries at build time.

MoltenVK is an alternative for getting the Vulkan renderer running on Apple
earlier, but adds a runtime dependency and should have its full bundle cost
measured. It still requires replacing Win32 setup/input/timing with an Apple host.
For that route, add a Metal surface, supported portability extensions/device
features, and the portability enumeration flag required by the Vulkan loader.
Select and pin Apple dependency revisions on the Mac; the current vendored Vulkan
headers/import library and DXC setup script are Windows build inputs.

Primary references checked when preparing this handover:
[MTKView](https://developer.apple.com/documentation/metalkit/mtkview),
[SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross),
[MoltenVK integration and portability](https://github.com/KhronosGroup/MoltenVK/blob/main/README.md).
These references are guidance, not evidence of an Apple build.

## Source map and concrete porting work

| Area | Existing implementation | Apple work |
| --- | --- | --- |
| Build/entry | `Build.ps1`, `Support/TinyDemo.props`, `Support/TinyRuntime.h`, per-demo `Window.c` | Add Apple targets; isolate Win32 types, naked entry, x87 assembly and SDK-specific calling conventions. Do not pass the Windows flags to Clang. |
| Minimal host | `Vulkan/Common/Vulkan.h`, `Entry.h`, `Graphics.h`, `Compute.h` | Reference for resource lifetime and synchronization; implement Metal device/queue, drawable and app lifecycle. |
| Triangle | `Vulkan/02_Triangle/Transform.h`; DX12 triangle shaders | Preserve row-vector matrix order, perspective, depth and Y conventions. |
| Julia/PostFX | shared DX11 HLSL; `Vulkan/Common/Julia.h` | Compute dispatch, bounds, uniform layout, float intermediate buffer, filter pass. |
| San Angeles | `San Angeles/Scene.h`, `Shaders.h`, `cams.h`, `shapes.h` | Replace x87/Win32 allocation helpers with portable equivalents while preserving geometry, random sequence, rounding and camera tracks. Keep reflections and depth behavior. |
| Terrain | `DirectX 11/05_Terrain/Terrain.hlsl`; DX12/Vulkan wrappers | Preserve the steady-altitude path, materials, forests, water and clouds. |
| Himalayas | `Himalayas/{A,B,C,D,Image,Common,Bindings}.hlsl` | Five compute pipelines, exact textures/bindings, temporal history, input and pacing; details below. |
| Shader storage | `Support/EmbedShader.ps1`, `Vulkan/Common/EmbedSpirv.ps1`, `Smolv.h` | Offline Apple compiler pipeline; SMOL-V remains lossless SPIR-V storage and is not a Metal-library compressor. |

Search before porting:

```sh
rg '__asm|__declspec|windows.h|HeapAlloc|VirtualAlloc|GetTickCount|QueryPerformance|CreateWindow|CreateWaitable' \
  'San Angeles' Himalayas Vulkan Support
```

Himalayas details that are easy to lose:

- A/B lookup textures initialize once. C/D each ping-pong between two history
  images; Image composites. A/B/C/D use RGBA32F with the original filtering;
  final output uses RGBA8. Preserve actual format/filtering support checks.
- Preserve 128 terrain steps, 15 normal octaves, 12 cloud steps, six cloud-shadow
  steps, and 3x3 humanoid antialiasing. Dispatch uses 8x8 groups with edge guards.
- Constants are three float4 values (48 bytes): resolution/time/frame,
  position/yaw, pitch/history-reset epoch/padding. Scene units are ten metres.
  Wind is (12,0,4) metres/second upstairs, half downstairs.
- D stores old camera basis, position, resolution, time and reset epoch in
  metadata texels. All metadata reads use mip 0. Do not convert those float
  surfaces to UNORM or let camera-motion code overwrite the metadata.
- Read old C/D before overwriting current history; preserve explicit resource
  dependencies. Reset history on view reset, resize or long time gaps. Handle
  rays behind the old camera, horizontal fog rays, and cameras inside/above clouds.
- Intermediate shader coordinates follow ShaderToy's bottom-up convention;
  Image flips the final output. Vulkan vertex shaders separately use DXC Y
  inversion. Audit each path when converting to Metal to avoid double inversion.
- Current Windows timing uses QPC and a high-resolution waitable timer. Replace
  this with Apple presentation/timing APIs, elapsed-time movement, foreground
  handling and background suspension. A 120 FPS request is not proof of 120
  visible refreshes or sustained performance on an iPhone/iPad.

## Suggested order and acceptance criteria

1. Record Mac model, OS, Xcode, target SDKs and attached iOS hardware. Verify the
   source commit and artifact hashes. Keep signing identities local.
2. Build and run Skeleton on macOS, then the matrix-driven Triangle. Establish
   resize, minimize/background, input, cleanup and Metal validation first.
3. Bring up compute with Julia and PostFX, then terrain and San Angeles. Compare
   deterministic captures with the Windows implementation at matching inputs.
4. Port Himalayas in pass order A/B, C, D, Image. Use the supplied captures and
   `Support/Validation/Himalayas/Capture.c` as the time/pose protocol. Add free
   camera and wind without reducing quality. Test history while moving/resetting.
5. Build iOS device and simulator as separate targets; run on real hardware.
   Test rotation, touch input, app suspension/resume, GPU memory and thermal
   behavior. Do not present simulator or shader translation as device validation.
6. Measure steady-state frame times after warm-up and each size component.
   Optimize only after capture/validation baselines are established. List native
   build, runtime, image comparison and blockers separately for each platform.

Expected completion: every supported demo runs on macOS and a physical iOS
device, shader/image differences are quantified, resource validation is clean,
and size/performance reports retain the full-quality scene. Do not claim this
completion based on this Windows handover.

## Windows reference and verification

The last GPU validation was on 2026-09-18: VS2019 v142/MSVC 14.29,
Windows SDK 10.0.22621.0, AMD Radeon RX 9070 XT. Prior verified Himalayas cadence
at the initial view was 120.00 / 119.67 / 119.99 FPS (DX11/DX12/Vulkan), measured
over 600 application frames after 60 warm-up frames. Packed sizes are
6,660 / 7,404 / 16,308 bytes. These are Windows results, not Apple projections.
See [saved evidence](docs/validation/windows-2026-09-18/README.md), the binary
manifest, and [reusable probes](Support/Validation/README.md).

To rebuild Windows references, use VS2019 C++/v142 and an installed Windows SDK:

```powershell
.\Build.ps1 -Configuration All -Compress
```

Pinned tool provenance is in `Tools/README.md`, `Tools/Vulkan/README.md` and
`Tools/smol-v/README.md`. First Vulkan shader compilation downloads the pinned
DXC Windows package; later builds use the cache. Ordinary demos have no external
shader files. Apple conversion must retain shader/scene attribution:
Himalayas CC BY-NC-SA 4.0; Mountains CC BY-NC-SA 3.0; San Angeles BSD option;
SMOL-V MIT. Preserve each supplied notice and review distribution terms before
an App Store/commercial release. Do not relicense the collection as uniformly MIT.

## Prompt for the next machine

> Continue graphicsdemoskeleton from codex/apple-handover. Read HANDOVER_APPLE.md,
> the shared demo READMEs and saved Windows validation first. Implement small
> native macOS and iOS Metal demos for the existing seven Vulkan-equivalent
> examples, starting with Skeleton and the matrix-driven Triangle. Reuse shared
> scene/shader algorithms; preserve all rendering quality and the free camera,
> moving clouds and 120 FPS target in Himalayas. Keep Windows custom-entry builds
> working. Add reproducible Apple builds, run native validation and deterministic
> captures, then measure actual executable/shader/bundle sizes and sustained FPS.
> Clearly distinguish Mac, iOS simulator and physical-device validation. Keep
> removed reduction examples removed. Never trade visual detail for a 4 KiB claim.
