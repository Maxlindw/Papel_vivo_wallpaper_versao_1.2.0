# Building Papel Vivo 2.0.1
This source extends the existing Win32/libmpv Player; video rendering remains in src/player.cpp. The launcher owns a kill-on-close Job Object. Core contains the native UI, persistent catalog and isolated media jobs.

## Windows x64
CMake 3.20+, C++17 compiler, Windows SDK; MinGW-w64 POSIX GCC 12.2 was used. CMake links the C/C++ runtime statically for these executables. libmpv is loaded dynamically, not rebuilt by this project.
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```
For cross compilation set CMAKE_SYSTEM_NAME=Windows and MinGW C/C++/RC compilers. Host-only tests can build on Linux with an ordinary C++17 compiler. Use a Debug test build so assert checks remain enabled.

## Runtime staging
Put PapelVivo.exe, PapelVivo.Core.exe and compatible libmpv-2.dll at the root. Supply ferramentas/ffmpeg.exe, ffprobe.exe and ffmpeg-gpu.exe, python/python.exe with the rembg dependency set, modelos/u2netp.onnx and tools/remove_background.py. Ship assets/Brisa-Azul.mp4, documentation and required license/source notices. Python 3.11.9, rembg 2.0.67 and ONNX Runtime 1.30.0 were used. The model is checked against its SHA-256 before execution.
CPU converter sources/recipe and preserved mpv/library sources are under Sources/ThirdParty in the installed distribution. tools/build-gpu.sh builds the additional FFmpeg 8.1.3 NVENC/Media Foundation helper using nv-codec-headers 12.1.14.0 and MinGW. Drivers are not bundled. GPU encoding can fail on unsupported hardware and falls back to libx264.

## Setup
NSIS 3.08 Unicode with MUI2. Review the stage folder, then run:
```
python installer/gerar_payload.py /absolute/path/to/stage
makensis -DPAYLOAD_ROOT=/absolute/path/to/stage -DOUTPUT_FILE=/absolute/path/to/PapelVivo-2.0.1-Setup.exe installer/PapelVivo.nsi
```
The payload list is generated and intentionally not included: it must match the runtime folder being packaged. Keep generated binaries, private logs, dumps, credentials and test fixtures outside the public source tree. The installer removes only owned payload paths and asks separately about deleting Player user data.

## Licensing and verification
Original notices are retained. Third-party libraries keep their licenses; libmpv may be replaced by an ABI-compatible build and library-modification debugging is allowed. Follow THIRD-PARTY-NOTICES.md. Tests under Wine do not certify real Windows hardware or every GPU. No signing certificate is included.
