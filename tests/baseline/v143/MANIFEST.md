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
| `CAUSES.md` | 6FA30C08C6DD36BD8CB070E5B14E27B808B7C64BF80CC2960E628AA0EB962EA5 | written by hand from the comparison against `tests/baseline/vs2013/` |
| `smoke-all.txt` | A94678D9E3FBB949D2648962DB4C3AEAE4541EEFB30F4DF3B12D86C619D9C652 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `smoke-all-digests.txt` | 0A6E0546AAA5D7B70E124192489268287C602ABF5942A5A3B508921314474619 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `pool-sizes.txt` | A2FA679AC1174C9082D1E3DE216F82FC16B5E20B35A41C09AC4F081CF9643D72 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks.txt` | C4272EC9C9E5F53647EE5B420D191C0BB58A0DA846046582E5DD00EB1CC3856B | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `cloth-headroom.txt` | 1A92A09CFF1E03A20D71E547932EF4D3CF23BCE142ACE90A839EC81BB5CE1A09 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `cloth-slack.txt` | B1FA9A9BEFAFEED5840D2070582F133E1F24E2D5FFF4084A97D210455D2FB35B | written by hand from the 37-01 Test probe (Runtime slack at the CLOTH point) |
| `hardening.txt` | 79E6FB275A17F199CFE178DC989447A1CB3A97870486BE10F5A1B5B31C7EDC7F | `capture-vs2013.ps1 -Tables` from `run.ps1 -Hardening` |
| `guard-revert.txt` | EDD7FF20F4D5F3B6CA0ADDCBE2E81DEB383888053973A0251E663DFD1E8D7062 | `capture-vs2013.ps1 -Tables` from `guard-revert.ps1` |
| `offthread.txt` | 994F3F31C1A458C5EF9C1530AE0E53033D6D63345519A72FD7B91BACDF9F4108 | `capture-vs2013.ps1 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Smoke All -SmokeConfiguration Test` and `-OffThreadSoundDir` from `soundon.ps1` |
| `perturb.txt` | 349B1946ECBBEE178419DF3E408A0F1DFA260E6F28A43117C3046FBC5F29D005 | `capture-vs2013.ps1 -Configuration Test -Tables -PerturbDir` from `run.ps1 -Configuration Test -Smoke Perturb` |
| `lua-bins.txt` | AC45F5CE4B2835058B306EFB3E9AEEE51BC1CA681B8FFE8993380EF17FD1B678 | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All` |
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
| `initializers.txt` | 32C8EA5C0CC23B85CCA7EEE44C766D2EE5FAB1967FC0008767064278F60999BD | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-di.txt` | 61BF7A2C9A6AFD5C5D7B14C3AF6D41B8930E5C7EB93E1B9FD3CB406C1A85B1FE | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |
| `initializers-discarded.txt` | E7EB9AA0EF23D33DAC7A5732DBA8C8EA20DF8FA88F2EA85C95E669EF3D30CDB6 | `capture-vs2013.ps1 -Map -OutDir tests\baseline\v143` |

## Notes

- Release is the reference configuration. The binaries are not in the repo.
- Every file was captured or re-checked on commit eff2850, whose game exe is the a2c980c build with the 20-12 and 20-14 fixes (eff2850 changes only tests). Release exe SHA-256 `17A3A864356C23D254EA6A302DA0FF591A3013B0F5135A18F64E99E130A71885`. `smoke-all*.txt`, `hardening.txt` and `guard-revert.txt` were regenerated there; `smoke-all-digests.txt` came out byte-identical. `digest-trace-hot1g_con.txt` was regenerated on a381f01. `digests.txt`, `digests-hog.txt`, `digests-debug.txt`, the other three digest traces and the four factory logs were captured on 1b1cbca/dce2810, and all ten, with `digest-trace-hot1g_con.txt`, are byte-identical when re-captured on eff2850. The `initializers*.txt` files come from dce2810.
- The pool size, memory peak and cloth headroom tables name their commit on their first line and cover the missions in `tests/smoke/missions-all.txt`. The memory peak table keeps the latest MEMREPORT line for each heap and pool. A pool freed before the `exit` report still has an `exit` row, with count 0 and its peak.
- `smoke-all-digests.txt` row `dedicated` is a real-time run, so its `state=`, `time=` and `rand=` vary between runs.
- `offthread.txt` comes from the Test configuration, because the off-thread counter exists only in Test and Asan builds; every other table here except `perturb.txt` is Release. Its `## sound-on` section holds geo1c_xl, kas2c_xl and hot1g_xl run with the renderer and sound on by `tests\baseline\soundon.ps1`, which also writes each case's `sound.txt`. `sound=opened` means `AudioSes.dll` was loaded in the game process; a `-nosound` control run never loads it, and `dsound.dll` is a static import, so it shows nothing.
- The serial-order flip re-captured `digests*.txt`, the four digest traces, `smoke-all*.txt`, `pool-sizes.txt`, `mem-peaks.txt`, `cloth-headroom.txt`, `perturb.txt` and `lua-bins.txt` once, under the `serial` default, in the commit that made it the default; a table that names its commit names that commit's parent, because the capture ran on the uncommitted flip. The four factory logs came out byte-identical, so the address and serial Factory orders are equal. `offthread.txt` is unchanged: its main section re-captured byte-identical under `serial`.
- `perturb.txt` comes from the Test configuration, because `-perturbheap` exists only in Test and Asan builds. It is the serial table: 66 rows, the six base runs and 60 perturbed and repeat runs, every one `equal`. The address table, where at least one run must split or crash, is in git history and in `33-PERTURB-RESULTS.md`. `top` and `fill` draw no random numbers, so their seed 1 and seed 2 rows repeat the same run as a repeatability control.
- `lua-bins.txt` is the serial Lua bin table: Test configuration, because the Lua spill counter exists only in Test and Asan builds; peak equals count on saturated bins, so the spill column is the demand signal for MPOOL-06. The DATA-04 address-against-serial comparison, taken where the Lua `Node` grew from 20 to 28 bytes and `Table` from 32 to 40, is in git history.
- `capture-vs2013.ps1 -Archive` and `-Verify` assume the VS2013 build identity and are not used for this directory.
