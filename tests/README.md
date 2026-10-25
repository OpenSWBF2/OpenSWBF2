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

| Switch | Meaning |
|--------|---------|
| `-Smoke` | `None`, `Standard` (default), `Dedicated`, `Crash`, `All` (every mission in `tests\smoke\missions.txt`, Dedicated and Crash) or `Twice` (Standard twice with `-seed 1`, prints `INFO: run-twice match` or `mismatch`, never fails) |
| `-SmokeTimeout` | per-case seconds (default 120); on expiry the process tree is killed with `taskkill /T /F` |
| `-SmokeConfiguration` | `Release` (default) or `Debug`; builds `BattlefrontII.vcxproj` unless `-NoBuild` |
| `-Movies` | drop `-norender` so movies and the renderer run |

Cases run one at a time with the exe directory as the working directory. Without retail GameData, or without the exe and the Bink, Galaxy and Steam DLLs beside it, every selected case prints `[SKIP] ... class=Skipped` and the game is not built. A skipped run exits 0.

Artifacts go to `artifacts\tests\<timestamp>\smoke\<case>\`: `run.log`, `finalscore.log`, any `.dmp` and `result.txt`.

| Class | Meaning | Token |
|-------|---------|-------|
| Skipped | precheck failed | SKIP |
| TimeOut | killed at the timeout | TIMEOUT |
| GsFailure | exit `0xC0000409` | CRASH |
| Fatal | exit 1 or 3, a `FATAL:` log line, or a dump | CRASH |
| Pass | exit 0 and `OPENSWBF2 RUN COMPLETE` in the log, no dump | PASS |
| InitFailure | exit 0 without the marker | FAIL |
| Unknown | anything else | FAIL |

Classes are decided in that order. Crash cases pass when the expected class, exit code and dump count are observed.

The runner prints `WARNING:` lines when the crash dump setup is missing. Import `tools\wer-localdumps.reg` once as administrator (HKLM only) so a `/GS` fast-fail leaves a dump and no Windows error dialog. The runner never writes the registry.

With `-Movies` the retail `vidmode.ini` in `data\_lvl_pc` decides the video mode, so a stale or fullscreen entry can change what the run shows.
