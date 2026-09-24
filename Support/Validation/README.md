# Windows reference probes

These are the Himalayas probes used for the September 18 validation, promoted
from ignored scratch output for the Apple handover. The launch scripts now find
VS2019/v142 and an installed Windows SDK instead of assuming one machine's path.
They build separate CRT-based test programs around the actual demo source;
the shipping demos still use their custom entry point and no CRT.

Build Debug and Release for the selected API first. Debug graphics probes need
Windows Graphics Tools (D3D) or an installed **32-bit** Khronos validation layer
(Vulkan). Set `VK_LAYER_PATH` if required; the Vulkan capture fails if validation
is unavailable. Synchronization validation is enabled by the runner.

```powershell
.\Build.ps1 -Project 'DirectX 11\07_Himalayas' -Configuration All -Compress
.\Support\Validation\Himalayas\Controls.ps1
.\Support\Validation\Himalayas\Capture.ps1 -Api 'DirectX 11' -Time 1000 -Route 0
.\Support\Validation\Himalayas\Benchmark.ps1 -Api 'DirectX 11'
.\Support\Validation\Himalayas\Smoke.ps1 -Api 'DirectX 11'
```

Repeat with `DirectX 12` and `Vulkan`. Capture executes 64 frames at a fixed
time in milliseconds; routes 0-4 are initial, translated/rotated, above upper
clouds, inside upper clouds, and inside lower clouds. Route 5 moves, rotates,
then resets at frame 48. See Capture.c for exact camera values. Fixed time
also fixes the cloud ray jitter; compare times 1000 and 10000 to isolate wind.
The unencoded frame.rgba output is 1280x720 tightly packed RGBA8, top row first.

Outputs go to ignored `build/himalayas-validation`. Benchmark uses the real
presentation loop, discards 60 warm-up frames and times 600 frames; it measures
application cadence, not monitor scanout. Smoke runs Debug/Release and a packed
EXE, including 35-second survival, minimize/restore and close/Escape checks.
Packed sizes should be measured after the normal compression mode, not INSTANT.

The saved September 18 evidence is in `docs/validation/windows-2026-09-18`.
Handoff-time build checks are separately recorded in
`docs/validation/handover-2026-09-23.json`; they do not imply new Apple validation.
