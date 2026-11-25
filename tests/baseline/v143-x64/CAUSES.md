# v143-x64 causes

Every pinned x64 table and every x64 change to it, one line each: `` `<table> <row>` | <cause> | <the cause> ``. A row key is the row's key columns joined by `/`, and `*` means every row of the table. A subject that is not a table row names what the cause pins. The detail of each cause is in the git history of this file.

## Rows

- `mem-peaks *` | C-X64-TABLES | Re-captured after the x64 Lua bin re-tune and the Lua heap cap; every row other than the Lua heap and `LuaN` pool rows equals the previous x64 pin (C-MEMPEAKS-X64-PARITY in the v143 folder).
- `mem-peaks *` | C-HEAPCAP-X64-Lua-RETUNE | The x64 Lua heap row moves in cap, peak, used and allocs because the re-tuned bins serve the x64 Lua sizes from pools.
- `mem-peaks *` | C-X64-RUNTIME-ALLOCS | x64 Runtime `allocs` exceed x86 by the extra x64 pool growths, because pool pages hold fewer pointer-width nodes.
- `pool-sizes *` | C-X64-TABLES | Equal to the x86 `pool-sizes.txt`, because script pool sizes are counts that do not depend on pointer width; kept as its own file.
- `cloth-headroom *` | C-X64-TABLES | Every row differs from x86 in `free` only, because the Runtime cap and the bytes used after load both depend on pointer width.
- `lua-bins *` | C-X64-LUA-BINS | The x64 Lua bin table, 48 bins per mission with spill 0 and grown 0, tuned for the x64 Lua object sizes.
- `lua-hist *` | C-X64-LUA-HIST | The x64 Lua demand per 4-byte bin, larger than x86 because Lua objects hold pointers; the bin re-tune works from it.
- `offthread *` | C-X64-TABLES | The headless section has the x86 rows with `crash-delayload-x64` for `crash-delayload`, all 0, because no off-thread source is new on x64.
- `offthread sound-on` | C-X64-TABLES-SOUNDON | The x64 sound-on section has the x86 shape and values once x64 sound-on runs stopped crashing.
- `commit *` | C-X64-COMMIT-TABLE | x64 `commit-flat` compares the settled game-owned commit with a pinned row per Steam state, hog mode and game state.

## Subjects with no table row

- `folder v143-x64` | C-X64-BASELINE-FOLDER | Pool block sizes, blocks per page and Runtime free bytes depend on pointer width, so the x64 build's layout-dependent tables live in their own folder.
- `commit rest` | C-X64-COMMIT-STEAM | The commit rest counted private regions the game does not own, such as the Steam overlay's, so the Test build keeps a ledger of its own reservations.
- `limit pool entries` | C-X64-POOL-ENTRIES | The 16 MiB pool byte ceiling was written for 4-byte pointers, so the entry byte limit scales with `sizeof(void*)` to keep x86 counts on x64.
- `heapcap x64-Lua` | C-HEAPCAP-X64-Lua-RETUNE | x64 `kLuaHeapSize` is 0x1570000 from the peak ratio 1.337 after the bin re-tune, and `kArenaSize` keeps the x64 Runtime cap.
- `limit cloth ceiling` | C-X64-CLOTH-CEILING | The cloth allocator charges a pointer-width node per page against its byte ceiling, so x64 widens the ceiling to hold at least the x86 allocations.
