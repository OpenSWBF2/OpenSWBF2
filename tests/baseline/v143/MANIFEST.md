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
| `CAUSES.md` | FB7DF4D51BCD30E253A145F31C3174CE3E059BF8AA1DB8BFB49F048D92E29A73 | written by hand from the comparison against `tests/baseline/vs2013/` |
| `smoke-all.txt` | D97D3927FBFF9F1609A4F8AF4D5A4ED7E66BC23FA504D0B0130AD07CC5E1B98F | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `smoke-all-digests.txt` | 19CEA63BB35A4F6CD975C5B76BB9A1B90B525F9BC30ABA7AD5CB15BB961FBB81 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `pool-sizes.txt` | A2FA679AC1174C9082D1E3DE216F82FC16B5E20B35A41C09AC4F081CF9643D72 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks.txt` | D24B076240DC0D686B07C7F1D33EF273F88E928C863E9C11878061FF3A7D41B0 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `cloth-headroom.txt` | 7C388EDBA9ADC4CF1D82A8C66FD7409CAF534B023E630AB59F9844312104F720 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `hardening.txt` | E82BC9D9E4809881593B71B254B649C31B2876E2227BD47021E2F939DF09DA6A | `capture-vs2013.ps1 -Tables` from `run.ps1 -Hardening` |
| `guard-revert.txt` | 53DEC42C83223A67019B6A07F431171A1276C468D246077A4CEEBD29483886E9 | `capture-vs2013.ps1 -Tables` from `guard-revert.ps1` |
| `offthread.txt` | 99464E886C366A3DE747B99589A71F3E1DA3A9533BF696DD8A5DD3054D43F6F7 | `capture-vs2013.ps1 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Smoke All -SmokeConfiguration Test` and `-OffThreadSoundDir` from `soundon.ps1` |
| `perturb.txt` | 795F0EAC42D40500FD32D4C32FCA9639A3518D2F30F2616819D9624AAAABFDD0 | `capture-vs2013.ps1 -Configuration Test -Tables -PerturbDir` from `run.ps1 -Configuration Test -Smoke Perturb` |
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
| `initializers.txt` | 184644E824752C8F2FC0D1CA29ADED564988EBAABE3ADA809E5929A750A2409F | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-di.txt` | 82389F9235DD1FFFFF1E96064501FDFCDFD19FE21CB5BA1D3F54E0B140B5E9DD | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-discarded.txt` | 62CCD0A3248EFDA4CCE34E98E8850187AE1618DB20FE583B72B70B327F9499B6 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |

## Notes

- Release is the reference configuration. The binaries are not in the repo.
- Every file was captured or re-checked on commit eff2850, whose game exe is the a2c980c build with the 20-12 and 20-14 fixes (eff2850 changes only tests). Release exe SHA-256 `17A3A864356C23D254EA6A302DA0FF591A3013B0F5135A18F64E99E130A71885`. `smoke-all*.txt`, `hardening.txt` and `guard-revert.txt` were regenerated there; `smoke-all-digests.txt` came out byte-identical. `digest-trace-hot1g_con.txt` was regenerated on a381f01. `digests.txt`, `digests-hog.txt`, `digests-debug.txt`, the other three digest traces and the four factory logs were captured on 1b1cbca/dce2810, and all ten, with `digest-trace-hot1g_con.txt`, are byte-identical when re-captured on eff2850. The `initializers*.txt` files come from dce2810.
- The pool size, memory peak and cloth headroom tables name their commit on their first line and cover the missions in `tests/smoke/missions-all.txt`. The memory peak table keeps the latest MEMREPORT line for each heap and pool. A pool freed before the `exit` report still has an `exit` row, with count 0 and its peak.
- `smoke-all-digests.txt` row `dedicated` is a real-time run, so its `state=`, `time=` and `rand=` vary between runs.
- `offthread.txt` comes from the Test configuration, because the off-thread counter exists only in Test and Asan builds; every other table here except `perturb.txt` is Release. Its `## sound-on` section holds geo1c_xl, kas2c_xl and hot1g_xl run with the renderer and sound on by `tests\baseline\soundon.ps1`, which also writes each case's `sound.txt`. `sound=opened` means `AudioSes.dll` was loaded in the game process; a `-nosound` control run never loads it, and `dsound.dll` is a static import, so it shows nothing.
- The serial-order flip re-captured `digests*.txt`, the four digest traces, `smoke-all*.txt`, `pool-sizes.txt`, `mem-peaks.txt`, `cloth-headroom.txt`, `perturb.txt` and `lua-bins.txt` once, under the `serial` default, in the commit that made it the default; a table that names its commit names that commit's parent, because the capture ran on the uncommitted flip. The four factory logs came out byte-identical, so the address and serial Factory orders are equal. `offthread.txt` is unchanged: its main section re-captured byte-identical under `serial`.
- `perturb.txt` comes from the Test configuration, because `-perturbheap` exists only in Test and Asan builds. It is the serial table: 63 rows, the seven base runs and 56 perturbed and repeat runs, every one `equal` over the whole mission. The 12 `top` rows of the capture were deleted by hand with that mode. The address table, where at least one run must split or crash, is in git history and in `33-PERTURB-RESULTS.md`. `fill` draws no random numbers, so its seed 1 and seed 2 rows repeat the same run as a repeatability control.
- `lua-bins.txt` is the serial Lua bin table: Test configuration, because the Lua spill counter exists only in Test and Asan builds; peak equals count on saturated bins, so the spill column is the demand signal for MPOOL-06. The DATA-04 address-against-serial comparison, taken where the Lua `Node` grew from 20 to 28 bytes and `Table` from 32 to 40, is in git history.
- Phase 41 re-captured `mem-peaks.txt`, `cloth-headroom.txt`, `smoke-all.txt`, `hardening.txt` (Release) and `lua-bins.txt` (Test) from mimalloc runs. `compare-tables.ps1` compares `mem-peaks.txt` (Lua rows included), `cloth-headroom.txt`, `pool-sizes.txt` and `smoke-all-digests.txt` exactly, so the hand-written cloth slack table was deleted.
- The phase 41 code review re-captured `mem-peaks.txt` and `cloth-headroom.txt` once more: `StickInfo` gained a 4-byte stick key, so its pool rows are 48 bytes and every mission's Runtime heap holds 240 more bytes. No other row moved.
- Phase 42 (Gametoast 0c1bbd3e) re-captured, with the user's approval, `smoke-all-digests.txt` (the spa3c_ass row only; the `dedicated` row keeps its earlier value, as it is wall-clock and not compared) and `mem-peaks.txt` from `artifacts/tests/20261112-054020` (C-FLYER-REACT), and `initializers*.txt` from the 277bc98c Release map (C-INIT-ORDER-42), all on game commit 277bc98c.
- Phase 43 (static-library split) re-captured, with the user's approval, `initializers.txt` and `initializers-di.txt` from the 4919bf44 Release map (C-SPLIT-STEAM-INIT), and added the `crash-stickdelete` row to `smoke-all-digests.txt` (the `dedicated` row keeps its pinned value) and the `crt-heap-ban` and `mimalloc-crt-hooks` lines to `hardening.txt` and `smoke-all.txt`. `smoke-all.txt` comes from a Release `-Smoke All` run with the new digest row pinned.
- `capture-vs2013.ps1 -Archive` and `-Verify` assume the VS2013 build identity and are not used for this directory.
