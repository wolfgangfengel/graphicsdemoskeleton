# Shared build tools

`Crinkler3.0b/` contains the unmodified official **Crinkler 3.0b** release,
dated July 30, 2026, including its manual and license.

- [Official release](https://github.com/runestubbe/Crinkler/releases/tag/v3.0b)
- [Original archive](https://github.com/runestubbe/Crinkler/releases/download/v3.0b/crinkler30b.zip)
- Archive SHA-256: `14017536cf7bbc908f607260d5be30f56d8df182a0fd8fcead750e771f40888a`

`Build.ps1` selects `Win64/Crinkler.exe` on 64-bit Windows and
`Win32/Crinkler.exe` otherwise. These are host architectures: both produce
the demos' **32-bit x86 executables**. All example `BuildSmall.bat` files
invoke that shared script. `BuildAll.bat` builds and packs every demo.

Crinkler 3.x and its packed output require SSE4.2. The compressor requires
Windows 10 or newer. The C compiler remains Visual Studio 2019's v142 toolset.

The default compression mode is `SLOW`. Pass `-CompressionMode VERYSLOW` to
a batch file to try stronger model estimation; compare final byte counts,
since it does not always improve the result. DirectX builds use the bundled tools.
`-CrinklerPath` is available for explicit future toolchain experiments.

[`Vulkan/`](Vulkan/README.md) contains the Vulkan headers and the setup script
for the pinned build-time DXC compiler. The first Vulkan shader build downloads
that compiler into an ignored cache; subsequent builds use the local copy.

[`smol-v/`](smol-v/README.md) contains the pinned upstream lossless SPIR-V
encoder and its license. Selected Vulkan demos use a small C decoder at startup;
the build verifies exact round trips with both the upstream and runtime decoders.
