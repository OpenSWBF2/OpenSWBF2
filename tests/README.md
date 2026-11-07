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
| `-Smoke` | `None`, `Standard` (default), `Twice`, `Dedicated`, `Crash`, `All` or `Perturb` |
| `-SmokeConfiguration` | game build used for smoke runs (default `Release`, or `Test`/`Asan` to match `-Configuration`) |
| `-SmokeTimeout` | per-case seconds for smoke runs (default 120) |
| `-Jobs N` | parallel smoke workers (default: half the logical processors, at most 8) |
| `-Movies` | run with the renderer instead of `-norender` |
| `-HogAddrSpace` | force the arena above 2 GB |
| `-SimOrder` | pass `-simorder address` or `-simorder serial` to every smoke case (the game defaults to `address`); every case that is expected to pass or time out must log `OPENSWBF2 SIMORDER mode=<mode>` and no `-simorder` warning |
| `-FactoryLog` | pass `-factorylog` to every smoke case, so each run logs its `OPENSWBF2 FACTORY` counts |
| `-Hardening` | run only the hardening checks |
| `-Clean` | delete the staged output folders and any junctions left under `artifacts` safely, and exit |

`run.ps1` exits 0 when everything passes, 1 on any failure and 2 on a setup error. Each case prints one of `[PASS]`, `[FAIL]`, `[SKIP]`, `[TIMEOUT]`, `[CRASH]` or `[KNOWN]`.

## Suites

`unit`, `characterisation`, `regression`, `known_bug`, `corpus`, `fuzz` and `stress`. `stress` runs only when named. `known_bug` cases document bugs that still exist. They print `[KNOWN]` and never fail the run.

Every `TEST_CASE` needs `* doctest::test_suite("<suite>")`. Names must be unique and must not contain `,`, `*` or `?`.

A failing fuzz case prints its seed and iteration. Replay it on the test exe with `--fuzz-seed=<n> --fuzz-iter=<i> --fuzz-dump --fuzz-out=<path>`.

## Smoke runs

