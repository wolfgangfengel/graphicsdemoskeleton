# Vulkan build tools

`include/` contains the Vulkan 1.3.275 C headers, with their source notices
and [Apache license](LICENSE-Headers.txt). Upstream is
[Khronos Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers).
The demos target Vulkan 1.0 and link the driver's `vulkan-1.dll` through
`lib/x86/vulkan-1.lib`. Only functions used by each demo become imports;
Crinkler packs their import table without a separate hand-written loader.

The unmodified x86 import library comes from the optional 32-bit component
of the official [LunarG Vulkan SDK 1.3.296.0](https://vulkan.lunarg.com/sdk/home#windows).
Its archived component is `com.lunarg.vulkan.32bit/1.3.296.0core32.7z`,
file `Lib32/vulkan-1.lib`. It contains import stubs, not a bundled Vulkan driver.
The loader's upstream license is [Apache 2.0](LICENSE-Headers.txt).

`Setup.ps1` downloads Microsoft's official
[DXC v1.9.2607](https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.9.2607)
Windows archive into the ignored `build/dxc` directory on first shader build.
It verifies SHA-256
`a1dfb116ba3eeae6a1582291b53a8e7bf65ad760676bd3194685c8f7367cd241`.
The extracted archive retains Microsoft's MIT/LLVM/MS license files.
Run Setup.ps1 beforehand to prepare an offline build.

The build-time compiler is x64; all demo executables remain x86. DXC compiles
HLSL to embedded SPIR-V. Neither DXC nor a shader compiler DLL is needed to
run an executable. No system SDK installation, PATH or registry changes are
made by Setup.ps1. Build hosts need 64-bit Windows and internet access on
the first shader build; subsequent builds use the cached compiler.

Debug builds enable `VK_LAYER_KHRONOS_validation` when a compatible 32-bit
layer is installed or supplied via `VK_LAYER_PATH`. The layer is optional
for running the demos. An x64-only layer cannot validate an x86 executable.
