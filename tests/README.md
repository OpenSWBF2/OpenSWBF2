# OpenSWBF2 tests

Tests are C++ and are declared the way Unreal Engine declares them. One exe, `OpenSWBF2Tests.exe`, holds the unit tests and the game-run tests. It is built with the same VS2022 `v143` toolset as the game and uses doctest 1.2.9.

## Requirements

- VS2022 with the x86 C++ tools, Windows SDK 10.0.26100.0 and DirectX SDK June 2010. `tests\vstools.ps1` finds the toolchain.
- `vcruntime140.dll` and `msvcp140.dll` in `C:\Windows\SysWOW64`, and `d3dx9_43.dll` for the tests exe. Win32 game runs that expect Galaxy also need `msvcr120.dll` and `msvcp120.dll`.
- Game-run tests need the retail GameData (`SWBF2_GAMEDATA` or the Steam install). Without it they skip.
- Import `tools\wer-localdumps.reg` once as administrator so a `/GS` fast-fail still leaves a dump. Run `tests\firewall.ps1` once from an elevated PowerShell so game runs can use the local network.
- The `rider-link` gate step needs Rider 2026.1, a JDK 21 and one online Gradle run, as `tools\rider-link\README.md` says.

## Build and run

    MSBuild tests\OpenSWBF2Tests.vcxproj /p:Configuration=Debug /p:Platform=Win32
    tests\run.ps1

The exe goes to `bin\Tests\<Platform>\<Config>\OpenSWBF2Tests.exe`. The configurations are `Debug`, `Release`, `Test` (Release plus `OPENSWBF2_TEST`) and `Asan` (Test plus `/fsanitize=address`).

A bare `run.ps1` builds, runs the unit tests and the `Standard` game runs, then runs the hardening checks. It exits 0 when everything passes, 1 on a failure and 2 on a setup error.

| Switch | Meaning |
|--------|---------|
| `-Configuration` | `Debug` (default), `Release`, `Test` or `Asan` |
| `-Asan` | same as `-Configuration Asan` |
| `-Platform` | `Win32` (default) or `x64` |
| `-NoBuild` | skip the build |
| `-Suite` | wildcard on the tags, for example `*[SmokeFilter]*` |
| `-Filter` | wildcard on the test name; both take a comma list |
| `-Fuzz`, `-Seed`, `-Iterations` | run the `[StressFilter][OnRequest]` tests with a seed and an iteration count |
| `-Smoke` | the game-run group: `None`, `Standard` (default), `Twice`, `Dedicated`, `Crash`, `All`, `Perturb`, `Soak`, `Vendor`, `Boot` or `SaveCross` |
| `-Case` | run only the game-run tests of the group whose names match |
| `-SmokeConfiguration` | the game build the game runs use |
| `-Soak` | `Short` (default) or `Long`: where `All` ends its `TimeOut` missions |
| `-Jobs N` | game processes at once |
| `-Hardening` | run only the hardening checks |
| `-Clean` | remove the staged output folders and any junction under `artifacts` safely, and exit |

## Tags

Every test carries exactly one of `[SmokeFilter]`, `[EngineFilter]`, `[ProductFilter]`, `[PerfFilter]`, `[StressFilter]` or `[NegativeFilter]`. Game-run tests also carry their group tag (`[Standard]`, `[Crash]` and so on). A test tagged `[OnRequest]` runs only when the tag filter names `[OnRequest]`. Select tests with `-Suite` (tags) and `-Filter` (names), or pass `--tags=` and `--name=` to the exe.

## Writing a unit test

    TEST_CASE_NAMED(FAreaTest1, "Unit::Area::does the thing", "[EngineFilter]")
    {
        AddExpectedError("pattern");
        CHECK(...);
    }

`TEST_CASE_NAMED` is in `harness\tags.h`. Names are unique and contain no `,`, `*`, `?` or `"`. Each test runs in its own process. A line at Error or above that no expectation covers fails the test. `AddExpectedError` and `AddExpectedMessage` take a case-insensitive regex and a count (1 by default), and the test fails unless the line occurs that many times.

An `[AsanCanary]` test passes only in an Asan run whose report names the kind in its `[AsanError=<kind>]` tag, with frame `#0` in the test's own file.

## Writing a game-run test

