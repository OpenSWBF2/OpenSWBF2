# FFmpeg subset

Generated: 2026-11-24

- Version: 9.0.2
- Source: https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz
- Source SHA-256: 8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
- Signature: https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz.asc, checked against https://ffmpeg.org/ffmpeg-devel.asc, primary key fingerprint FCF986EA15E6E293A5644F10B4322F04D67658D8
- Local patch: patches/5195d6f9.patch, upstream commit 5195d6f9729ad00b93e4d0f0bea309416fb4713e (avcodec/bink: check init_get_bits8() for failure, CVE-2026-125068), SHA-256 8e0158c1095d60f704d4aadd231066081f34c93934960666d92e6b390d343064
- Local patch: patches/tx-subtx-free.patch, not upstream (avutil/tx: free the sub-transform array of a failed codelet; an allocation failure in a binkaudio_dct open leaked one block, cause C-FFMPEG-TX-LEAK in tests/baseline/v143/CAUSES.md), SHA-256 67cc7007829f377493d9c5f4f7fae718765e1242dc1d09a2c63619724fa16e02
- Configure Win32: configure --toolchain=msvc --arch=x86 --target-os=win32 --enable-static --disable-shared --disable-programs --disable-doc --disable-debug --disable-autodetect --disable-network --disable-asm --disable-pthreads --disable-w32threads --disable-os2threads --disable-everything --disable-avdevice --disable-avfilter --disable-swresample --disable-swscale --enable-demuxer=bink --enable-decoder=bink,binkaudio_dct,binkaudio_rdft --malloc-prefix=ossf2_
- Configure x64: configure --toolchain=msvc --arch=x86_64 --target-os=win64 --enable-static --disable-shared --disable-programs --disable-doc --disable-debug --disable-autodetect --disable-network --disable-asm --disable-pthreads --disable-w32threads --disable-os2threads --disable-everything --disable-avdevice --disable-avfilter --disable-swresample --disable-swscale --enable-demuxer=bink --enable-decoder=bink,binkaudio_dct,binkaudio_rdft --malloc-prefix=ossf2_
- Config edit: each config.h gets HAVE_ALIGNED_MALLOC 1 -> 0 and HAVE_POSIX_MEMALIGN 0 -> 1, so av_malloc, av_realloc and av_free reach ossf2_posix_memalign, ossf2_realloc and ossf2_free instead of the unprefixed CRT _aligned_* functions (cause C-FFMPEG-ALLOC-CONFIG in tests/baseline/v143/CAUSES.md).
- Config fixed values: each config.h gets CONFIG_THIS_YEAR 2026 and the CC_IDENT of cl 19.44.35230 for its platform in place of the configure date and the installed compiler, so -Verify does not depend on the calendar or the Visual Studio update. No vendored source uses either name, and the script fails if one does (cause C-FFMPEG-CONFIG-FIXED in tests/baseline/v143/CAUSES.md).
- Local sources: openswbf2/ossf2_alloc.c and openswbf2-ffmpeg.def are ours, compiled into or linked with the DLL.
- changes.diff holds both patches, both config.h edits and the local sources as new files.
- Dropped sources: libavcodec/file_open.c, libavformat/file_open.c (per-library copies of libavutil/file_open.c that would define ff_open twice in one DLL)
- Sources: 179 .c files from the OBJS, OBJS-yes, STLIBOBJS and STLIBOBJS-yes lists of libavutil, libavcodec, libavformat
- Headers: 235 .h files from the cl /showIncludes closure of those sources on Win32 and x64
- Other included files: 6 template sources that other sources include and that are not compiled on their own (libavcodec/bit_depth_template.c, libavcodec/hpel_template.c, libavcodec/jfdctint_template.c, libavcodec/pel_template.c, libavcodec/simple_idct_template.c, libavutil/tx_template.c)
- Counts: libavutil/hwcontext_stub.c is selected by OBJS-$(!CONFIG_VULKAN), which configure turns on (!CONFIG_VULKAN=yes in ffbuild/config.mak), so make compiles it too. A count that ignores the !CONFIG_ form gets one source fewer, and a closure count that takes the included templates for headers gets the headers plus the templates.
- File list SHA-256: 91ff029fd2d7e5bb90f34667e1d8cc3ee73b8c0a295f1174a2d8c9c759109597 (the sorted relative paths of the vendored sources and closure files, one per line, LF)
- Generated files: config/Win32/config.h, config/x64/config.h and config/common/ (identical on both platforms)
- Regenerate: powershell -NoProfile -ExecutionPolicy Bypass -File tools/update-ffmpeg.ps1; check: the same with -Verify

## Exemptions

FFmpegFL.vcxproj builds openswbf2-ffmpeg.dll under OpenSWBF2.Game.props. Each exemption from those flags is per warning or per file, with its cause in tests/baseline/v143/CAUSES.md:
- /wd4146 on every FFmpegFL source, after /sdl on the command line (C-FFMPEG-WD4146)
- /wd4703 on libavutil/encryption_info.c, libavformat/url.c, libavformat/avio.c, libavformat/av1.c and libavcodec/raw.c only (C-FFMPEG-WD4703)
- Optimization MinSpace on libavcodec/avcodec.c in Debug only (C-FFMPEG-DEBUG-O1)
- bcrypt.dll import kept (C-FFMPEG-BCRYPT): avformat_alloc_context -> av_opt_set_defaults -> av_opt_set_defaults2 -> set_string_color -> av_parse_color -> av_get_random_seed -> BCryptOpenAlgorithmProvider, BCryptGenRandom, BCryptCloseAlgorithmProvider. These are static calls from an export the decoder needs, so /OPT:REF keeps them; the /VERBOSE:REF link discards none of them. Measured links that export only avformat_alloc_context, avformat_open_input, avcodec_open2 or avutil_version each import bcrypt.dll too.
- Link: /OPT:REF /OPT:ICF and /Gy in every configuration. Debug compiles at /Od, which does not imply /Gy, and without /Gy the linker cannot drop ff_tempfile (libavutil/file_open.c), whose free of the tempnam result is the only CRT heap call left after the config edit; the Debug DLL then imports free and the filesystem functions. With /Gy no Debug, Release or Test DLL imports api-ms-win-crt-heap-l1-1-0.dll.
- Asan: the DLL imports malloc, calloc and free from api-ms-win-crt-heap-l1-1-0.dll because VCASAN.lib(vcasan.obj), which /fsanitize=address links into every image, references them. No FFmpegFL object references them after /OPT:REF.
