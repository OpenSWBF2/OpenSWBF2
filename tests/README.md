# OpenSWBF2 tests

Headless doctest harness (doctest 1.2.9, vendored). Built with the same VS2013 `v120_xp` toolset as the game.

## Build

    MSBuild tests\OpenSWBF2Tests.vcxproj /p:Configuration=Debug /p:Platform=Win32

Output: `bin\Tests\<Config>\OpenSWBF2Tests.exe`.

## Run

    tests\run.ps1

Builds, then runs every case in its own process. Switches (the only ones):

| Switch | Meaning |
|--------|---------|
| `-Configuration` | `Debug` (default) or `Release` |
| `-NoBuild` | skip the MSBuild step |
| `-Suite` | run one suite (`stress` only runs when named) |
| `-Filter` | wildcard on case name (PowerShell `-like`) |
| `-Fuzz` | run only the `stress` suite, passing seed and count to each case |
| `-Seed` | fuzz seed (default 1) |
| `-Iterations` | fuzz iterations (default 100000) |
| `-Timeout` | per-case seconds (default 10); the runner kills the process after Timeout+10 s |

Results: `artifacts\tests\<timestamp>\results.txt` plus one stdout file per case (`artifacts\` is self-ignored).

## Exit codes and status tokens

Case exe exit codes: 0 pass, 1 fail, 3 harness error (missing input), 77 skip, 124 watchdog timeout, anything else a crash (hex code printed).

Exe prints `[RUN]` at start; the runner prints one of `[PASS] [FAIL] [SKIP] [TIMEOUT] [CRASH] [KNOWN]` per case.
A case that does not report exactly `test cases: 1` with one passed is a FAIL (a zero-match filter cannot go green).
run.ps1 exits 1 on any FAIL/TIMEOUT/CRASH outside known_bug, 2 on setup errors or when zero cases run, else 0.

## Suites

`unit`, `characterisation`, `regression`, `known_bug`, `corpus`, `fuzz`, `stress` (stress is excluded from the default run).
`known_bug` cases document bugs that still exist: their failures print as `[KNOWN]` under a `Known bugs` heading and never fail the run.
A known_bug case that passes prints `WARNING: known_bug case passed, bug may be fixed`.

Test names must be unique and must not contain `,` `*` or `?` (doctest filter syntax).

## GameData tests

Tests that need retail GameData skip cleanly (`[SKIP]`, exit 77) when it is missing. Force the skip path with:

    $env:SWBF2_GAMEDATA='C:\nonexistent'

## Fuzz repro

Failing fuzz cases print their seed and iteration. Replay with
`--fuzz-seed=<n> --fuzz-iter=<i> --fuzz-dump --fuzz-out=<path>` on the test exe.

## Requirements

`C:\Windows\SysWOW64\d3dx9_43.dll` (DirectX June 2010 redist) and `msvcr120.dll` (VC++ 2013 x86 runtime). run.ps1 checks both first.

## CI runner (manual, one-off)

`.github/workflows/tests.yml` runs on push and same-repo PRs on `[self-hosted, windows, v120]`. Register the runner by hand:

1. Machine with VS2013 Update 5 and DirectX SDK June 2010 (`DXSDK_DIR` set at machine level).
2. Download the Actions runner and configure it: `config.cmd --labels v120` (the `self-hosted` and `windows` labels are automatic; label case may matter, keep them lowercase).
3. Run it as a normal low-privilege user, not NETWORK SERVICE (GameData tests skip if the data is unreadable).
4. Keep no secrets on the box.
5. In the repo settings (public repo) set "Require approval for all external contributors" for fork workflows. The workflow also refuses fork PRs by `if:` guard.
tests/selftest.ps1 runs negative checks of run.ps1 against stub executables; it needs no built test exe, only the same preflight DLLs as run.ps1.

## Smoke runs

`run.ps1` also launches the real `BattlefrontII.exe` for smoke runs. A plain `tests\run.ps1` runs the unit stage and then the Standard smoke case (geo1c_con). `-Suite`, `-Filter` and `-Fuzz` skip smoke. Naming `-Smoke` runs only smoke.

The Standard smoke also runs `display-switches` (all eight display switches: `-borderless -adapter 0 -refresh 60 -vsync half -fps 120 -nowaitquery -nodpi`) and `display-badini` (`-adapter abc -refresh 99999 -displayini tests\smoke\display-bad.ini`, a checked-in fixture of 5 KB of mixed bytes plus bad `[Display]` keys). Both must reach RUN COMPLETE; under `-norender` the display settings are parse-only.

| Switch | Meaning |
|--------|---------|
| `-Smoke` | `None`, `Standard` (default), `Dedicated`, `Crash`, `All` (every mission in `tests\smoke\missions.txt`, Dedicated and Crash) or `Twice` (Standard twice with `-seed 1`, the two runs must produce the same `finalscore.log` line apart from the timestamp, reported as `[PASS] smoke:run-twice match` or `[FAIL] smoke:run-twice mismatch`; then `nab2g_c` twice, a mission that never ends, until the `OPENSWBF2 SIM DIGEST` run-log line, which carries the mission time and both random seeds after 1500 turns, matches: `smoke:run-twice-digest`) |
| `-SmokeTimeout` | per-case seconds (default 120); on expiry the process tree is killed with `taskkill /T /F` |
| `-SmokeConfiguration` | `Release` (default) or `Debug`; builds `BattlefrontII.vcxproj` unless `-NoBuild` |
| `-Movies` | drop `-norender` so movies and the renderer run |
| `-HogAddrSpace` | append `-hogaddrspace` to every Standard, Dedicated, All-mission and Twice case so the arena lands above 2 GB, and require the `OPENSWBF2 ADDRSPACE` line with `arena=0x8...` or higher, `laa=1`, `steam=1` and non-zero `steam_api`, `Galaxy` and `binkw32` bases (`GalaxyPeer` is only mapped after an online login, so any value passes). Crash cases are unchanged. Off by default; `-Smoke All -HogAddrSpace` takes about 2 hours |
| `-Jobs N` | number of concurrent smoke workers. The default (0) picks half the logical processors, capped at 8 and at least 1; `-Jobs 1` runs serially. Each worker gets its own run folder under the run's artifacts directory (`smoke-workers\wN`) with a copied exe and DLLs and a `data` junction to the staged `bin\Win32\<Config>\data`. Output and results stay in case order. Twice cases and the aslr-base probe stay serial, and a hung case is killed by process tree for that case only. Headless (`-norender`) runs use a message-only window and take no single-instance mutex, which is what lets workers coexist |

Cases run one at a time with the exe directory as the working directory. Without retail GameData, or without the exe and the Bink, Galaxy and Steam DLLs beside it, every selected case prints `[SKIP] ... class=Skipped` and the game is not built. A skipped run exits 0.

Artifacts go to `artifacts\tests\<timestamp>\smoke\<case>\`: `run.log`, `finalscore.log`, any `.dmp` and `result.txt`.

| Class | Meaning | Token |
|-------|---------|-------|
| Skipped | precheck failed | SKIP |
| TimeOut | killed at the timeout | TIMEOUT |
| GsFailure | exit `0xC0000409` | CRASH |
| Fatal | exit 1, 3, 4 or 5, a `FATAL:` log line, or a dump | CRASH |
| Pass | exit 0 and `OPENSWBF2 RUN COMPLETE` in the log, no dump | PASS |
| InitFailure | exit 0 without the marker | FAIL |
| Unknown | anything else | FAIL |

Classes are decided in that order. Crash cases pass when the expected class, exit code and dump count are observed.

Exit code 4 is a Lua panic (an uncaught Lua error). Exit code 5 is a hang: the watchdog saw no frame update for `-hangtimeout` seconds, wrote a dump and a `FATAL: hang` line. The `-crashtest` kinds are `nullderef`, `divzero`, `stackoverflow`, `debugbreak`, `abort`, `invalidparam`, `fastfail`, `arenaoom`, `debugheapoom`, `heapfail`, `luaoom`, `luapanic`, `hang` (spins in the mission loop; the smoke case passes `-hangtimeout 5`) and `check` (calls `ensure(false)` then `check(false)`: exit 3, one dump, `FATAL: check failed`, `ensure summary: 1 failures at 1 sites`).

The reset kinds script the headless fake device (`-norender` only; a real device is never touched). `resetinitlost`: the first `Init` gets `DEVICELOST` from `Reset` three times, then recovers. `resetrecover`, `resetfatal` and `resetflap` arm at frame 30 and drop window focus first, so recovery must not depend on focus. `resetrecover` fails `Reset` three times and recovers. `resetfatal` changes the present params, fails every `Reset` with `INVALIDCALL`, reverts once and exits Fatal. `resetflap` fails 12 times, reports `DEVICELOST` once, fails 12 more times and recovers (`recovered after 25 attempts`, one `RESET failed` line); the 24 failures exceed the 20-failure bound unless a lost period starts a new count. `resetlaterinit` restarts the mission, then in the second `Init` reports `DEVICELOST` for 60 polls while advancing the reset clock by 1 s per poll (60 s, past the 30 s startup bound) and posts `WM_CLOSE` during the wait. The case expects the close to be deferred and run after `Init` recovers, so its expected class is `InitFailure` (exit 0, no `RUN COMPLETE`).

`-logcmds` takes one quoted value, for example `-logcmds "all Warning, LogNet Verbose"`. The categories are `LogNet`, `LogLua`, `LogLoad`, `LogMemory`, `LogSim`, `LogRender`, `LogAudio`, `LogCore` and `LogTemp`. Every log line starts `[t=<seconds>] [f=<frame>] <Category> <Severity> <file>:<line>: `.

### Log strictness

A smoke case also fails on any Error, Bug or Fatal `[t=` line in its `run.log` (only that file, matched case-sensitively), any `ensure failed: ` line, or a legacy `Message Severity: 3`, `4` or `5` line, unless a regex in `tests\smoke\expected-errors.txt` covers it. The file has `[global]` and `[case-name]` section headers with one regex per line; blank lines and lines starting with `#` are ignored. `[global]` is empty. Unprefixed Lua `print` output never counts. The T1 test exe is strict in the same way: `ExpectLog("text")` and `ExpectEnsure("text")` (named RAII variables) declare an expected Error-or-above line, an undeclared one or an unmatched declaration makes the exe exit 1 at the end of `main`. The hardening step also fails if any file under `src\LuaFL` or `src\ZlibFL` mentions `RedLog.h` (`hardening:redlog-vendor`).

The runner prints `WARNING:` lines when the crash dump setup is missing. Import `tools\wer-localdumps.reg` once as administrator (HKLM only) so a `/GS` fast-fail leaves a dump and no Windows error dialog. The runner never writes the registry.

With `-Movies` the retail `vidmode.ini` in `data\_lvl_pc` decides the video mode, so a stale or fullscreen entry can change what the run shows.

## LAA comparison

`tests\laa-compare.ps1 [-Runs N] [-Configuration Release] [-NoBuild] [-Jobs 8] [-OutDir path]` is opt-in and not part of `run.ps1`. It copies the built exe, patches only the copy with `editbin /LARGEADDRESSAWARE:NO`, then runs the Standard mission `-Runs` times (minimum 5) with the patched copy and `-Runs` times with the real exe plus `-hogaddrspace`, in parallel worker folders. It checks both headers with `dumpbin`, checks the arena sits below 2 GB for the patched copy and above it for the real exe, and compares the `finalscore.log` key and the `[f=N]` frame count of every run. The key and frame count must be identical across all runs on both sides. Nothing is masked beyond the date and time prefix. It prints `[PASS] laa-compare` or `[FAIL] laa-compare` (exit 1) and skips with exit 0 when GameData or DLLs are missing. 5 runs per side take about 20 seconds with 8 jobs.

## Hardening

`tests\run.ps1 -Hardening` builds the game (no GameData needed) and runs only the hardening step: `dumpbin /headers` and `/loadconfig` on `bin\Win32\<SmokeConfiguration>\BattlefrontII.exe` plus a count of `/GS-` in the cl tlog, which is also checked for `/sdl` on every entry. It prints one `[PASS]`/`[FAIL] hardening:<flag>` line each for dynamicbase, nxcompat, laa, reloc, safeseh, gs and sdl, and any missing flag fails the run. dumpbin is found through `VS120COMNTOOLS`; if it is missing the run exits 2. The default run also performs this step.

With GameData, `hardening:aslr-base` launches the game once and requires `MainModule.BaseAddress` to differ from 0x400000. Windows picks the base once per boot, so two launches are not compared. Without GameData the line is `[SKIP]`.

`-Smoke All` reads `tests\smoke\missions-all.txt` (name and expected class: Pass, TimeOut or Fatal). TimeOut cases run the full `-SmokeTimeout` and pass only if the log shows `OPENSWBF2 MISSION RUNNING`, which the game writes on the first mission frame with `-exitongameover`. A `GsFailure` always fails. Every smoke case also passes `-nofocuspause`, so the game keeps running when its window loses focus (no pause, minimise or inactive sleep), which lets a `-Movies` run sit in the background. Headless (`-norender`) runs create a hidden window and the smoke script runs the game at BelowNormal priority, so a run does not take focus or slow the desktop.

`tests\guard-revert.ps1` proves the T1 regression tests fail without their fix. It is a developer tool and is not run by CI. It adds a detached git worktree under `artifacts\guard-revert\wt` (never under TEMP, where MSBuild misses header edits), overlays the current working tree on it, builds T1 once as the control, then for each row in `tests\guardrevert\rows.ps1` reverts one guard (a literal anchor that must occur exactly once), rebuilds and runs the named cases. Run it with `powershell -File tests\guard-revert.ps1`, or `-Row LOCAL-01,NETF-03` for a subset; `-Keep` leaves the worktree. In the table, `Control` must be GREEN and `Reverted` must be RED (the case failed, crashed or timed out). STILL-GREEN means the test cannot detect the revert, WRONG-RED means a smoke row with a `RedRx` pattern failed without that pattern in its log (it failed for some other reason), NO-MATCH means the case name is wrong, BUILD-FAIL or ERROR means the row itself is broken. The exit code is 0 only if every row is RED, and the table is saved to `artifacts\guard-revert\<stamp>\summary.txt`. The main tree is never modified; the script reports any change in `git status`.

The fresh-install default resolution follows the chosen adapter's desktop size (D-04); saved profiles and -norender/smoke runs are unchanged.
