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
since it does not always improve the result. No tool downloads occur during
builds. `-CrinklerPath` is available for explicit future toolchain experiments.
