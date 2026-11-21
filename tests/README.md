# OpenSWBF2 tests

Headless doctest 1.2.9 harness, built with the same VS2022 `v143` toolset as the game.

## Build

    MSBuild tests\OpenSWBF2Tests.vcxproj /p:Configuration=Debug /p:Platform=Win32

Output is `bin\Tests\<Platform>\<Config>\OpenSWBF2Tests.exe`. Configurations are `Debug`, `Release`, `Test` (Release plus `OPENSWBF2_TEST`) and `Asan` (Test plus `/fsanitize=address`).

## Requirements

- VS2022 with the x86 C++ tools, Windows SDK 10.0.26100.0 and DirectX SDK June 2010. `tests\vstools.ps1` finds the toolchain and exits 2 if it is missing.
- In `C:\Windows\SysWOW64`: `vcruntime140.dll` and `msvcp140.dll`, plus `d3dx9_43.dll` for the tests exe only; game runs load the `d3dx9_43.dll` the build stages beside the exe. Win32 smoke runs outside the Vendor group also need `msvcr120.dll` and `msvcp120.dll`, because they expect Galaxy on; x64 runs do not. Without them, `vendor-present` and `vendor-vcrt-off` skip.
- Smoke runs need retail GameData (`SWBF2_GAMEDATA` or the Steam install). Without it, those cases skip.

## Run

    tests\run.ps1

