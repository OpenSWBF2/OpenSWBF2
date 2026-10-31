# x64 census: PTR-02 files before and after Phase 22

This table compares census-1 (a lower bound), the untracked post-PTR-01 run census-22-pre (the honest before; counts copied from the Phase 22 plan 01 summary) and census-1b (after), for the PTR-02 files only.

Counts are unique (file, line, code) x64 sites, given as C4311/C4312/C4302. The census-1b numbers come from `census-1b.csv`; a file with no row there has 0/0/0. The x64 obj column is taken from `census-1b-tu.csv`.

| File | census-1 | census-22-pre | census-1b | census-1b x64 obj |
|------|----------|---------------|-----------|-------------------|
| `src\RedEngineFL\Memory\RedMemory.cpp` | 4/3/4 | 4/3/4 | 0/0/0 | yes |
| `src\Battlefront2\Source\GameMemory.cpp` | 3/3/3 | 3/3/3 | 0/0/0 | yes |
| `src\Battlefront2\Source\ConnectivityGraph.cpp` | 1/1/1 | 1/1/1 | 0/0/0 | yes |
| `src\SoundFL\source\sndallocbitmap.h` | 0/0/0 | 0/0/0 | 0/0/0 | header |
| `src\SoundFL\source\sndallocbitmap.cpp` | 0/0/0 | 0/0/0 | 0/0/0 | yes |
| `src\SoundFL\source\sndsamplebankbase.h` | 0/0/0 | 0/0/0 | 0/0/0 | header |
| `src\SoundFL\source\sndsamplebankbase.cpp` | 0/1/0 | 0/1/0 | 0/0/0 | yes |
| `src\SoundFL\source\win\sndsamplebank.cpp` | 0/1/0 | 0/1/0 | 0/0/0 | yes |
| `src\SoundFL\source\win\sndengine.cpp` | 1/0/1 | 1/0/1 | 0/0/0 | yes |
| `src\SoundFL\source\sndfreelist.cpp` | 1/1/1 | 1/1/1 | 0/0/0 | yes |
| `src\Battlefront2\Source\GameVoiceChatDecoder.cpp` | 3/3/3 | 3/3/3 | 0/0/0 | yes |
| `src\Battlefront2\Source\GameVoiceChat.cpp` | 3/3/3 | 3/3/3 | 0/0/0 | yes |
| `src\RedEngineFL\Graphics\PC\pcRedRenderer.h` | 0/0/0 | 0/0/0 | 0/0/0 | header |
| `src\RedEngineFL\Graphics\PC\pcRedRenderer.cpp` | 0/0/0 | 0/0/0 | 0/0/0 | no |
| `src\Battlefront2\Source\NetGame.cpp` | 0/0/0 | 0/0/0 | 0/0/0 | no |
| `src\Battlefront2\Source\EntityFlyer.cpp` | 1/0/1 | 1/0/1 | 0/0/0 | yes |
| `src\Battlefront2\Source\EntityPathFollower.cpp` | 1/0/1 | 1/0/1 | 0/0/0 | yes |
| `src\Battlefront2\Source\SoldierAnimationData.cpp` | 1/5/1 | 1/5/1 | 0/0/0 | yes |
| `src\PebbleFL\Common\PblPtrOrder.h` | 2/0/2 | 2/0/2 | 0/0/0 | header |

All 63 sites in the census-22-pre site list are gone.

## Remaining sites

None. Every PTR-02 file is at 0/0/0 in census-1b.

## Limit of the measurement

`pcRedRenderer.cpp` and `NetGame.cpp` still produce no x64 object, so their 0/0/0 is incomplete, as it was in census-1 and census-22-pre. Neither file has an x64 error of its own: the x64 log has no compile line for them, because the x64 batch fails on the three x64 error sites that are left. Those sites are owned by later work:

- `src\Battlefront2\Source\NetPacket.h:174` C2338 (`NetPktGroup layout`, an x86-only layout pin; `NetGame.cpp` includes `NetPacket.h`): Phase 25, ROADMAP scope "Header spacers for addressed members ... are modelled".
- `src\Battlefront2\Source\pcMain.cpp:493` C2440 (WNDPROC): Phase 23, ROADMAP scope "`WindowProc`, `DlgProc` and the keyboard hook use `WPARAM`/`LPARAM`/`LRESULT`/`INT_PTR`".
- `src\Battlefront2\Source\GameSoundEngine.cpp:157` C2440 (`SndAlloc(unsigned int)` callback): not a PTR-02 file; carried as a deferred item from Phase 22.

After Phase 23 and Phase 25 land, rerun the census on both files. Any C4311/C4312/C4302 found there then goes to a gap-closure plan.
