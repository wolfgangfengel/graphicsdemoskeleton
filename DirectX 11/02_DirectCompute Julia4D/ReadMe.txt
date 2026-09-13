Requirements:
- Visual Studio 2019 with MSVC v142 and a Windows 10/11 SDK
- Windows 10/11 (tested)
- DirectX 11 capable GPU
- The D3D11 debug layer (Windows Graphics Tools) for Debug builds

Select Debug / Win32 and press F5 in GraphicsDemo.sln.
Run BuildSmall.bat for build\Release\Intro.exe.
See the root README.md for the shared build and verification commands.

Size with VS 2019/MSVC 14.29.30133, SDK 10.0.22621.0, and Crinkler 3.0b:
- Previous packed EXE: 2,631 bytes.
- Default BuildSmall.bat: 2,095 bytes.
- Extended compression: 2,089 bytes, saving 542 bytes (20.6%).
  BuildSmall.bat -CompressionMode VERYSLOW -OrderTries 25000 -HashTries 1000
- Uncompressed Release EXE: 10,752 bytes, previously 19,456 bytes.

The demo still renders the animated quaternion Julia set at 1280x720 for about
30 seconds, with self-shadowing, changing fractal parameters and colors, a
hidden cursor, and Escape/window-close handling. The parameter and color
transitions retain their shared approximately 20-second interval and LCG.

Correctness changes:
- Phong shading uses the camera position and reflect(-L, N). Previously the
  ray direction was passed as the camera position and the reflection vector
  pointed the wrong way, producing misplaced specular highlights.
- Every HRESULT-returning device/resource/shader creation call and Present
  is checked. Failure exits with that HRESULT, without using invalid outputs.
- The window-creation result is checked as well.
- A default-usage constant buffer is updated from a complete 128-byte CPU
  structure with UpdateSubresource. There is no unchecked mapped pointer.

Size changes:
- The normal-estimation loop is retained with [loop], rather than expanded
  ten times. This keeps all ten iterations and reduces embedded shader code
  from 14,756 to 6,576 bytes, including the lighting correction.
- One animation phase replaces duplicate mu/color update bookkeeping.
- Constant matrix, viewport, detail, zoom, and shadow fields are initialized
  once on the CPU. Shader and constant-buffer bindings are also set once.
- Static descriptors replace descriptor copies and an unused GetDesc call.
- Release uses stdcall helpers and process teardown for resource reclamation.
  Debug retains the validation layer and explicit COM resource cleanup.

Both configurations retain the custom naked x86 winmain and no CRT imports.
Only d3d11.dll, kernel32.dll, and user32.dll are imported by the normal builds.
Crinkler's loader also supplies its own loader/error-reporting code. The usual
16-bit float truncation remains enabled, so packed animation timing can differ
slightly from the uncompressed build.

Validation:
- Six representative fractals matched the lighting-corrected, unrolled shader
  in every display-clamped RGBA channel, with no nonfinite output values.
- GPU timestamps on the test machine averaged 0.494 ms per dispatch versus
  0.517 ms for the unrolled reference; no slowdown was observed in this test.
- An instrumented 30-second Release run exercised animation, the transition
  after 20 seconds, and hide/show. Six readbacks showed changing images, and
  the D3D11 debug layer reported zero warnings or errors.
- Six injected failure cases (device, back buffer, constant buffer, UAV,
  shader, and Present) exited with the injected HRESULT.
- Debug, Release, and the final packed EXE completed their full timed runs
  with exit code zero. A separate window-close run exited cleanly.

References:
https://www.cs.cmu.edu/~kmcrane/Projects/QuaternionJulia/
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-reflect
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource
