### C-X64-BASELINE-FOLDER

- No digest row moved. No x86 table moved. No x64 table changed content.
- Change (phase 49, D-07, D-08, D-09): the layout-dependent tables of the x64 build get their
  own folder, `tests\baseline\v143-x64`, with the same file names as `tests\baseline\v143`.
  Pool block sizes, blocks per page and Runtime free bytes depend on pointer width, so an x64
  run cannot be judged exactly against an x86 table, and an x86 table must never move to fit an
  x64 run (D-01).
- Rule: `compare-tables.ps1 -Platform x64` reads a table from this folder when the file exists
  here and from `tests\baseline\v143` otherwise (digests, x87 inventory and the other
  layout-independent tables); `-Platform Win32` always reads `tests\baseline\v143`. A table
  here exists as its own pinned file even when it equals the x86 file byte for byte (D-09), so
  a later layout change on one platform cannot hide behind the other.
- `mem-peaks.txt` is `tests\baseline\v143\mem-peaks-x64.txt` moved byte for byte (SHA-256
  unchanged). Its capture and its strict pool-row reader are C-MEMPEAKS-X64-PARITY in
  `tests\baseline\v143\CAUSES.md` (48-18): `compare-tables.ps1 -Platform x64 -MemPeaksX64`
  reads it here and its pair `mem-peaks.txt` from `tests\baseline\v143`.
- Readers: `compare-tables.ps1` names the reader of every file here, and the `baseline-readers`
  hardening check scans both folders. `capture-vs2013.ps1 -Verify -Baseline v143-x64` checks
  `MANIFEST.md` (gate step `verify-v143-x64`).
- Status: Fixed, 49-01, 2026-11-20.

### C-X64-COMMIT-STEAM

- No digest row moved. No x86 table moved. No table in this folder changed content.
- Symptom: x64 Test `commit-flat` with the Steam client running failed (`artifacts\tests\20261121-011111`:
  mission 9 rest 2109440 above the cycles 1-8 peak, bound 0), and the Steam-closed x64 `rest` took one
  of three values from run to run (585728, 1110016 or 1183744).
- Cause 1 (the step): `rest = private - heaps - arena - threads + gamecrt` counted every committed
  private region the game does not own. With Steam running, `steam_api64.dll` loads `steamclient64.dll`,
  whose allocator `tier0_s64.dll` commits 2 MiB blocks with `VirtualAlloc` on `DLL_THREAD_ATTACH` and
  never frees them. About 30 s after start the RPC runtime's garbage-collection timer starts two
  thread-pool workers at once and the second gets a new block, so the step lands at a wall-clock time.
  Proven by a thread-attach toggle (four threads alive at once with attach move the step to that row;
  the same threads with `THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH` add nothing). The 32-bit Steam client
  keeps that data in a process heap, so x86 `rest` held 2494464 constant Steam bytes instead.
- Cause 2 (the run-to-run values): mimalloc 3.6.1 `MI_PAGE_CACHE_OFFSET` offsets the first block of
  every page by a hash of the page's absolute address, which changes how many blocks fit in a page and
  so the pages and bytes committed; and mimalloc's page map commits one 72 KiB submap per 512 MiB range
  that holds a page, so two OS-placed 64 MiB arenas cost one or two submaps. Both follow ASLR. Proven
  by forcing the arena addresses in a scratch build (the same build gives layout A at one address and
  layout B at another; with `MI_PAGE_CACHE_OFFSET=0` both addresses give identical columns; forcing the
  two arenas into two 512 MiB ranges adds exactly one 73728-byte submap).
- Fix: the Test build keeps a ledger of the game's own `VirtualAlloc` reservations (mimalloc's
  `_mi_prim_alloc`, `_mi_prim_alloc_huge_os_pages` and `_mi_prim_free`, which mimalloc offers no hook
  for, and the reservation-bit leaves of `RedMemoryMi.cpp`); COMMITPARTS ends with `gamevm=`, the
  committed bytes of those reservations outside the first arena, and `commit-flat` takes
  `rest = gamevm + gamecrt` on both platforms. `MI_PAGE_CACHE_OFFSET=0` is set for mimalloc in every
  configuration on both platforms (`mimalloc-defines` checks it). On x64 the game reserves one 256 MiB
  region that the OS places and that is moved to the next 512 MiB boundary only when it would cross one,
  and gives it to mimalloc as its arena, so every arena page is in one page-map range and `arena` covers
  all of mimalloc's arena memory (a later overflow arena would be a ledger base and count in `gamevm`).
- Runs (x64 Test, hog, `commit-flat` alone): Steam running `artifacts\gate49\p04\runs\x64-on-1`,
  `x64-on-2`, `x64-on-3`; Steam closed `x64-off-1`, `x64-off-2`, `x64-off-3`. All six pass and give the
  byte-identical arena, gamevm and gamecrt series (arena 3928064 then 4452352 from the second shell row,
  gamevm 282624, gamecrt 65536); the old formula still shows the 2101248-byte step in the three
  Steam-running runs. Win32 Test (`win32-on-1`): `arena` and `gamecrt` equal
  `artifacts\tests\20261118-220340` row for row, `gamevm` 45056, rest 77824 on every row; the
  process-heap fields moved with the Steam session (the pre-change exe gives the same values in the
  same session).
- Falsifier: the fix is wrong if the x64 game-owned rest steps in cycles 9-16 of a Steam-running run;
  if an attached-thread toggle moves `gamevm`; if a Steam-running and a Steam-closed run give different
  arena, gamevm or gamecrt series; if x86 rest is not 77824 on every row; or if a ledger base ever holds
  a `tier0_s64` block.
- Tests: T1 `commit parts count only game-owned bytes`, `mimalloc pages start their first block at an
  address-independent offset`, `mimalloc arena region stays inside one page-map range`; guard rows
  `GAMEVM-LEDGER`, `GAMEVM-LEDGER-X64`, `MIOFFSET-T1`, `MIOFFSET-T1-X64`, `MIREGION-X64`, `MIDEF-03`,
  `COMMIT-GAMEVMLOG`.
- Status: Fixed, 49-04, 2026-11-21.
- Update (49-05): the region now starts on a 256 MiB boundary inside the OS-placed probe. With
  `MI_SECURE=0` (Debug and Asan) mimalloc separates and aligns its page meta data, so
  `mi_manage_os_memory_ex` needs a 256 MiB-aligned start on x64 and refused the region whenever the
  probe was not aligned ("after alignment, the size of the arena becomes too small", 5 of 8 Debug
  x64 tests exe runs), and the game then fell back to OS-placed arenas. A 256 MiB-aligned 256 MiB
  region also never crosses a 512 MiB page-map range, so the 49-04 invariant holds in every
  configuration. Test and Release (`MI_SECURE=4`, 64 KiB alignment) give the same `arena` and
  `rest` as before (C-X64-COMMIT-TABLE runs).

### C-X64-COMMIT-TABLE

- No digest row moved. No x86 table moved. `commit.txt` is new; no other table here changed.
- Change (phase 49, D-15, D-16): x64 `commit-flat` compares the settled game-owned commit of a
  run with a pinned row as well as with its own cycles 1-8. `commit.txt` is tab-separated:
  `# commit <sha>`, the header `steam`, `hog`, `state`, `arena`, `rest`, then one row per Steam
  state (`yes`, `no`), hog mode and game state (`mission`, `shell`). `compare-tables.ps1 -Commit`
  takes every COMMITPARTS row after cycle `CommitSettled` (cycles 9-16) and requires `arena` and
  `rest = gamevm + gamecrt` to equal the row chosen by the run's recorded `steam=` and hog mode.
  A missing row fails naming the keys, and a run-params without `steam=` fails as a missing
  input. The Steam state only chooses the row; it is never a verdict on its own.
- Columns kept: `arena` and `rest`, the quantities C-X64-COMMIT-STEAM proves game-owned and exact
  run to run. Dropped: `private`, `heaps`, `threads`, `threadcount`, `crtbusy`, `crtcommitted`
  and `crtlarge`, because they hold foreign bytes (Steam's thread blocks and process-heap
  blocks, OS thread-pool stacks) and change with the Steam session; `gamevm` and `gamecrt` are
  not separate columns because the verdict reads only their sum.
- Rows: only hog mode 1, the mode every x64 smoke step uses. No hog-0 row is pinned, so an x64
  `commit-flat` run without `-HogAddrSpace` fails as a missing row until two identical hog-0
  captures pin one. The Steam-running and Steam-closed rows are equal (arena 4452352, rest
  348160 in both states); they are separate rows because D-15 keeps a baseline per Steam state.
- Captured with `capture-commit.ps1 -Platform x64 -SteamState Running|Closed -HogAddrSpace`
  (x64 Test, `commit-flat` alone, on 79299a77 with the 49-05 changes, exe sha256
  `fc3efd86...3150c2e1`). Two runs per Steam state, byte-identical below `# commit`, every case
  and check passing: Steam running `artifacts\tests\20261121-051322` and `20261121-051503`;
  Steam closed `20261121-051645` and `20261121-051834`. The four runs before the region
  alignment update (exe `8a81229e...c4e893`: `20261121-044739`, `20261121-044920`,
  `20261121-045119`, `20261121-045308`) and the six 49-04 runs (`artifacts\gate49\p04\runs`)
  give the same values.
- The x64 ADDRSPACE expectation needed a fix for the Steam-closed runs: with Steam closed the game
  logs `steam=0` with steam_api64 loaded above 4 GB (the DLL loads, `SteamAPI_Init` fails), and the
  pattern accepted `steam=0` only with no module, so every x64 hog case failed on the Steam state
  alone. `steam=0` now also accepts a module above 4 GB; `steam=1` still requires one.
- Falsifier: wrong if two lone captures of one Steam state differ below `# commit`, or if a
  Steam-closed and a Steam-running capture give different `arena` or `rest`. A pinned value
  that moves by exactly 73728 bytes means an overflow arena's page-map submap (C-X64-COMMIT-STEAM).
- Tests: selftest fixtures for the row check (match, a value 4096 off, a missing row, the `no`
  rows for a `steam=no` run, a missing `steam=` key, a space-separated table, no x86 table) and
  for the `capture-commit.ps1` refusals.
- Status: Fixed, 49-05, 2026-11-21.

### C-X64-POOL-ENTRIES

- No digest row moved. No x86 table moved. No table in this folder changed.
- Cause: the pool byte ceiling `kPoolBytesMax` (16 MiB) was written for 4-byte pointers. Every
  entry limit derived from it is `16 MiB / block size`, so on x64, where a pointer-holding block
  is larger, a pool holds fewer entries at its limit than on x86. A pool or mod count accepted on
  x86 (`MemoryPool::Create` `refused count`, growth `refused growth`, the SetMemoryPoolSize and
  AddWalkerType clamps, the StickInfo and mine `DenseList` capacities) was refused or clamped
  lower on x64.
- Fix (phase 49, D-11): `RedScriptLimits::kPoolEntryBytesMax = kPoolBytesMax / 4 * sizeof(void*)`
  is the entry ceiling in bytes. It is used by both `PoolBytesOk` calls in `RedMemoryPool.cpp`,
  the `uMaxBytes` of `kSetMemoryPoolSize` and `kAddWalkerType`, and the two list capacities
  (`kStickListCapacity` in `StickInfo.h`, `kMineListCapacity` in `EntityMine.h`). On x86 it equals
  `kPoolBytesMax`: every Win32 Release (1068) and Test (806) object is byte-identical to before
  (`coffcmp.py`, debug sections excluded). The heap byte limits (temp heap, artist steal, heap
  cap) are heap sizes, not entry limits, and are unchanged. Every x64 block is at most twice its
  x86 block, so no x64 entry ceiling is below x86's; the x64 ceiling is bounded at 32 MiB per pool
  and stays below the x64 Runtime heap.
- Growth step: `mGrow = (size + 63) / size` is a byte step, so a larger x64 block grows by fewer
  entries (SoldierAnimation 6 at 12 bytes on x86, 3 at 24 on x64). Growth happens on demand up to
  the ceiling, so the step changes the slack after growth, never a limit. The pools whose pinned
  x64 count differs from x86 for this reason are AmmoCounter, ConnectivityGraphFollower,
  EnergyBar, ParticleTransformer::SizeTransf and SoldierAnimation (71 rows). Evidence: on every
  row of both `mem-peaks.txt` tables whose pool is live (count above 0; a destroyed or view pool
  has count 0 on both platforms) the configured count (count minus grown times step) is equal
  on both platforms, replaying configured count plus whole steps to the pinned peak gives the
  pinned count and grown count, the peaks of these pools are equal, and no archived x64 run log
  under `artifacts\tests\2026112*` (598 runs) has `refused growth`; the only `refused count` lines
  are the injected `crashtesthuge` pool of `crash-poolcount`.
- Peaks that differ: `Combo::DamageSample` has a lower x64 peak in 6 missions (tat2g_eli 2843
  against 2561). That is C-COMBO-BLOCKS: a 28-byte element packs into 32-byte x64 blocks, so the
  same arrays hold fewer blocks. It never grew and its count is equal.
- Not asserted here: the Lua bin pools (`LuaN`), whose bin set is per platform (49-08).
- SoundFL voice and stream storage are not `MemoryPool` rows of these tables. The same T1 counts
  them in entries from `Snd::EngineBase::kNumVoices` and `kNumStreams`: at least 32 voices and 6
  streams on both platforms, and on x86 exactly 0xA800 and 0x1446A68 bytes. Guard rows
  `SNDPOOL-VOICES-X64`, `SNDPOOL-STREAMS-X64`.
- Tests: T1 `pool capacities in entries` (every label and row of both pinned tables: block sizes,
  configured count, replayed capacity, peaks, the growth and packed lists with stale checks, and
  on the running platform the exact entry ceiling of Create, growth and both script clamps,
  probed on a small test heap) and `pool capacities in entries for the stick and mine lists`
  (x86 349525 and 24966, x64 524288 and 33288). Guard rows `POOLENT-CONST-X64`,
  `POOLENT-CREATE-X64`, `POOLENT-GROW-X64`, `POOLENT-SCRIPT-X64`, `POOLENT-WALKER-X64`,
  `POOLENT-STICK-X64`, `POOLENT-MINE-X64`.
- Falsifier: wrong if a re-captured x64 `mem-peaks.txt` shows a pool whose peak differs from x86
  outside C-COMBO-BLOCKS and the Lua bins, a configured count that differs, or a `refused growth`
  line in an x64 run.
- Status: Fixed, 49-06, 2026-11-21.

### C-X64-LUA-HIST

- `lua-hist.txt` is the x64 Lua demand histogram, the archived table the x64 bin re-tune (D-10,
  plan 49-08) works from. Same producer and columns as `tests/baseline/v143/lua-hist.txt`
  (C-LUA-HIST there): per 4-byte bin `(size + 3) >> 2` up to 640 plus an `over` row, the peak
  live blocks and the total allocation calls counted in `Lua_Red_realloc` and `Lua_Red_Free`,
  from the turn1500 block of each Test `-Smoke All` mission.
- Why it differs from x86: Lua objects hold pointers, so their sizes grow on x64 (the busiest
  x86 bin 10 becomes x64 bin 20, 112 becomes 224, 224 becomes 448). It is the x64 demand, not
  the x86 table in other units.
- It does not depend on the bin table: the counters see the requested size before a pool is
  chosen, so re-tuning the x64 bins changes the x64 `lua-bins.txt` but must leave this file
  byte-identical.
- Exact: two x64 Test `-Smoke All -Soak Short -HogAddrSpace` runs at 502047e0 with Steam
  running gave byte-identical captures below `# commit`: `artifacts\tests\20261121-072832` and
  `artifacts\tests\20261121-073224`, each captured with `capture-vs2013.ps1 -Platform x64
  -Configuration Test -Tables -LuaBinsDir <run folder>` (the `run.ps1` lua-bins block stays off
  under hog until 49-13). The x86 pair is `artifacts\tests\20261121-072049` and `-072442`.
- Read by `compare-tables.ps1 -Platform x64 -LuaHist`; on x64 the unscoped `lua-hist.txt` entry
  yields to this folder's entry, so the table is compared once.
- Falsifier: wrong if a re-capture at the same game code differs below `# commit`, or if a change
  to `gShellPoolInfo` or `gGamePoolInfo` alone changes it.
- Status: pinned, 49-07, 2026-11-21.

### C-X64-LUA-BINS

- No digest row moved. No x86 table moved. No table in this folder changed: `lua-hist.txt`
  re-captured with the new bins is byte-identical below `# commit`.
- Cause: the x86 Lua bin table (`gShellPoolInfo`, the table missions use) was tuned for 4-byte
  pointers. On x64 the Lua object sizes grow (C-X64-LUA-HIST): the busy x64 bins 112, 224 and
  448 (284,879, 449,544 and 216,696 calls) have no pool in it, and bins 20, 12 and 30 have 74,
  457 and 25 entries against x64 peaks of 2,945, 1,682 and 876, so most of their allocations
  went to the Lua heap: 18,023 Lua heap
  allocations in geo1c_con against 626 with the new table.
- Rule, applied to `lua-hist.txt` (109 missions, 360 numbered bins with calls, `over` excluded
  because it is not one bin): candidates are the bins with non-zero calls; the 48 with the most
  calls summed over all missions are taken, ties to the smaller bin, because a pool saves time per
  call; each count is the largest per-mission peak of live blocks in that bin. That peak is the
  live demand that Phase 39's peak plus spill bounded, so no mission spills. The 48 bins cover
  3,165,682 of 3,220,002 calls (98.3%); the pools take 2,996,424 bytes of Lua heap at 8-byte
  blocks. Script: `artifacts\gate49\p08\derive.py`.
- Block rounding: x64 rounds pool blocks to `kPoolBlockAlign` (8), so bins 1 and 2, 3 and 4, and
  so on share a block size. `LocatePool` matches the exact bin `(size + 3) >> 2` against
  `mBin`, never the block size, and each bin has its own pool and `IsPresent` range, so every
  chosen bin is served by its own pool.
- Fix (phase 49, D-10, D-02): the table is the file-local `sMissionPoolInfo64` at the end of
  `LuaMemory.cpp`, selected in `CreatePools` by `sizeof(void*) == 8 && !bMission`; the shell
  branch keeps `gGamePoolInfo` on both platforms (no shell histogram). `gGamePoolInfo` and
  `gShellPoolInfo` are unchanged. x86 objects: Win32 Release and Test `LuaMemory.obj` give
  `coffcmp.py` `same 1 diff 0`, have no `sMissionPoolInfo64` symbol and equal section sizes
  except `.debug$S` (Release); no `__LINE__` moved.
- Timing (D-10, Phase 39 method): x64 Release, mimalloc, `__rdtsc` around the `Lua_Red_realloc`
  and `Lua_Red_Free` entry calls from Lua (inner frees not counted twice), whole mission to exit,
  median of 4 runs, smoke `-Smoke All` arguments without hog, one instrumented build choosing the
  table at `CreatePools`. Ticks:

  | Mission | No bins (a) | x64 table (b) | x86 table (c) | a / b | c / b |
  |---------|------------:|--------------:|--------------:|------:|------:|
  | geo1c_con | 17,372,528 | 3,817,363 | 11,951,381 | 4.551 | 3.131 |
  | dag1c_ctf | 18,352,573 | 3,995,226 | 12,275,923 | 4.594 | 3.073 |
  | kas2c_uber | 17,039,688 | 4,030,772 | 12,142,353 | 4.227 | 3.012 |
  | hot1g_con | 19,108,793 | 4,005,363 | 11,953,618 | 4.771 | 2.984 |
  | spa3c_ass | 33,166,544 | 6,501,213 | 15,039,100 | 5.102 | 2.313 |
  | total | 105,040,127 | 22,349,938 | 63,362,375 | 4.700 | 2.835 |

  Lua heap peak (largest `MEMREPORT heap=Lua` peak in the run) and heap allocations:

  | Mission | Cap | Peak (a) | Peak (b) | Peak (c) | Heap allocs (a) | (b) | (c) |
  |---------|----:|---------:|---------:|---------:|----:|----:|----:|
  | geo1c_con | 29,229,056 | 3,247,492 | 3,410,888 | 4,416,436 | 29324 | 626 | 18023 |
  | dag1c_ctf | 29,229,056 | 3,257,480 | 3,412,068 | 4,426,432 | 29395 | 629 | 18082 |
  | kas2c_uber | 29,229,056 | 3,254,812 | 3,413,780 | 4,422,184 | 29401 | 633 | 18073 |
  | hot1g_con | 29,229,056 | 3,246,972 | 3,411,372 | 4,416,300 | 29325 | 627 | 18017 |
  | spa3c_ass | 29,229,056 | 3,307,172 | 3,413,752 | 4,455,092 | 30278 | 636 | 18265 |

  SIM DIGEST is equal across the three tables in every mission. Decision by the 5% rule: no bins
  is 4.2 to 5.1 times slower than the new table (4.70 in total), so the bins stay and take the
  new table. Raw logs and driver: `artifacts\gate49\p08\timing\` (build from 855fd082 plus
  `instrument.diff`).
- Spill: an x64 Test `-Smoke All -Soak Short -HogAddrSpace` run with the new table, captured with
  `capture-vs2013.ps1 -Platform x64 -Configuration Test -Tables -LuaBinsDir`, has spill 0, grown
  0 and peak at most count on all 5,232 `lua-bins.txt` rows (109 missions, 48 bins each); 73 rows
  reach their count exactly. Its `lua-hist.txt` is byte-identical to this folder's below
  `# commit` (`compare-tables.ps1 -Platform x64 -LuaHist`: `[PASS] tables:lua-hist-x64
  keys=26017`). Run `artifacts\tests\20261121-083851`, capture `artifacts\gate49\p08\tables`.
- Tests: T1 `lua mission bin table is the re-tuned one` pins all 48 x64 entries and keeps the x86
  checks (`{ 743, 8, 0 }` and the rest) through `sizeof(void*)`. Guard row `MPOOL06-SELECT-X64`
  (x64 Test) makes x64 use the x86 table and goes RED. `MPOOL06-TABLE` and `MPOOL06-SELECT` are
  unmoved.
- Falsifier: wrong if an x64 Test `-Smoke All` capture with this table shows a spill above 0, if
  the x64 `lua-hist.txt` re-capture differs, or if a Win32 `LuaMemory.obj` code or data section
  differs from before this change.
- Status: Fixed, 49-08, 2026-11-21.

### C-X64-TABLES

- No digest row moved. No x86 table moved. `lua-hist.txt` and `commit.txt` are unchanged.
- Change (phase 49, D-07, D-09, plan 49-13): `mem-peaks.txt` is re-captured after the x64 Lua
  bin re-tune and the Lua heap cap below, and `pool-sizes.txt`, `cloth-headroom.txt`,
  `lua-bins.txt` and `offthread.txt` get their x64 sibling. With `commit.txt` and `lua-hist.txt`
  every table of D-07 now has its own x64 file. `run.ps1` captures and compares all of them on
  x64 under `-HogAddrSpace` (Win32 keeps its rules), against this folder.
- Captures, x64, `-HogAddrSpace`, Steam running, from the exe of the 49-13 work commit (built
  from its tree before the commit, so the files name its parent 9d3e9374 on their first line):
  - Release: `run.ps1 -Platform x64 -Configuration Release -NoBuild -Smoke All -Soak Short
    -HogAddrSpace`, `artifacts\tests\20261121-134014` and `artifacts\tests\20261121-134936`;
    `mem-peaks.txt`, `pool-sizes.txt` and `cloth-headroom.txt` under `tables\` are byte-identical
    below `# commit` (`smoke-all-digests.txt` differs only in the skipped `dedicated` row; it is
    shared and not pinned here).
  - Test: `run.ps1 -Platform x64 -Configuration Test -NoBuild -Smoke All -Soak Short
    -HogAddrSpace`, `artifacts\tests\20261121-134457` and `artifacts\tests\20261121-135427`, each
    captured with `capture-vs2013.ps1 -Platform x64 -Configuration Test -NoBuild -Tables
    -LuaBinsDir <run> -OffThreadDir <run>` into `artifacts\gate49\p13\final-test-1` and
    `final-test-2`; `lua-bins.txt`, `lua-hist.txt` and `offthread.txt` are byte-identical below
    `# commit`, equal to the captures `run.ps1` made itself in `<run>\lua-bins` and
    `<run>\offthread`, and `lua-hist.txt` equals this folder's.
  - The same four runs at the exe before the Lua cap change (`20261121-131802`, `-132714`,
    `-132243`, `-133155`) were byte-identical to each other too. The final tables differ from
    them only in the Lua cap column of `mem-peaks.txt` (109 rows, 29229056 to 22478848).

#### mem-peaks.txt

- Against the previous pin (2e0803ec, C-MEMPEAKS-X64-PARITY): every row except the Lua ones is
  identical. The Lua heap row moves in cap (C-HEAPCAP-X64-Lua-RETUNE), peak, used and allocs, for
  example cor1c_c turn1500 peak 4496164 to 3434180 and allocs 18663 to 676, because the re-tuned
  bins serve the x64 sizes (C-X64-LUA-BINS). The `LuaN` pool rows follow the new table: 11 keys
  per mission are gone (Lua34, 38, 41, 46, 53, 54, 58, 66, 67, 98, 160; the x86 bin 320 has no
  row in either x86 file), 12 are new (Lua25, 27, 48, 52, 72, 76, 92, 96, 108, 112, 224, 448), and the bins in both
  sets change count and peak.
- Against the x86 pair `tests\baseline\v143\mem-peaks.txt` (13827 x64 rows, 13718 x86 rows, 109
  missions each; every non-Lua key in both): 856 non-Lua rows are equal. The differing classes:
  - Heap cap of Application, Lua and Runtime: the x64 caps of C-HEAPCAP-X64-RULE.
  - Heap peak and used of Application, Lua and Runtime: object sizes grow with pointer width
    (Application 125040 to 175104, Runtime 19346544 to 25162848 at the largest peaks).
  - Pool size (7412 rows, 85 labels): block sizes grow with pointer width (C-MEMPEAKS-X64-PARITY).
  - Pool count (71 rows) and grown (159 rows): only AmmoCounter, ConnectivityGraphFollower,
    EnergyBar, ParticleTransformer::SizeTransf and SoldierAnimation, the growth-on-demand list of
    C-X64-POOL-ENTRIES (the growth step `(size + 63) / size` is smaller at the larger block size).
  - Pool peak (6 rows): only Combo::DamageSample (C-COMBO-BLOCKS).
  - Lua allocs (109 rows): the bins above.
  - Runtime allocs (109 rows, x64 30 to 364 above x86): each growth of a pool allocates one Runtime block, and in 104 of 109
    missions the extra x64 allocs equal the extra x64 growths. In geo1c_xl, hot1g_xl, kas2c_uber,
    kas2c_xl and kas2g_xl (Runtime row from the exit block) x64 has 1, 2, 5, 2 and 2 more Runtime
    allocations than the pool rows account for (kas2c_uber: 3 already at turn 1500, where only
    SoldierAnimation grows differently). They are the pages and free list of the pvs displayable
    sub-allocator, C-X64-RUNTIME-ALLOCS; the values are the same as in the 48-18 pin.
- Reader: `compare-tables.ps1 -Platform x64` compares the table exactly (`tables:mem-peaks`). The
  strict pair reader `-MemPeaksX64` checks every `LuaN` pool key against the bins of this
  folder's `lua-bins.txt` (mission and source) and pairs every other key exactly with the x86
  file; the derived heap caps come from it (`tables:heap-caps-x64`, smoke `HeapCapsX64`).

#### pool-sizes.txt

- Equal to `tests\baseline\v143\pool-sizes.txt` below `# commit` (0 lines differ): the script
  pool sizes are counts from the mission scripts and do not depend on pointer width. Kept as its
  own file (D-09). Reader `tables:pool-sizes-x64`.

#### cloth-headroom.txt

- All 109 rows differ in `free` only (`enabled`, `low` and `high` are equal, both clamp to 131072
  and 262144). `free` is the Runtime free bytes after load minus 0x20800 (`GameState.cpp`), so it
  is the Runtime cap minus the bytes used after load minus 133120, and both terms depend on
  pointer width:
  - cor1c_c: x86 32235168 - 13049488 - 133120 = 19052560; x64 41943040 - 16728768 - 133120 =
    25081152 (used after load x64/x86 1.282).
  - geo1c_con: x86 32235168 - 13103824 - 133120 = 18998224; x64 41943040 - 16737616 - 133120 =
    25072304 (1.277).
- The x64 free is 3911120 to 6566592 bytes above x86 on every row. Reader `tables:cloth-x64`.

#### lua-bins.txt

- The x64 bin table of C-X64-LUA-BINS: 5232 rows (109 missions, 48 bins each), spill 0 and grown
  0 on every row. It differs from the x86 file by design (other bins, counts and 8-byte blocks).
  Reader `tables:lua-bins-x64`.

#### offthread.txt

- Headless section only: 163 case rows, all with count 0, sources 0 and dropped 0 (crash-fastfail
  `-` as on x86). The only row that differs from the x86 headless section is the case name
  `crash-delayload-x64` for `crash-delayload`, a separate x64 Crash case by design (its digest
  row maps to the shared `crash-delayload`); no new off-thread source exists on x64.
- No `## sound-on` section in 49-13: it needed an x64 sound-on run (`soundon.ps1`), which crashed
  until Phase 50 AV-03. 50-13 added it (C-X64-TABLES-SOUNDON). The reader stops at `## sound-on`
  on both platforms. Reader `tables:offthread-x64`.
- Falsifier: wrong if a fresh x64 hog capture at the same game code differs below `# commit`, or
  if a Win32 table under `tests\baseline\v143` changes.
- Status: pinned, 49-13, 2026-11-21.

### C-X64-TABLES-SOUNDON

- No digest row moved. No x86 table moved. In this folder only `offthread.txt` changed: its
  `# commit` line and a new `## sound-on` section. Every row between them is unchanged.
- Contents: in the C-X64-TABLES family, the x64 `## sound-on` section of `offthread.txt`. It has the
  same shape as the x86 section: `geo1c_xl`, `hot1g_xl` and `kas2c_xl` with `sound=opened`, then
  one off-thread row per mission, each `0 0 0` (count, sources, dropped). The values are the same
  as in the x86 section, so there is no off-thread source on x64 with sound and the renderer on.
- Why it was missing: x64 sound-on crashed in `Snd::DSBufferRenderer::UpdateObject` until the
  AV-03 `DSBufferRenderer::ResetGain` fix of Phase 50 (guard row `RESETGAIN-X64`), so 49-13 pinned
  the headless section only (the 49-13 note in C-X64-TABLES `offthread.txt`).
- Runs, x64 Test, hog, Steam running, exe from 8b9e9406:
  - headless section: `run.ps1 -Platform x64 -Configuration Test -Smoke All -HogAddrSpace`,
    `artifacts\tests\20261122-163427`. All 163 cases pass, and `tables:offthread-x64` passes
    against the 49-13 pin.
  - sound-on: `soundon.ps1 -Platform x64`, run twice, into `artifacts\gate50\p13\sound1` and
    `sound2`. Both runs pass for all four lines: three missions with `sound=opened offthread=1` and
    a `-nosound` control with `sound=not opened`. Each mission log has `OPENSWBF2 SOUND open=ok` and
    `OPENSWBF2 MEMREPORT offthread count=0 sources=0 dropped=0`.
  - captures: `capture-vs2013.ps1 -Platform x64 -Configuration Test -NoBuild -Tables -Baseline
    v143-x64 -OffThreadDir <run> -OffThreadSoundDir <sound1|sound2>`, into
    `artifacts\gate50\p13\cap1` and `cap2`. The two `offthread.txt` are byte-identical, the
    `# commit` line included.
  - Everything between `# commit` and `## sound-on` is byte-identical to the 49-13 pin.
  - The 50-12 folder `artifacts\gate50\p12\soundon-x64` was not used: it ran an older exe, and the
    capture refuses it (`map timestamp mismatch`).
- Recorded, not compared: `compare-tables.ps1` and `repin.ps1` stop at `## sound-on`, as on x86, so
  no verdict reads the section.
- Falsifier: wrong if a fresh `soundon.ps1 -Platform x64` capture at the same game code differs below
  `# commit`, or if any x64 sound-on run logs a non-zero off-thread count.
- Status: pinned, 50-13, 2026-11-22.

### C-HEAPCAP-X64-Lua-RETUNE

- No digest row moved. x64 `kLuaHeapSize` 0x1570000 (logged cap 22478848, was 0x1BE0000 and
  29229056 in C-HEAPCAP-X64-Lua of `tests\baseline\v143\CAUSES.md`); x64 `kArenaSize` 0x54D2160
  (was 0x5B42160), so Runtime keeps its x64 cap 41943040. x86 stays 0x1000000 and 0x4000000.
- Peaks: x86 2617536 (kam1c_c, turn1500), x64 3500728 (kam1c_c, turn1500) after the 49-08 bin
  re-tune (was 4553172 with the x86 bins); x64/x86 1.337. ceil(3500728 * 16777216 / 2617536) =
  22438076, rounded to 22478848 (C-HEAPCAP-X64-RULE).
- Cause: the x64 Lua heap peak fell because the re-tuned bins serve the x64 sizes from pools
  instead of single heap blocks (C-X64-LUA-BINS); the cap follows the rule. Runtime (peak
  25162848, derived 41943040) and Application (175104, 22413312) did not move. The arena is the
  previous one minus the 0x670000 the Lua heap gave back.
- Proof: the x64 boot run `artifacts\tests\20261121-133836` logs Lua 22478848 and Runtime
  41943040, which `compare-tables.ps1 -Platform x64 -MemPeaksX64 -RunLog` derives from the new
  `mem-peaks.txt`. Win32 Release and Test `GameMemory.obj` and `pcMain.obj` are unchanged
  (`coffcmp.py` same 2, diff 0 each). Guard row `HEAPCAP-X64` re-anchored to the new line;
  `crash-buildheapfail` expects the x64 size 22478848.
- Status: Fixed, 49-13, 2026-11-21.

### C-X64-RUNTIME-ALLOCS

- No digest row moved. No table changed: this entry explains Runtime `allocs` values already pinned
  in `mem-peaks.txt` (C-X64-TABLES), and no game code changed.
- Rows: x64 Runtime allocs minus x86 Runtime allocs equals the extra x64 pool growths in 104 of
  109 missions. In the exit rows of geo1c_xl, hot1g_xl, kas2c_uber, kas2c_xl and kas2g_xl it is
  1, 2, 5, 2 and 2 more.
