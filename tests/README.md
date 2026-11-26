# OpenSWBF2 tests

Tests are C++ and are declared the way Unreal Engine declares them. One exe, `OpenSWBF2Tests.exe`, holds the unit tests and the game-run tests. It is built with the same VS2022 `v143` toolset as the game and uses doctest 1.2.9.

## Requirements

- VS2022 with the x86 C++ tools, Windows SDK 10.0.26100.0 and DirectX SDK June 2010. The gate finds them with `vswhere`.
- `vcruntime140.dll` and `msvcp140.dll` in `C:\Windows\SysWOW64`, and `d3dx9_43.dll` for the tests exe. Win32 game runs that expect Galaxy also need `msvcr120.dll` and `msvcp120.dll`.
- Game-run tests need the retail GameData (`SWBF2_GAMEDATA` or the Steam install). Without it they skip.
- Import `tools\wer-localdumps.reg` once as administrator so a `/GS` fast-fail still leaves a dump. Run `tests\gate.cmd firewall` once from an elevated prompt so game runs can use the local network.
- The `rider-link` gate step needs Rider 2026.1, a JDK 21 and one online Gradle run, as `tools\rider-link\README.md` says.

## Build and run

    tests\gate.cmd -Tier plan -Step t1-Debug

`tests\gate.cmd` is the one entry point. It is a script only because it builds the first C++ program, the gate. The exe goes to `bin\Tests\<Platform>\<Config>\OpenSWBF2Tests.exe`. The configurations are `Debug`, `Release`, `Test` (Release plus `OPENSWBF2_TEST`) and `Asan` (Test plus `/fsanitize=address`).

