# Componentes e licenças / Components and licenses
Papel Vivo 2.0.1 — kz. Original code notices: LICENSE and LICENCA-PAPEL-VIVO.txt; pre-existing MIT notice: Licenses/PapelVivo-ate-1.0.2-MIT.txt. Original components and artwork retain their applicable terms; third-party terms prevail over conflicting restrictions.

| Component | Included build | License / notices |
|---|---|---|
| libmpv | mpv a2cd83c56 / FFmpeg 513df1c; original DLL unchanged | LGPLv3 variant; LGPL/GPL texts and primary source archive included |
| FFmpeg / ffprobe CPU | FFmpeg 7.1.1, libx264, libvpx 1.15.0, dav1d 1.5.0, zlib 1.3.1 | GPLv2-or-later configuration plus each library's terms; complete converter source archives and recipe included |
| FFmpeg GPU helper | FFmpeg 8.1.3, h264_nvenc/h264_mf; no external codec libraries | LGPL-2.1-or-later; source in primary FFmpeg archive, recipe and headers archive included |
| nv-codec-headers | 12.1.14.0 | MIT-style NVIDIA header notice included; requires separately installed compatible GPU driver |
| nlohmann/json | 3.11.3 | MIT notice included |
| NSIS | 3.08 | Original NSIS component terms included |
| Python | 3.11.9 embedded | python/LICENSE.txt |
| rembg | 2.0.67 | MIT, Daniel Gatis; license included |
| U2Netp | Local 4.6 MB model | U-2-Net Apache-2.0 notice included; see modelos/model.json |
| ONNX Runtime | 1.30.0 CPU | MIT plus ThirdPartyNotices preserved in Python package |
| NumPy, Pillow, llvmlite, numba, pymatting, requests and other runtime dependencies | Exact versions in installed *.dist-info/METADATA | Original licenses retained in Python packages/dist-info; see those texts |

Sources/PapelVivo-2.0.1-Fontes.zip contains application source. Sources/ThirdParty retains the primary mpv/FFmpeg sources, converter dependency source archives and build recipes. Existing primary-library archives are preserved rather than claiming every transitive source obligation has been independently audited. Before public redistribution, verify corresponding-source completeness for the unchanged third-party binaries and fulfill all applicable terms. Merely linking a project homepage is not a substitute for those obligations.

Replacing libmpv-2.dll with an ABI-compatible library and debugging library modifications are permitted. Original copyright/license texts are not edited to remove required credits. This distribution is not an assertion of exclusive authorship of third-party work.

Upstream: https://mpv.io/ ; https://ffmpeg.org/ ; https://code.videolan.org/videolan/x264 ; https://github.com/FFmpeg/nv-codec-headers ; https://github.com/danielgatis/rembg ; https://github.com/xuebinqin/U-2-Net ; https://www.python.org/ ; https://onnxruntime.ai/ ; https://nsis.sourceforge.io/