- Callsite (both maps, census build of plan 49-14):
  - pages: `util::VNewAllocator::Allocate < util::SubAllocator::AllocateEx <
    util::SubAllocator::Allocate < pvs::Sector::RegisterDynamic < pvs::Manager::RegisterDynamic <
    EntitySoldier::EntitySoldier`, 1024 bytes on both platforms;
  - free list: `util::BaseArray<void *>::Reserve_ < util::SubAllocator::DeallocateEx <
    util::SubAllocator::Deallocate < pvs::Sector::UnregisterDynamic < GameObject::~GameObject`,
    2048 bytes first request on both platforms.
  - The allocator is `pvs::displayableSuballoc_` (`Portal.cpp`), a
    `util::StaticSubAlloc<util::SLNode<pvs::IDisplayable *>>` behind every `pvs::Sector::dynamics_`
    list.
- Size quantity: `SLNode<pvs::IDisplayable *>` is a pointer and a next pointer, 8 bytes on x86 and
  16 on x64. `StaticSubAlloc` calls `SetElemSize(sizeof(T), 0x1000)`, which halves the page while
  it holds more than 5 nodes, so the page is 1024 bytes on both platforms: 128 nodes on x86, 64 on
  x64. The free list (`Vector<void *>`) reserves `max(2 * nodes per page, 16)` entries, 256 on x86
  and 128 on x64.
- Rule: pages are allocated when the free list is empty and are not freed before
  `pvs::IManager::ClearInstance`, so the page count is ceil(peak live nodes / nodes per page). The
  free list doubles (`GrowArray`) each time a `DeallocateEx` pushes it past its capacity, so its
  growths are the doublings from the reserve to the peak free-list length. A smaller node count
  per page therefore gives x64 more pages for the same nodes, and a smaller reserve gives more
  free-list growths.
- Decision: a pointer-width effect, not a defect. The page byte size is the same and the node
  grows with pointer width; nothing is limited. `physLimit_` (0x40000000) is compared with a total
  that starts at 0 on each `Allocate` call, so it never refuses; there is no entry cap; every
  Runtime row of both tables and every census block has `failed=0`, so the no-page and failed
  free-list paths were never taken. Growth happens on demand, so the step changes slack, never a
  limit (the C-X64-POOL-ENTRIES growth-step standard).
- Pinned rows with a non-zero residual (x86 and x64 count at the callsite, from the census):

  | mission | source | pages x86 | pages x64 | free list x86 | free list x64 | residual |
  |---------|--------|----------:|----------:|--------------:|--------------:|---------:|
  | geo1c_xl | exit | 1 | 2 | 0 | 0 | 1 |
  | hot1g_xl | exit | 1 | 3 | 0 | 0 | 2 |
  | kas2c_uber | exit | 3 | 7 | 1 | 2 | 5 |
  | kas2c_xl | exit | 2 | 4 | 0 | 0 | 2 |
  | kas2g_xl | exit | 2 | 4 | 0 | 0 | 2 |

  The page counts are the `RegisterDynamic` pages; the one page `pvs::Sector::Sector` takes is
  equal on both platforms. In every other pinned row both callsites have equal counts, so the
  rule adds 0. The T1 `x64 runtime allocs equal the extra pool growths plus their named sources`
  (`tests\unit\pool_entries.cpp`) checks every Runtime row of both tables against the extra pool
  growths plus these entries, and fails on an entry that no longer matches.
- Census: a Test-only per-callsite count at both `allocs` increments for the Runtime heap
  (`artifacts\spike49\p14\census.diff`, fixed table, no allocation), reset with the Runtime heap
  stats. All 109 All missions on both platforms (`artifacts\spike49\p14\census\all-x64`,
  `all-Win32`; x64 under `-hogaddrspace`): 436 blocks, census total equal to the logged Runtime
  `allocs`, 0 dropped, every `heap=` and `pool=` line equal to the committed Test runs
  (`artifacts\tests\20261121-134457`, `-144200`) and equal SIM DIGEST lines (`census\closure.txt`).
  In all 218 mission and source pairs the pool growth callsite differs by exactly the extra pool
  growths and the other callsites sum to the residual (`census\residual.txt`).
- Toggle (`artifacts\spike49\p14\toggle\toggle.txt`): `RES49_P14_TOGGLE=1` gives `SetElemSize`
  twice the page bytes on x64 and half on Win32, so each platform gets the other's nodes per page.
  On x64 the residual of all five missions goes to 0 in both blocks (kas2c_xl turn1500 keeps the
  1 of the cloth callsite below); on Win32 the counts become x64's. Only these two callsites
  change, the controls kas2c_con and geo1c_con do not change, every `pool=` line is equal, and
  the SIM DIGEST lines are equal with the toggle on and off. Runs `toggle\off1-x64`, `on1-x64`,
  `off1-Win32`, `on1-Win32`.
- Second callsite, no pinned row: `util::VNewAllocator::Allocate <
  util::ListAllocatorImpl::ExternalAlloc_ < util::ListAllocatorImpl::Allocate < util::Allocate_ <
  EntityCloth::Init` (8192-byte cloth blocks). `DLNode<Buffer>` is 16 bytes on x86 and 32 on x64,
  so each cloth allocation takes 16 more bytes and x64 can need a block earlier. This happens
  only in unpinned blocks: the exit block after the soak of cor1c_ctf (+2), myg1c_ctf, pol1c_ctf,
  tat2c_ctf and uta1c_1flag (+1 each), and kas2c_xl turn1500 (+1). With `RES49_P14_TOGGLE=2`
  (Win32 requests 16 bytes larger) Win32 matches x64 in all six and nothing else changes (runs
  `toggle\off-Win32`, `on2-Win32`, `off-x64`). The extra blocks below the cloth ceiling are a
  pointer-width growth step: blocks are taken on demand, so the wider header changes slack, not
  a limit. The ceiling part (x64 holding fewer cloth allocations than x86 at the byte ceiling
  `maxmem_`) is fixed by `C-X64-CLOTH-CEILING`; the fullest block, hot1g_xl exit, uses 22 of the
  28 x86-width blocks on both (`census\cloth-ceiling.txt`).