This builds the tests, then runs each case in its own process and the Standard smoke run. Results go to `artifacts\tests\<timestamp>\`.

| Switch | Meaning |
|--------|---------|
| `-Configuration` | `Debug` (default), `Release`, `Test` or `Asan` |
| `-Asan` | same as `-Configuration Asan` |
| `-Platform` | `Win32` (default) or `x64` |
| `-NoBuild` | skip the build |
| `-Suite` | run one suite |
| `-Filter` | wildcard on the case name |
| `-Fuzz`, `-Seed`, `-Iterations` | run the `stress` suite with a seed and iteration count |
| `-Smoke` | `None`, `Standard` (default), `Twice`, `Dedicated`, `Crash`, `All`, `Perturb`, `Soak`, `Vendor` or `Boot` |
| `-SmokeConfiguration` | game build used for smoke runs (default `Release`, or `Test`/`Asan` to match `-Configuration`) |
| `-Soak` | `Short` (default, turn 9300) or `Long` (turn 617300): where `-Smoke All` ends its `TimeOut` missions |
| `-Jobs N` | parallel smoke workers (default: half the logical processors, at most 8) |
| `-Movies` | run with the renderer instead of `-norender` |
| `-HogAddrSpace` | force the first allocator region above 2 GB |
| `-FactoryLog` | pass `-factorylog` to every smoke case, so each run logs its `OPENSWBF2 FACTORY` counts |
| `-MiPurge` | pass `-mipurge <ms>` to every smoke case; the expected `OPENSWBF2 MIOPTIONS` line then carries `purge_delay=<ms>` |
| `-Hardening` | run only the hardening checks |
| `-Clean` | delete the staged output folders and any junctions left under `artifacts` safely, and exit |
| `-Rejudge <runDir>` | judge an archived run folder again with today's expectations, without building or starting the game; writes `results-rejudge.txt` and fails any case with no archived output |

`run.ps1` exits 0 when everything passes, 1 on any failure and 2 on a setup error. Each case prints one of `[PASS]`, `[FAIL]`, `[SKIP]`, `[STALL]`, `[CRASH]` or `[KNOWN]`.

Smoke cases have no time limit. Every case runs with `-hangtimeout 0` (except `crash-hang`, which tests the game's own 5 s watchdog), and a case fails as `STALL` only after 60 s with no new turn and no load progress, counted in time the game was not waiting for a CPU. The game is first asked to write its own hang dump; it is killed only if it is still running 60 s later. Unit test cases (T1) wait the same way: each case and each fuzz or corpus item counts as progress, and the tests exe answers the stall request by printing `[STALL] <case> <item>` and exiting 124.

## Suites

`unit`, `characterisation`, `regression`, `known_bug`, `corpus`, `fuzz` and `stress`. `stress` runs only when named. `known_bug` cases document bugs that still exist. They print `[KNOWN]` and never fail the run.

Every `TEST_CASE` needs `* doctest::test_suite("<suite>")`. Names must be unique and must not contain `,`, `*` or `?`.

A failing fuzz case prints its seed and iteration. Replay it on the test exe with `--fuzz-seed=<n> --fuzz-iter=<i> --fuzz-dump --fuzz-out=<path>`.

## Smoke runs

Smoke runs launch the real `BattlefrontII.exe` headless. Each case writes `run.log`, `stdout.txt` and `stderr.txt` (the first and last MiB of each stream), `finalscore.log` and any dump to `artifacts\tests\<timestamp>\smoke\<case>\`, and the run writes its switches, exe hashes and WER state to `run-params.txt`.

Every case runs in a clean worker folder under `artifacts\smoke-workers\<Platform>\<Config>\smoke\w<N>`, cleared before each case, because the exe reads `display.ini`, `SaveGames` and chat files from its own folder. Before each case the worker's DLLs are copied again from the build folder, then the case's `Omit` (file names to remove) and `Swap` (target name to a read-only source path) are applied to that worker copy only. A case whose `NeedsFile` path or `Swap` source is missing prints `[SKIP]` with its `SkipReason` and does not start. Two runs at once on one configuration need disjoint worker ranges: `-WorkerBase <n>` starts a run at worker n+1, and every run must stay within the logical processor count. Run `tests\firewall.ps1` once from an elevated PowerShell to allow every worker path on the local subnet and clear stale rules; `-List` previews the changes and `-Root` takes more trees.

| Group | What it runs |
|-------|--------------|
| `Standard` | geo1c_con, `heapcap`, the display-switch cases, the digest cases and `novictory-con`; the display-switch cases that check only start-up lines end at turn 1 (`-soakturns 1`); Test and Asan builds add `repeat-water` (`nab2g_c -repeatmission`, both turn-1500 digests equal); Asan adds `asan-hot1g_con` and `asan-spa3c_ass` |
| `Twice` | the same missions twice with a fixed seed; the results and digest traces must match |
| `Dedicated` | dedicated-server cases |
| `Crash` | deliberate faults through `-crashtest <kind>`, including `crash-capexhaust`, `pool-full`, `pool-full-strict`, `crash-stall` (a supervised hang the harness reports and the game dumps), `crash-delayload` and its x64 twin `crash-delayload-x64` (a Steam call with `steam_api.dll` or `steam_api64.dll` absent; the FATAL line names the DLL), `crash-early` and `crash-heapinit` (faults before the command line is parsed and during heap start-up); each must leave the expected exit code, log line and dump; on x64 `crash-nullderef` runs under `-hogaddrspace` and its dump's exception address, module and offset must equal the FATAL line's |
| `All` | every mission in `tests\smoke\missions-all.txt` (`<mission> [<class>]`), plus Dedicated and Crash; a Release run then compares digests, mem-peaks, pool-sizes and CLOTH exactly against the tables in `tests\baseline\v143` through `compare-tables.ps1` and fails on any moved row; a Test run compares the Lua pool bins, spill counts included, against `lua-bins.txt` and the main section of `offthread.txt` (source thread ids ignored) |
| `Perturb` | Test or Asan only: seven digest missions, each as a base run, under `-perturbheap` pad/fill/pool (one seed per mission, 1 and 2 alternating down the list), `-hogaddrspace` and `-repeatmission`; each run is traced for the whole mission (nab2g_c, which never ends, to turn 100000) and every turn must equal its base line for line; writes `perturb.tsv` |
| `Soak` | `asan-soak`: nab2g_c run to turn 815800 with `-soakturns`, for the long ASan run; no other group holds it |
| `Vendor` | start-up runs with every vendor DLL present, or with one removed or swapped for one of the wrong bitness; each service turned off must log exactly one `OPENSWBF2 VENDOR <service> off` Warning; Win32 runs pass at turn 1, x64 runs end at the `-crashtest fastfail` carrier right after the probe; `vendor-wan-nogalaxy` starts an online dedicated server without `Galaxy.dll` and must exit 1 with a FATAL line naming `-lan` |
| `Boot` | headless boot: on both platforms the shell, ended after 300 updates (x64 under `-hogaddrspace`); on x64 also hot1g_con, tat2g_eli and nab2g_c to turn 1500 under `-hogaddrspace` |

A case fails on any Error, Bug or Fatal line in its `run.log` unless a regex in `tests\smoke\expected-errors.txt` covers it. In T1, declare an expected error with `ExpectLog("text")` or `ExpectEnsure("text")`.

Import `tools\wer-localdumps.reg` once as administrator, so that a `/GS` fast-fail still leaves a dump.

`run-params.txt` records the host's hardware-fault event counts (`whea_before`, `whea_after`, `whea_delta`); a failing run with new events is marked `Suspected hardware:` but still fails. All mission runs carry `-digesttrace 1500`.

### Sim digest

At turn 1500 the game logs `OPENSWBF2 SIM DIGEST ...`, a hash of the simulation state. `tests\smoke\digest-expected.txt` holds the expected line for each digest case, and Release and Test must match it exactly. Change a row only with a written cause in `tests\baseline\v143\CAUSES.md`.

`-digesttrace N` logs the hash for every turn up to N. `-factorylog` logs the Factory creation order.

`-novictory` makes `MissionVictory` and `MissionDefeat` do nothing, so a mission runs until the watchdog, and logs `OPENSWBF2 NOVICTORY`. It has no effect in a network game and logs one Warning there. `novictory-con` checks that geo1c_con is still running at turn 9300, past its CON end at 9014, and still running at turn 18500, past turn 18452 where its losing team runs out of reinforcements; it ends there with `-soakturns 18500`. `All` runs every `_1flag`, `_ctf` and `_ass` mission without a digest row with `-novictory` and marks it `TimeOut`; such a line marked `Pass` is rejected. A `TimeOut` mission never ends by itself, so `All` ends it with `-soakturns` at the `-Soak` count and requires its turn-1500 digest and `SOAK` lines.

`-perturbheap <seed>[:pad,fill,pool]` (Test and Asan builds only) pads, 0xA5-fills or shuffles heap and pool blocks, and the order freed pool blocks are reused, from a seeded generator; with no mode list all three are on.

`-repeatmission` (Test and Asan builds only, with `-exitongameover`) sends the first mission back to the shell after its turn-1500 digest and plays it again; the second run logs its own digest and exits. Without `-exitongameover` it logs a warning and is ignored. If the first run ends before turn 1500, it logs `OPENSWBF2 REPEAT skipped` and exits after that run. `-repeatmission N` with N from 1 to 16 replays the mission N times, so `commit-flat` (N = 16) plays it 17 times.

`-shellfirst` (Test and Asan builds only) starts in the shell and enters the named mission on the first shell update, the path a player takes from the menu. The Test and Asan Standard group runs it as `shell-first` with its own digest row, and run.ps1 checks that its Runtime heap at turn 1500, and that of the repeated mission in `repeat-digest`, equal the `geo1c_con` case.

At every state cleanup, Test and Asan builds scan static data and the heaps that outlive a mission for weak handles (`PblHandle`) that still point at an object in the mission heaps, alive or destroyed, and log `OPENSWBF2 HANDLESCAN state=<mission|shell> live=<n> dead=<n>`. Handle ids restart at every mission entry, so such a handle could match a new object; `repeat-digest` and `shell-first` require both counts to be 0.

When a state has ended and the next one builds its heaps, the game logs `OPENSWBF2 COMMIT state=<mission|shell> bytes=<n>` for the state that ended: the process commit charge (`PagefileUsage`) after the old heaps are destroyed and trimmed, so it counts only what outlives the state. Test builds follow it with `OPENSWBF2 COMMITPARTS state=<mission|shell> private=<n> gamecrt=<n> heaps=<n> threads=<n> threadcount=<n> arena=<n> crtbusy=<n> crtcommitted=<n> crtlarge=<n>`: the committed private bytes in the address space, the live bytes the game holds on the CRT heap (every game CRT heap call goes through `PblCrtHeap`, enforced by the `crt-heap-ban` hardening check), the committed bytes of every process heap, the committed stack and TEB pages of every thread, the thread count, the committed bytes of the mimalloc arena, and the CRT process heap figures for diagnosis. `commit-flat` splits each row into the arena and the rest (`private - heaps - arena - threads + gamecrt`): the Windows heaps keep freed pages committed, the OS starts thread-pool workers on its own timers, and the commit charge also holds page tables that follow the address layout, so only the game's own bytes are counted. It fails when either part of a mission or shell row in cycles 9 to 16 is above that part's highest value for the same kind in cycles 1 to 8.

`-exitonmovieclose` (Test and Asan builds only, with `-exitongameover`) ends the run once the mission is running and a Bink movie has closed (`OPENSWBF2 MOVIE close`). The Test Standard group runs `campaign-movies`, `nab2g_c` with rendering and sound on and this switch, which must log `OPENSWBF2 MOVIE open`, `OPENSWBF2 MOVIE close` and, in the exit report, `OPENSWBF2 MEMREPORT offthread count=0 sources=0 dropped=0`.

`-digestdetail <turn>` (Test and Asan builds only) logs `OPENSWBF2 DETAIL turn=<t> ...` lines for that turn and the next two: the mission time, then each character's, team's, command post's and world object's fields that the sim digest hashes, floats as raw bits. It does not change the digest. The Win32 Test Standard group runs it as `digest-detail` on geo1c_con, which must keep the geo1c_con digest.

Other builds skip `-perturbheap`, its value, `-repeatmission`, `-shellfirst`, `-exitonmovieclose` and `-digestdetail` with its value, and log `-<switch> needs a Test or Asan build, ignored`.

x86 and x64 builds compute the same headless simulation and log the same digests. The join handshake has no field that identifies the build or its bitness, so a host accepts a peer of the other platform without a check. Mixed x86 and x64 network play is not supported until the protocol carries a build field and a two-peer x86/x64 test exists.

### Game switches

- `-heapcap <Name>=<bytes>[K|M]` raises a named heap's cap.
- `-mipurge <ms>` sets the mimalloc purge delay.
- `-poolerrors` makes a pool that overflows and grows an Error.
- `-soakturns <n>` (n from 1) ends the mission at turn n of its last segment, logs `OPENSWBF2 SOAK turns=<n>` and exits normally. A case that ends this way requires its verdict line and the `SOAK` line. Every smoke case ends by its own output: game over, a crash, or `-soakturns`. A case expected to end by itself carries the work cap `-soakturns 617300` and fails if it reaches it.

## ASan

    tests\run.ps1 -Asan
    tests\run.ps1 -Asan -Smoke Dedicated
    tests\run.ps1 -Asan -Smoke Soak
    tests\run.ps1 -Asan -NoBuild -Smoke Crash
    tests\selftest.ps1 -Configuration Asan

ASan covers the stack, globals, the CRT heap, and the game's heaps and pools. Each allocator poisons free memory and unpoisons exactly the bytes requested, so overflows and use after free inside a pool report as `use-after-poison`. Any `asan.*` report file, or exit code 66, fails the case.

Smoke cases that crash on purpose (`$script:SmokeAsanExcluded` in `smoke\cases.ps1`) are skipped under ASan.

Decide whether a handled object is alive with `PblHandle::IsLive()` or `GetObjectA()`, never by reading the object. The `handle-raw` check enforces this.

## CI

`.github/workflows/tests.yml` runs on a self-hosted `[self-hosted, windows, v143]` runner. The `test` job runs `run.ps1 -Configuration Test` and `selftest.ps1`. The `asan` job runs `run.ps1 -Asan`, `-Asan -Smoke Dedicated` and `selftest.ps1 -Configuration Asan`. The `x64` job builds the x64 Test exe and tests, runs `run.ps1 -Platform x64 -Configuration Test -Smoke None -Hardening` and `luac-compare.ps1`; it never starts the game exe. To set up the runner:

1. Install VS2022, the Windows SDK and DirectX SDK June 2010 (`DXSDK_DIR` set at machine level).
2. Run `config.cmd --labels v143`.
3. Run it as a normal low-privilege user, with no secrets on the machine.
4. Require approval for external contributors' workflows.
5. The runner needs Steam running with the overlay off for app 6060.

A weekly and on-demand `full` job calls the scripts directly, not `gate.ps1`: Release `-Smoke All -Soak Long`, Test `-Smoke Perturb` and `-Smoke All -Soak Short`, `guard-revert.ps1 -Full`, `run.ps1 -Asan`, `-Asan -Smoke Soak` and `-Asan -Smoke Dedicated`, and `x64-census.ps1`. Every job's `timeout-minutes` is 7200, the 5-day limit of a self-hosted job, so no job ends on elapsed time; a hung case ends through the stall detector.

`tests\selftest.ps1` checks the runner scripts themselves against stub executables. It also checks the game and tests exes of Debug and of its `-Configuration`, so both must be built.

## Staged output

`bin\Win32\<Config>\data` contains junctions into the retail GameData. Anything that follows them can delete the retail install. Remove staged output only with `tests\run.ps1 -Clean`. Any extra worktree that builds the game must set `SWBF2_NOSTAGE=true`. Worker folders and worktrees are removed only through `Remove-HarnessTree` in `smoke.ps1`, which unlinks a worker's own `data` junction and refuses any other junction; never force-remove a worktree.

x64 builds take their vendor DLLs from the folder named by `SWBF2_VENDOR_X64` (see the top-level README) and stage nothing when it is unset. `unicows.dll` is no longer staged; staging deletes a stale copy from the output folder.

## Hardening

The hardening step checks the exe's security flags (ASLR, DEP, LAA, SafeSEH, `/GS`, `/sdl`), its imports (`import-modules` and `import-delay-modules` check the static and delay import tables against per-platform lists in `smoke\cases.ps1`; on x64 the import rows run while the other binary checks wait for Phase 52), that `unicows.dll` is not staged, listed as a staging or worker file, imported, or named under `src` (`unicows-staged`), that the hashes in `lib\vendor-pins.props` are well formed and the `StageVendor` target checks the D3DX package and each platform's `d3dx9_43.dll` against them and extracts the DLL again whenever the cached copy's hash differs (`d3dx9-pins`), that the `steam_api64.dll` pin is well formed and `StageVendor` checks the file against it before every copy (`steam-api64-pins`), on x64 that the staged `steam_api64.dll` has the pinned hash and exports every function the exe delay-imports from it and every name in `lib\x64\steam_api64.def` (`steam-api64-staged`, SKIP when not staged), the required and banned compiler flags in the exe and every library project, the `static.c` mimalloc defines for each configuration, that mimalloc's CRT hooks sit in `.CRT$XIB`/`.CRT$XLB`/`.CRT$XLY` in the map (`mimalloc-crt-hooks`), and these source patterns:

| Check | Fails on |
|-------|----------|
| `new-size-t` | `operator new`/`delete` with a 32-bit size type, in `src` or `tests` |
| `win32-callbacks` | casts to Win32 callback types, or window procs with the wrong signature |
| `galaxy-vtable` | hand-written vtable math under `src\Steam` |
| `fastcall-edx` | dummy EDX parameters on `__fastcall` functions, not in `$script:FastcallAllow` (`smoke\cases.ps1`) |
| `d3dx-decl` | hand-written D3DX declarations, outside the x64 port's `pcD3dxLinkX64.cpp` |
| `char-spacer` | byte-array spacers not in `$script:SpacerAllow` (`smoke\cases.ps1`) |
| `offset-hack` | raw byte-offset object access not in `$script:OffsetAllow` (`smoke\cases.ps1`) |
| `serial-value` | a read of a thread or collision serial counter or field outside the helper definitions in `$script:SerialAllow` (`smoke\cases.ps1`) |
| `handle-raw` | a raw handle id read, `PBL_HANDLE_ID`, a freed-fill (`0xDDDDDDDD`) compare or an object-vs-saved id compare outside `PblHandle.h`/`.cpp` |
| `gamma-get` | any `GetDeviceGammaRamp` call |
| `mi-api-ban` | a mimalloc API call outside `src\mimalloc` and `RedMemoryMi.cpp` |
| `snd-alloc-size-t` | `SndAlloc`, `SndTempAlloc` or a sound allocator function pointer with a 32-bit size type |
| `galaxy-getter-cast` | a Galaxy getter returning `void*`, or a cast or `void*` temporary on a Galaxy getter's result |
| `galaxy-factory-seam` | a `GalaxyFactory::` call outside `src\Steam\gfVendor.cpp` |
| `steamid-abi` | a call of an `ISteamUser` method that returns or takes `CSteamID`, or a by-value `CSteamID` outside `src\Steam\steamworks.h`; the x64 convention for them is not settled |
| `t1-suite` | a test case that `run.ps1` would never run |
| `addr-order` | a cast of `this` or a pointer to an integer, `char*` or `void*` for ordering, a pointer cast to an integer as an `Add(`/`Move(` key, a pointer relational compare, `std::less`/`std::greater` on a pointer type, or `PblPtrOrderKey`/`PblPtrAfter`, under `src\Battlefront2` and `src\PebbleFL`, outside `$script:AddrOrderAllow` (`smoke\cases.ps1`); pointers are found by name (`this`, `p` and `pp` prefixes), so a pointer named otherwise is not caught |

When the game and tests configurations are both Release, `compare-tables.ps1 -Hardening` compares the hardening lines with `baseline\v143\hardening.txt` (`tables:hardening`).

A `-Smoke Vendor` run also prints `vendor:d3dx9-staged`: it passes when the staged `d3dx9_43.dll` beside the exe has the platform's pinned hash, fails when it differs or is missing, and skips when the output folder was not staged. It is not a hardening line, so a Release hardening run without staging still matches the pinned table.

## Gate tiers

| Tier | When | Command | Steps |
|------|------|---------|-------|
| Per plan | every change | `tests\gate.ps1 -Tier plan -Platform Win32,x64` | Debug, Release and Test builds on both platforms; `run.ps1` (Debug T1, hardening, Release Standard smoke); Release `-Smoke Vendor`; the Release Boot group; x64 Test T1; `selftest.ps1 -Configuration Test`; narrow `guard-revert.ps1 -Narrow` (rows added or edited, or whose revert target or test file changed since their proof) |
| Pull request | CI `test` and `asan` jobs | `tests\gate.ps1 -Tier pr [-Job test\|asan]` | test: Debug and Test builds, `run.ps1 -Configuration Test`, `selftest.ps1 -Configuration Test`; asan: Debug and Asan builds, `run.ps1 -Asan`, `-Asan -Smoke Dedicated`, `selftest.ps1 -Configuration Asan` |
| Branch close | before merging a branch | `tests\gate.ps1 -Tier branch -Platform Win32,x64` | all four builds; `run.ps1`; Release `-Smoke Twice`, `-Smoke All` (x64 under `-HogAddrSpace` too) and `-Smoke Vendor`; Release `-Smoke Boot` and Asan `-Smoke Boot` (x64); the Crash group on x64; `selftest.ps1 -Configuration Test`; selective `guard-revert.ps1`; x64 Test and Asan T1 with the tlog hardening; `luac-compare.ps1`; the x87 inventory check |
| Milestone close | before a release | `tests\gate.ps1 -Tier milestone -Platform Win32,x64` | all four builds; `run.ps1`; Release Twice, All (x64 under `-HogAddrSpace` too) and Vendor; Release Boot and Asan Boot (x64); Test default, Perturb and All at the short count; Asan default, Dedicated and Soak; both self-tests; `capture-vs2013.ps1 -Collect`, `-Map`, `-Layouts`, Debug `-DebugDigests` and `-Verify -Baseline v143` with their compares; `x64-census.ps1`; selective `guard-revert.ps1`; x64 Test and Asan T1 with the tlog hardening; `luac-compare.ps1`; the x87 inventory check |

Plan, PR and branch runs, and every Test All run, end `TimeOut` missions at turn 9300 (`-Soak Short`), so the soak past turn 9300 is proven only in milestone and weekly runs, by Release All at the long count (`-Soak Long`, turn 617300) and the ASan `Soak` group. `guard-revert.ps1 -Full` is a manual run. x64 steps build, run T1 and the tlog checks, and start the game exe for the Vendor group, which exits right after the vendor DLL probe, and for the Boot and Crash groups, plus Release All at branch and milestone; `-Platform` defaults to `Win32`. Wall times per step, before and after, are in `tests\baseline\gate-times.tsv`; a full milestone run writes `after_s`.

    tests\gate.ps1 -Tier plan|pr|branch|milestone [-Platform Win32,x64] [-Step <names>] [-Resume] [-ListOrder]

Before any step the gate checks the machine and exits 2 if any check fails: Steam running and logged in, the Steam overlay off for app 6060, an inbound allow rule for every game exe path the tier can use (`tests\firewall.ps1`), the WER LocalDumps key and DontShowUI, free memory and disk space, and the build tools and runtimes. Builds run first, then the other steps from shortest to longest by `gate-times.tsv` (steps with no time first); `-ListOrder` prints the order and exits.

`gate.ps1` runs a tier's steps from `tests\gate\tiers.ps1`. A plan re-runs every step whose key changed. A step is skipped only when `tests\guardrevert\steps.tsv` holds a record with the same keys and its archived output (under `OPENSWBF2_EVIDENCE_DIR`, default `%LOCALAPPDATA%\OpenSWBF2\evidence`) still passes; when only an expectation file changed, the archived output is judged again without running the step. Records are written only from a clean tree. `-Step` runs the named steps and the builds they need, and `-Job` only the steps of that CI job and the steps with no job; both report the tier `partial` and do not count as a tier run; `-Resume` continues the last run from its ledger.

## Guard revert

    powershell -File tests\guard-revert.ps1 [-Row ID,...] [-Narrow] [-Configuration Test] [-Workers N]

This proves the regression tests catch their bugs. For each row in `tests\guardrevert\rows.ps1`, it reverts one fix in a worktree, rebuilds and runs the named cases. With the fix (`Control`), every row must be GREEN. With the fix reverted, every row must be RED. STILL-GREEN means the test does not detect the revert. A smoke row whose case skips (a needed file is absent) prints `SKIP <reason>`, records no proof and fails the run as unproven. A smoke row with `Config` builds and runs that game configuration instead of Release. The table is saved to `artifacts\guard-revert\<stamp>\summary.txt`.

By default it re-runs only the rows whose dependencies changed since their recorded proof in `tests\guardrevert\proven.tsv`. `-Full` runs every row, and `-ListSelected` prints the decision for each row without building. Dependencies come from the main tree's tlogs, so its Debug and Asan test builds and its Release game build must be current, or the affected rows are re-run. A `CARRY` line names the commit where the proof was recorded. That commit can be local-only and later squashed away; the proof rests on its tree and a digest of the row's dependency contents. `-Platform` (default `Win32`) picks the build paths, and a proof only carries on the platform it was recorded on. `-Narrow` selects only rows that were added or edited, or whose revert target or test file changed since the commit of their proof; it ignores global triggers and other dependency changes, prints the other stale rows as `SKIP` and records nothing for them, so they stay stale until a branch or milestone run proves them. It cannot be combined with `-Full`.

Rows run in parallel on `-Workers` workers (default: logical processors divided by `-BuildJobs`, the per-worker `msbuild /m`). Worker N keeps its own worktree at `<WorktreeRoot>\<Platform>\wN` (default root `<repo>-worktrees`, never under TEMP), built with `SWBF2_NOSTAGE=true` and reused with `checkout --force` and `clean -fd`, so its `obj` and `bin` build incrementally from run to run; `-WorkerBase` shifts its smoke worker folders like `run.ps1`.

`-Resume <folder>` continues an interrupted run from that folder's `ledger.tsv`; give it the same row arguments, and it skips only the rows already passed under the same row, dependency, toolset, tree, configuration and platform keys.

## Other tools

- `tests\x64-census.ps1 -Name census-N` rebuilds the solution for x64 in Release, Debug, Test and Asan and lists every pointer-truncation and format warning site. The output goes to `tests\baseline\census`.
- `tests\luac-compare.ps1 [-Configuration Release] [-Platform Win32]` builds LuaCompiler for Win32 and x64 and fails unless both write the same bytes for every indexed script, and for the four lvls when GameData is found.
- `tests\laa-compare.ps1` compares runs with and without `/LARGEADDRESSAWARE`.
- `tests\baseline\x87-inventory.ps1 -Exe <x86 Release exe> -Map <map> -Out <tsv>` lists every function of the x86 Release exe that runs x87 code, with its flags, an address-free pattern and a verdict. `-Check tests\baseline\v143\x87-inventory.tsv -Exe <exe> -Map <map>` fails on a new, changed or removed function or on a row with no verdict.
- `tests\baseline\capture-vs2013.ps1` captured the VS2013 reference before the move to v143.
- `tests\baseline\compare-tables.ps1 -Tables <dir>` compares captured digests, mem-peaks, pool-sizes and CLOTH exactly against `tests\baseline\v143`; `-LuaBins` and `-OffThread` compare those tables instead. `-Collect`, `-Map` and `-DebugDigests` compare the output of `capture-vs2013.ps1 -Collect`, `-Map` and `-DebugDigests` byte for byte. Its reader map names the reader of every file in `tests\baseline\v143`, and the `baseline-readers` hardening check fails on a file with no reader.
- `tests\baseline\capture-vs2013.ps1 -Verify -Baseline v143` checks `tests\baseline\v143\MANIFEST.md`.
- `tests\baseline\repin.ps1 -RunDir <runDir> -Table <table> -Key <key>` copies named rows from a run into the pinned file when the run's exe hash equals the current exe's. Each row needs a `row: <table> <key>` line in a `CAUSES.md` entry, and a re-pin waits for the user's approval.
- `selftest.ps1`, `laa-compare.ps1`, `capture-vs2013.ps1` and `soundon.ps1` take `-Platform` like `run.ps1`.
- `tests\corpus\README.md` describes the float-parse and math corpora.
