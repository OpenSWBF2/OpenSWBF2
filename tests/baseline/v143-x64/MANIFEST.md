# v143-x64 baseline manifest

## Build

| Key | Value |
|-----|-------|
| toolset | v143 |
| platform | x64 |
| configuration | Release |

Why this folder exists, and the cause of every file in it, are in [CAUSES.md](CAUSES.md).

## Text artifacts

| File | SHA-256 | Produced by |
|------|---------|-------------|
| `CAUSES.md` | BA1C6BDE2802F46B5AEBD0792196D2BDA6655B582F983D098563E86330FE9610 | written by hand |
| `cloth-headroom.txt` | 41AEB30AEEEAD219D7902AFA2466EAEA49E92BA7272DD573B69FDD7594F47E16 | `capture-vs2013.ps1 -Platform x64 -Tables` from `run.ps1 -Platform x64 -Configuration Release -Smoke All -HogAddrSpace` |
| `commit.txt` | CBC526594DCBF8439B42D654C160CC8972A1F065E8764DFFF1D7C3BD157A88D1 | `capture-commit.ps1 -Platform x64 -HogAddrSpace` with `-SteamState Running` and `Closed`, two byte-identical captures per Steam state |
| `lua-bins.txt` | 0EFAB44960946F206921DAA9E71B9307F0B01EB54B4E154A21A63D9B6BE249B0 | `capture-vs2013.ps1 -Platform x64 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Platform x64 -Configuration Test -Smoke All -HogAddrSpace` |
| `lua-hist.txt` | 9AFF9F6B2C07256C9D154DF4B3386B3BA98EC88E2713F1581F72611F9034116C | `capture-vs2013.ps1 -Platform x64 -Configuration Test -Tables -LuaBinsDir` from `run.ps1 -Platform x64 -Configuration Test -Smoke All -HogAddrSpace` |
| `mem-peaks.txt` | A5091DFC387F4BA961EA87F955F7B0CCA2671E5D3B010EC7430C9086207C65E2 | `capture-vs2013.ps1 -Platform x64 -Tables` from `run.ps1 -Platform x64 -Configuration Release -Smoke All -HogAddrSpace` |
| `offthread.txt` | ED95BE4D101396DE63B55B9937F4A2F2C329645B1566076FA7175AEF93A411AA | `capture-vs2013.ps1 -Platform x64 -Configuration Test -Tables -OffThreadDir` from `run.ps1 -Platform x64 -Configuration Test -Smoke All -HogAddrSpace`; no sound-on section (Phase 50 AV-03) |
| `pool-sizes.txt` | 8F41DB67DA10ECBD1F1E12ADF06DECFA9153C7BB288C84B8F211CB697295E087 | `capture-vs2013.ps1 -Platform x64 -Tables` from `run.ps1 -Platform x64 -Configuration Release -Smoke All -HogAddrSpace` |

## Notes

- Each table names its commit on its first line.
- `commit.txt` was captured on 79299a77 (C-X64-COMMIT-TABLE in `CAUSES.md`, plan 49-05); `smoke.ps1` `Test-CommitFlat` reads it through `compare-tables.ps1 -Commit` with the run's recorded `steam=` and hog mode.
- `mem-peaks.txt`, `pool-sizes.txt`, `cloth-headroom.txt`, `lua-bins.txt` and `offthread.txt` were captured from the 49-13 exe (first line 9d3e9374, its parent; C-X64-TABLES in `CAUSES.md`), each from two byte-identical x64 `-HogAddrSpace` captures. `mem-peaks.txt` replaces the 2e0803ec table (C-MEMPEAKS-X64-PARITY in `tests/baseline/v143/CAUSES.md`) moved here from `tests/baseline/v143/mem-peaks-x64.txt` in 49-01 (C-X64-BASELINE-FOLDER). `offthread.txt` has no `## sound-on` section until Phase 50 AV-03.
- `lua-hist.txt` was captured on 502047e0 (C-X64-LUA-HIST in `CAUSES.md`, plan 49-07) from two byte-identical x64 Test `-Smoke All -HogAddrSpace` captures; a bin re-tune must leave it unchanged.
- `compare-tables.ps1 -Platform x64` reads a table from this folder when it exists here and from `tests/baseline/v143` otherwise.
- `capture-vs2013.ps1 -Verify -Baseline v143-x64` checks this manifest.
- `compare-tables.ps1` holds the reader of every file here, and the `baseline-readers` hardening check fails on a file with no reader.
