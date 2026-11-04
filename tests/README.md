# OpenSWBF2 tests

Headless doctest 1.2.9 harness, built with the same VS2022 `v143` toolset as the game.

## Build

    MSBuild tests\OpenSWBF2Tests.vcxproj /p:Configuration=Debug /p:Platform=Win32

Output is `bin\Tests\<Config>\OpenSWBF2Tests.exe`. Configurations are `Debug`, `Release`, `Test` (Release plus `OPENSWBF2_TEST`) and `Asan` (Test plus `/fsanitize=address`).

## Requirements

- VS2022 with the x86 C++ tools, Windows SDK 10.0.26100.0 and DirectX SDK June 2010. `tests\vstools.ps1` finds the toolchain and exits 2 if it is missing.
- In `C:\Windows\SysWOW64`: `d3dx9_43.dll`, `vcruntime140.dll` and `msvcp140.dll`. Smoke runs also need `msvcr120.dll` and `msvcp120.dll`.
- Smoke runs need retail GameData (`SWBF2_GAMEDATA` or the Steam install). Without it, those cases skip.

## Run

    tests\run.ps1

This builds the tests, then runs each case in its own process and the Standard smoke run. Results go to `artifacts\tests\<timestamp>\`.

| Switch | Meaning |
|--------|---------|
| `-Configuration` | `Debug` (default), `Release`, `Test` or `Asan` |
| `-Asan` | same as `-Configuration Asan` |
| `-NoBuild` | skip the build |
| `-Suite` | run one suite |
| `-Filter` | wildcard on the case name |
| `-Fuzz`, `-Seed`, `-Iterations` | run the `stress` suite with a seed and iteration count |
| `-Timeout` | per-case seconds (default 10) |
| `-Smoke` | `None`, `Standard` (default), `Twice`, `Dedicated`, `Crash` or `All` |
| `-SmokeConfiguration` | game build used for smoke runs (default `Release`, or `Test`/`Asan` to match `-Configuration`) |
| `-SmokeTimeout` | per-case seconds for smoke runs (default 120) |
| `-Jobs N` | parallel smoke workers (default: half the logical processors, at most 8) |
| `-Movies` | run with the renderer instead of `-norender` |
| `-HogAddrSpace` | force the arena above 2 GB |
| `-Hardening` | run only the hardening checks |
| `-Clean` | delete the staged output folders safely and exit |

`run.ps1` exits 0 when everything passes, 1 on any failure and 2 on a setup error. Each case prints one of `[PASS]`, `[FAIL]`, `[SKIP]`, `[TIMEOUT]`, `[CRASH]` or `[KNOWN]`.

## Suites

`unit`, `characterisation`, `regression`, `known_bug`, `corpus`, `fuzz` and `stress`. `stress` runs only when named. `known_bug` cases document bugs that still exist. They print `[KNOWN]` and never fail the run.

Every `TEST_CASE` needs `* doctest::test_suite("<suite>")`. Names must be unique and must not contain `,`, `*` or `?`.

A failing fuzz case prints its seed and iteration. Replay it on the test exe with `--fuzz-seed=<n> --fuzz-iter=<i> --fuzz-dump --fuzz-out=<path>`.

## Smoke runs

Smoke runs launch the real `BattlefrontII.exe` headless. Each case writes `run.log`, `finalscore.log` and any dump to `artifacts\tests\<timestamp>\smoke\<case>\`.

| Group | What it runs |
|-------|--------------|
| `Standard` | geo1c_con, plus the display-switch cases and the digest cases |
| `Twice` | the same missions twice with a fixed seed; the results and digest traces must match |
| `Dedicated` | dedicated-server cases |
| `Crash` | deliberate faults through `-crashtest <kind>`; each must leave the expected exit code, log line and dump |
| `All` | every mission in `tests\smoke\missions-all.txt`, plus Dedicated and Crash |

A case fails on any Error, Bug or Fatal line in its `run.log` unless a regex in `tests\smoke\expected-errors.txt` covers it. In T1, declare an expected error with `ExpectLog("text")` or `ExpectEnsure("text")`.

Import `tools\wer-localdumps.reg` once as administrator, so that a `/GS` fast-fail still leaves a dump.

### Sim digest

At turn 1500 the game logs `OPENSWBF2 SIM DIGEST ...`, a hash of the simulation state. `tests\smoke\digest-expected.txt` holds the expected line for each digest case, and Release and Test must match it exactly. Change a row only with a written cause in `tests\baseline\v143\CAUSES.md`.

`-digesttrace N` logs the hash for every turn up to N. `-factorylog` logs the Factory creation order.

## ASan

    tests\run.ps1 -Asan
    tests\run.ps1 -Asan -Smoke Twice
    tests\run.ps1 -Asan -Smoke Dedicated
    tests\run.ps1 -Asan -NoBuild -Smoke Crash
    tests\selftest.ps1 -Configuration Asan

ASan covers the stack, globals, the CRT heap, and the game's heaps and pools. Each allocator poisons free memory and unpoisons exactly the bytes requested, so overflows and use after free inside a pool report as `use-after-poison`. Any `asan.*` report file, or exit code 66, fails the case.

Smoke cases that crash on purpose (`$script:SmokeAsanExcluded` in `smoke.ps1`) are skipped under ASan. Smoke timeouts are scaled by 15.

Weak handles read the id of an object that may already be freed, so write handle liveness checks with `PBL_HANDLE_ID(p)`.

## CI

`.github/workflows/tests.yml` runs on a self-hosted `[self-hosted, windows, v143]` runner. The `test` job runs `run.ps1 -Configuration Test` and `selftest.ps1`. The `asan` job runs the ASan commands above. To set up the runner:

1. Install VS2022, the Windows SDK and DirectX SDK June 2010 (`DXSDK_DIR` set at machine level).
2. Run `config.cmd --labels v143`.
3. Run it as a normal low-privilege user, with no secrets on the machine.
4. Require approval for external contributors' workflows.

`tests\selftest.ps1` checks the runner scripts themselves against stub executables.

## Staged output

`bin\Win32\<Config>\data` contains junctions into the retail GameData. Anything that follows them can delete the retail install. Remove staged output only with `tests\run.ps1 -Clean`. Any extra worktree that builds the game must set `SWBF2_NOSTAGE=true`. Never `git worktree remove --force` a tree that was ever staged.

## Hardening

The hardening step checks the exe's security flags (ASLR, DEP, LAA, SafeSEH, `/GS`, `/sdl`), the required and banned compiler flags, and these source patterns:

| Check | Fails on |
|-------|----------|
| `new-size-t` | `operator new`/`delete` with a 32-bit size type, not in `$script:NewSizeAllow` (`smoke.ps1`) |
| `win32-callbacks` | casts to Win32 callback types, or window procs with the wrong signature |
| `galaxy-vtable` | hand-written vtable math under `src\Steam` |
| `fastcall-edx` | dummy EDX parameters on `__fastcall` functions, not in `$script:FastcallAllow` (`smoke.ps1`) |
| `d3dx-decl` | hand-written D3DX declarations |
| `char-spacer` | byte-array spacers not in `$script:SpacerAllow` (`smoke.ps1`) |
| `offset-hack` | raw byte-offset object access not in `$script:OffsetAllow` (`smoke.ps1`) |
| `gamma-get` | any `GetDeviceGammaRamp` call |
| `t1-suite` | a test case that `run.ps1` would never run |

## Guard revert

    powershell -File tests\guard-revert.ps1 [-Row ID,...] [-Configuration Test] [-Keep]

This proves the regression tests catch their bugs. For each row in `tests\guardrevert\rows.ps1`, it reverts one fix in a scratch worktree, rebuilds and runs the named cases. With the fix (`Control`), every row must be GREEN. With the fix reverted, every row must be RED. STILL-GREEN means the test does not detect the revert. The table is saved to `artifacts\guard-revert\<stamp>\summary.txt`.

## Other tools

- `tests\x64-census.ps1 -Name census-N` counts x64 warnings and errors on scratch copies of the projects. The output goes to `tests\baseline\census`.
- `tests\laa-compare.ps1` compares runs with and without `/LARGEADDRESSAWARE`.
- `tests\baseline\capture-vs2013.ps1` captured the VS2013 reference before the move to v143.
- `tests\corpus\README.md` describes the float-parse and math corpora.