For a run outside the gate, start the built exe from `tests\` with `--runner`:

    cd tests
    ..\bin\Tests\Win32\Debug\OpenSWBF2Tests.exe --runner --out=..\artifacts\mine --platform=Win32 --config=Debug --exclude=excluded.tsv --name=Unit::Gate::*

| Switch | Meaning |
|--------|---------|
| `--out=<dir>` | the run folder |
| `--platform=`, `--config=` | the build being tested |
| `--tags=`, `--name=` | wildcards on the tags and the test name; both take a comma list |
| `--exclude=<file>` | the exclude list, `excluded.tsv` |
| `--game-dir=<dir>` | run the game-run tests against the game in that folder |
| `--jobs=N` | game processes at once |
| `--soak=Short` or `Long` | where `All` ends its `TimeOut` missions |
| `--param=` | the parameter of a complex test |
| `--data=<run folder>` | judge an archived run with the after-all tests |
| `--fuzz-seed=`, `--fuzz-count=`, `--fuzz-iter=`, `--fuzz-out=` | the `[StressFilter][OnRequest]` fuzz tests |

The runner exits 0 when everything passes, 1 on a failure and 2 on a setup error.

## Tags

Every test carries exactly one of `[SmokeFilter]`, `[EngineFilter]`, `[ProductFilter]`, `[PerfFilter]`, `[StressFilter]` or `[NegativeFilter]`. Game-run tests also carry their group tag (`[Standard]`, `[Crash]` and so on). A test tagged `[OnRequest]` runs only when the tag filter names `[OnRequest]`. A test tagged `[AfterAll]` runs after the other tests of a run whose tag filter names its group. Select tests with `--tags=` and `--name=`.

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

A gate step writes `results.txt`, `run-params.txt` and the runner's `index.json`, which has Unreal's report fields, into `artifacts\gate\<timestamp>\<Platform>\<step>\run\`.

## Baselines

`tests\baseline\v143\` holds the pinned tables the game runs compare against, and `tests\baseline\v143-x64\` holds the x64 ones. `MANIFEST.md` in each folder lists the hash of every file, and `CAUSES.md` has one line per pinned row with its cause. A pinned value changes only with a cause line and the user's approval. `tests\smoke\digest-expected.txt` holds the turn-1500 digest of each digest test.

## Gate

    tests\gate.cmd -Tier plan|phase|milestone [-Platform Win32,x64] [-Step <names>] [-Resume] [-ListOrder]
    tests\gate.cmd clean [-Platform Win32,x64] [-Config Debug,Release,Test,Asan]
    tests\gate.cmd firewall [--list]

| Tier | When |
|------|------|
| `plan` | each plan runs the steps it names with `-Step` |
| `phase` | once, when a phase closes: every build on both platforms, hardening, unit tests, the game-run groups and Asan |
| `milestone` | before a release: the phase steps plus the long All run, Perturb, Soak, the captures and `laa-compare` |

`tests\gate.cmd` finds Visual Studio, builds `bin\Tools\OpenSWBF2Gate.exe` from `tests\gate\OpenSWBF2Gate.vcxproj` and runs it. The steps are in `tests\gate\steps.ini`. A step's key holds the binaries it lists in `Hashes`, the test sources it lists in `Sources` with their include closure from the compiler's tlog, and the repo files it lists in `Reads`; a step that reads a repo file outside `Reads` fails. A step runs only when no passing record has the same key. The records are in `steps.tsv` and `ledger.tsv` in the evidence root (`OPENSWBF2_EVIDENCE_DIR`, default `%LOCALAPPDATA%\OpenSWBF2\evidence`), with the archived output of each step. The gate keeps the three newest folders under `artifacts\gate`. Before any step it checks that Steam is running with the overlay off for app 6060, the firewall rules, the WER key, free memory and disk space.

`clean` removes `bin\<Platform>\Release` and `bin\Tests\<Platform>\Debug`, or `bin\<Platform>\<Config>` and `bin\Tests\<Platform>\<Config>` with `-Config`, then every junction under `artifacts`. `firewall --list` prints the inbound rules it would add and the stale ones it would remove; `firewall` applies them and needs an elevated prompt.

The `Unit::Gate::*` tests test the gate.

## Hardening

Each hardening row is a test of the same name. `Hardening::Source::*` checks source patterns, project files and vendor pins once, in the `hardening-source` step. `Hardening::Binary::*` and `Hardening::FFmpeg::*` check each build's exe and DLL flags, imports, compiler flags and map, in the `hardening-binary-<Config>` steps; `Hardening::Binary::d3dx9-staged` checks the staged `d3dx9_43.dll` after the `vendor-Release` runs. The allow-lists are in `tests\hardening\allow.txt`, one entry per allowed site, with the reason in the entry. The `Unit::Hardening::*` tests check the rows on fixture text.

## Staged output

`bin\<Platform>\<Config>\data` holds junctions into the retail GameData. Anything that follows them can delete the retail install. Remove staged output only with `tests\gate.cmd clean`, and build any extra worktree with `SWBF2_NOSTAGE=true`.

## Tools

- The `Tables::*` tests build the tables of an All, Twice, capture or map run and compare them with the pinned ones; `--data=<run folder>` re-judges an archived run. `Baseline::Verify::v143` and `v143-x64` check a baseline folder against its manifest.
- `OpenSWBF2Tests.exe --repin --run=<run folder> --table=<name> --key=<a,b>` copies named rows from a run into a pinned table when the run's exe is the current exe and `CAUSES.md` has a line for each row.
- `OpenSWBF2Tests.exe --capture-commit --platform=<p> --steam=Running|Closed [--hog] --out=<dir>` takes the x64 commit rows from a lone run.
- The `X87::Inventory` tests list every x87 function of the Win32 Release exe into `x87-inventory.tsv` in the run folder and compare it with the pinned inventory; `--data=<run folder>` re-judges an archived one.
- The `laa-compare` step runs `Game::Laa::compare`, which compares runs with and without `/LARGEADDRESSAWARE`.
- The `luac-compare` step checks that the Win32 and x64 LuaCompiler write the same bytes.
- Old names in `MANIFEST.md` and `CAUSES.md`: `capture-vs2013.ps1 -Tables`, `-Collect`, `-Map`, `-DebugDigests` and `-Verify` are the `Tables::All`, `Tables::Collect`, `Tables::Map`, `Tables::DebugDigests` and `Baseline::Verify` tests, `compare-tables.ps1` is the `Tables::*` compares, `repin.ps1` is `--repin`, `capture-commit.ps1` is `--capture-commit`, `x87-inventory.ps1` is the `X87::Inventory` tests, `run.ps1 -Smoke <group>` is the gate step of that group, `run.ps1 -Hardening` is the hardening steps, `run.ps1 -Clean` is `tests\gate.cmd clean`, `firewall.ps1` is `tests\gate.cmd firewall`, and `capture-vs2013.ps1 -Layouts`, `soundon.ps1`, `guard-revert.ps1` and `x64-census.ps1` are retired.
- `tests\corpus\README.md` describes the float-parse and math corpora.