Smoke runs launch the real `BattlefrontII.exe` headless. Each case writes `run.log`, `finalscore.log` and any dump to `artifacts\tests\<timestamp>\smoke\<case>\`.

Parallel workers run from fixed paths under `artifacts\smoke-workers\<Config>\`. Run `tests\firewall.ps1` once from an elevated PowerShell to allow them on the local subnet and clear stale rules; `-List` previews the changes.

| Group | What it runs |
|-------|--------------|
| `Standard` | geo1c_con, plus the display-switch cases and the digest cases |
| `Twice` | the same missions twice with a fixed seed; the results and digest traces must match |
| `Dedicated` | dedicated-server cases |
| `Crash` | deliberate faults through `-crashtest <kind>`; each must leave the expected exit code, log line and dump |
| `All` | every mission in `tests\smoke\missions-all.txt` (`<mission> [<class>] [<seconds>]`), plus Dedicated and Crash |
| `Perturb` | Test or Asan only: six digest missions, each as a base run and under `-perturbheap` pad/top/fill/pool (seeds 1 and 2) and `-hogaddrspace`; passes when at least one run splits from its base, and writes `perturb.tsv` |

A case fails on any Error, Bug or Fatal line in its `run.log` unless a regex in `tests\smoke\expected-errors.txt` covers it. In T1, declare an expected error with `ExpectLog("text")` or `ExpectEnsure("text")`.

Import `tools\wer-localdumps.reg` once as administrator, so that a `/GS` fast-fail still leaves a dump.

### Sim digest

At turn 1500 the game logs `OPENSWBF2 SIM DIGEST ...`, a hash of the simulation state. `tests\smoke\digest-expected.txt` holds the expected line for each digest case, and Release and Test must match it exactly. Change a row only with a written cause in `tests\baseline\v143\CAUSES.md`.

`-digesttrace N` logs the hash for every turn up to N. `-factorylog` logs the Factory creation order.

`-perturbheap <seed>[:pad,top,fill,pool]` (Test and Asan builds only) pads, top-allocates, 0xA5-fills or shuffles heap and pool blocks from a seeded generator; with no mode list all four are on.

`-repeatmission` (Test and Asan builds only, with `-exitongameover`) sends the first mission back to the shell after its turn-1500 digest and plays it again; the second run logs its own digest and exits. Without `-exitongameover` it logs a warning and is ignored. If the first run ends before turn 1500, it logs `OPENSWBF2 REPEAT skipped` and exits after that run.

`-shellfirst` (Test and Asan builds only) starts in the shell and enters the named mission on the first shell update, the path a player takes from the menu. The Test and Asan Standard group runs it as `shell-first` with its own digest row, and run.ps1 checks that its Runtime heap at turn 1500, and that of the repeated mission in `repeat-digest`, equal the `geo1c_con` case.

At every state cleanup, Test and Asan builds scan static data and the heaps that outlive a mission for weak handles (`PblHandle`) that still point at an object in the mission heaps, alive or destroyed, and log `OPENSWBF2 HANDLESCAN state=<mission|shell> live=<n> dead=<n>`. Handle ids restart at every mission entry, so such a handle could match a new object; `repeat-digest` and `shell-first` require both counts to be 0.

Other builds skip `-perturbheap`, its value, `-repeatmission` and `-shellfirst`, and log `-<switch> needs a Test or Asan build, ignored`.

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

A weekly and on-demand `full` job runs the milestone tier from the gate tiers table.

`tests\selftest.ps1` checks the runner scripts themselves against stub executables.

## Staged output

`bin\Win32\<Config>\data` contains junctions into the retail GameData. Anything that follows them can delete the retail install. Remove staged output only with `tests\run.ps1 -Clean`. Any extra worktree that builds the game must set `SWBF2_NOSTAGE=true`. Never `git worktree remove --force` a tree that was ever staged.

## Hardening

The hardening step checks the exe's security flags (ASLR, DEP, LAA, SafeSEH, `/GS`, `/sdl`), the required and banned compiler flags, and these source patterns:

| Check | Fails on |
|-------|----------|
| `new-size-t` | `operator new`/`delete` with a 32-bit size type |
| `win32-callbacks` | casts to Win32 callback types, or window procs with the wrong signature |
| `galaxy-vtable` | hand-written vtable math under `src\Steam` |
| `fastcall-edx` | dummy EDX parameters on `__fastcall` functions, not in `$script:FastcallAllow` (`smoke.ps1`) |
| `d3dx-decl` | hand-written D3DX declarations |
| `char-spacer` | byte-array spacers not in `$script:SpacerAllow` (`smoke.ps1`) |
| `offset-hack` | raw byte-offset object access not in `$script:OffsetAllow` (`smoke.ps1`) |
| `serial-value` | a read of a thread or collision serial counter or field outside the helper definitions in `$script:SerialAllow` (`smoke.ps1`) |
| `gamma-get` | any `GetDeviceGammaRamp` call |
| `snd-alloc-size-t` | `SndAlloc`, `SndTempAlloc` or a sound allocator function pointer with a 32-bit size type |
| `galaxy-getter-cast` | a Galaxy getter returning `void*`, or a cast or `void*` temporary on a Galaxy getter's result |
| `t1-suite` | a test case that `run.ps1` would never run |

## Gate tiers

| Tier | When | Gates |
|------|------|-------|
| Per plan | every change | build the configs the change touches; `tests\run.ps1` (T1, hardening and flag checks, Standard smoke); `tests\guard-revert.ps1` (selective) |
| Branch close | before merging a branch | Debug, Release, Test and Asan builds; `run.ps1 -Smoke None`, `-Smoke Standard`, `-Smoke Twice`, `-Smoke Dedicated`; selective `guard-revert.ps1`; the wire-order and baseline tools when net or sim code changed |
| Milestone close or weekly CI | before a release, and the `full` CI job | `run.ps1 -Smoke All`; `guard-revert.ps1 -Full`; `run.ps1 -Asan`, `-Asan -Smoke Twice`, `-Asan -Smoke Dedicated`; `x64-census.ps1` |

## Guard revert

    powershell -File tests\guard-revert.ps1 [-Row ID,...] [-Configuration Test] [-Keep]

This proves the regression tests catch their bugs. For each row in `tests\guardrevert\rows.ps1`, it reverts one fix in a scratch worktree, rebuilds and runs the named cases. With the fix (`Control`), every row must be GREEN. With the fix reverted, every row must be RED. STILL-GREEN means the test does not detect the revert. The table is saved to `artifacts\guard-revert\<stamp>\summary.txt`.

By default it re-runs only the rows whose dependencies changed since their recorded proof in `tests\guardrevert\proven.tsv`. `-Full` runs every row, and `-ListSelected` prints the decision for each row without building. Dependencies come from the main tree's tlogs, so its Debug and Asan test builds and its Release game build must be current, or the affected rows are re-run. A `CARRY` line names the commit where the proof was recorded. That commit can be local-only and later squashed away; the proof rests on its tree and a digest of the row's dependency contents.

## Other tools

- `tests\x64-census.ps1 -Name census-N` counts x64 warnings and errors on scratch copies of the projects. The output goes to `tests\baseline\census`.
- `tests\laa-compare.ps1` compares runs with and without `/LARGEADDRESSAWARE`.
- `tests\baseline\capture-vs2013.ps1` captured the VS2013 reference before the move to v143.
- `tests\corpus\README.md` describes the float-parse and math corpora.
