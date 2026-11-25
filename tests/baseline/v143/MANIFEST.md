# v143 baseline manifest

## Build

| Key | Value |
|-----|-------|
| toolset | v143 |
| cl | 19.44.35229 |
| MSVC tools version | 14.44.35207 |
| ucrtbase.dll | 10.0.26100.9711 |
| vcruntime140.dll | 14.51.36247.0 |
| configuration | Release |
| commit | eff28502bae0530cde8ce09c734c37cd1dc4f8ef |
| tracked tree clean | yes |

Differences from `tests/baseline/vs2013/` and their causes are in [CAUSES.md](CAUSES.md).

## Text artifacts

| File | SHA-256 | Produced by |
|------|---------|-------------|
| `CAUSES.md` | B359FBE78CFB4F64DEB369A2975F4DFB0190E60FB3A1CE83C4F268B9BB9CA451 | written by hand from the comparison against `tests/baseline/vs2013/` |
| `smoke-all-digests.txt` | 7296407B0E03CF7AF42319AE3B44C72F631DC765C324464AE8CCDD9B9C772DA3 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `pool-sizes.txt` | A2FA679AC1174C9082D1E3DE216F82FC16B5E20B35A41C09AC4F081CF9643D72 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks.txt` | D24B076240DC0D686B07C7F1D33EF273F88E928C863E9C11878061FF3A7D41B0 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `cloth-headroom.txt` | 7C388EDBA9ADC4CF1D82A8C66FD7409CAF534B023E630AB59F9844312104F720 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `offthread.txt` | 2D70A010C5B9E2DA6F6A2E79D230A3D505C72D065FE5066F17207379561E4DD5 | `capture-vs2013.ps1 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Smoke All -SmokeConfiguration Test` and `-OffThreadSoundDir` from `soundon.ps1` |
| `lua-bins.txt` | A6E3FF46D1E8A6D69232061361199590BC6FB0C1AEEF6EFF0A0E19F69DA5936A | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All` |
| `lua-hist.txt` | CB394BFAF9FDEF86E9C4A51C16F9AAEDFABD0882A150490E6A8EEFE3F88B488A | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All` |
| `digests-debug.txt` | 43410C1C6F2D0877EC3D088DAF7E6DB8E1EA721AFEB5DABBE0634765398AE52E | `capture-vs2013.ps1 -DebugDigests` from `run.ps1 -Smoke Twice -SmokeConfiguration Debug` |
| `digests.txt` | C366231B400687A272F3030E8BBD88D597475225CF040B39E9FF9A4C55D537C9 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digests-hog.txt` | C366231B400687A272F3030E8BBD88D597475225CF040B39E9FF9A4C55D537C9 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-geo1c_con.txt` | AFFEBB16D317C5209C79B868F30A1BE624221D7BA4F391828221FB4E633473AF | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-hot1g_con.txt` | 581CAEFF4A3AB8BF5D28AF7A6FFCB8FC8B573B6E83BC2C56326AEF6B28835840 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-nab2g_c.txt` | 36D3D65BC29277A31F0D6F19038C3721DC74604505BCB0096495324F021F468C | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-spa3c_ass.txt` | 0E7B9CDEC9BEEF30858FEFA7FFEFC25291BABAD2057E0748892C146451A11ECF | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-dag1c_ctf.txt` | C6EFAE14F11C89BA4F306A7DCF503BD09189F52B270E54354DD3B3492C363715 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-kas2c_uber.txt` | 8E17DE177026D327DD4DE5E6FE4D0A68188AEE3C05B1EAAE8723D60200696862 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-geo1c_con.txt` | 3A9A62BC7946C8D00184CC755C15AF05A00C47BB4AE1CC1079FB2A2599032B3A | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-hot1g_con.txt` | ABE2A1CE250B9820092243654E00676957035F6682C5BBB9C74C58CACF01F5BD | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-nab2g_c.txt` | 68A112D1C57A0F92D0B11C6B63507C444C14F3656BB7D882E68510685377FCAC | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-spa3c_ass.txt` | 528D729A022C9EDE3E8BE2E737047416FF7663D685BDF440BC3BB65472EB3563 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `initializers.txt` | DFAB8F8E42253C47AA0D88E56845E8E39DBAA531EA828E06A1FD7C74721B5320 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-di.txt` | 2C89B86B8D5836A6069AA6F99237253E30635B9C99FF7A17BCF4AE4D3BAA7D80 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-discarded.txt` | F9656D1101DEA5FE04D1DC5F96876DBAC83954CA710272BBE4D8DCFEED4BCCA0 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `x87-inventory.tsv` | 8B1B550A6B5524704B51DC49747FF9C8FF9B9E6F3C0E3AFC89597B39F4D524AC | `x87-inventory.ps1 -Exe bin\Win32\Release\BattlefrontII.exe -Map obj\Win32\Release\BattlefrontII.map -Out tests\baseline\v143\x87-inventory.tsv` |
| `movie-goldens.txt` | D509FB737B257A12E868095E2E9CDD1773E0AD363EA01D5908EF782EC4693AFE | built from the `Movie frame` lines of the 51-04 `-movietest` runs on the Win32 Test build, two byte-identical runs per segment; the `## binkw32 ingame digest` line from the 51-06 `ingame-movie-digest` runs, two byte-identical sound-on runs and one `-nosound` run |
| `movie-frames.txt` | 467DDFDE166F59741D202D0CCE369E7114C4B9F6B824F5B753AFFEBF3CAA0839 | the hashes the T2 `movie frame hashes match the pinned FFmpeg path` computes through `MovieDecoder` and `ConvertYuv420ToXrgb` on the Win32 Test build, two byte-identical runs; the x64 Test run gave the same bytes |

