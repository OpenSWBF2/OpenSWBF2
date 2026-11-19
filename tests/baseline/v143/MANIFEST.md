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
| `CAUSES.md` | 40894BE10F76B13FBFA5196E1A9F49529DEE234B72AB82AA4E05AAE972AA79E9 | written by hand from the comparison against `tests/baseline/vs2013/` |
| `smoke-all-digests.txt` | 8D0250F595A13012B400A0C104844C675791F0D1413E5D42E2F4DE7BECAA3E35 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `pool-sizes.txt` | A2FA679AC1174C9082D1E3DE216F82FC16B5E20B35A41C09AC4F081CF9643D72 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks.txt` | D24B076240DC0D686B07C7F1D33EF273F88E928C863E9C11878061FF3A7D41B0 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks-x64.txt` | 76D87EF85DA1685C3D5DD2C975CA1444BC2B9912A46F2DEBC790157D61EA8DA0 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Platform x64 -Configuration Release -Smoke All -HogAddrSpace` |
| `cloth-headroom.txt` | 7C388EDBA9ADC4CF1D82A8C66FD7409CAF534B023E630AB59F9844312104F720 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `hardening.txt` | 1A67C5F6F2F98B8BEB1867B3831446998C4D34F8DA5F2859605E241B8A6664D5 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Hardening` |
| `offthread.txt` | 8DB5C6AC2EFA4531BE0618584D35A3A21CB6FBD0E12809421BD5ED69AAD40BC4 | `capture-vs2013.ps1 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Smoke All -SmokeConfiguration Test` and `-OffThreadSoundDir` from `soundon.ps1` |
| `lua-bins.txt` | A6E3FF46D1E8A6D69232061361199590BC6FB0C1AEEF6EFF0A0E19F69DA5936A | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All` |
| `digests-debug.txt` | B0856D73AF662CC495ECC019194E0DE150C71A8BA648B9211A33C29AEB18A122 | `capture-vs2013.ps1 -DebugDigests` from `run.ps1 -Smoke Twice -SmokeConfiguration Debug` |
| `digests.txt` | 9AE18CAC28450E27A9AD9992321734DFAE2BBBFF90C50FAA6EBD99518B89D6B7 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digests-hog.txt` | 9AE18CAC28450E27A9AD9992321734DFAE2BBBFF90C50FAA6EBD99518B89D6B7 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-geo1c_con.txt` | 90874B7E3EDB3B9412E64692CF4038EDA5EC97BE719ED712F0EAB044DFB99C57 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-hot1g_con.txt` | 21509E3C46BDE4C15ED1435FB1639F9E38B291EF070CF0EB3701755002B93994 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-nab2g_c.txt` | D81004C7FD74558721DF792795F156E08D085E6CFFED20F8BB3D78C13080AC21 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-spa3c_ass.txt` | 68AEF8A5CEEF5BB4335436E3EDA69D3E7368EF9C7A0CAB97834885E437A6560F | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-geo1c_con.txt` | 0C3C380156342E1C492092575630152541B648FF038F8B6285A840C6F73D9C7A | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-hot1g_con.txt` | 21739D89813BFA37EC08B53900C0EDE5AD79CA96D8DFDEBAEDE5692127F3E5D6 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-nab2g_c.txt` | 2192336505D7A03D0ABD400927A962AFB6EF09358AB6E572657AC7A4EC0C94E7 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `factory-spa3c_ass.txt` | 0FDB0B17217BBC000279BBCD49AE05681C8ACB199306878FE0AA8E485714CABF | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `initializers.txt` | DFAB8F8E42253C47AA0D88E56845E8E39DBAA531EA828E06A1FD7C74721B5320 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-di.txt` | 2C89B86B8D5836A6069AA6F99237253E30635B9C99FF7A17BCF4AE4D3BAA7D80 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-discarded.txt` | F9656D1101DEA5FE04D1DC5F96876DBAC83954CA710272BBE4D8DCFEED4BCCA0 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |

## Notes

- Release is the reference configuration. The binaries are not in the repo.
- Every file was captured or re-checked on commit eff2850, whose game exe is the a2c980c build with the 20-12 and 20-14 fixes (eff2850 changes only tests). Release exe SHA-256 `17A3A864356C23D254EA6A302DA0FF591A3013B0F5135A18F64E99E130A71885`. `hardening.txt` was regenerated there and `smoke-all-digests.txt` came out byte-identical. `digest-trace-hot1g_con.txt` was regenerated on a381f01. `digests.txt`, `digests-hog.txt`, `digests-debug.txt`, the other three digest traces and the four factory logs were captured on 1b1cbca/dce2810, and all ten, with `digest-trace-hot1g_con.txt`, are byte-identical when re-captured on eff2850. The `initializers*.txt` files come from dce2810.
- The pool size, memory peak and cloth headroom tables name their commit on their first line and cover the missions in `tests/smoke/missions-all.txt`. The memory peak table keeps the latest MEMREPORT line for each heap and pool. A pool freed before the `exit` report still has an `exit` row, with count 0 and its peak.
- `smoke-all-digests.txt` row `dedicated` is a real-time run, so its `state=`, `time=` and `rand=` vary between runs.
- `offthread.txt` comes from the Test configuration, because the off-thread counter exists only in Test and Asan builds; every other table here except `lua-bins.txt` is Release. Its `## sound-on` section holds geo1c_xl, kas2c_xl and hot1g_xl run with the renderer and sound on by `tests\baseline\soundon.ps1`, which also writes each case's `sound.txt`. `sound=opened` means `AudioSes.dll` was loaded in the game process; a `-nosound` control run never loads it, and `dsound.dll` is a static import, so it shows nothing.
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
- `capture-vs2013.ps1 -Verify -Baseline v143` checks this manifest. `-Archive` assumes the VS2013 build identity and is not used for this directory.
- `compare-tables.ps1` holds the reader of every file here, and the `baseline-readers` hardening check fails on a file with no reader.