- Falsifier: wrong if a Runtime row of either table changes without the T1 failing, or if a
  census of the same game code gives other counts at either callsite.
- Status: explained, 49-14, 2026-11-21.

### C-X64-CLOTH-CEILING

- No digest row moved. No x86 table moved. No table in this folder changed.
- Cause: the cloth `ListAllocator` (one caller, `EntityCloth::GetSpecialAllocator`) charges whole
  pages (the base page, 8192-byte pages, `size + 2 * sizeof(DLNode<Buffer>)` pages) against the
  byte ceiling `maxmem_`, and every page holds one `DLNode<Buffer>` header per block. The header
  is 16 bytes on x86 and 32 on x64, so the same ceiling holds fewer cloth allocations on x64: a
  request of 4064 bytes fits twice in an x86 page and once in an x64 page, and 131040 bytes fit
  the x86 base page of 131072 but not the x64 one. With `RES49_P14_TOGGLE=2` (Win32 requests 16
  bytes larger) Win32 matches x64 in all 34 pattern rows and all 57 mission rows below.
- Rule (C-X64-POOL-ENTRIES standard, decided for 49-15): at the ceiling x64 never holds fewer
  cloth allocations than x86, with the smallest x64 budget that guarantees it, and headers stay
  their natural width.
- Fix: `ListAllocatorImpl::WideCeiling_` is the ceiling in the `ExternalAlloc_` compare. For base
  B and x86 ceiling C it returns the larger of C and, for every rounded request size R up to
  max(B, C), the smallest x64 ceiling at which R repeated reaches the x86 count for R:
  `B + ceil((N16 - NH_base) / perPageH) * pageH`, where `Nh = floor((B - h) / (R + h)) +
  floor((pg - h) / (R + h)) * floor((C - B) / pg)`, `pg = max(8192, R + 2h)`, `NH_base` is the x64
  base page alone and H is `sizeof(DLNode<Buffer>)`. The loop runs only when the header is wider
  than the x86 16 bytes, so on x86 it returns C: every Win32 Release (1069) and Test (807) object
  is byte-identical to before (`artifacts\gate49\p15\coffcmp-Release.txt`, `coffcmp-Test.txt`).
  `GameState.cpp`, the logged `low`/`high` and `FormatCloth` are unchanged.
- Evidence (`artifacts\spike49\p15\design.txt`, `design-counts.txt`, driver outputs
  `driver\fx-*`, `driver\bigfx-*`; the driver builds `ListAllocator.cpp` at both widths):
  - Requests: all 109 All missions on both platforms, Test-only cloth log
    (`artifacts\spike49\p15\cloth-log.diff`, runs `all-x64`, `all-Win32`): the same create,
    setmax, init and free stream on both, 17 distinct per-cloth patterns, no init refused
    (`cloth-requests.txt`).
  - Before the fix x64 holds fewer than x86 in 23 of 34 pattern rows (patterns repeated from base
    131072 and 32768 to the 262144 ceiling), 41 of 57 mission rows (each mission's real order,
    then its inits repeated), 237 of 560 synthetic, 1203 of 8192 single sizes, 168 of 400 random
    mixed and 217 of 300 random orders with frees.
  - With the fix x64 is below x86 in 0 of 9549 such rows and 0 of 34244 rows of single sizes up to
    max(B, C).
  - Minimal: the ceiling equals the smallest fixed ceiling that reaches the x86 count for the
    worst request size: 499712 for base 32768 and 262144 (R 4064), 401408 for base 131072 and
    262144 (R 4064), 65568 for the load-time allocator of base and ceiling 32768 (R 32736). A
    byte less fails that row. No mixed or ordered row needs more than its configuration's worst
    single size.
  - Rejected: the 49-06 scaling `maxmem_ * sizeof(void*) / 4` is below x86 for the load-time
    allocator (65536 against 65568) and above the minimum elsewhere; `C + (C - B) + 8192` is
    below x86 when the growth part is small against the base (base 131072, ceiling 163840, R 32
    needs 221184); a credit of 16 bytes per header is below in 23 of 34 pattern rows.
- T1 `cloth list allocator holds as many allocations at its ceiling on x64 as on x86`
  (`tests/unit/list_allocator.cpp`): the 34 pattern rows, 6 worst-size rows and 57 mission rows
  through `ListAllocator::Create` and `SetMaxMem`, against x86 counts held as literals (equal on
  Win32, at least equal on x64). Before the fix it passed on Win32 and failed 70 rows on x64
  (`artifacts\gate49\p15\t1-red-x64.txt`, e.g. `pattern 300,60 base 131072 ceiling 262144 count
  868 x86 916`, `pattern 4064,4064 base 32768 count 35 x86 64`); after it, it passes on both.
  Guard row `CLOTHCAP-X64` reverts the compare to `maxmem_`.
- Consumers: no cloth init was refused in any x64 run, so no run reached the old ceiling, also
  during load; no allocation below it changes. `cloth-headroom.txt`, `mem-peaks.txt`,
  `pool-sizes.txt` and the digests compare PASS on the fixed x64 Release exe under
  `-HogAddrSpace` (`artifacts\tests\20261121-172725`), so no heap peak and no cap moves
  (`GameMemory.h`, `HEAPCAP-X64`, `crash-buildheapfail`, `hcCaps` unchanged), and the
  `C-X64-RUNTIME-ALLOCS` census and `kRuntimeAllocSources` are unaffected. Runtime bound:
  `iHigh <= iFree` is unchanged; the x64 byte ceiling is at most 499712 for every game input
  (iLow 32768 to 131072, iHigh up to 0x40000) and at most iHigh + 237568, and the smallest x64
  `free` in `cloth-headroom.txt` is 16843920, so the x64 ceiling stays below the free Runtime
  bytes in every mission.
- Falsifier: wrong if any cloth request order holds fewer allocations on x64 than on x86 at the
  same base and ceiling, or if a smaller x64 ceiling reaches the x86 count for every request size.
- Status: Fixed, 49-15, 2026-11-21.