## Notes

- Release is the reference configuration. The binaries are not in the repo.
- Every file was captured or re-checked on commit eff2850, whose game exe is the a2c980c build with the 20-12 and 20-14 fixes (eff2850 changes only tests). Release exe SHA-256 `17A3A864356C23D254EA6A302DA0FF591A3013B0F5135A18F64E99E130A71885`. `hardening.txt` was regenerated there and `smoke-all-digests.txt` came out byte-identical. `digest-trace-hot1g_con.txt` was regenerated on a381f01. `digests.txt`, `digests-hog.txt`, `digests-debug.txt`, the other three digest traces and the four factory logs were captured on 1b1cbca/dce2810, and all ten, with `digest-trace-hot1g_con.txt`, are byte-identical when re-captured on eff2850. The `initializers*.txt` files come from dce2810.
- The pool size, memory peak and cloth headroom tables name their commit on their first line and cover the missions in `tests/smoke/missions-all.txt`. The memory peak table keeps the latest MEMREPORT line for each heap and pool. A pool freed before the `exit` report still has an `exit` row, with count 0 and its peak.
- `smoke-all-digests.txt` row `dedicated` is a real-time run, so its `state=`, `time=` and `rand=` vary between runs.
- `offthread.txt` comes from the Test configuration, because the off-thread counter exists only in Test and Asan builds; every other table here except `lua-bins.txt` is Release. Its `## sound-on` section holds geo1c_xl, kas2c_xl and hot1g_xl run with the renderer and sound on by `tests\baseline\soundon.ps1`, which also writes each case's `sound.txt`. `sound=opened` means the game logged `Sound open=ok`; a `-nosound` control run logs no such line.
- The serial-order flip re-captured `digests*.txt`, the four digest traces, `smoke-all-digests.txt`, `pool-sizes.txt`, `mem-peaks.txt`, `cloth-headroom.txt` and `lua-bins.txt` once, under the `serial` default, in the commit that made it the default; a table that names its commit names that commit's parent, because the capture ran on the uncommitted flip. The four factory logs came out byte-identical, so the address and serial Factory orders are equal. `offthread.txt` is unchanged: its main section re-captured byte-identical under `serial`.
- `lua-bins.txt` is the serial Lua bin table: Test configuration, because the Lua spill counter exists only in Test and Asan builds; peak equals count on saturated bins, so the spill column is the demand signal for MPOOL-06. The DATA-04 address-against-serial comparison, taken where the Lua `Node` grew from 20 to 28 bytes and `Table` from 32 to 40, is in git history.
- Phase 41 re-captured `mem-peaks.txt`, `cloth-headroom.txt`, `hardening.txt` (Release) and `lua-bins.txt` (Test) from mimalloc runs. `compare-tables.ps1` compares `mem-peaks.txt` (Lua rows included), `cloth-headroom.txt`, `pool-sizes.txt` and `smoke-all-digests.txt` exactly, so the hand-written cloth slack table was deleted.
- The phase 41 code review re-captured `mem-peaks.txt` and `cloth-headroom.txt` once more: `StickInfo` gained a 4-byte stick key, so its pool rows are 48 bytes and every mission's Runtime heap holds 240 more bytes. No other row moved.
- Phase 42 (Gametoast 0c1bbd3e) re-captured, with the user's approval, `smoke-all-digests.txt` (the spa3c_ass row only; the `dedicated` row keeps its earlier value, as it is wall-clock and not compared) and `mem-peaks.txt` from `artifacts/tests/20261112-054020` (C-FLYER-REACT), and `initializers*.txt` from the 277bc98c Release map (C-INIT-ORDER-42), all on game commit 277bc98c.
- Phase 43 (static-library split) re-captured, with the user's approval, `initializers.txt` and `initializers-di.txt` from the 4919bf44 Release map (C-SPLIT-STEAM-INIT), and added the `crash-stickdelete` row to `smoke-all-digests.txt` (the `dedicated` row keeps its pinned value) and the `crt-heap-ban` and `mimalloc-crt-hooks` lines to `hardening.txt`.
- Phase 44 (plan 44-18) re-captured, with the user's approval, `initializers.txt`, `initializers-di.txt` and `initializers-discarded.txt` without the section:offset column (C-INIT-ADDR); every other byte equals the previous files. The `CAUSES.md` hash had not been updated after 44-14 and is current again.
- Phase 44 (plan 44-18d) added one row each to `smoke-all-digests.txt` and `offthread.txt` for the new `crash-stall` case (C-CASE-CRASHSTALL); no other row changed.
- Phase 46 (plan 46-05) added one row each to `smoke-all-digests.txt` and `offthread.txt` for the new `crash-delayload` case (C-CASE-CRASHDELAYLOAD); no other row changed.
- Phase 46 (plan 46-07) added the `unicows-staged` line to `hardening.txt` (C-HARDEN-UNICOWS). The `hardening.txt` and `CAUSES.md` hashes had not been updated after 46-01, 46-04 and 46-05 and are current again.
- Phase 46 (plan 46-12) added the `d3dx9-pins` line to `hardening.txt` (C-HARDEN-D3DX9).
- Phase 46 (plan 46-13) added the `steamid-abi` and `steam-api64-pins` lines to `hardening.txt` (C-HARDEN-STEAMID, C-HARDEN-STEAMPIN).
- Phase 47 (plan 47-03) added C-COMBO-BLOCKS to `CAUSES.md`; no table changed.
- Phase 47 (plan 47-04) added C-CASE-CRASHEARLY: `smoke-all-digests.txt` and `offthread.txt` gained the `crash-early` and `crash-heapinit` rows, and C-CASE-PARTICLESOUND: both gained the `crash-particlesound` row; no other row moved.
- Phase 47 (plan 47-09) added `mem-peaks-x64.txt`, the x64 Release `-Smoke All -HogAddrSpace` peaks the x64 heap sizes are derived from, and C-HEAPCAP-X64-RULE, -Application, -Lua, -Runtime and -KEPT to `CAUSES.md`; no x86 table changed.
- Phase 48 (plan 48-01) added C-X64-FMA3 to `CAUSES.md`; no table changed.
- Phase 48 (plan 48-03) added C-X64-D3DX to `CAUSES.md`; no table changed.
- Phase 48 (plan 48-04) updated C-X64-D3DX in `CAUSES.md` (MatrixInverse and Vec4Transform ported); no table changed.
- Phase 48 (plan 48-06) updated C-X64-D3DX in `CAUSES.md` (RotationX/Y/Z transcribed in ml64); no table changed.
- Phase 48 (plan 48-07) updated C-X64-D3DX in `CAUSES.md` (RotationAxis transcribed in ml64, the `d3dx-imports` row); no table changed.
- Phase 48 (plan 48-08) added C-X64-X87TRIG to `CAUSES.md` (the x86 CRT trig fallback transcribed in ml64 for x64); no table changed.
- Phase 48 (plan 48-10) added C-X64-CRTF to `CAUSES.md` (the CRT float route and the `crt-float-imports` and `crt-trig-route` rows); no table changed.
- Phase 48 (plan 48-11) added C-X64-FMOD and C-X64-NANOP to `CAUSES.md` and the sim math corpus to C-X64-CRTF; no table changed.
- Phase 48 (plan 48-12) named the x64 Smoke guard rows D3DX-LINK-X64S and CRTF-X64S in C-X64-D3DX and C-X64-CRTF; no table changed.
- Phase 48 (plan 48-13) added C-X64-PATHROLL to `CAUSES.md` (the path follower roll target, x87 multiply add on x64); no table changed.
- Phase 48 (plan 48-14) added C-X64-CONVOYDIST and C-X64-CYLRAY to `CAUSES.md` (the convoy follow distance, x87 add multiply on x64, and the cylinder ray cap re-entry, x87 multiply add on x64); no table changed.
- Phase 48 (plan 48-16) added `x87-inventory.tsv`, the x87 inventory of the x86 Release exe (D-12), generated by `x87-inventory.ps1` from the 74bbc43d game build; no other file changed.
- Phase 48 (plan 48-17) filled the verdict and evidence of 175 open rows of `x87-inventory.tsv` by hand (`match:callret`, `match:crt-convert`, `fixed:C-X64-*` and `offpath:` reasons outside the rule names); 39 rows stay open as x86/x64 gaps for gap-closure plans; no other file changed.
- Phase 48 (plan 48-21) added C-X64-SACHEAT, C-X64-BARRAGE, C-X64-JETJUMP, C-X64-SOLDACTION, C-X64-FLYERRAY, C-X64-TARGETPRIO, C-X64-IDLEBOB, C-X64-FLYERTRICK, C-X64-DPS and C-X64-RESERVE to `CAUSES.md` (x87 multiply add, multiply subtract and add multiply on x64) and set 12 open rows of `x87-inventory.tsv` to `fixed:C-X64-*` or `match:exhaustive`; no other table changed.
- Phase 48 (plan 48-22) added C-X64-SNIPEHINT, C-X64-COMBATRANGE, C-X64-SPAWNTIME, C-X64-ADRENALINE, C-X64-CATAPULT and C-X64-VISIONDIST to `CAUSES.md` (x87 add add, sub sub, sub div, mul div and add mul add chains and two unrounded returns on x64) and set 11 open rows of `x87-inventory.tsv` to `fixed:C-X64-*`, `match:callret` or `match:exhaustive`; no other table changed.
- Phase 48 (plan 48-23) added C-X64-BOARDRANGE, C-X64-DOT3 and C-X64-LIGHTFMOD to `CAUSES.md` (an x87 add add, the three-term dot and the fmod route on x64) and set 12 open rows of `x87-inventory.tsv` to `fixed:C-X64-*`, `match:callret` or `match:exhaustive`; no other table changed.
- Phase 48 (plan 48-24) added C-X64-CANNONSPREAD and C-X64-TERRAINRAY to `CAUSES.md` (the cannon spread chain and the terrain ray recursion at x87 precision on x64) and set the last 4 open rows of `x87-inventory.tsv` to `fixed:C-X64-*`; no other table changed.
- Phase 48 (plan 48-18) re-captured `mem-peaks-x64.txt` after digest parity, with pool keys equal to `mem-peaks.txt`, and added C-MEMPEAKS-X64-PARITY to `CAUSES.md`; no x86 table changed.
- Phase 48 (plan 48-25) added the `host-fault-capture` and `all-checkpoints` lines to `hardening.txt` (C-HOSTFAULT-CHECKPOINT); no other table changed.
- Phase 48 (plan 48-26) added the `crash-particleburst` row to `smoke-all-digests.txt` and `offthread.txt` (C-CASE-PARTICLEBURST); no other row moved.
- Phase 48 (plan 48-19) updated C-X64-X87TRIG (`errno` for +-inf on x64), C-X64-CRTF and C-X64-FMOD (the fmod two-NaN tie compared on the same machine) in `CAUSES.md`; no table changed.
- Phase 49 (plan 49-01) moved `mem-peaks-x64.txt`, byte for byte, to `tests/baseline/v143-x64/mem-peaks.txt` (C-X64-BASELINE-FOLDER in `tests/baseline/v143-x64/CAUSES.md`); no other file changed.
- Phase 49 (plan 49-05) added C-X86-NO-COMMIT-TABLE to `CAUSES.md`: x86 gets no `commit.txt` and keeps its self-relative `commit-flat` rule; no table changed.
- Phase 49 (plan 49-07) added `lua-hist.txt`, the Test Lua demand histogram per 4-byte bin, and C-LUA-HIST to `CAUSES.md`; no other table changed.
- Phase 49 (plan 49-09) added C-SAVE-LIGHTUD-WIDTH to `CAUSES.md`: Lua save items write and skip a light userdata at 4 bytes on both builds; no table changed.
- Phase 49 (plan 49-13) marked C-HEAPCAP-X64-Lua and C-HEAPCAP-X64-Runtime as superseded by C-HEAPCAP-X64-Lua-RETUNE and pointed C-MEMPEAKS-X64-PARITY at the re-captured x64 table (both in `tests/baseline/v143-x64/CAUSES.md`); no table changed.
- Phase 49 (plan 49-16) added `digest-trace-dag1c_ctf.txt` and `digest-trace-kas2c_uber.txt`, the per-turn references for the digest smoke cases of those missions, from two byte-identical Win32 `-Collect` captures (an x64 capture gives the same bytes). Every other `-Collect` file equals its pinned copy byte for byte.
- Phase 49 (plan 49-17) added C-DIGEST-TRACE-49 to `CAUSES.md`, the cause of those two traces; no table changed.
- Phase 51 (plan 51-01) added C-FFMPEG-VENDOR-SCOPE and C-FFMPEG-ALLOC-CONFIG to `CAUSES.md`; no table changed.
- Phase 51 (plan 51-02) added C-FFMPEG-WD4146, C-FFMPEG-WD4703, C-FFMPEG-DEBUG-O1 and C-FFMPEG-BCRYPT to `CAUSES.md` and corrected the source count in C-FFMPEG-VENDOR-SCOPE to 179; no table changed.
- Phase 51 (plan 51-03) added the `ffmpeg-license`, `ffmpeg-flags`, `ffmpeg-dll`, `ffmpeg-imports` and `ffmpeg-crt-heap` lines to `hardening.txt` (C-FFMPEG-HARDENING-ROWS) and C-FFMPEG-ASAN-IMPORTS to `CAUSES.md`; no other table changed.
- Phase 51 (plan 51-04) added `movie-goldens.txt`, the binkw32 frame hashes and facts captured before the FFmpeg switch (C-MOVIE-GOLDENS), and C-MOVIE-GOLDENS to `CAUSES.md`; no other table changed.
- Phase 51 (plan 51-05) added the `## colour` section to `movie-goldens.txt`, the exact binkw32 colour model the FFmpeg player implements, and its paragraph to C-MOVIE-GOLDENS in `CAUSES.md`; no other table changed.
- Phase 51 (plan 51-06) added the `## binkw32 ingame digest` section to `movie-goldens.txt`, the rendered fixed-rate `nab2g_c` digest the FFmpeg player must reproduce (MOVIE-09), and C-MOVIE-INGAME-DIGEST to `CAUSES.md`; no other table changed.
- Phase 51 (plan 51-07) added C-MOVIE-HEADER-CAPS to `CAUSES.md`, the Bink header caps and their budget; no table changed.
- Phase 51 (plan 51-10) added `movie-frames.txt`, the exact FFmpeg-path frame pins shared by Win32 and x64, each equal to its binkw32 golden (C-MOVIE-FRAMES), and C-MOVIE-FRAMES to `CAUSES.md`; no other table changed.
- Phase 51 (plan 51-12) added C-FFMPEG-TX-LEAK to `CAUSES.md`, the second local FFmpeg patch; no table changed.
- Phase 51 (plan 51-15) added the `bink-gone` line to `hardening.txt` (C-BINK-GONE-ROW); no other table changed.
- Phase 51 (plan 51-17) added C-FFMPEG-CONFIG-FIXED to `CAUSES.md`, the fixed configure date and compiler in each FFmpeg `config.h`; no table changed.
- Phase 51 (plan 51-17) extended C-BINK-GONE-ROW in `CAUSES.md` with the binkw32 and binkw64 name rule for game source; no table changed.
- Phase 51 (plan 51-17) extended C-MOVIE-HEADER-CAPS in `CAUSES.md` with the frame rate, largest frame and frame span bounds and the exact clock compare; no table changed.
- Phase 51 (plan 51-17) added C-MOVIE-HWGAIN to `CAUSES.md`, the movie renderer gain flag and the played-byte count; no table changed.
- Phase 51 (plan 51-17) lowered the movie frame cap to 16000 and counted the movie audio in C-MOVIE-HEADER-CAPS in `CAUSES.md`; no table changed.
- Phase 51 (plan 51-18) added C-X87-INVENTORY-RESYNC to `CAUSES.md` and three `match:exact` rows to `x87-inventory.tsv` (`FinishSend` and `Remove`, which the inventory lost where dumpbin decoded through a switch table, and the new `ConvertAudioToS16`), regenerated with `-Out` over the pinned file from the 51-18 Release build; no other row and no other table changed.
- `capture-vs2013.ps1 -Verify -Baseline v143` checks this manifest. `-Archive` assumes the VS2013 build identity and is not used for this directory.
- `compare-tables.ps1` holds the reader of every file here, and the `baseline-readers` hardening check fails on a file with no reader.