Game-run tests live in `tests\game\`. A test declares a `GameRun`, gives it the game's arguments, then calls `Start()` and `Wait()`. One test may hold several runs. The run fails on a FATAL line, an ensure, a stall, an unexpected exit code, a missing `**** TEST COMPLETE. EXIT CODE: <n> ****` line, an Asan report, an Error, Bug or Fatal line no `AddExpectedError` covers, a wrong expectation count or a wrong dump count. On Test and Asan builds a run can also run in-game tests, which are declared with `IMPLEMENT_SIMPLE_AUTOMATION_TEST` and selected with `-autotest`. A test that expands to one child per line of a data file uses `COMPLEX_TEST_CASE_NAMED`, as `tests\game\missions.cpp` does.

Each run gets a clean worker folder under `artifacts\smoke-workers` and writes `run.log`, `stdout.txt`, `command.txt`, `finalscore.log` and any dump to `<out>\smoke\<run>\`. Rendered runs are allowed only on Test and Asan, which honour `-handsoff`.

## Exclude list

`tests\excluded.tsv` lists the tests that must not run, with the columns `Test`, `Reason`, `Platforms` and `Configs`. An empty cell means all. A row is checked only on the platforms and configurations it names.

## Results

Each `run.ps1` call writes a folder `artifacts\tests\<timestamp>\` with `results.txt`, `run-params.txt` and the runner's `index.json`, which has Unreal's report fields. Unit tests report to `unit\index.json` and game runs to `index.json`. `run.ps1` keeps the three newest folders and any folder a gate run names.

## Baselines

`tests\baseline\v143\` holds the pinned tables the game runs compare against, and `tests\baseline\v143-x64\` holds the x64 ones. `MANIFEST.md` in each folder lists the hash of every file, and `CAUSES.md` has one line per pinned row with its cause. A pinned value changes only with a cause line and the user's approval. `tests\smoke\digest-expected.txt` holds the turn-1500 digest of each digest test.

## Gate

    tests\gate.ps1 -Tier plan|phase|milestone [-Platform Win32,x64] [-Step <names>] [-Resume] [-ListOrder]

| Tier | When |
|------|------|
| `plan` | each plan runs the steps it names with `-Step` |
| `phase` | once, when a phase closes: every build on both platforms, hardening, unit tests, the game-run groups and Asan |
| `milestone` | before a release: the phase steps plus the long All run, Perturb, Soak, the captures and `x64-census.ps1` |

The steps are in `tests\gate\tiers.ps1`. Each step lists its `Inputs`, and a step runs only when no passing record has the same key. The records are in `steps.tsv` and `ledger.tsv` in the evidence root (`OPENSWBF2_EVIDENCE_DIR`, default `%LOCALAPPDATA%\OpenSWBF2\evidence`), with the archived output of each step. The gate keeps the three newest folders under `artifacts\gate`. Before any step it checks that Steam is running with the overlay off for app 6060, the firewall rules, the WER key, free memory and disk space.

`tests\gate\gate.tests.ps1` and `tests\hardening.tests.ps1` test the gate and the hardening scanners. They run as the `gate-tests` and `hardening-tests` steps when their inputs change.

## Hardening

`tests\hardening.ps1` checks the exe's security flags, its imports, the compiler flags of every project, the vendor pins and a set of source patterns. Its allow-lists are in `tests\hardening\allow.ps1`, one entry per allowed site, with the reason in the entry.

## Staged output

`bin\<Platform>\<Config>\data` holds junctions into the retail GameData. Anything that follows them can delete the retail install. Remove staged output only with `run.ps1 -Clean`, and build any extra worktree with `SWBF2_NOSTAGE=true`.

## Tools

- `tests\baseline\capture-vs2013.ps1` captures tables from a run (`-Tables`, `-Collect`, `-Map`) and checks a baseline folder against its manifest (`-Verify -Baseline v143` or `v143-x64`).
- `tests\baseline\compare-tables.ps1 -Tables <dir>` compares captured tables against the pinned ones.
- `tests\baseline\capture-commit.ps1` takes the x64 commit rows from a lone run.
- `tests\baseline\repin.ps1` copies named rows from a run into a pinned table when the run's exe is the current exe and `CAUSES.md` has a line for each row.
- `tests\baseline\x87-inventory.ps1` lists every x87 function of the x86 Release exe and checks the pinned inventory.
- `tests\laa-compare.ps1` compares runs with and without `/LARGEADDRESSAWARE`.
- `tests\luac-compare.ps1` checks that the Win32 and x64 LuaCompiler write the same bytes.
- `tests\x64-census.ps1` lists every x64 pointer-truncation and format warning site into `artifacts\census`.
- `tests\corpus\README.md` describes the float-parse and math corpora.
