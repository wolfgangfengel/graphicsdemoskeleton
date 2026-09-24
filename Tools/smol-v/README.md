# SMOL-V shader packing

The unmodified `smolv.cpp` and `smolv.h` come from
[Aras Pranckevicius's SMOL-V](https://github.com/aras-p/smol-v), revision
`9dd54c379ac29fa148cb1b829bb939ba7381d8f4` (also recorded in `revision.txt`).
We use its MIT license; retain [license.txt](license.txt) with distributions.

`Build.ps1` builds the x64 `build/Encode.exe` helper with VS2019/v142.
This build-time helper uses the C++ runtime; the demo executables do not.
`Encode.cpp` encodes each stripped SPIR-V module and verifies that both the
upstream decoder and the actual demo decoder restore every input byte.
A failed round trip stops the shader build.

The runtime decoder is [Vulkan/Common/Smolv.h](../../Vulkan/Common/Smolv.h),
a compact C adaptation of the pinned upstream decoder. It supports version 1
streams from this encoder, with no allocation, C++ containers or CRT calls.
It is intended for trusted embedded shaders. The Vulkan host allocates an
aligned output buffer, decodes before `vkCreateShaderModule`, and frees the
buffer after module creation. Decoding never occurs in the rendering loop.

`GenerateMetadata.ps1` regenerates `Vulkan/Common/SmolvOps.h` from the opcode
metadata in the pinned `smolv.cpp`. Regenerate it when updating upstream, then
rebuild every shader so both round-trip checks run again.

Each Vulkan project selects `<UseSmolv>0</UseSmolv>` or `1`. Compare complete
Crinkler-packed EXEs, including the decoder, before changing that choice.
Small shaders can be cheaper as raw SPIR-V. The shader target copies the
SMOL-V license beside executables that use its decoder.
