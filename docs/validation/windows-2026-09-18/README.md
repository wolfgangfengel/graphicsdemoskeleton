# Saved Windows validation — September 18, 2026

Historical evidence, copied from the locally validated build outputs on September
23. It is not a new runtime test on either Windows or Apple. The three Himalayas
build/smoke JSON records and capture/benchmark logs are preserved, together with
the original six Vulkan demos' size and pixel-comparison reports.

Himalayas readbacks use 64 frames and fixed shader time 1000 ms at routes 0-4;
route 5 moves/rotates/resets. DX11 and DX12 fixed-pose images match exactly.
Vulkan differs by 0.667-2.309 mean absolute RGBA units (0-255). The 1000-versus-
10000 ms fixed-camera wind comparison changes 238,309 pixels. All captured
debug layers report zero errors/warnings; DX11 float probes find zero NaN/Inf.

`binaries.json` inventories all 19 available packed Windows outputs and their
ordinary counterparts with fresh SHA-256 checksums, sizes and original build
timestamps. They are preserved artifacts, not newly compressed on September 23.
The GitHub reference download contains the binaries, licenses, raw SPIR-V and
RGBA images. Its manifest can be verified with `Support/VerifyReference.py`.

The current source's fresh handover build/probe result is recorded separately in
`../handover-2026-09-23.json`. Apple build/runtime/device status remains untested.
