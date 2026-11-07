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
| `CAUSES.md` | 3ED73E66C3D1CB029FAFEA275D08CEC7AD10383D5ADC1218A9A83E3E4E9D14B9 | written by hand from the comparison against `tests/baseline/vs2013/` |
| `smoke-all.txt` | 0C44E698C621B0852ADC8E089744C27DDBCBF1BDFF71B2C2BC6C6D8C0BC26352 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `smoke-all-digests.txt` | 745F41BA50A0A720C6D172B6A69AFBF05BAE007A8EABEAA420C784FBA9B63393 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `pool-sizes.txt` | D43CFD8DB6E464EC432559DAFB4F6034C6557D89C5CC6769A88DCF7909F3B860 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `mem-peaks.txt` | 7E04AB6350B36202C3A057CCFC4447A38A4227BD6A70AF1A16DEDE4692AD1865 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `cloth-headroom.txt` | F292282F69E75FBCFC543367ACC2A850DE351F9F32B22B792CED55D1A0E98349 | `capture-vs2013.ps1 -Tables` from `run.ps1 -Smoke All` |
| `hardening.txt` | 79E6FB275A17F199CFE178DC989447A1CB3A97870486BE10F5A1B5B31C7EDC7F | `capture-vs2013.ps1 -Tables` from `run.ps1 -Hardening` |
| `guard-revert.txt` | 725BE60CAA91CF091EE68813224D78C5F153CE99BF3A89CE82ACF633BF38DEC5 | `capture-vs2013.ps1 -Tables` from `guard-revert.ps1` |
| `offthread.txt` | 994F3F31C1A458C5EF9C1530AE0E53033D6D63345519A72FD7B91BACDF9F4108 | `capture-vs2013.ps1 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Smoke All -SmokeConfiguration Test` and `-OffThreadSoundDir` from `soundon.ps1` |
| `perturb.txt` | 2FE99AD131838277AFE814F730B57928843A245AC293ADA77837352235B6ADB1 | `capture-vs2013.ps1 -Configuration Test -Tables -PerturbDir` from `run.ps1 -Configuration Test -Smoke Perturb` |
| `lua-bins.txt` | 5B111D9E6F4290A74835015729CFDE0C4D6BDC210705A16DB808FB995D9EA24C | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All -SimOrder address` |
| `lua-bins-serial.txt` | A48C431BF90B98270C7E4CB40C36CB659569EEEADD212DE6E81676414788728C | `capture-vs2013.ps1 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Configuration Test -Smoke All -SimOrder serial` |
| `factorylog-serial.txt` | 55695BA5257F11BE8AB1D5D72C405B34F96E485E852AEC45541DED36058F4146 | `capture-vs2013.ps1 -Configuration Test -Tables -FactoryAddressDir -FactorySerialDir` from `run.ps1 -Configuration Test -Smoke All -SimOrder address|serial -FactoryLog` |
| `digests-debug.txt` | D285E11C8F109D5C586D948E8F2A653B2EDD89D7367838A74D6EC10BA3B1DB44 | `capture-vs2013.ps1 -DebugDigests` from `run.ps1 -Smoke Twice -SmokeConfiguration Debug` |
| `digests.txt` | A6C3F1C777CDB1F84A3C932A6932EA5721C4B8898E72A49024C606866EC76E3D | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digests-hog.txt` | A6C3F1C777CDB1F84A3C932A6932EA5721C4B8898E72A49024C606866EC76E3D | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-geo1c_con.txt` | A3EA86CB6038FAB58178D3549E67E8EF8675347EE1DFD194233F280A6A0EF6F5 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-hot1g_con.txt` | C22AA71DB3546F113343377DFA6786B14E19D82B9817F510E54EC74CBF67C594 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-nab2g_c.txt` | F7B2B8670597008821CA60FE2CFF9C7F51C95D2FE42E51B7DB8D5C96953FCBDA | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
| `digest-trace-spa3c_ass.txt` | 47EB12CE384D249E5FC947E37DC8C5C13F6F89A95A1DD899B687503434AC8186 | `capture-vs2013.ps1 -Collect -OutDir tests\baseline\v143` |
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
- `perturb.txt` comes from the Test configuration, because `-perturbheap` exists only in Test and Asan builds. Its rule is inverted under `-simorder address`, where at least one run must split or crash, and Phase 36 expects every row equal under `serial`; `top` and `fill` draw no random numbers, so their seed 1 and seed 2 rows repeat the same run as a repeatability control. It was re-captured after `Combo` stopped indexing pool arrays from their first block, so `pool` rows record a split turn where they used to record `crash class=Fatal`.
- `lua-bins.txt` is the DATA-04 before table: Test configuration, because the Lua spill counter exists only in Test and Asan builds, in address mode, taken on the commit named in its header, before any Lua change of this phase; peak equals count on saturated bins, so the spill column is the demand signal for MPOOL-06.
- `factorylog-serial.txt` is the LHASH-05 gate: every `OPENSWBF2 FACTORY` line of two Test `-Smoke All -factorylog` runs, one per `-simorder` mode, on the insertion-order tree, both COUNTS lines included, compared byte for byte per mission; unlike `perturb.txt` it has no inverted rule, every row must be `equal` or carry a cause, and FLIP-02 cannot land while any row carries one.
- `lua-bins-serial.txt` is the DATA-04 after table: the serial run above, on the tree where the Lua `Node` grew from 20 to 28 bytes and `Table` from 32 to 40, so the shift comes from the node vectors and tables moving bins and not from a header byte; same format as `lua-bins.txt`, so the two compare bin by bin.
- `capture-vs2013.ps1 -Archive` and `-Verify` assume the VS2013 build identity and are not used for this directory.
