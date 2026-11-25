# v143 against the VS2013 baseline

Every difference between the v143 Release build and `tests/baseline/vs2013/`, with its cause.

`tests/baseline/repin.ps1` copies a row only when a cause entry below holds the line
`row: <table> <key>` for it, where `<key>` is the row's key columns joined by `/`.

## Toolchain

| Key | VS2013 | v143 |
|-----|--------|------|
| toolset | v120_xp | v143 |
| cl | 18.00.40629 | 19.44.35229 |
| MSVC tools | 12.0 | 14.44.35207 |
| CRT | msvcr120.dll 12.00.40664.0 | ucrtbase.dll 10.0.26100.9711, vcruntime140.dll 14.51.36247.0 |
| Windows SDK | 7.1A | 10.0.26100.0 |
| Windows build | | 26200.9711 |
| DXSDK | June 2010 | June 2010 |

Two builds are named below. The Phase 20 build is the first v143 build (commit 1b1cbca, exe
SHA-256 `403C48C2...86BB14ED`). The fixed build is the main tree after 20-14 (commit a2c980c,
exe SHA-256 `17A3A864...30A71885`), which neutralises C1, C2 and C3 and fixes C-OOB-SIDE (see
`## Causes`). The tables in this directory come from the fixed build.

## Digest missions

`Compare-DigestTrace tests\baseline\vs2013\digest-trace-<m>.txt tests\baseline\v143\digest-trace-<m>.txt`
returns the first split turn (0 = equal). Each v143 trace has 1600 turns. The split turns were
re-measured on the fixed build.

| Mission | Final digest (turn 1500) | First split turn | Cause |
|---------|--------------------------|------------------|-------|
| geo1c_con | same | 0 | none in the window; C2 after turn 1600, see below |
| nab2g_c | same | 0 | none |
| hot1g_con | same | 0 (1397 on the Phase 20 build) | C1, fixed; see below |
| spa3c_ass | same until 26-06, then differs | 0 until 26-06, then 555 | C-SLOT-SOLDANIM since 26-06 (fixed, digest re-baselined); C2 after turn 1600, see below |

`digests.txt` and `digests-hog.txt` are byte-identical to VS2013, and every `state=` matches
`tests/smoke/digest-expected.txt`, where no row has changed. No reference digest moved in Phase 20.
In 20-15 the Standard group gained `digest-dag1c_ctf` and `digest-kas2c_uber`, whose rows are the
VS2013 turn-1500 digests of the C1 mission dag1c_ctf (first split 202) and the C2 mission
kas2c_uber (first split 636); guard-revert rows SIMFIX-C1S and SIMFIX-C2S turn them RED when the
C1 or C2 fix is reverted (SIMFIX-C2S no longer did on the Phase 28 merged tree and was removed, see C-D14-RECHECK).

In 26-06 the `digest-spa3c_ass` row moved for cause C-SLOT-SOLDANIM, the only row changed since
Phase 20. The `spa3c_ass` traces and digests in this directory are from before that fix.

hot1g_con: on the Phase 20 build only turns 1397 and 1398 differed, and only in `state=`
(`0x1A9B6C97` vs `0x6D1DD393`, `0x38735EB1` vs `0xAFCEE16D`). `rand=` was identical on every turn,
and turn 1399 onward was identical. Field-level logging built under both toolchains on the same
staged data pins it to character 32 (team 2, world key 554088578): `position.y` was 1 ulp lower on
v143, `BF22BD23` vs `BF22BD24` at turn 1397 and `BF247374` vs `BF247375` at turn 1398 (about
-0.636). Velocity, health, every other character and world object, all command posts and the
mission time were identical. At turn 1398 the velocity is `00000000 BF7A7EA0 80000000`, a
landing/settle frame, and the next turn writes the position back to the same value on both
builds, so the run reconverges at 1399. The cause is C1: the NEUTRAL-C1 build removes both turns
and NEUTRAL-C2 leaves them. On the fixed build `digest-trace-hot1g_con.txt` equals the VS2013
trace on all 1600 turns.

Window: the 4 reference digests match VS2013 through turn 1600 (hot1g_con 1397-1398 excepted on
the Phase 20 build, as above). On the Phase 20 build geo1c_con first split at turn 1629 (first
`rand=` split 2911) and spa3c_ass at turn 2445 (first `rand=` split 35967), both after the
1600-turn trace window. Those later splits are why their final scores differed while their
turn-1500 digests and reference traces matched (see `## Final score vs VS2013`). Both later
splits are C2. On the fixed build both equal VS2013 in every turn to game over, geo1c_con over
all 9014 turns.

The `<cmath>` float overloads are load-bearing. A v143 build without them splits geo1c_con at turn
127 and changes its turn-1500 digest (`state=0xED7EDAC9` instead of `0x16D65714`).

## Causes

Every v143 difference from VS2013 in the sim and in `finalscore.log` has one of these five
causes (C1, C2, C3, C-OOB-SIDE and, since 26-06, C-SLOT-SOLDANIM). C1 and C2 were found with the D-15 bisect (code-bug screen, then `<cmath>`, `/fp`, CRT,
per-TU toolchain) on the first split of dag1c_ctf and geo1c_con. Each mission was then attributed
by experiment, see `## -Smoke All vs VS2013`. C3 was found with the same bisect on the one
geo1c_xl turn that still differed with C1 and C2 neutralised. The evidence (field and stage
traces, neutral builds, probes) is kept under `artifacts\gap20`, which is not tracked.

### C1

- Function: `RedTerrain::GetInterpolatedHeight` in `src/RedEngineFL/Application/RedTerrain.cpp`
  (TU `RedTerrain.cpp`), `// FUNCTION: SWBF2 0x006c1720`, line 241 on the Phase 20 build: the last
  branch, `fH = (GetHeight(iX + 1, iZ + 1) - fH0) * fFracZ`.
- Mechanism: float codegen. VS2013 emits `fstp [ebp-8]; movss xmm0,[ebp-8]; subss xmm0,[ebp-4];
  mulss xmm0,[ebp+0Ch]`, which rounds to float after the subtract and after the multiply. cl 19.44
  emits `call GetHeight; fsub dword ptr [ebp-4]; fmul dword ptr [ebp+0Ch]; fstp dword ptr [ebp-8]`,
  which keeps the difference on the x87 stack at 53-bit precision and rounds once after the
  multiply. `/fp:precise` allows both. The other three branches are SSE on both builds, so 1
  terrain sample in 4 is affected (even row, `fFracZ <= fFracX`). At dag1c_ctf turn 202 every
  input is bit-identical and the height is `3DA8635E` on VS2013 against `3DA8635C` on v143. That
  height feeds the collision depth, then `mOverlap`, then the soldier's `position.y` snap, which
  is why most C1 missions first split in one soldier's `position.y` (1 ulp of a 0.125-1 height,
  2^-26 to 2^-24 absolute).
- Class: toolchain-inherent. The code is well-defined float arithmetic.
- D-15 step that reproduced it: step 4, per-TU toolchain. `RedTerrain.cpp` compiled by VS2013 with
  the v120 flags and linked into the v143 exe gives `3DA8635E`, and dag1c_ctf then equals VS2013
  over all 1600 turns. The steps before it did not hit: the code-bug screen found no
  out-of-range index, no NaN or out-of-range conversion and no pointer-dependent order;
  `<cmath>`: the path calls only `RedTerrain::GetHeight` and inline `cvttss2si`/`cvtdq2ps`, so
  there is no math call to swap; `/fp`: the compile flags are the same, and `/fp:strict` on
  `RedTerrain.cpp` alone keeps the x87 pair and the split at 202; CRT: the path calls no libm or
  helper symbol, and a probe built against msvcr120 and against the UCRT prints the same bits.
- Neutralising experiment: NEUTRAL-C1 forces the line 241 subtract and multiply to SSE single
  with `_mm_sub_ss`/`_mm_mul_ss`, and dag1c_ctf then equals VS2013 over 1600 turns (first split
  202, then none).
- Status: neutralised in the main tree in 20-12 (a381f01) with the same `_mm_sub_ss`/`_mm_mul_ss`
  form. T1 `terrain interpolated height rounds each step to float` pins it, and guard-revert rows
  SIMFIX-C1 (T1) and SIMFIX-C1S (the `digest-dag1c_ctf` smoke row) go RED when it is reverted.
  Fix check: every C1 mission and hot1g_con equals VS2013 in every traced turn on the fixed
  build, with the VS2013 final line.
- Phase 20 effect: the first split of 13 missions (dag1c_con, dag1c_ctf, dag1g_con, dag1g_ctf,
  end1g_1flag, end1g_con, hot1g_xl, kas2c_c, kas2c_xl, hot1g_hunt, kas2c_con, kas2g_con,
  kas2g_xl), the hot1g_con blip, and the later splits of kas2c_uber (1705) and pol1c_con (5909).

### C2

- Function: `AIDifficulty::GetTargetBubbleShrinkRate` in `src/Battlefront2/Source/AIDifficulty.cpp`
  (TU `AIDifficulty.cpp`) line 206, `// FUNCTION: SWBF2 0x00438ed0`. Its result is consumed by
  `fShrink = GetTargetBubbleShrinkRate(..) * dt` in `Controller::UpdateTargetBubbleSize`,
  `src/Battlefront2/Source/Controller.cpp` line 219.
- Mechanism: float codegen. VS2013 emits `divss; subss xmm0; movss [ebp+8]; fld [ebp+8]` and
  returns a value rounded to float. Under `/fp:precise`, cl 19.44 emits `call GetFloatSkill; fsubr
  dword ptr [__real@bdcccccd]; ret`, so `-0.1f - skill` comes back in ST0 at 53-bit precision
  without the rounding at return that the `/fp:precise` documentation describes. The caller's
  `fmul dword ptr [dt]` is the same instruction on both builds and rounds the product once. With
  identical inputs (skill 11, dt `3D0852D8`) `fShrink` is `BCAAC366` on VS2013 against `BCAAC365`
  on v143. From turn 5 every AI controller's `mAvailableBubbleCharge` is 1 ulp off. That charge
  drives the target-bubble random walk, then the aim bubble, the heading, the turn input, the
  soldier rotation (geo1c_con turn 1623) and the velocity (turn 1629), which is why most C2
  missions first split in a velocity with an equal position. Later the AI's Sim random draws
  differ (geo1c_con from turn 2911).
- Class: toolchain-inherent. The code is well-defined.
- D-15 step that reproduced it: step 2, `/fp`. `/fp:strict` on `AIDifficulty.cpp` alone makes v143
  round before the return (`subss/divss/mulss/subss; movss [ebp+8]; fld`), gives `BCAAC366`, and
  geo1c_con then equals VS2013 in all 9014 turns, final score included. The steps before it did not
  hit: the code-bug screen found integer skill from constant tables, no pointer order and no NaN
  or out-of-range conversion; `<cmath>`: there is no math call or intrinsic on either path.
- Neutralising experiment: NEUTRAL-C2 sets `/fp:strict` on `AIDifficulty.cpp` only, and geo1c_con
  equals VS2013 to game over (first split 1629, then none). It leaves dag1c_ctf at 202, so C1 and
  C2 are independent.
- Status: neutralised in the main tree in 20-12 (a381f01) with `<FloatingPointModel>Strict</FloatingPointModel>`
  on the `AIDifficulty.cpp` item in `BattlefrontII.vcxproj`. T1 `ai difficulty compiles with fp
  strict` pins it, and guard-revert rows SIMFIX-C2 (T1) and SIMFIX-C2S (the `digest-kas2c_uber`
  smoke row) go RED when it is reverted. Because cl rejects `/fp:strict` together with
  `/fp:contract` (D8016), guard-revert row FLAG-FPCONTRACT now goes RED by build failure on the
  first `/fp:strict` TU. Fix check: every C2 mission equals VS2013 in every traced turn on the
  fixed build, with the VS2013 final line. On the Phase 28 merged tree SIMFIX-C2S no longer went
  RED and was removed (see C-D14-RECHECK).
- Phase 20 effect: the first split of 16 missions (cor1g_con, kas2c_uber, myg1c_ctf, geo1c_con,
  geo1c_xl, kam1g_con, mus1c_con, nab2c_con, pol1c_con, pol1g_con, spa3c_ass, spa6c_ass,
  spa7c_ass, spa9g_ass, tat2g_eli, yav1c_con), and the later splits of dag1c_con (2862), dag1g_con
  (7440), end1g_con (7298) and kas2g_xl (8152).

### C3

- Function: `WeaponCannon::Fire` in `src/Battlefront2/Source/WeaponCannon.cpp` (TU
  `WeaponCannon.cpp`), `// FUNCTION: SWBF2 0x0067f320`, line 750: the yaw spread,
  `fYaw += (PblRandom::GetSim().GetFloat() - PblRandom::GetSim().GetFloat()) * GetYawSpread()`.
- Mechanism: float codegen, in the opposite direction from C1 and C2: here VS2013 keeps the x87
  precision and v143 rounds. `GetYawSpread` returns `mStanceSpread + fKick + fYaw` in ST0 at 53-bit
  precision on both builds (here 0 + 0.04247 + 0.004363, which needs 28 mantissa bits). VS2013
  emits `call GetYawSpread; fmul dword [r3-r4]; fadd dword [pattern yaw]; fstp dword [fYaw]`, so it
  multiplies and adds the unrounded return value on the x87 stack and rounds to float once. Under
  `/fp:precise`, cl 19.44 emits `call GetYawSpread; fstp dword [ebp-0B4h]; movss xmm2,[ebp-0B4h];
  mulss xmm2,xmm1; addss xmm2,[pattern yaw]`, which rounds the return value to float (`3D3FD5D5`)
  before an SSE single multiply and add. With every input bit-identical, `fYaw` is `BD040B0D` on
  VS2013 against `BD040B0E` on v143. One ulp of yaw gives one ulp of `sin(yaw)` and of the fire
  direction's x (`3ED036D4` against `3ED036D5`), then the bullet velocity, the hit direction in
  `OrdnanceBullet::Collide` and `EntitySoldier::DirectionKill`'s `mVelocity.x += vDir.x * 5.0f` on
  the killed soldier. The pitch line has the VS2013 shape on both builds. Only an AI shot whose
  yaw spread sum is not a float, followed by a kill, shows it in the digest, which is why it
  appeared once in 20226 turns.
- Class: toolchain-inherent. The code is well-defined float arithmetic.
- D-15 step that reproduced it: step 2, `/fp`. `<FloatingPointModel>Strict</FloatingPointModel>`
  on `WeaponCannon.cpp` alone makes v143 emit the VS2013 yaw sequence and gives `BD040B0D`. The
  steps before it did not hit: the code-bug screen found member floats with constructor-set
  values, Sim `PblRandom` draws evaluated in the same order with equal values, no pointer-dependent
  order and no NaN or out-of-range conversion; `<cmath>`: `cos` and `sin` take `(double)float`
  arguments and compile to the same `__libm_sse2_cos_precise`/`__libm_sse2_sin_precise` calls on
  both builds, and their argument already differs before the call. A probe that recomputes Fire's
  arithmetic both ways gives the VS2013 bits for the x87 sequence and the v143 bits for the SSE
  sequence under both msvcr120 and the UCRT.
- Side effect of `/fp:strict`: Fire's `cos` and `sin` become UCRT `_cos`/`_sin` calls. A sweep of
  8000002 float inputs (4000001 on [-pi, pi] and 4000001 random magnitudes from 2^-15 to 2^16)
  gives the same result hash (`319C91DB`) for VS2013 `/fp:precise` (msvcr120 `__libm_sse2_*`),
  v143 `/fp:precise` (UCRT `__libm_sse2_*`) and v143 `/fp:strict` (UCRT `_cos`/`_sin`), so the
  routine change does not change any `cos` or `sin` result.
- Neutralising experiment: NEUTRAL-C3 adds the `/fp:strict` item on `WeaponCannon.cpp` to a v143
  build that already has the C1, C2 and C-OOB-SIDE fixes, and geo1c_xl then equals VS2013 over all
  20226 turns to game over, `state=` and `rand=`, with the VS2013 final line (split 19926, then
  none).
- Status: neutralised in the main tree in 20-14 (a2c980c) with
  `<FloatingPointModel>Strict</FloatingPointModel>` on the `WeaponCannon.cpp` item in
  `BattlefrontII.vcxproj`. T1 `weapon cannon fire compiles with fp strict` pins it, and
  guard-revert row SIMFIX-C3 goes RED when it is reverted. Only that T1 source pin and SIMFIX-C3
  guard C3: no turn-1500 digest row can catch a split at turn 19926. Fix check: all 30 proof traces
  (the 29 drift missions and hot1g_con) equal VS2013 in every traced turn on the fixed build,
  `state=` and `rand=`, and every game-over mission has the VS2013 final line; geo1c_xl is equal
  over all 20226 turns.
- Phase 20 effect: none on any first split or final score. With C1 and C2 neutralised it was the
  geo1c_xl turn 19926 (char 119, team 2, `vel.x` `40D74737` against `40D74738`, 1 ulp), equal
  again from turn 19927.

### C-OOB-SIDE

- Function: `GameLoop::Update` in `src/Battlefront2/Source/GameLoop.cpp` (TU `GameLoop.cpp`),
  `// FUNCTION: SWBF2 0x005323e0`, line 1572 on the Phase 20 build: the `finalscore.log` write,
  `sSideNamePrefixes[pTeam->mSide]`.
- Mechanism: out-of-bounds read. `Team::TranslateTeamName` (`Team.cpp:291`) returns side_HEROES = 6
  and side_VILLAINS = 7 for the hero teams, and `sSideNamePrefixes` has 6 entries of `char[4]`.
  Index 6 and 7 read the 8 bytes after the table. In the VS2013 exe those are the next float
  constants (`00 00 A0 40`, `00 00 F0 41`), which print as empty names. In the v143 exe they are an
  absolute pointer that ASLR relocates at load, so the text changes from run to run (`?j?` and
  `\D?` in the Phase 20 table) and can run on into the next entry.
- Class: code bug. It does not touch the sim: the score, points, mission time and every traced
  turn are the same with and without the fix.
- D-15 step: none, it is not a sim cause. It was found in the 20-11 attribution as the only
  difference left in tat2g_eli's final line under NEUTRAL-C12, and the bytes behind index 6 and 7
  were read from each exe at the map address of `sSideNamePrefixes`.
- Neutralising experiment: the code-bug rule, a VS2013-toolset build plus a fix against a v143
  build plus the same fix (entries `Hro` and `Vil` for sides 6 and 7), gives identical tat2g_eli
  lines, `L=Hro 32u@11P -1r 178p W=Vil 32u@14 -1r 180p`.
- Status: fixed in 20-12 (c539eee). `SideNamePrefix` in `src/Battlefront2/Source/SideNames.h`
  returns the table entry for sides 0-5 and an empty name otherwise, which is the VS2013 text.
  T1 `side name prefix is bounded` and `final score uses the bounded side name`, guard-revert row
  SIDENAME-01. Fix check: tat2g_eli's final line equals VS2013's,
  `tat2g_eli 1371s L= 32u@11P -1r 178p W= 32u@14 -1r 180p`.

### C-SLOT-SOLDANIM

- Function: `AnimationFinder::AssignAnimation` in `src/Battlefront2/Source/SoldierAnimatorClass.cpp`
  (TU `SoldierAnimatorClass.cpp`), `// FUNCTION: SWBF2 0x00645170`, the `new SoldierAnimation(...)`
  calls (line 1232 in the first report), through `SoldierAnimation::operator new` into
  `SoldierAnimation::sMemoryPool` (`MemoryPool("SoldierAnimation", 0xc)`, `SoldierAnimation.cpp`).
- Mechanism: a 16-byte object in a 12-byte pool slot. The decomp class had a fourth member,
  `char* mName`, which nothing uses and which the retail slot size of 0xc rules out. `/sdl` zeroes
  the whole object (16 bytes) in `new` before the constructor runs, so every allocation overwrote
  the first 4 bytes of the next slot: the free-list link of a free slot (which ended the free list
  and made the pool grow early) or the `m_pZephyrAnim` of a live neighbour animation. Which of the
  two moves spa3c_ass was not separated.
- Class: code bug (decomp layout error plus a memory overwrite in every build).
- Evidence: the Asan smoke reported `use-after-poison` WRITE of size 16 in `_asan_memset` from
  `AssignAnimation` in every mission once `MemoryPool` was hooked (26-ASAN-FINDINGS row 4). A
  temporary `static_assert(sizeof(Owner) <= N)` after every literal-sized `MemoryPool` in `src`
  failed only for `SoldierAnimation`.
- Affected row: `digest-spa3c_ass` (Twice group) only. With the fix, the spa3c_ass digest trace
  equals the earlier v143 trace through turn 554 and splits at turn 555. The other five gated rows
  (geo1c_con, twice-digest-1/2, digest-hot1g_con, trace-seed-1), `digest-dag1c_ctf` and
  `digest-kas2c_uber` are unchanged, the geo1c_con, nab2g_c and hot1g_con traces are identical over
  1600 turns, all four `factory-<m>.txt` logs are byte-identical (creation order unchanged), and
  the `-hogaddrspace` spa3c_ass digest equals the normal one.
- Values: before `rand=529082281/2897934758 state=0x7385D175`, after
  `rand=2329233471/2392100820 state=0x247DCD93` (turns=1500 time=50.000 snd=49.923 on both), the
  same on Release and Test.
- Status: fixed in 26-06 by removing `mName` (`sizeof(SoldierAnimation)` is 0xc). T1 `soldier
  animation fits its pool slot`, guard-revert row SOLDANIM-01. The `digest-spa3c_ass` row in
  `tests/smoke/digest-expected.txt` was re-baselined by decision D-29. The `spa3c_ass` row (the
  `-Smoke All` case of the same mission) was not edited and was not measured in 26-06.

## Factory

All four `factory-<m>.txt` logs are byte-identical to VS2013. D-16 passed: Factory creation order,
and so `mnetIndex`, is unchanged.

## Initializers

| | VS2013 | v143 |
|-|--------|------|
| in-range initializers (`initializers.txt`) | 1681 | 944 |
| `??__E` dynamic initializers (`initializers-di.txt`) | 1695 | 949 |
| discarded (`initializers-discarded.txt`) | 17 | 8 |
| objs with initializers | 405 | 331 |

328 objs have initializers on both sides, and their order is identical (0 inversions). 75 game
objs no longer have a dynamic initializer because v143 constant-initializes them. One obj gains
one: `netDrag.obj` `bandwidthQueue` (`netPktGroup[3]`, the dev-only drag simulator). The CRT
sentinel objs change name (`cinitexe.obj`, `crtexew.obj` become `initializers.obj`,
`exe_winmain.obj`). Factory order is unaffected, as the identical factory logs show.

## Float-parse corpora

`odf.txt` (784 retail tokens): 0 of 784 differ. D-17 was not triggered.

`edge.txt`: 12 of 61 lines differ.

| Family | Lines | v143 result |
|--------|-------|-------------|
| long mantissa (`1.0000000596046447753906250001`) | 1 | `sscanf` rounds to `3F800001` |
| denormal (`1e-45`) | 1 | `sscanf` gives `00000001` |
| hex (`0x1p3`, `0x10`) | 2 | parsed as hex, 8 and 16 |
| dangling exponent (`1e`, `1e+`) | 2 | `sscanf` ret -1, target untouched |
| `1.5e-x` | 1 | `sscanf` ret 0, target untouched |
| nan/inf (`nan`, `nAn`, `inf`, `-inf`, `infinity`) | 5 | ret 1, non-finite |

On the game path the nan/inf and dangling-exponent tokens go through `OdfValue::Sanitize`
(TOOL-07), which gives the VS2013 results. The hex, long-mantissa, denormal and `1.5e-x` deltas
are accepted UCRT differences; every retail token in `odf.txt` parses identically.

## Math corpus

179 of 10636 lines differ, in Release and Debug.

| Form | Lines | Cause |
|------|-------|-------|
| `atan2.f`, `atan2.d` | 4, 4 | `atan2(±inf, ±inf)` is ±π/4 or ±3π/4 instead of nan |
| `fmod.f`, `fmod.d` | 25, 46 | `fmod(-0, y)` is -0 instead of +0; `fmod(x, ±inf)` is x instead of nan |
| `pow.ff`, `pow.dd` | 3, 3 | `pow(1, nan)` and `pow(nan, ±0)` are 1 |
| `pow.fi`, `pow.di` | 66, 28 | C++11 `pow(T, int)` promotes to double `pow`; VS2013 used `_Pow_int`, up to 1 ulp |
| sin, cos, tan, asin, acos, atan, sqrt, exp, log, log10, floor, ceil | 0 | |

The D-15 bisect proved no CRT function load-bearing for the sim. The CRT step did not hit for C1:
its path calls no libm or helper symbol, and the probe gives the same bits on msvcr120 and on the
UCRT. C2 and C3 were reproduced at the `/fp` step, before the CRT step, and the `_cos`/`_sin`
calls that the C3 fix brings in give the same results as the `__libm_sse2_*` routines (see `### C3`).
There is no `pow(float, int)` call site in game code. The sim drift on the Phase 20 build is the
codegen causes C1, C2 and C3 under `## Causes`, and with all three neutralised the fixed build
equals VS2013 on every attributed mission.

`math corpus x87 pc24`: the same 179 lines. Under `_PC_24`, Debug reports 273 `sqrt.d`/`atan.d`
lines following x87 precision control and Release reports 0, the same as VS2013.

## -Smoke All vs VS2013

`tests\run.ps1 -NoBuild -Smoke All` on the fixed v143 Release build: 169 PASS, 0 FAIL. The 150
`smoke:<mission>` rows match `tests/baseline/vs2013/smoke-all.txt` exactly, mission by mission and
class by class (Pass or TimeOut): 150 passing on VS2013, 150 passing on v143, and no mission
changed result. The Phase 20 build gave the same 150 rows. The other rows in `smoke-all.txt`
differ only because the v143 hardening block has the four `flag-*` checks (169 PASS instead of
165) and a different ASLR base.

`smoke-all-digests.txt` has the turn-1500 digest and the final-score line of every mission. On
the fixed build every row equals VS2013 except `dedicated`. On the Phase 20 build 137 of 150
`state=` values were identical to VS2013. The 13 that differed:

| Mission | First split turn | Turns that differ (of 1600) | First `rand=` split | First field | Cause |
|---------|------------------|-----------------------------|---------------------|-------------|-------|
| cor1g_con | 1463 | 134 | none | char 5 `vel.x`, 8 ulp | C2 |
| dag1c_con | 231 | 782 | none | char 4 `pos.y`, 1 ulp | C1, then C2 from 2862 |
| dag1c_ctf | 202 | 912 | none | char 25 `pos.y`, -64 ulp | C1 |
| dag1g_con | 681 | 920 | none | char 43 `pos.y`, -2 ulp | C1, then C2 from 7440 |
| dag1g_ctf | 293 | 1277 | none | char 36 `pos.y`, -256 ulp | C1 |
| end1g_1flag | 394 | 1195 | none | char 14 `pos.y`, 1 ulp | C1 |
| end1g_con | 683 | 348 | none | char 13 `pos.y`, -2 ulp | C1, then C2 from 7298 |
| hot1g_xl | 764 | 837 | none | char 8 `pos.y`, 4 ulp | C1 |
| kas2c_c | 1067 | 479 | none | char 53 `pos.y`, -1 ulp | C1 |
| kas2c_uber | 636 | 923 | none | char 7 `vel.x/y/z`, -2/-3/2 ulp | C2, then C1 from 1705 |
| kas2c_xl | 1065 | 532 | 1393 | char 163 `vel.y`, -1 ulp | C1 |
| myg1c_ctf | 1125 | 434 | none | char 14 `vel.y/z`, -2/-1 ulp | C2 |
| dedicated | | | | | not deterministic, see below |

Method: the archived VS2013 Release exe (`vs2013-release-1168dd4.zip`, the build that produced
`tests/baseline/vs2013/smoke-all.txt`) ran each mission twice with `-digesttrace 1600`, and the
Phase 20 v143 exe ran it once, on the same staged data. The two VS2013 runs are identical on every
turn (split 0) and their final digests equal the VS2013 table. Two full Phase 20 `-Smoke All` runs
gave the
same 12 values. geo1c_con, the control, is identical on all three. So both builds are
deterministic and the drift is between toolchains.

Attribution: the first field is the first differing field from field-level logging of a
VS2013-toolset build and a v143 build of the same tree over `[split-3, split+3]`, each checked
against its own toolchain's trace (char = character index, ulp = v143 minus VS2013). The cause is
from a neutral-build experiment: the NEUTRAL-C1 and NEUTRAL-C2 v143 builds, each with one cause
neutralised, and NEUTRAL-C12 with both, ran every mission to game over (TimeOut missions to turn
1600) against the VS2013 run. A mission belongs to the cause whose own neutral build moves its
first split later or removes it while the other cause's build leaves it at the same turn. No
mission was moved by both single-cause builds or by neither. Under NEUTRAL-C12 every mission
equals VS2013 in every traced turn, except tat2g_eli's side-name text (C-OOB-SIDE) and one turn
of geo1c_xl (C3, see `## Final score vs VS2013`). A later split that remains under the attributing build belongs to the other cause,
as the table shows. `rand=` stays identical to turn 1600 in 11 of the 12. In kas2c_xl the drift
reaches a random draw at turn 1393. That is C1: NEUTRAL-C1 removes both its `state=` split at
1065 and its `rand=` split at 1393, and NEUTRAL-C2 leaves both.

`dedicated` is a host and a client over loopback and is not deterministic: two v143 runs gave
`0xD9DDDD36` and `0x256999AB`. It passes on both toolchains.

## Final score vs VS2013

`finalscore.log` is written at game over: after 309 s for the CON missions and up to 1638 s for
assault (spa6c_ass), long after the turn-1500 digest. A split after turn 1600 does not show in the
digest or in a 1600-turn trace but still changes the score. On the Phase 20 build the final-score
columns of 24 missions differed from VS2013. Their first split turn and first field come from a
`-digesttrace` run to game over on both exes and the field window above. The cause is the
attribution experiment above. On the fixed build all 24 final lines equal VS2013.

| Mission | VS2013 | v143 (Phase 20) | Winner | First split turn | First field | Cause | Status |
|---------|--------|-----------------|--------|------------------|-------------|-------|--------|
| cor1g_con | 309s W=All 32u@9P 78r, L=Imp 32u@16 43r | 309s L=All 32u@11P 49r, W=Imp 32u@14 68r | flip | 1463 | char 5 `vel.x`, 8 ulp | C2 | now equal after fix |
| dag1c_con | 309s L=Rep 32u@11P 68r, W=CIS 32u@14 85r | 309s L=Rep 32u@11P 72r, W=CIS 32u@14 92r | same | 231 | char 4 `pos.y`, 1 ulp | C1, then C2 from 2862 | now equal after fix |
| dag1g_con | 309s L=All 32u@14P 40r, W=Imp 32u@11 89r | 309s L=All 32u@14P 50r, W=Imp 32u@11 99r | same | 681 | char 43 `pos.y`, -2 ulp | C1, then C2 from 7440 | now equal after fix |
| end1g_con | 309s W=All 29u@8P 104r, L=Imp 29u@17 64r | 309s W=All 29u@7P 109r, L=Imp 29u@18 55r | same | 683 | char 13 `pos.y`, -2 ulp | C1, then C2 from 7298 | now equal after fix |
| geo1c_con | 309s L=Rep 28u@15P 15r, W=CIS 28u@10 95r | 309s L=Rep 28u@15P 36r, W=CIS 28u@10 101r | same | 1629 | char 11 `vel.x/y/z`, 1/-1/-3 ulp | C2 | now equal after fix |
| geo1c_xl | 682s L=Rep 80u@11P 320p, W=CIS 80u@14 350p | 694s L=Rep 80u@11P 316p, W=CIS 80u@14 350p | same | 2261 | char 10 `vel.y/z`, 1/6 ulp | C2, then C3 at 19926 | now equal after fix; the turn-19926 residual is C3, fixed |
| hot1g_hunt | 231s L=Loc 32u@11P 28p, W=All 32u@14 54p | 231s L=Loc 32u@11P 33p, W=All 32u@14 51p | same | 731 | char 39 `pos.y`, 1 ulp | C1 | now equal after fix |
| hot1g_xl | 498s L=Imp 120u@11P 220p, W=All 120u@14 350p | 472s L=Imp 120u@11P 186p, W=All 120u@14 350p | same | 764 | char 8 `pos.y`, 4 ulp | C1 | now equal after fix |
| kam1g_con | 309s W=All 32u@10P 113r, L=Imp 32u@15 85r | 309s W=All 32u@11P 108r, L=Imp 32u@14 89r | same | 2154 | char 6 `vel.x/z`, -1/-2 ulp | C2 | now equal after fix |
| kas2c_con | 309s W=Rep 29u@11P 102r, W=CIS 29u@14 102r | 309s W=Rep 29u@11P 109r, L=CIS 29u@14 99r | flip | 5542 | char 27 `pos.z`, -1 ulp | C1 | now equal after fix |
| kas2c_uber | 309s W=Rep 50u@11P 7r, L=CIS 250u@14 3r | 309s W=Rep 50u@10P 11r, L=CIS 250u@15 3r | same | 636 | char 7 `vel.x/y/z`, -2/-3/2 ulp | C2, then C1 from 1705 | now equal after fix |
| kas2c_xl | 419s L=Rep 160u@11P 327p, W=CIS 160u@14 354p | 417s L=Rep 160u@11P 335p, W=CIS 160u@14 352p | same | 1065 | char 163 `vel.y`, -1 ulp | C1 | now equal after fix |
| kas2g_con | 309s W=All 29u@11P 122r, L=Imp 29u@14 112r | 309s W=All 29u@11P 119r, L=Imp 29u@14 111r | same | 3685 | chars 31 and 43 `vel.y`, -1 ulp | C1 | now equal after fix |
| kas2g_xl | 377s W=All 160u@11P 351p, L=Imp 160u@14 332p | 393s W=All 160u@11P 351p, L=Imp 160u@14 315p | same | 3000 | char 299 `pos.y`, -1 ulp | C1, then C2 from 8152 | now equal after fix |
| mus1c_con | 309s L=Rep 32u@15P 22r, W=CIS 32u@10 78r | 309s L=Rep 32u@12P 37r, W=CIS 32u@13 73r | same | 1080 | char 16 `vel.x`, 1 ulp | C2 | now equal after fix |
| nab2c_con | 309s W=CIS 32u@7P 85r, L=Rep 32u@18 38r | 309s W=CIS 32u@7P 89r, L=Rep 32u@18 42r | same | 5206 | char 16 `vel.x`, 1 ulp | C2 | now equal after fix |
| pol1c_con | 309s W=CIS 32u@8P 95r, L=Rep 32u@17 55r | 309s W=CIS 32u@7P 90r, L=Rep 32u@18 38r | same | 3091 | char 3 `vel.x/y/z`, 1/1/1 ulp | C2, then C1 from 5909 | now equal after fix |
| pol1g_con | 309s W=All 32u@7P 99r, L=Imp 32u@18 37r | 309s W=All 32u@7P 93r, L=Imp 32u@18 48r | same | 5605 | char 31 `vel.y/z`, -3/2 ulp | C2 | now equal after fix |
| spa3c_ass | 1317s L=Rep 32u@11P 171p, W=CIS 32u@14 196p | 1337s L=Rep 32u@11P 178p, W=CIS 32u@14 189p | same | 2445 | char 33 `hp`, 1 ulp | C2 | now equal after fix |
| spa6c_ass | 1638s L=Rep 32u@15P 86p, W=CIS 32u@10 180p | 1580s L=Rep 32u@15P 80p, W=CIS 32u@10 189p | same | 12022 (`rand=` 12008) | char 25 `hp`, a different damage roll | C2 | now equal after fix |
| spa7c_ass | 1024s L=Rep 32u@15P 65p, W=CIS 32u@10 180p | 1074s L=Rep 32u@15P 70p, W=CIS 32u@10 180p | same | 21452 | char 14 `vel.x/z`, -6/-5 ulp | C2 | now equal after fix |
| spa9g_ass | 1134s W=Imp 32u@11P 181p, L=All 32u@14 173p | 1098s L=Imp 32u@11P 173p, W=All 32u@14 185p | flip | 5523 | char 58 `vel.z`, -1 ulp | C2 | now equal after fix |
| tat2g_eli | 1371s L= 32u@11P 178p, W= 32u@14 180p | 1371s L=`?j?` 32u@11P 178p, W=`\D?` 32u@14 180p | same | 20534 | char 27 `vel.x`, -1 ulp; side-name text | C2 (sim), C-OOB-SIDE (text) | now equal after fix |
| yav1c_con | 309s W=CIS 32u@7P 106r, L=Rep 32u@18 33r | 309s W=CIS 32u@7P 106r, L=Rep 32u@18 40r | same | 3410 | chars 4 and 15 `vel.x/z`, 3/1 ulp | C2 | now equal after fix |

The columns are the side, units, ticket or point counts of each team; the `0p` and `-1r` columns
that every row shares are left out. cor1g_con, kas2c_con and spa9g_ass are the winner flips:
cor1g_con and spa9g_ass under C2, kas2c_con (W/W on VS2013, W/L on v143) under C1. In spa6c_ass
the drift reaches a Sim random draw at turn 12008 before any hashed field differs, and the
different draw shows at 12022 as a different damage amount on char 25. tat2g_eli's sim split at
20534 changes no column; its final line differed only in the side-name text, which is
C-OOB-SIDE. The other 23 differences are score, unit, point or time changes that follow from the
split.

geo1c_xl: with C1 and C2 neutralised, the `-digesttrace` run to game over still differed from
VS2013 at one turn, 19926 of 20226, in one field (char 119, team 2, `vel.x` `40D74737` vs
`40D74738`, 1 ulp). That residual is C3, and it is fixed. The fixed build equals VS2013 on every
traced turn of the 30 proof traces (the 29 drift missions and hot1g_con, to game over or to turn
1600 for the TimeOut missions and hot1g_con), geo1c_xl over all 20226 turns.

## Phase 28: Gametoast d5596ee against 87d3f55

Phase 28 merged Gametoast/swbf2decomp `d5596ee` and re-applied our changes on top of it. The
reference is 87d3f55, the last commit before the merge, built Release v143 in the staged main
tree. Every difference in SIM DIGEST, Factory order, layout pins, corpora and the network
Write/Read order between 87d3f55 and the merged branch is listed here with its cause. Each one
is either fixed in src or accepted with retail evidence (decision D-13; Factory order and wire
format under D-16).

Method: `-factorylog -digesttrace 1600` runs of the six digest missions on both builds, then
per-turn probes (StateHash parts, per-object position, velocity and rotation, per-thread and
per-function checkpoints, raw object words) built into an 87d3f55 worktree and the branch to find
the first differing field, function and input. The probes were never committed.

| Mission | 87d3f55 digest (turn 1500) | Branch digest | First split turn | Cause |
|---------|----------------------------|---------------|------------------|-------|
| geo1c_con | `state=0x16D65714` | same | 334 before the fix, then 0 | C-SOLDIER-SLOPE (fixed) |
| nab2g_c | `state=0xA880CE00` | same | 1291 before the fix, then 0 | C-SOLDIER-SLOPE (fixed) |
| dag1c_ctf | `state=0xD9F4B470` | same | 628 before the fix, then 0 | C-SOLDIER-SLOPE (fixed) |
| kas2c_uber | `state=0x765A955F` | same | 14 before the fix, then 0 | C-SOLDIER-SLOPE (fixed) |
| hot1g_con | `rand=2763759203/2960762048 state=0x1FE5B21F` | `rand=403802221/2528439372 state=0x2F003610` | 620 (`rand=` 739) | C-ADDR-FLYER (accepted) |
| spa3c_ass | `rand=2329233471/2392100820 state=0x247DCD93` | `rand=529082281/2897934758 state=0x7385D175` | 555 (`rand=` 668) | C-ADDR-FLYER (accepted) |

Only the `digest-hot1g_con`, `hot1g_con`, `trace-seed-1`, `digest-spa3c_ass` and `spa3c_ass` rows
of `tests/smoke/digest-expected.txt` changed, all for C-ADDR-FLYER. The v143 trace and factory
files in this directory are unchanged.

### C-SOLDIER-SLOPE

- Function: `EntitySoldier::Move(float fDeltaTime, float fForward, float fStrafe, float fTurn, float
  fAcceleration)` in `src/Battlefront2/Source/EntitySoldier.cpp`, `// FUNCTION: SWBF2 0x004ee590`,
  the grounded slope terms `fSlopeFront` and `fSlopeSide`.
- Gametoast change: b217b519 ("More math and vector API fixes") and a70bfddb ("Mostly correct DX
  vector calls") rewrote the function and wrote `-(vFront.y * fDeltaTime * GRAVITY_FORCE)`, which
  is `(vFront.y * dt) * G`. 87d3f55 computed `fGravity = fDeltaTime * GRAVITY_FORCE` first, then
  `-(vFront.y * fGravity)`.
- Retail evidence: the retail exe at 0x004EE867-0x004EE882 does `movaps xmm0,xmm5` (dt),
  `mulss xmm0,[007B22C4h]` (GRAVITY), then `mulss xmm7,xmm0` and `mulss xmm6,xmm0` for
  `vFront.y` and `vSide.y`, so retail is `vFront.y * (dt * G)`, the 87d3f55 order.
- Effect: kas2c_uber turn 14, character 419 (team 3): velocity x and y 1 ulp off (`3F9CCB06
  BFD578A4` against `3F9CCB05 BFD578A3`) after `EntitySoldier::Move`, with position, thread order
  and every input identical. It moved geo1c_con (a blip at 334-364), nab2g_c (1291), dag1c_ctf
  (628) and kas2c_uber (14), and kas2c_uber's turn-1500 digest.
- Status: fixed on Gametoast's line as `-(vFront.y * (fDeltaTime * GRAVITY_FORCE))` and the same for
  `vSide` (ledger L-3336). After the fix the four missions equal 87d3f55 in every one of 1600
  traced turns and at turn 1500. C2 still holds: kas2c_uber is the C2 guard mission and its row is
  unchanged.

### C-ADDR-FLYER

- Functions: `EntityFlyer::GetBranchParams` (`// FUNCTION: SWBF2 0x004b6120`,
  `src/Battlefront2/Source/EntityFlyer.cpp`) and `EntityPathFollower::Roll`
  (`// FUNCTION: SWBF2 0x004d6a10`, `src/Battlefront2/Source/EntityPathFollower.cpp`). Both add
  `(float)(((unsigned int)this >> 2) & 0xf)` to the mission time to give each path-following flyer
  its own speed and roll wobble. The term reads the object's heap address.
- The code is the same on both sides apart from the cast type (87d3f55 `uintptr_t`, Gametoast
  `unsigned int`, the same value on x86). What changed is the flyer's address: the merged tree
  allocates objects of Gametoast's sizes (retail-matching layouts, for example `PblHashTable`
  gaining `_iNumEntries` in 7f59a099, `NG_ChatMsg::msg[128]`), so the heap moved.
- Evidence: hot1g_con turn 619, character 44 (team 2, a flyer following a path): `params.mWorldSpeed`
  is `4234E84A` on 87d3f55 and `4237F48F` on the branch with every other input of
  `EntityPathFollower::Update` equal, and the address bits are 11 against 3. spa3c_ass turn 555:
  address bits 15 against 7. With the term set to 0 in both functions on both builds (a
  temporary experiment), the branch equals 87d3f55 in every one of 1600 traced turns and at turn
  1500 on all six missions. So nothing else separates the two sims in this window.
- Judgement: accepted. The functions are retail code and unchanged; the digest moved only because
  object addresses moved, and the merged layouts are Gametoast's retail-matching ones. Retail has
  different addresses again, so neither digest is a retail value. It is the same mechanism as
  C-SLOT-SOLDANIM (26-06), where a layout change moved spa3c_ass at the same turn 555. Lockstep is
  not affected: every peer runs the same exe and allocation sequence. A peer on a different build
  already differed before this phase.
- Re-baselined rows: `digest-hot1g_con`, `hot1g_con`, `trace-seed-1` to
  `rand=403802221/2528439372 state=0x2F003610`, and `digest-spa3c_ass`, `spa3c_ass` to
  `rand=529082281/2897934758 state=0x7385D175` (turns=1500 time=50.000 snd=49.923 on both).

### C-2DRADIUS

- Gametoast 9a5f4c4a ("Embedded AI objects and header contracts") made
  `CollisionObject::SetCollisionRadius` (`// FUNCTION: SWBF2 0x00454da0`) also set `m2dRadius`, and
  removed the separate `Set2DCollisionRadius` calls that followed it with the same value
  (RedTerrain hole read, Chunk, EntityBuilding, EntityDroid, EntityGeometry). EntityProp keeps its
  own smaller 2D radius after the call.
- Retail evidence: retail 0x00454da0 stores the radius to `[ecx+14h]` (`mCollisionSphere.w`) and
  `[ecx+18h]` (`m2dRadius`), as Gametoast does.
- Effect: the removed calls set the same value, so they change nothing. The new behaviour is that
  callers that never set the 2D radius (soldier stance changes, traps, water, terrain) now set it.
  A build with the 87d3f55 semantics restored gives the same digests and traces as the branch on
  hot1g_con and spa3c_ass, and the other four missions equal 87d3f55, so no row moves.
- Judgement: accepted (closer to retail).

### C-PTR03-SENTINEL

- `PblOrderedList::Add` (`src/Battlefront2/Source/Thread.h`) sets the head sentinel to the int
  maximum `0x7FFFFFFF` in Gametoast's code. 87d3f55 used the maximum of the key type (PTR-03,
  removed with the other D-26 convention rows).
- Effect: none in the game. The only instantiation is `PblOrderedList<Thread, int>` (retail
  template 0x00657860), whose keys come from `PblPtrOrderKey` and fit in int on x86; the float
  sentinel is `0x4F000000` either way. No list with a 64-bit key exists in src.
- Status: accepted. T1 `ordered list sentinel is the key maximum` now expects `0x7FFFFFFF` for the
  `long long` list.

### C-PIN-28

The layout pins that 28-06 and 28-07 changed or removed (ledger L-3296..L-3306, L-3310) follow
Gametoast's declarations, not a layout change:

| Type | Change | Gametoast commit |
|------|--------|------------------|
| GameObject | `mCreatedTime` protected, pinned through a derived struct at 0x230 | be630e9f |
| RedBorderElement | members private; `sizeof` 0x260 kept | 85aff8b |
| FlareVertex, FlareCompressedVertex | TU-local in pcLensFlare.cpp | 5fbe86b5 |
| AI::VOHelper::QueuedSound | TU-local in VOHelper.cpp | 85aff8b |
| NetDest | TU-local in NetComm.cpp; `tests/nettest.cpp` keeps a 0x60 copy | df0cadba |
| CNetFindGame | direct-join spacers named, same offsets | b9000d5e |
| NetJoinSlot | TU-local in NetComm.cpp | 3ce644dc |
| NetAddr | `pubAddr`/`pubPort` replaced by an 8-byte `GalaxyID`, `sizeof` 0x14 | b9000d5e |
| Snd::StreamMixer, Snd::StreamSplitter | base classes, same offsets | f79c6323, 85aff8b |
| Snd::StreamSourceProducer | members private; `sizeof` 0x18 kept | a4016437 |
| MD5_CTX | renamed from MD5Context, same offsets | 8c43b53c |

None of our fixes reads these types through raw offsets, so no pin needed a friend or accessor
(D-26/D-27). NetAddr's first 8 bytes go on the wire raw (see C-WIRE-ORDER), so its pins now also
check `offsetof(NetAddr, mGalaxyId) == 0` and `sizeof(GalaxyID) == 8` (L-3337).

### C-D14-RECHECK

- C1 (`RedTerrain.cpp`, SSE line) and C3 (`WeaponCannon.cpp` under `/fp:strict`) keep their
  neutralisations; their expressions were not rewritten.
- C2: Gametoast 4a02a2b6 moved the auto-balance and space-assault functions into the `/fp:strict`
  `AIDifficulty.cpp` TU, and 7efdb5b8 wrote `GetRangedSpeed` with `Clamp`/`Interpolate` (the same
  formula). kas2c_uber, the C2 guard mission, and geo1c_con equal 87d3f55 in every traced turn, and
  the `digest-kas2c_uber` row is unchanged.
  Guard row SIMFIX-C2S (which removes `/fp:strict` from `AIDifficulty.cpp`) stays GREEN on the
  merged tree: with the flag removed kas2c_uber still gives the same turn-1500 digest. The C2
  source (`GetTargetBubbleShrinkRate`, `GetFloatSkill_RangedLinear`, `GetFloatSkill`) is the same
  as 87d3f55, but in the enlarged TU cl 19.44 now inlines `GetFloatSkill` into it. Under
  `/fp:precise` it computes `-0.1f - t` in SSE (`subss`), and under `/fp:strict` it computes
  `(-1.1f - -0.1f) * t - 0.1f` (`subss; mulss; subss`). Both store the float to memory before the
  `fld`, so the unrounded x87 return that caused C2 is gone, and both forms give the same float for
  every skill level, because `-1.1f - -0.1f` rounds to exactly `-1.0f`. With the flag removed,
  every digest smoke case is unchanged: `geo1c_con` (full game and final score), `digest-dag1c_ctf`,
  `digest-kas2c_uber`, `pacing-realtime`, `digest-hot1g_con`, `digest-spa3c_ass`, both
  `trace-seed` traces and the run-twice digests. The flag still changes codegen in about 30 other
  functions of the TU, but no smoke case shows a value moving. The flag stays (D-10, D-14,
  CLAUDE.md). T1 row SIMFIX-C2 pins it. No smoke anchor exists, so the SIMFIX-C2S row was removed
  in Phase 28 (ledger L-3338).
- WeaponCannon: Gametoast rewrote three comparisons in `UpdateFire`/`FireDone` so that they differ
  only for NaN (`!(mSalvoDelay > 0.0f)` to `mSalvoDelay <= 0.0f` and two more). ODF values are
  sanitised before they reach those fields, and no traced turn differs.
- Lua limits (6c41a198, `LUA_MAXCALLS` 256, `LUA_MAXCSTACK` 1024) moved no smoke case.
- Status: no change needed.

### C-SHADER-INIT

Not a baseline difference but the reason the first branch Release build could not run a mission:
Gametoast 7f59a099 gave `PblHashTable` a constructor that calls `Clear()`, so
`pcRedShader::s_ShaderFactoryByRenderType` (`// SYNTHETIC: SWBF2 0x004072e0`) has a real dynamic
initializer. Under our project's link order it ran after the shader TUs had registered and wiped
them, and `RedBitmapElement::Init` dereferenced a null interface shader (access violation at
`BattlefrontII.exe+0x39A121` before the first frame). `#pragma init_seg(lib)` in
`pcRedShader.cpp` runs it before the user-level registrations, as `RedColor.cpp` and
`pcPblMatrix.cpp` already do (ledger L-3335, build).

### Factory

All six missions create the same classes in the same order with the same `mNetIndex` on both
builds (`-factorylog`, D-16). The `-factorylog` line prints `idx=%d` again, so the logs are
byte-identical to `factory-<m>.txt`.

### C-WIRE-ORDER

D-16 wire check: `wire-order.ps1` (phase tool) collects, per function keyed by its `// FUNCTION:`
address, the ordered `Write*`/`Read*`/`SendP2PPacket`/`ReadP2PPacket`/`PeekP2PPacket`/`send`/
`sendto`/`recv`/`recvfrom` lines of the Net*.cpp files, NetPacket.h, NetWire, the voice transports,
pcNetSocket.cpp, the Galaxy P2P files and every file a matched function moved to, at 87d3f55 and on
the branch. It printed 201 `DIFF` and 4 `MISSING` lines. Each was judged; none changes the bits on
the wire, so none is fixed and 85aff8b's order is kept everywhere.

- The bit width of every single-argument `Write(x)`/`Read(x)` was taken from the compiler, not the
  source: the six `NetPktGroup` bool/int/float overloads were marked `[[deprecated]]` in a scratch
  compile of every wire TU on both sides, and each C4996 location was mapped to its call. All
  bool overloads write 1 bit like the `WriteBits(x, 1)` they replaced, and every 32-bit
  `Write(int)`/`Read(int&)`/float site on the branch has a 32-bit counterpart on 87d3f55.
- 145 lines are rename-only: the same calls in the same order with the same widths and literal
  arguments; only argument expressions differ.
- The other 60 have the class given below. "branch-swap" and "branch-restructure" mean an inverted
  or restructured if/else whose paths write the same bits in the same order; "parser" means the
  comparison tool split a multi-line or `->` argument wrongly and the widths are the same.
- NetAddr: `WriteAddr`/`ReadAddr` (0x005d5aa0/0x005d5b00) send 8 raw bytes then the private-address
  bits. Those 8 bytes are `pubAddr`/`pubPort` on 87d3f55 and the `GalaxyID` on the branch, the
  same first 8 bytes of the struct. No other code copies a `NetAddr` into a packet.
- `ReadShellUpdate` reads the 2 arch bits once on 87d3f55, in d5596ee and on the branch; the double
  read existed only in the merge commit M and was removed in 28-11.

Line list (the `DIFF` and `MISSING` lines exactly as `wire-order.ps1` prints them):

- `DIFF src/Battlefront2/Source/LuaCallbacks_IFace.cpp 0x00586410 ScriptCB_GetLeaderBoardPlayerData`: not-wire: StatsManager::ReadStatsGetResults is a GameSpy stats query, not a packet read; Gametoast added the index argument
- `DIFF src/Battlefront2/Source/NetAdmin.cpp 0x005b1920 SendAdminMessage`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetAdmin.cpp 0x005b16b0 NetAdmin::ReceiveConsole`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetComm.cpp 0x005b2eb0 PackHeader`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005ba110 NetGame::ApplyLateMoves`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c28e0 EntityItem::WriteCreate`: branch-restructure: the found branch writes bAttach, which is true there; every other path writes one false bit
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c2ad0 EntityItem::ReadCreate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005b8920 NetGame::SendClient`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bc1a0 WriteDamageOwner`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bc270 ReadDamageOwner`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bd340 ReadObjectPtr`: width-split: base ReadBits(uId, bSpecial ? 8 : 6) became an 8-bit read in the never-dies branch and a 6-bit read in the other
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005b9340 NetGame::ReceiveHost`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005b8f70 NetGame::ReceiveClient`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d24b0 WriteDestMask`: parser: base (mask >> 32) hides the 32-bit width from the splitter; both write low then high 32 bits
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2430 ReadWaiting`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c23f0 ReadEvent_UpdateObjective`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c0990 ReadEvent_ChangeCommandPostTeams`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c7ee0 ReadTrackable`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c7450 AloBootVote::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c22d0 ReadEvent_AddObjective`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cb040 ReadMiniMove`: unrolled: the 4-axis loop is unrolled, same ReadFixed(-1, 1, 3) per axis
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cbba0 SpawnDisplay::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2fd0 ReadSpawnCamera`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c55f0 AloReinforcements::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c6140 AloMapDisplayMarkers::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d0a60 ReadSuicideRequest`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cb970 ReadCaptureDisplay`: constant: CAPTURE_RATIO_BITS is 6
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cff00 ReadLockedTargets`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bb570 GetUpdateTurn`: rename-only: same calls, same widths, same order; only argument expressions differ
- `MISSING 0x005d3cc0 <anon> src/Battlefront2/Source/NetGame.cpp`: template: the explicit specialisation became the generic ReadFixed with StickAxisConvert (-1, 1, 7)
- `MISSING 0x005d3cf0 <anon> src/Battlefront2/Source/NetGame.cpp`: template: the explicit specialisation became the generic ReadFixed with MouseAxisConvert (-16, 16, 11)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c10e0 ReadEvent_CorrectChunkCluster`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d23a0 WriteWaiting`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2a80 ReadPlayerMoves`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cae30 ReadMove`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c0d00 ReadEvent_CreateChunkCluster`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c99c0 ReadMoves`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c4da0 ReadUpload`: branch-restructure: the uploadState if/else became a switch with the cases swapped; reads within each state are in the same order
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cc440 ReadSpawnDisplay`: unrolled: the 2-viewport loop is unrolled into viewport 0 plus a 1-iteration guest loop
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c1d90 ReadEvent_DestroyTowCable`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c1600 ReadEvent_ConnectTowCable`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bb530 ReceiveUpdate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bef70 ReadNetEvent`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d1900 NetGame::ReadChatMessageFromServer`: constant: 276 is the base buffer size (sizeof(message) was 276)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c7ad0 AtleastOnce::ReadAll`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c96f0 ReceiveReplica`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bf9b0 ReceiveReliableNetEvents`: callee-signature: ReadEvent_CreateChunkCluster takes the matrix argument; its own reads are compared under 0x005c0d00
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bc530 ReadCreate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bcfc0 ReadObject`: branch-swap: the isFullState if/else is inverted
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bb5b0 ReadUpdate`: callee-signature: StatsManager::ReadStatsChallenge is an empty stub on both sides (0x00530890)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c9f50 ReceivePrimary`: callee-signature: StatsManager::ReadStatsResponse is an empty stub on both sides (0x005308b0)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005caf90 WriteMiniMove`: unrolled: the 4-axis loop is unrolled, same WriteFixed(-1, 1, 3) per axis
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c39a0 NetGame::WriteSpectator`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bc160 WriteNullObject`: branch-swap: the isFullState if/else is inverted
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c3fe0 NetGame::WritePreGame`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c6740 AloMiniStats::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c0950 WriteEvent_ChangeCommandPostTeams`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c7760 AloMissionTimer::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c24b0 WriteEvent_UpdateInvincibility`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bafa0 WriteSpectatorInfo`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d09c0 WriteSuicideRequest`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c72c0 AloBootVote::Update`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c1020 WriteEvent_CorrectChunkCluster`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c0b90 WriteEvent_CreateChunkCluster`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cfd70 WriteLockedTargets`: branch-restructure: early continues became nested if/else; each path writes the same bits; NetNdObj::obj is the first member that base read through *(GameObject**)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c1c30 WriteEvent_DestroyTowCable`: branch-restructure: early return became if/else; one false bit when there is no weapon or owner
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c1390 WriteEvent_ConnectTowCable`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2260 WriteTurnLowBits`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c3320 WriteSwitchResponse`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d1180 WriteBootRequest`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c2280 WriteEvent_AddObjective`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c3440 WriteSwitchObjRequest`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c70d0 AloNextGameInfo::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c5f50 AloMapDisplayMarkers::Write`: branch-restructure: has-actor and has-region flags are written per branch instead of as one bool expression
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cad00 WriteMove`: parser: base (bits >> 30) hides the width; the CntBitFields members mBoolInputs:30 and mMouseMode:2 are the same low 30 and high 2 bits
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2bb0 WritePingData`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c7d10 WriteTrackable`: branch-restructure: early returns became nested if/else, same bits per path
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d0080 WriteSpawnSelect`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cb6d0 WriteCaptureDisplay`: branch-restructure: the team branches are reordered; CAPTURE_RATIO_MAX is 63 and CAPTURE_RATIO_BITS is 6
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c49e0 WriteUpload`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bbd50 WriteCreate`: branch-swap: the initial-player if/else is inverted; ENT_SOLDIER=1, ENT_DROIDEKA=4, ENT_POST_* are 12..14 as in base
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bbf80 WriteObject`: branch-swap: the isFullState if/else is inverted
- `MISSING 0x005d3c60 <anon> src/Battlefront2/Source/NetGame.cpp`: template: the explicit specialisation became the generic WriteFixed with StickAxisConvert (-1, 1, 7)
- `MISSING 0x005d3c90 <anon> src/Battlefront2/Source/NetGame.cpp`: template: the explicit specialisation became the generic WriteFixed with MouseAxisConvert (-16, 16, 11)
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c0570 WriteEvent_Message`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c9900 WriteMoves`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c9510 SendToReplicas`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005c6c00 AloTrackable::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cc1e0 WriteSpawnDisplay`: unrolled: the guest loop writes one false bit when there is no guest, as base did
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2740 WritePlayerMoves`: parser: the -> in the root argument hides the 4-bit width; both write the low 4 bits of mBoolInputs
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d2ef0 WriteSpawnCamera`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d1820 NetGame::SendChatMessageToServer`: branch-swap: the empty-queue and parsed-locally paths still write the 0xffff0000 marker
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005d19b0 NetGame::SendChatMessageToClient`: branch-swap: the empty-queue and parsed-locally paths still write the 0xffff0000 marker
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bf580 SendReliableNetEvents`: branch-restructure: exactly one trailing false bit on each path
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005be9b0 WriteNetEvent`: branch-swap: the damage-multiplier if/else is inverted
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005cc720 WriteObjects`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetGame.cpp 0x005bb100 WriteUpdate`: branch-swap: the isFullState if/else is inverted; WriteSwitchObjRequest stays in the non-full-state branch
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d5bc0 ReadSessionId`: same-bytes: an unsigned __int64 read instead of two words, 8 bytes
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d72d0 HandleFindPacket`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d7450 HandleFoundPacket`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetPacket.cpp 0x005d9790 NetPktGroup::WriteFixed`: branch-split: doHiRes || iBits >= 32 split into two checks; the quantised value is the same expression
- `DIFF src/Battlefront2/Source/NetPacket.cpp 0x005d9860 NetPktGroup::ReadFixed`: branch-split: doHiRes || iBits >= 32 split into two checks; (1 << iBits) - 2 equals base uMax - 1
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d5b00 ReadAddr`: same-bytes: NetAddr's first 8 bytes are the GalaxyID (b9000d5e) where base had pubAddr/pubPort; 8 raw bytes either way
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e3cd0 NetSetup::ReadGameUpdate`: sizeof-rename: name[32] and unsigned __int64 id have the base sizes
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d5ba0 WriteSessionId`: same-bytes: an unsigned __int64 write instead of two words, 8 bytes
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d5aa0 WriteAddr`: same-bytes: NetAddr's first 8 bytes are the GalaxyID (b9000d5e) where base had pubAddr/pubPort; 8 raw bytes either way
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e7c30 ReadFilesAck`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e7000 WriteLoad`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e7a00 ReadFiles`: sizeof-rename: buffer[2048] has the base size
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e7670 WriteFiles`: constant: 2048 is the base block size; ReadData is a file read, not a packet read
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e3a90 NetSetup::WriteGameUpdate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e5bb0 WriteShellMove`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e5d10 ReadShellMove`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e54b0 WriteJoin`: constant: _countof(converted) is 66
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e0e10 WriteLaunch`: branch-swap: the hostIndex if/else is inverted; base had no third branch either
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e1290 ReadLaunch`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e5700 ReadJoin`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e5fc0 WriteShellUpdate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e6690 ReadShellUpdate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005dfc50 CNetJoinGame::Update`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005dc4e0 CNetHostGame::Update`: branch-restructure: same reject precedence (full, then password, then version)
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005ddc90 CNetFindGame::Update`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetSetup.cpp 0x005e2640 CNetPostLoadHostGame::Update`: branch-restructure: same reject precedence (full, then password, then version)
- `DIFF src/Battlefront2/Source/NetMatch.cpp 0x005d6880 HandleQueryPackets`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f61c0 WriteString`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f58c0 ReadNetLOD`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f60b0 NetTrigger::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5be0 WriteCosSin`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f6000 WriteWeapon`: branch-swap: the null-weapon if/else is inverted
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ed430 WalkerLegPair::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0c40 AddSendPilots`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eb4e0 Asteroid::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f1d00 CommandBuilding::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0820 FlagItem::WriteCreate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f08a0 FlagItem::ReadCreate`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0970 FlagItem::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0a40 FlagItem::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f14b0 EntityBuildingArmed::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f1650 EntityBuildingArmed::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f19a0 EntityBuildingAnimatedArmed::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eb590 EntityFlyer::AddSends`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ec860 EntityWalker::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); ComputeNetLOD returns only 0 or 1; target flag written per branch
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ecca0 EntityWalker::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ee8b0 EntityDroid::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); ComputeNetLOD returns only 0 or 1
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eeaf0 EntityDroid::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eef30 EntityHover::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); ComputeNetLOD returns only 0 or 1; target flag written per branch
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ef2f0 EntityHover::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f51d0 EntityDroid::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f52d0 EntityDroid::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f3de0 EntityWalker::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f3fb0 EntityWalker::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5410 MountedTurret::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ef7d0 MountedTurret::Write`: branch-restructure: the target flag is written per branch
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005efaf0 MountedTurret::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5580 MountedTurret::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f4490 EntityHover::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f45f0 EntityHover::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5950 ReadUnitVector`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f59e0 ReadPositionVector`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5a40 WriteMatrix`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5a70 ReadMatrix`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5b70 ReadQuaternion`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5c90 WriteVelocity`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f5da0 ReadVelocity`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0220 PowerupItem::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f02a0 PowerupItem::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f07b0 EntityTrap::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f07e0 EntityTrap::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f11a0 EntityRemoteTerminal::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f11c0 EntityRemoteTerminal::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005efed0 EntityMine::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f0010 EntityMine::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8330 Weapon::Write`: parser: the -> in the argument hides the 1-bit width; Write(bool) is WriteBits(v, 1)
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e86f0 WeaponCannon::Write`: parser: multi-line arguments; same WriteFixed widths and the same FIRE/FIRE2 condition
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8830 WeaponCannon::Read`: branch-restructure: the mState switch became an if chain; ReadFixed(mStateLimit, 6 bits) stays on FIRE/FIRE2
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8d30 WeaponDispenser::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8d90 WeaponDispenser::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8e20 WeaponGrenade::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8e80 WeaponGrenade::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8f80 WeaponInvisibility::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e8fc0 WeaponInvisibility::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e90b0 WeaponLauncher::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e90e0 WeaponLauncher::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e9190 WeaponRemote::Write`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e9290 WeaponAreaEffect::Write`: parser: the -> in the argument hides the 1-bit width
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e92f0 WeaponAreaEffect::Read`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eb6d0 EntityPathFollower::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f28c0 GameObject::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e9550 EntitySoldier::AddSends`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ed7b0 EntityDroideka::AddSends`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005eb920 EntityFlyer::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); ComputeNetLOD returns only 0 or 1
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ebfe0 EntityFlyer::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f3500 EntityFlyer::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f3910 EntityFlyer::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ede90 EntityDroideka::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f47c0 EntityDroideka::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f4ba0 EntityDroideka::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ed850 EntityDroideka::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); the -> in one argument hides a 1-bit width
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005e9660 EntitySoldier::WriteObject`: netlod: WriteNetLOD is base SetNetPrecision (0x005f5880, same body); ComputeNetLOD returns only 0 or 1
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f2910 EntitySoldier::WriteLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005ea140 EntitySoldier::ReadObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f2ec0 EntitySoldier::ReadLateState`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/NetState.cpp 0x005f27e0 EntityBuildingArmedDynamic::WriteObject`: rename-only: same calls, same widths, same order; only argument expressions differ
- `DIFF src/Battlefront2/Source/pcNetSocket.cpp 0x00617e50 NetSocket::SendSocket`: branch-swap: the LAN and Galaxy branches are swapped; iDest > 64 && iDest == 68 equals iDest == 68
- `DIFF src/Battlefront2/Source/pcNetSocket.cpp ReadGalaxy ReadGalaxy`: rename-only: same calls, same widths, same order; only argument expressions differ

## Phase 29: Gametoast ea864010 against 7c27dee

Phase 29 merged Gametoast/swbf2decomp `ea864010` and re-applied our changes on top of it. The
reference is 7c27dee, the Phase 28 result, built Release v143 in the staged main tree. Every
difference in SIM DIGEST, Factory order, layout pins, corpora and the network Write/Read order
between 7c27dee and the branch is listed here with its cause, the Gametoast commit in
d5596ee..ea864010 and the retail evidence (D-14; Factory order and wire format under D-16).

Method: `-factorylog -digesttrace 1600` runs of the six digest missions on both builds
(`artifacts/p29-base`), then temporary probes built into a 7c27dee worktree and the detached
branch worktree (never committed): per-object position, velocity and health; the Global and Sim
random seeds around every thread update; the call stack of every Sim draw; the candidates,
target points and ray tests of `ExplosionClass::CreateExplosion`; TreeGrid add, update and remove
calls near a point; light updates and turn-offs. Each cause was confirmed by reverting only that
change on the branch: with the three changes below reverted, all six missions equal 7c27dee in
every one of 1600 traced turns and at turn 1500 (experiment E2,
`artifacts/p29-base/exp-e2`).

| Mission | 7c27dee digest (turn 1500) | Branch digest | First split turn | Cause |
|---------|----------------------------|---------------|------------------|-------|
| geo1c_con | `rand=1354762085/2950811992 state=0x16D65714` | same | 1 (state only, 289 turns, rand never moves) | C-WALKER-GRAVITY (accepted) |
| nab2g_c | `rand=2206058303/116923646 state=0xA880CE00` | same | 0 | none |
| hot1g_con | `rand=403802221/2528439372 state=0x2F003610` | `rand=403802221/2528439372 state=0xC95E64E8` | 1 (state only, 922 turns, rand never moves) | C-WALKER-GRAVITY (accepted) |
| spa3c_ass | `rand=529082281/2897934758 state=0x7385D175` | `rand=1799084171/444301954 state=0xC77ED51E` | 1019 (Sim seed 1015) | C-PILOT-DELETE (accepted) |
| dag1c_ctf | `rand=2837596015/2599533116 state=0xD9F4B470` | `rand=423450483/1140324658 state=0xED6D9B0A` | 1065 (Global seed 1064) | C-FLAG-PICKUP (accepted) |
| kas2c_uber | `rand=612937807/386973628 state=0x765A955F` | same | 0 | none |

`tests/smoke/digest-expected.txt` rows changed: `digest-hot1g_con`, `hot1g_con`, `trace-seed-1`
(C-WALKER-GRAVITY), `digest-spa3c_ass`, `spa3c_ass` (C-PILOT-DELETE), `digest-dag1c_ctf`,
`dag1c_ctf` (C-FLAG-PICKUP). The factory logs are byte-identical on all six missions, so no
`factory-*.txt` changed. As in Phase 28, the v143 `digest-trace-*.txt` files in this directory are
the vs2013 comparison record and are not rewritten.

### C-WALKER-GRAVITY

- Function: `EntityWalker::PostCollisionUpdate` (`// FUNCTION: SWBF2 0x00505670`,
  `src/Battlefront2/Source/EntityWalker.cpp`), the grounded gravity constant
  `float fGravity = m_bIsJumping ? -9.8f : -29.400002f;`.
- Gametoast change: 4a34a604 ("Ryan: memory pools using sizeof") wrote `-29.400002f`, which is
  `0xC1EB3334`. d5596ee and 7c27dee had `-29.4f` (`0xC1EB3333`), one ulp smaller in magnitude.
- Retail evidence: the retail exe loads the constant from `0x007B25C4`, which holds `34 33 EB C1`
  (`0xC1EB3334`), next to `-9.8f` at `0x007B25A4`. Gametoast's value is the retail value.
- Effect: every walker's grounded `mVelocity.y` differs by one ulp after the gravity step (geo1c_con
  turn 1, `BF7A7EA0` against `BF7A7EA1`, positions equal). The velocity enters the state hash, so
  geo1c_con and hot1g_con show state-only differences while walkers settle; the rand seeds never
  move. geo1c_con returns to the same turn-1500 digest; hot1g_con keeps a different state hash at
  turn 1500.
- Confirmation: with only `-29.4f` restored on the branch (and the unrelated C-CRITSYS-SHIELD
  revert, which moves nothing), geo1c_con and hot1g_con equal 7c27dee in every one of 1600 turns.
- Judgement: accepted (retail constant). Re-baselined rows: `digest-hot1g_con`, `hot1g_con`,
  `trace-seed-1` to `rand=403802221/2528439372 state=0xC95E64E8` (turns=1500 time=50.000
  snd=49.923).

### C-PILOT-DELETE

- Function: `Controllable::Kill` (`// FUNCTION: SWBF2 0x00483ba0`,
  `src/Battlefront2/Source/Controllable.cpp`), the `PILOT_VEHICLE` case.
- Gametoast change: 8d20cf5c ("Ryan: gameplay, weapons, combat fixes") caches
  `Controllable* pPilot = mPilot;` and uses `pPilot` for `InstantDeath`, `Kill` and `delete`.
  d5596ee and 7c27dee re-read `mPilot` for each step. `mPilot->Kill()` clears the vehicle's
  `mPilot`, so 7c27dee's `delete mPilot` deleted a null pointer and left the dead pilot soldier
  alive in the world.
- Retail evidence: retail loads `mPilot` once (`mov ebx,[edi+0D0h]` at `0x00483BE6`), calls
  `Kill` through `ebx` (`0x00483C97`) and then the scalar deleting destructor through `ebx`
  (`mov eax,[ebx]; push 1; call [eax]` at `0x00483CCF`). Retail deletes the cached pilot, as
  Gametoast does.
- Effect: spa3c_ass turn 1015, a piloted flyer is destroyed. On 7c27dee the dead pilot stays in
  the TreeGrid at the flyer's position, so the flyer's `mExplosionDestruct` finds it
  (`GetObjectsInRange` returns 3 objects, the soldier among them) and
  `EntitySoldier::ApplyRadiusPush` draws one Sim random number for the chunk roll. On the branch
  the soldier was deleted in `Controllable::Kill` (called from `EntityFlyer::Kill`), the
  explosion finds 2 objects, and that flyer update draws 23 numbers instead of 24. The Sim seed
  splits at 1015 and the state at 1019.
- Confirmation: experiment E2 (above).
- Judgement: accepted (retail control flow; 7c27dee leaked a dead soldier per destroyed piloted
  vehicle). Re-baselined rows: `digest-spa3c_ass`, `spa3c_ass` to
  `rand=1799084171/444301954 state=0xC77ED51E`.

### C-FLAG-PICKUP

- Function: `FlagItem::CollisionCallback` (`// FUNCTION: SWBF2 0x00524fd0`,
  `src/Battlefront2/Source/FlagItem.cpp`), the pickup gate.
- Gametoast change: 8d20cf5c ("Ryan: gameplay, weapons, combat fixes") changed
  `(!mStationary || mClass->mLifeSpan - mLifeTimer > 0.5f)` to
  `(mStationary || mClass->mLifeSpan - mLifeTimer > 0.5f)`.
- Retail evidence: retail tests `byte ptr [esi+258h]` (`EntityItem::mStationary`; `mClass` is at
  +25Ch, `mLifeTimer` at +260h, `mOwner` at +264h, the same order as EntityItem.h) at
  `0x00524FFD` and jumps straight to the pickup (`jne 00525025`) when it is non-zero; only a zero
  byte falls through to the `mLifeSpan - mLifeTimer > 0.5f` test (`0x007B2014` = 0.5f). That is
  `mStationary || ...`, Gametoast's form.
- Effect: dag1c_ctf, a soldier picks up a flag one turn earlier on the branch. The pickup turns
  off the flag's `EntityLight` (`FlagItem::CollisionCallback` -> `EntityLight::TurnOff`, turn
  1063), so one fewer light draws `PblRandom::GetGlobalFloat()` per frame from frame 1064. The
  Global seed splits at 1064 and the state at 1065. The Sim seed is unaffected until then.
- Confirmation: experiment E2 (above).
- Judgement: accepted (retail condition). Re-baselined rows: `digest-dag1c_ctf`, `dag1c_ctf` to
  `rand=423450483/1140324658 state=0xED6D9B0A`.
- Smoke expectation: with retail pickups the dag1c_ctf CTF game ends by captures (`W=Rep` 5,
  `L=CIS` 4 at mission time 501 s, about 15 s of wall time) instead of still running at the 30 s case
  limit. On 7c27dee and in E2 the mission is still running at the limit. The `digest-dag1c_ctf`
  case in `tests/smoke.ps1` now expects `Pass` with the default timeout (same log checks), and
  `dag1c_ctf` in `tests/smoke/missions-all.txt` expects `Pass`.
- 29-10 `-Smoke All`: the same gate ends 33 more flag games by captures before the 120 s case
  limit, so their `missions-all.txt` lines go from `TimeOut` to `Pass`:
  - CTF: cor1c_ctf, cor1g_ctf, dag1g_ctf, geo1c_ctf, kas2c_ctf, kas2g_ctf, mus1c_ctf, mus1g_ctf,
    myg1c_ctf, myg1g_ctf, nab2c_ctf, nab2g_ctf, pol1c_ctf, pol1g_ctf, tat2c_ctf and tat2g_ctf.
  - 1-flag: end1g_1flag, fel1c_1flag, fel1g_1flag, hot1g_1flag, spa1g_1flag, spa3c_1flag,
    spa6c_1flag, spa7c_1flag, spa8g_1flag, spa9g_1flag, tan1c_1flag, tan1g_1flag, tat3c_1flag,
    tat3g_1flag, uta1c_1flag, yav1c_1flag and yav1g_1flag. tat3c_1flag and tat3g_1flag were `Fatal` (see C-TAT3-STALE).
  - On a fresh 7c27dee Release build (scratch worktree `artifacts/wt-tat3`), tat3c_1flag is still
    running at the limit.
  - dea1c/dea1g, kam1c/kam1g and uta1g 1-flag still run past the limit and stay `TimeOut`.
  - The game end is deterministic (hot1g_1flag ends at frame 67204 and uta1c_1flag at frame 71747
    in both runs), but the wall time is not. end1g_1flag, hot1g_1flag, tat3c_1flag and uta1c_1flag
    take 80 to 110 s with 8 jobs and are close to the 120 s case limit. uta1c_1flag timed out at
    121 s in the first run and passed at 107 s in the second. Their `missions-all.txt` lines carry
    a 240 s per-case limit as a third token.

### C-CRITSYS-SHIELD

- Function: `SpaceAssault_RandomCritSysDamage` (`// FUNCTION: SWBF2 0x00439770`,
  `src/Battlefront2/Source/AIDifficulty.cpp`, `/fp:strict` TU).
- Gametoast change: 5e6d8759 caps the damage on `pTarget->mCurShield` (when it is above 0) with
  `Min(Min(15000.0f, fBestDamage * 5.0f), mCurShield)`. 7c27dee capped on `mCurHealth` with
  `fCap = min(fBestDamage * 5, 15000, mCurHealth)` written as two ifs.
- Retail evidence: retail reads `movss xmm1,[ebx+150h]` (`0x0043989C`) with the Damageable
  subobject at +140h (`lea ecx,[ebx+140h]` before `ApplyDamage`), so the field is Damageable+10h:
  vtable, `mCurHealth`, `mMaxHealth`, `mAddHealth`, then `mCurShield` at +10h. The min order is
  `comiss xmm2,xmm0; ja` (15000 kept only when `fBestDamage * 5 > 15000`) then
  `comiss xmm1,xmm0; ja` (the cap kept only when `mCurShield > cap`), which is
  `Min(Min(15000, x), shield)` with `Min(a, b) = a < b ? a : b`, NaN included.
- Effect: none in the six traced missions. Reverting it on the branch leaves spa3c_ass (the only
  space assault mission traced) on the branch digest.
- Judgement: accepted (retail field and order); a behaviour change toward retail with no row moved.

### C-TAT3-STALE

- Rows: `tat3c_con`, `tat3g_con`, `tat3c_1flag` and `tat3g_1flag` in `tests/smoke/missions-all.txt`. They
  expected `Fatal`: the Phase 3 `0xC0000005` at `d3dx9_43.dll+0x1BA553`, which Phase 6 traced to a
  bad pose pointer in `BuildRootWorldMatrix<32>`.
- Finding: none of the four crashes on the branch. A fresh 7c27dee Release build run from the
  staged main tree's working directory with the same arguments doesn't crash either:
  - tat3c_con and tat3g_con reach the turn-1500 digest and exit 0;
  - tat3c_1flag is still running at the limit.
- So this is not a Phase 29 difference. The `Fatal` lines were already stale at 7c27dee, and
  Phase 28 didn't run `-Smoke All`. The crash went away in the d5596ee code merged in Phase 28:
  - 694871b2 ("Zephyr 100%") rewrote `ZephyrSkeleton::BuildWorldMatrices` without the D3DX
    quaternion and matrix calls in which the fault was taken;
  - the Phase 29 run has no d3dx9 fault and no dump.
- Judgement: re-baselined to the observed classes. The two `_con` lines become `Pass`. The two
  `_1flag` lines become `Pass` through C-FLAG-PICKUP, which ends their flag game.
- Open: `ZephyrSkeleton<N>`'s constructor and `Open(shared)` still leave `m_pPoseFinal` unset,
  as retail does. Only the callers' pose `Open` keeps `BuildWorldMatrices` safe.

### Phase 29 D-13 items (29-D13-ITEMS.tsv)

Every line of `29-D13-ITEMS.tsv` was checked against 7c27dee. E2 (the three causes above reverted,
everything else of the branch kept) equals 7c27dee in every traced turn of all six missions, so
none of these rewrites moves a value in the traced window. The reasons below say why they are
equal in general:

- RedTerrain (5aa2e604, e3c65b6b; C1 TU): `PointInSphere` compares the same `x*x + y*y + z*z`
  (`D3DXVec3LengthSq` sums in that order) with the operands of `>` swapped; `GetInterpolatedHeight`
  uses `Clamp`, equal to the two ifs for every non-NaN value, and our C1 SSE line (L-1101) is
  unchanged; `GetInterpolatedLight` evaluates the same expressions in another order with no side
  effects; `RayHit` replaces `fabsf(d) < EPSILON` by `(d < 0 ? -d : d) < EPSILON`, equal for every
  input including -0 and NaN; `_RayHit` swaps the two terms of a two-term sum and of
  `(vEnd + vStart) * 0.5f`, and float addition of two terms is commutative. No change.
- WeaponCannon (00233e01, 525b1bc3, 8d20cf5c; C3 TU): the compare rewrites (`!(x > 0)` to
  `x <= 0` and branch swaps) differ only for NaN, which OdfValue keeps out of these fields;
  `NotifySoldierState` uses a branchy abs whose only difference from `fabsf` is the sign of -0 in a
  value that is only compared; `UpdateFire`'s salvo loop
  (`while (mSalvoCount > 0 && mSalvoDelay <= 0.0f)`) fires and draws the same as the old loop with
  its inner break, because it is entered only with `mSalvoCount > 0`; `GetYawSpread` reads the target
  through `GameObject::GetMatrix` instead of the CollisionObject base, which is the same final
  overrider (`EntityGeometry::GetMatrix`). Retail calls it through the CollisionObject subobject
  (`lea esi,[eax+0Ch]; call [edx+40h]` at `0x0067E92D`) and sums `(x*x + y*y) + z*z`, the order of
  `D3DXVec3LengthSq`. `WeaponCannon::Update` takes the sound position from the render matrix; it
  feeds no sim value. No change.
- CommandPost::BuildPost (c2fc0345): `SquaredLength` is `D3DXVec3LengthSq`, the same terms in the
  same order. No change.
- EntitySoldier, EntityWalker, EntityFlyer, EntityHover, EntityDroideka, SoldierAnimator,
  OrdnanceBullet, WeaponMelee (b644efc2, 471c1e05, 525b1bc3, c3ce70af, 00233e01, 34105915,
  8d20cf5c): the whole-file float rewrites. Bisected by experiment: E1 (walker constant only)
  and E2 (the three causes) leave no traced turn different on any mission, so of all these
  rewrites only the walker constant moved a value. A scan of every changed float literal in
  `git diff 7c27dee -- src` (bit patterns compared per file) finds no other literal that moved by a
  few ulps. EntitySoldier::Move now computes `fGravityStep = fDeltaTime * GRAVITY_FORCE` first
  (b644efc2), the retail order (C-SOLDIER-SLOPE). No change.
- AIDifficulty (5e6d8759; C2 TU): `GetIntSkill`, `GetFlyerSpeedRange`, the random-flyer loops and
  the auto-balance roll give the same values for every non-NaN input; the crit-system cap is
  C-CRITSYS-SHIELD. kas2c_uber, the C2 guard mission, is identical in every traced turn. No change.
- TeamManager range queries (00233e01, 4a34a604): `SquaredLength` and `CrossProduct` from the
  PblMath helpers use the terms of the hand-expanded code; no traced turn moves. No change.

### Re-check of the Phase 28 causes

- C-SOLDIER-SLOPE: Phase 29 re-check: superseded by b644efc2 ("fGravityStep = fDeltaTime *
  GRAVITY_FORCE; -(vFront.y * fGravityStep)"), which is the retail order; L-1960 is
  `superseded`. geo1c_con, nab2g_c, dag1c_ctf (before C-FLAG-PICKUP) and kas2c_uber equal 7c27dee
  in every traced turn, which is the proof.
- C-ADDR-FLYER: Phase 29 re-check: unchanged. `EntityFlyer::GetBranchParams` and
  `EntityPathFollower::Roll` still add `((unsigned int)this >> 2) & 0xf`. The pool census
  (29-08) found every pool size unchanged, and E2 equals 7c27dee on hot1g_con and spa3c_ass in
  every traced turn, so the heap layout did not move again.
- C-2DRADIUS: Phase 29 re-check: unchanged. `SetCollisionRadius` still sets `m2dRadius`; 8d20cf5c
  added a `Set2DCollisionRadius(fRadius)` with the same value after it in `FlagItem`, which
  changes nothing.
- C-PTR03-SENTINEL: Phase 29 re-check: unchanged. `PblOrderedList::Add` still sets `0x7FFFFFFF`;
  Gametoast only swapped the two head-link stores in the constructor and made `Add` inline.
- C-PIN-28: Phase 29 re-check: unchanged. `tests/unit/layout_pins.cpp` is identical to 7c27dee and
  every pin passes on the branch.
- C-D14-RECHECK: Phase 29 re-check: re-derived for 5aa2e604/e3c65b6b (C1), 5e6d8759 (C2) and
  8d20cf5c (C3) in "Phase 29 D-13 items" above. Both `/fp:strict` TUs keep the flag. kas2c_uber
  (C2) and dag1c_ctf up to the C-FLAG-PICKUP split (C1) are unchanged.
- C-SHADER-INIT: Phase 29 re-check: unchanged. `#pragma init_seg(lib)` is still in
  `pcRedShader.cpp`. The six new upstream TUs (pcFlashyTextElement, FriendBuddyList, TreeLine,
  pcRedFootprint, pcRedVertexCache, PblPrintfWrapper) caused no startup crash; every mission ran
  on the first staged branch build.
- C-WIRE-ORDER: Phase 29 re-check: re-derived. `wire-order.ps1 -Base 7c27dee` with
  compiler-resolved widths on both sides (`FILES 19`, `COMPARED 1381 base=1382 root=1381`) prints no
  `DIFF` or `MISSING` line; per file the multiset of resolved overload widths is identical. The
  base function without a match has no Write/Read call. `VoiceChatNode.cpp` (outside the wire set)
  was checked by hand: `SendAudioPacket` (0x006743b0) still writes the header bitmap, then the
  payload, then sends; `Header::SendSetup` writes the same bits in the same order; and
  `((n + 7) / 8 * 8) / 8` equals `(n + 7) >> 3` for unsigned n.
- Factory: Phase 29 re-check: all six factory logs are byte-identical between 7c27dee and the
  branch.

## Phase 30: Gametoast 472a77d1 against 5db7e6ac

Phase 30 merged Gametoast/swbf2decomp `472a77d1` (10 commits past `ea864010`) and re-applied our
changes on top of it. The reference is 5db7e6ac, the Phase 29 result, built Release v143 in the
staged main tree (`artifacts/p30-base`). Every difference in SIM DIGEST, Factory order, layout pins,
corpora and the network Write/Read order between 5db7e6ac and the branch is listed here with its
cause, the Gametoast commit in ea864010..472a77d1 and the retail evidence (Phase 30 D-18/D-19;
Factory order and wire format under Phase 29 D-16).

Method: `-factorylog -digesttrace 1600` runs of the six digest missions on both builds
(`artifacts/p30-base`, `artifacts/p30-branch`), the Standard, Twice, Dedicated and Crash smoke
sets, the layout pins in `tests/unit/layout_pins.cpp`, and `wire-order.ps1 -Base 5db7e6ac` with
compiler-resolved overload widths on both sides. Retail evidence is from `dumpbin /disasm` of the
retail Steam `BattlefrontII.exe`.

| Mission | 5db7e6ac digest (turn 1500) | Branch digest | First split turn | Cause |
|---------|-----------------------------|---------------|------------------|-------|
| geo1c_con | `rand=1354762085/2950811992 state=0x16D65714` | same | none (1600 of 1600 turns equal) | none |
| nab2g_c | `rand=2206058303/116923646 state=0xA880CE00` | same | none (1600 of 1600 turns equal) | none |
| hot1g_con | `rand=403802221/2528439372 state=0xC95E64E8` | same | none (1600 of 1600 turns equal) | none |
| spa3c_ass | `rand=1799084171/444301954 state=0xC77ED51E` | same | none (1600 of 1600 turns equal) | none |
| dag1c_ctf | `rand=423450483/1140324658 state=0xED6D9B0A` | same | none (1600 of 1600 turns equal) | none |
| kas2c_uber | `rand=612937807/386973628 state=0x765A955F` | same | none (1600 of 1600 turns equal) | none |

No `tests/smoke/digest-expected.txt` row changed. The six factory logs are byte-identical, so no
`factory-*.txt` changed, and every one of the 1600 traced turns is identical on all six missions,
so no `digest-trace-*.txt` changed. `tests/baseline/vs2013` and the corpora are unchanged. The
only baseline edit is the layout pin block for `RawControllerInputs` and `FLInputManager`
(C-INPUT-ALIGN).

### C-INPUT-ALIGN

- Function: the `RawControllerInputs` layout in `src/Battlefront2/Source/FLInputManager.h`, read by
  `// FUNCTION: SWBF2 0x00414b80` (`src/Battlefront2/Platform/Pc/pcFLJoystick.cpp`) and stored in
  `FLInputManager::s_instance` (`// GLOBAL: SWBF2 0x01ebdc50`).
- Gametoast change: 7ad37c55 ("Core, HUD, and input APIs") removed the `mRawInputStorage` union
  and the `mReservedBeforeMode`, `mReservedBeforeSensitivity` and `mReservedBeforeMousePosition`
  spacers, and typed `mMouseSensitivity[5]`, `mMouseOldX` and `mMouseOldY` as `double`. Every
  member offset is unchanged (the pins on `mMode` 0x8, `mMouseSensitivity` 0x2610,
  `mMouseOldX` 0x2880 and `mMouseOldY` 0x2888 hold, and `sizeof(RawControllerInputs)` stays
  0x28a0), but the `double` members give
  `RawControllerInputs` and `FLInputManager` 8-byte alignment, and `sizeof(FLInputManager)` grows
  from 0x2f84 to 0x2f88 with 4 bytes of tail padding.
- Retail evidence: retail reads and writes those fields as doubles (`movsd xmm4,[esi+2880h]` at
  `0x0041500E`, `movsd xmm4,[esi+2888h]` at `0x004150A8`, `movsd [esi+2880h],xmm2` and
  `movsd [esi+2888h],xmm1` at `0x00415172`), so the retail type has 8-byte alignment under the
  default packing. `s_instance` sits at `0x01ebdc50` (8-aligned), and the next global,
  `AI::FollowGoal::rttiHashFollowGoal`, is at `0x01ec0bd8` = `0x01ebdc50 + 0x2f88`. The retail
  object therefore spans 0x2f88 bytes, as Gametoast's type does.
- Effect: none on the sim. No source uses the 0x2f84 size, `s_instance` is static (no pool or
  heap allocation depends on it), and every traced turn is identical.
- Judgement: accepted (retail types and size). Re-baselined pins in `tests/unit/layout_pins.cpp`:
  `alignof(RawControllerInputs) == 8`, `sizeof(FLInputManager) == 0x2f88`,
  `alignof(FLInputManager) == 8`; the pins on the removed spacers `mReservedBeforeMode`,
  `mReservedBeforeSensitivity` and `mReservedBeforeMousePosition` are replaced by pins on the named
  members `mMouseOldX` (0x2880) and `mMouseOldY` (0x2888).

### C-PBLLIST-RENDER

- Function: `PblList<T>::Clear` (`src/PebbleFL/Common/PblList.h`) and its callers
  `RedTexture::Init` (`// FUNCTION: SWBF2 0x006ba2a0`, `pcRedTexture.cpp`), the
  `pcRedIndexBuffer` init and cleanup (`0x006ca130`, `0x006c9df0`, `pcRedIndexBuffer.cpp`) and the
  `pcRedVertexBuffer` cleanup (`0x006cabc0`, `pcRedVertexBuffer.cpp`).
- Gametoast change: cf7b956a ("Annotation fixes and IsDone / Clear") deleted the deleting
  `PblList::Clear()` and renamed the unlink-only `RemoveAll()` to `Clear()`.
- Finding: these renderer lists never used the template. `PblList<RedTexture>::Clear`
  (`0x006ba260`), `PblList<pcRedIndexBuffer>::Clear` (`0x006c9f80`) and
  `PblList<pcRedVertexBuffer>::Clear` (`0x006ca790`) are explicit specialisations in those files,
  each `while (_iCount > 0) Remove(_head._pNext);`, byte-identical at 5db7e6ac and on the branch.
  The 30-D13-ITEMS.tsv "Old" column (deleting) was wrong; the behaviour did not change.
- Retail evidence: retail `0x006ba260`, `0x006c9f80` and `0x006ca790` only unlink (`dec [ecx+10h]`,
  clear `_pList` and `_pObject`, relink the neighbours, loop while the count is positive; no call).
  `0x006ba2a0` ends in `mov ecx,7DE9A0h; jmp 006BA260`, `0x006ca130` calls `0x006c9f80` twice and
  `0x006cabc0` calls `0x006ca790`. `pcRedIndexBuffer::Cleanup` (`0x006c9df0`) unlinks and
  releases every buffer itself (the inlined unlink at `0x006C9EC0`, then `Release` until the
  reference count is 0) before it clears the then empty lists. Retail's own `Clear` templates (0x0043a180, 0x0056fa50,
  0x00695020, 0x006edbe0, 0x00526ab0) are the unlink-only form too.
- Effect: none. The one owning caller that relied on deletion, `~PrecipitationEffect`, deletes its
  regions with an explicit unlink-then-delete loop (SUNFLARE-02, 30-05).
- Judgement: accepted (retail behaviour, no change). No row moved.

### C-DOT-ORDER-30

- Functions (30-D13-ITEMS.tsv): `RedTerrain::_RayHit` (a360dd99), `EntitySoldier::CollisionCallback`
  (`0x004e5380`), `EntitySoldier::PostCollisionUpdate` (`0x004e6660`),
  `EntitySoldier::UpdateWeaponAndAimer` (`0x004e6f80`), `EntitySoldier::Render` (`0x004e23d0`),
  `EntityHover::Move` (`0x004c3fa0`), `EntityHover::CollisionCallback` (`0x004c66a0`),
  `EntityFlyer::PostCollisionUpdate` (`0x004b1e70`), `EntityDroideka::CollisionCallback`
  (`0x004a0e00`) (472a77d1, "Animation, collision, and geometry"),
  `CollisionManager::UpdateSimple` (`0x0045f600`), `CollisionManager::UpdateRestricted`
  (`0x0045f970`) (a360dd99, "More proper render and effect calls"), `Aimer::GetLeadPos`
  (`0x0043d7f0`) and `Aimer::Update` (`0x0043db20`) (7ad37c55, "Core, HUD, and input APIs").
- Gametoast change: hand-expanded three-term sums written `a.y*b.y + a.x*b.x + a.z*b.z` became
  `DotProduct`, `SquaredLength` or `Length`, which inline `D3DXVec3Dot`, `D3DXVec3LengthSq` and
  `D3DXVec3Length` and sum `(a.x*b.x + a.y*b.y) + a.z*b.z`.
- Finding: only the first two terms swap. Each product has the same operands (multiplication is
  commutative), float addition of two terms is commutative, and the third term is added last in
  both forms, so `(y + x) + z` and `(x + y) + z` are the same value bit for bit, NaN and signed zero
  included. The vector differences fed to `DotProduct` (`mEyePoint - mAimStart`, `vFrom - vTo`) are
  the same per-component subtractions the old code wrote inline. `Clamp`, `Min`, `Max`, `Interpolate`,
  `fabsf`, `PblQuaternion(axis, angle)` and `PblMatrix(q, t)` were checked in 30-07 to give the old
  values for every input.
- Effect: none. All six missions are identical to 5db7e6ac in every one of 1600 traced turns.
  `EntitySoldier::Render` is on the render path only (a dedicated host never renders).
- Judgement: accepted (same values by construction). No row moved.

### C-FENV-TERRAIN

- Function: `CollisionPrimitive::CollideTerrain` (`// FUNCTION: SWBF2 0x00467dd0`,
  `src/Battlefront2/Source/CollisionPrimitive.cpp`) and the old `TerrainDepth` helper.
- Gametoast change: 472a77d1 ("Animation, collision, and geometry") removed the
  `#pragma fenv_access(on)`/`(off)` pair that Gametoast's initial commit (85aff8b2) had put around
  these functions, and inlined the depth as `GetHoleHeight - vTrans.y + collHeight`.
- Finding: the expressions keep their terms and order, and the cylinder `sqrt` is `sqrtf` of the
  same sum (a double `sqrt` of a float rounded back to float is the correctly rounded float root,
  which `sqrtf` returns). Only the `fenv_access` restriction on code generation is gone.
- Effect: none. Every traced turn is identical on all six missions, and the CollideTerrain path runs
  for every soldier and vehicle on terrain in each of them.
- Judgement: accepted (no source value change; the pragma was a matching device, not retail
  behaviour). No row moved.

### C-SIM-REWRITE-30

- Functions: the upstream-only sim files rewritten by 0a5ec338 ("Animation, vehicles, geometry")
  and 472a77d1 with no ledger row, so not read hunk by hunk in 30-07: `CollisionMesh.cpp`,
  `WalkerLegPair.cpp`, `SoldierAnimator.cpp`, `SoldierAnimatorLowRes.cpp`, `EntityDroid.cpp`,
  `EntityProp.cpp`, `ProceduralAnimation.cpp`, `TreeGrid.cpp`, `VehicleCoverHelper.cpp`,
  `Combo.cpp`, `EntityPath.cpp`, `BoundaryManager.cpp`, `UnitFlyAgent.cpp`,
  `UnitTrooperAgent.cpp` and `GameModel.cpp`.
- Gametoast change: hand-expanded vector math replaced by the `PblVector`/`PblMatrix` helpers.
- Finding: 30-D13-ITEMS.tsv asked for a bisect "if a digest moves". None moved: the six factory
  logs are byte-identical and every one of 1600 traced turns is identical on all six missions
  (geo1c_con, nab2g_c, hot1g_con, spa3c_ass, dag1c_ctf, kas2c_uber), which cover soldiers, walkers,
  hovers, flyers, droidekas, props, paths and AI on ground and space maps. The Twice, Dedicated and
  Crash sets pass on the same build.
- Judgement: accepted (no observable difference). No row moved.

### C-WIRE-ORDER-30

- Functions: the 26 `DIFF` lines of `wire-order.ps1 -Base 5db7e6ac` with compiler-resolved widths on
  both sides (`FILES 19`, `COMPARED 1381 base=1381 root=1384`, no `MISSING` and no `ADDED` line;
  the three extra root functions have no Write/Read call). Each is classified in the phase's
  `tools/wire-classes.tsv`.
- Gametoast change: d1bfef35 ("Sound, voice, and network reconstruction") rewrote the NetState
  reads with named `bool` locals, `Read(bool&)`/`Write(bool)` and `reinterpret_cast` enum reads,
  rebuilt `UnpackHeader` around `NetComm::PktHeader`, swapped the zoom branches of
  `Weapon::Write`/`Weapon::Read` and replaced the `mBankPitchAngle` range literal by
  `PblMath::fTwoPi`.
- Finding, by class:
  - Text only (17 lines: `UnpackHeader` 0x005b2f10 and NetState 0x005f1650, 0x005f1af0, 0x005ecca0,
    0x005eeaf0, 0x005ef2f0, 0x005f52d0, 0x005efaf0, 0x005e7fd0, 0x005e8e80, 0x005eb7e0, 0x005ede90,
    0x005f4ba0, 0x005e9660, 0x005ea140, 0x005f2ec0, 0x005f1db0): the resolved shape is identical.
    `Read(bool&)` is `ReadBits(uValue, 1)` and `Write(bool)` is `WriteBits(bValue != 0, 1)`
    (`NetPacket.h`); `UnpackHeader` still reads u8, u16, u8, u8 in that order.
  - Branch swap (0x005e8330 `Weapon::Write`, 0x005e84d0 `Weapon::Read`): `mZoomRate != 0.0f` with
    `WriteFixed(..., 5)` first became `mZoomRate == 0.0f` with the 1-bit flag first. The two tests
    are exact complements for every value including NaN, so each runtime path writes and reads the
    same field and width as before.
  - Constant token (0x005eb920, 0x005ebfe0, 0x005f3500, 0x005f3910, `EntityFlyer` object and late
    state): `-PblMath::fTwoPi, PblMath::fTwoPi` for `-6.2831854820251465f, 6.2831854820251465f`.
    `fTwoPi = fPi * 2.0f` with `fPi = 3.141592654f` (`0x40490FDB`) is `0x40C90FDB`, the bits of
    `6.2831854820251465f`; the 16-bit width is unchanged.
  - Tool parse (`ReadGalaxy`, 0x00618250 `NetSocket::RecvGalaxySockets`): the base calls span two
    lines and the parser kept only the first; both sides pass the same arguments
    (`netGalaxyReceiveBuffer, capacity, &size, peer.mGalaxyId, 0`; capacities 2048 and
    `1012 - packetPrefix.value`). Only a redundant `INetworking*` cast is gone.
  - Typed getter (0x00617e50 `NetSocket::SendSocket`): `GetGalaxyNetworkingForNetComm` now returns
    `INetworking*`, so the cast before `SendP2PPacket` is gone; both sides call
    `SendP2PPacket(pPeer->mGalaxyId, pData, iLen, 2, 0)` with the same arguments.
- Effect: none. No Write/Read order or width changed, so the wire format is unchanged.
- Judgement: no wire change; nothing to fix or accept.

### Phase 30 D-13 items (30-D13-ITEMS.tsv)

| Test | Cause |
|------|-------|
| layout-pin FLInputManager.h | C-INPUT-ALIGN |
| pbllist-clear pcRedTexture.cpp, pcRedIndexBuffer.cpp, pcRedVertexBuffer.cpp | C-PBLLIST-RENDER |
| float RedTerrain::_RayHit | C-DOT-ORDER-30 (`GetInterpolatedNormal` now calls `Normalize`, which is `D3DXVec3Normalize`, on the same cross product) |
| digest-risk EntitySoldier::CollisionCallback, ::PostCollisionUpdate, ::UpdateWeaponAndAimer, ::Render; EntityHover::Move, ::CollisionCallback; EntityFlyer::PostCollisionUpdate; CollisionManager::UpdateSimple, ::UpdateRestricted; Aimer::GetLeadPos, ::Update; EntityDroideka::CollisionCallback | C-DOT-ORDER-30 |
| digest-risk CollisionPrimitive::CollideTerrain | C-FENV-TERRAIN |
| digest-risk upstream-only sim files | C-SIM-REWRITE-30 |

### Re-check of the Phase 29 causes

- C-WALKER-GRAVITY: Phase 30 re-check: unchanged. 0a5ec338 and 472a77d1 rewrote `EntityWalker.cpp`
  but kept `float fGravity = m_bIsJumping ? -9.8f : -29.400002f;`; `mVelocity.y = fGravity *
  fDeltaTime + mVelocity.y` became `mVelocity.AddY(fGravity * fDeltaTime)`, a two-term sum with
  the operands swapped (commutative). geo1c_con and hot1g_con are identical in every traced turn.
- C-PILOT-DELETE: Phase 30 re-check: unchanged. 7ad37c55 only replaced the local `EntityFlyer`
  stub declaration in `Controllable.cpp` with `#include "EntityCarrier.h"`; `Controllable::Kill`
  still caches `pPilot` and deletes it. spa3c_ass is identical in every traced turn.
- C-FLAG-PICKUP: Phase 30 re-check: unchanged. cf7b956a and a360dd99 changed the `Event<FlagItem,
  ...>` triggers and `AddToTeamManager`; the pickup gate is still `mStationary || ...`. dag1c_ctf is
  identical in every traced turn.
- C-CRITSYS-SHIELD: Phase 30 re-check: unchanged. 7ad37c55 only dropped `const` from the
  `Team*` parameters of `GetRandomFlyer`, `SpaceAssault_RandomFlyerKill` and
  `SpaceAssault_RandomCritSysDamage`; the cap is still `Min(Min(15000.0f, fBestDamage * 5.0f),
  pTarget->mCurShield)`. `AIDifficulty.cpp` keeps `/fp:strict`.
- C-TAT3-STALE: Phase 30 re-check: unchanged. ea864010..472a77d1 does not touch `src/Zephyr`.
- Phase 28 causes whose files 472a77d1's range touched: C-SHADER-INIT, Phase 30 re-check:
  unchanged; cf7b956a and 2443aca3 changed `pcRedShader.cpp`, `#pragma init_seg(lib)` is still at
  line 17, and no mission crashed at startup. C-ADDR-FLYER, Phase 30 re-check: unchanged; both
  `((unsigned int)this >> 2) & 0xf` sites remain (`EntityFlyer.cpp`, `EntityPathFollower.cpp`), and
  hot1g_con and spa3c_ass are identical in every traced turn. C-D14-RECHECK, Phase 30 re-check:
  unchanged; `RedTerrain.cpp` (C1, a360dd99 and cf7b956a) is covered by C-DOT-ORDER-30 and our
  C1 SSE line is unchanged; `WeaponCannon.cpp` (C3) and `PblOrderedList.h` (C-PTR03-SENTINEL) are
  untouched by the range. C-WIRE-ORDER, Phase 30 re-check: re-derived as C-WIRE-ORDER-30.
- Factory: Phase 30 re-check: all six factory logs are byte-identical between 5db7e6ac and the
  branch.

## Phase 33: pvs lists and the first mission from the shell

This records a deliberate Release change that no direct-start row can see.

### C-PVS-SHELLFIRST

- No digest row moved.
- Change: `PostStateCleanup` re-reserves the pvs displayable free and page lists in the Application
  heap after every cleanup, the shell's included (33-04, kept by the 33 review fix for WR-04).
- Before: a mission started from the menu found the lists emptied by the shell's cleanup and
  regrew them in the Runtime heap during load. A mission started from the command line found them
  reserved in the Application heap by static initialisation. The two paths gave different Runtime
  layouts.
- Proof (Test, `-shellfirst`, which starts the shell and then the named mission): Runtime at turn
  1500 for geo1c_con is `peak=13985712 used=13262816` from the command line and from the shell
  with this change, and `peak=13986288 used=13263392` (576 bytes more) from the shell with the
  re-reserve limited to mission cleanup. The repeated mission of `-repeatmission` gives the same
  pair: equal to the command-line start with the change, 576 bytes more without it.
- Sim: the shell-first digests of geo1c_con, hot1g_con, spa3c_ass, kas2c_uber and dag1c_ctf are
  identical with and without the change, so no simulation difference was observed.
- Judgement: accepted. Every mission entry now starts from the pvs state of a fresh process.
  `shell-first` (Test and Asan) gates the path with its own digest row, and run.ps1 checks that
  its Runtime at turn 1500, and the repeated mission's, equal the `geo1c_con` case.

## Phase 36: serial sim order

Every digest row moved once, when the default sim order became `serial`. This is the one digest
re-baseline of the milestone.

### C-LAYOUT-SERIAL

- Commit: 27e2f31d. `PblSimOrder.cpp` defaults to `PBL_SIMORDER_SERIAL` in every configuration,
  and every order-dependent baseline was re-captured under it in the same commit. `-simorder
  address` stays accepted.
- Proof: the Test Perturb group under the serial default is equal in every run (66 rows: 6 base
  and 60 equal, pad, fill, top, pool, hog and repeat included; `artifacts\tests\20261107-154647`).
  The same build under `-simorder address` splits 26 of 60 (pad 2, top 12, pool 12;
  `artifacts\tests\20261107-155311`). Release and Test give the same rows, and two Release
  `-Smoke All` runs give equal digests in all 137 digest-bearing cases other than `dedicated`.
- Rows of `tests/smoke/digest-expected.txt`, old (address) -> new (serial). `turns=1500`,
  `time=50.000` (52.033 for nab2g_c) and `snd=49.923` (49.956 for shell-first) did not change.
  - `geo1c_con`, `repeat-digest`: `rand=1354762085/2950811992 state=0x16D65714` ->
    `rand=234993775/2651921600 state=0x65CD7716`
  - `nab2g_c`, `twice-digest-1`, `twice-digest-2`: `rand=2206058303/116923646 state=0xA880CE00` ->
    `rand=1364591897/822169274 state=0x112D7CE2`
  - `hot1g_con`, `digest-hot1g_con`, `trace-seed-1`: `rand=2763759203/2960762048
    state=0x23F2B567` -> `rand=6704969/2285451412 state=0x82584D2E`
  - `spa3c_ass`, `digest-spa3c_ass`: `rand=1799084171/444301954 state=0xC77ED51E` ->
    `rand=723220111/3966278132 state=0x7758D69F`
  - `dag1c_ctf`, `digest-dag1c_ctf`: `rand=423450483/1140324658 state=0xED6D9B0A` ->
    `rand=3141009605/1956807958 state=0x0D75FA96`
  - `kas2c_uber`, `digest-kas2c_uber`: `rand=612937807/386973628 state=0x765A955F` ->
    `rand=3352953385/1695809336 state=0xAA5AA809`
  - `shell-first`: `rand=1204355361/2225627158 state=0x59BA0C43` ->
    `rand=3081717179/1987697560 state=0xE1B0D0FF`
- Rows of `digests.txt` (and the byte-identical `digests-hog.txt`), old -> new. The old
  hot1g_con and spa3c_ass rows were older captures than the digest-expected rows they now equal.
  - geo1c_con: `state=0x16D65714` -> `state=0x65CD7716`
  - nab2g_c: `state=0xA880CE00` -> `state=0x112D7CE2`
  - hot1g_con: `rand=2763759203/2960762048 state=0x1FE5B21F` -> `rand=6704969/2285451412
    state=0x82584D2E`
  - spa3c_ass: `rand=529082281/2897934758 state=0x7385D175` -> `rand=723220111/3966278132
    state=0x7758D69F`
- Rows of `digests-debug.txt`, old -> new: twice-digest-1 `0x4996D99B` -> `0xE93085A2`,
  digest-hot1g_con `0x278F8853` -> `0x94E6C26F`, digest-spa3c_ass `0x222876E4` -> `0xF44AC841`.
  The four `digest-trace-*.txt` files and `smoke-all-digests.txt` were re-captured in the same
  commit; their hashes are in MANIFEST.md.
- First split turn: the first turn where the `-digesttrace 1600` of `-simorder address` and of
  `serial` differ, from the two Perturb base runs of the same Test build. On every mission the
  first split is in `state=`, with `rand=` still equal on that turn.

  | Mission | First full-line split | First state split |
  |---------|-----------------------|-------------------|
  | geo1c_con | 4 | 4 |
  | nab2g_c | 19 | 19 |
  | hot1g_con | 11 | 11 |
  | spa3c_ass | 11 | 11 |
  | dag1c_ctf | 11 | 11 |
  | kas2c_uber | 4 | 4 |

- Lua: under `serial`, `next()` walks the hash part in insertion order and `resize` keeps that
  order. At each collection a key whose value is nil becomes a dead key, so a key stored again
  after a collection goes to the list end instead of reviving its old place. `tostring` and
  `print` keep `%p`, so a script that keys a table by `tostring(x)` stays address-dependent; no
  Perturb run shows one.
- Mod-facing, SER-04 wobble: both wobble sites of a flyer (`EntityFlyer::GetBranchParams` and
  `EntityPathFollower::Roll`) take one phase from the owning object's handle id `& 0xf`. Retail
  phased each from its own `this`, so the path follower wobbled out of phase with the flyer body;
  under serial the two are in phase. Mods that tune flyer or carrier roll can see it.
- Outcome: dea1g_1flag ended on its own under serial (the Empire won 5-4 at 85.9 s, 4145 s of game
  time) where address ran to the watchdog. Flag (`_1flag`, `_ctf`) and assault (`_ass`) cases in
  `-Smoke All` without a digest row now run with `-novictory` and expect `TimeOut`, so no flag or
  assault outcome gates smoke. Apart from the `-novictory` lines, no `missions-all.txt` class
  changed under serial.
- Limit: the engine-forced Lua collections are frame-paced (`DoPeriodicLuaGC`, every 64 calls
  from the game loop and the interface callbacks). Whether a re-stored key revives therefore
  follows the rendered-frame count: reproducible single-process at a fixed rate, as in every
  digest and Perturb run, but not across machines or lockstep peers. A recycled lightuserdata
  pointer still revives before the next collection. Moving the collections onto the sim-turn
  cadence is deferred.
- Water timers, 1deb26d9: `PostStateCleanup` now resets `s_fSplashTimeout` (`0x01faa460`) and
  `s_fWakeTimer` (`0x01faa46c`), which retail kept as function-local statics of
  `EntitySoldier::Update` that leaked into the next mission. With the leak, a soldier landing in
  water in a repeated mission built a splash emitter whose `ParticleEmitter::Reset` takes 6
  `_Global` draws, so a repeat run diverged from a fresh one. The second and later missions of
  one session therefore differ from retail, against a retail peer included. No single-mission
  digest moved; the `twice-digest-*` and `repeat-digest` rows were captured with the reset in
  place.
- History, C-LAYOUT-HOT1G (Phase 33): fb63997b (the glyph cache block grows by 4 bytes) and
  e4a4bc99 (`HUD::Item::SetName`) changed no simulation code but moved hot1g_con from
  `rand=403802221/2528439372 state=0xC95E64E8` to `rand=2763759203/2960762048 state=0x23F2B567`
  under address order. Adding an unused 4 bytes to the same glyph cache allocation on 108efaa0
  gave fb63997b's digest exactly, with traces identical through turn 619 and split at turn 620
  (`pad`). Under serial the pad split is gone; the row was re-captured here anyway, so that
  cause is folded into this one.
- Judgement: accepted. Removing address order changes order by design. Any later digest change
  needs its own cause and a reason why it is not a bug.

## Phase 42: Gametoast 0c1bbd3e against e9a7cc62

Phase 42 merged Gametoast/swbf2decomp `0c1bbd3e` (28 commits past `472a77d1`) and re-applied our
changes on top of it. The reference is e9a7cc62, the Phase 41 result (P41), built Release v143 in the
staged main tree (`artifacts/p42-base`, exe `BattlefrontII-p41.exe`). Every difference in SIM DIGEST,
final score, CLOTH, mem-peaks, pool-sizes, lua-bins, Perturb, Debug digests, Factory order, digest
traces, layout pins, initializer tables, the network Write/Read order and the corpora between P41
and the branch is listed here with its cause, the Gametoast commit in 472a77d1..0c1bbd3e and the
retail evidence (Phase 42 D-09). Every row that moved was re-pinned once in 42-10, with the user's
approval on 2026-11-12.

Method: the Release, Test and Debug smoke sets on the branch (Release `-Smoke All` with the table
comparison against `tests/baseline/v143`, Test `-Smoke All` with lua-bins, Test `-Smoke Perturb`,
Debug `-Smoke Twice`), `-factorylog -digesttrace 1600` runs of the six digest missions on the branch
(`artifacts/p42-branch`, compared with `artifacts/p42-base`), the initializer tables from the map,
the layout pins and corpus replays in T1, and `wire-order.ps1` with compiler-resolved widths on both
sides. A move is localised with a whole-game digest trace on both exes, then a per-turn trace of the
callers of the global random stream, and confirmed by reverting only the upstream change on the
branch. Retail evidence is from `dumpbin /disasm:nobytes` of the retail Steam `BattlefrontII.exe`.

| Mission | Turn-1500 digest on P41 and the branch | First split turn | Cause |
|---------|----------------------------------------|------------------|-------|
| geo1c_con | equal to its `tests/smoke/digest-expected.txt` row | none (1600 of 1600 turns equal) | none |
| nab2g_c | equal to its `tests/smoke/digest-expected.txt` row | none (1600 of 1600 turns equal) | none |
| hot1g_con | equal to its `tests/smoke/digest-expected.txt` row | none (1600 of 1600 turns equal) | none |
| spa3c_ass | equal to its `tests/smoke/digest-expected.txt` row | 3250 of 21874 (whole game) | C-FLYER-REACT |
| dag1c_ctf | equal to its `tests/smoke/digest-expected.txt` row | none (1600 of 1600 turns equal) | none |
| kas2c_uber | equal to its `tests/smoke/digest-expected.txt` row | none (1600 of 1600 turns equal) | none |

Of the 109 `-Smoke All` missions only spa3c_ass moved. Its turn-1500 digest is unchanged, so no
`tests/smoke/digest-expected.txt` row moves; its final score and its 19 exit pool peaks in
`mem-peaks.txt` move. The six factory logs and the 1600-turn digest traces are byte-identical, so no
`factory-*.txt` or `digest-trace-*.txt` changes. CLOTH, pool-sizes and the other 108 digest and score
rows are equal. Test `-Smoke All` (lua-bins, 5123 keys, and the off-thread
check), Test `-Smoke Perturb` (`serial equal runs=56/56`, rows equal to `perturb.txt`), Debug
`-Smoke Twice` (Debug digests), the layout pins and the corpus replays are unchanged.

### C-FLYER-REACT

- No digest row moved. spa3c_ass keeps its turn-1500 `digest-expected.txt` row; its
  `smoke-all-digests.txt` final score and 19 `mem-peaks.txt` exit pool rows move (below).
- Function: `EntityFlyer::Update` (`// FUNCTION: SWBF2 0x004ac460`,
  `src/Battlefront2/Source/EntityFlyer.cpp`), the block that makes a fired-at AI flyer call
  `EntityFlyer::TargetReact` (`// FUNCTION: SWBF2 0x004b1b90`), which draws
  `PblRandom::GetGlobalInt() % 100`.
- Gametoast change: aded7a4a ("More proper type casting") rewrote the target test. The old
  condition called `pFlyer->GetFollowingPath()` (`// FUNCTION: SWBF2 0x00477870`), which is
  `mFollowingPath && mState == FLYING && IsAlive() && mAIEng.ai.mFreeFlyTime <= 0.0f && !mInRoll &&
  !mInFlip`. The new condition spells out `mFollowingPath && mState == FLYING && IsAlive() &&
  !mInRoll && !mInFlip`, without the free-fly term, and the reaction test is
  `0.0001f >= mReactionTime` instead of `!(mReactionTime > 0.0001f)` (different only for NaN).
- Evidence: a whole-game `-digesttrace` of spa3c_ass on both exes is equal for 3249 turns. At turn
  3250 only the global random seed differs (`rand=1874424079/...` on P41, `3248828441/...` on the
  branch, state hash and sim seed equal). A probe that records the caller of every global draw
  (same patch on both trees, traces unchanged by it) gives 104 draws in turn 3250 on P41 and 105 on
  the branch; the extra one is `EntityFlyer::Update`, at the return from the `TargetReact` call.
  Reverting only this condition to `pFlyer->GetFollowingPath()` on the branch gives P41's trace on
  all 21874 turns, P41's final score and P41's pool peaks.
- Retail evidence: retail `EntityFlyer::Update` tests, at `0x004AD5B7..0x004AD648`,
  `mPlayerId < 0`, the pilot type, `mPilot`, `mClass->mbFlyerVsFlyer`, `mFollowingPath`
  (`test dl,8` on `[ecx+5B4h]`), `mState == 2`, the alive bit, `mInRoll`/`mInFlip` (`test dl,3`),
  `mbQueuePoint`, `mbGoMinSpeed` and `mProcedurallyAnimated`, then calls `004B1B90` at `0x004AD6A1`.
  There is no free-fly test, and the reaction test is `comiss 0.0001f,[edi+1A70h]` with `jb` over
  the call, which skips it for NaN as `0.0001f >= mReactionTime` does. Gametoast's new form is the
  retail one; P41's call to the full `GetFollowingPath()` was a decompilation inaccuracy.
- Effect: a flyer that is in free flight when an AI fires at it now runs `TargetReact`, which draws
  one global random number and can start a turn, evade or trick. On spa3c_ass that first happens at
  turn 3250, and the game ends at 716 s instead of 737 s.
- Moves: spa3c_ass in `smoke-all-digests.txt` (final score `737s ... 32u@10P -1r 188p ... 32u@15
  -1r 155p` becomes `716s ... 32u@11P -1r 181p ... 32u@14 -1r 156p`; the turn-1500 digest is
  unchanged) and the 19 spa3c_ass `exit pool` rows of `mem-peaks.txt` (Aimer, AmmoCounter, EnergyBar,
  EntityFlyer, EntityLight, FLEffectObject::OffsetMatrix, Obstacle, Ordnance, ParticleEmitter,
  ParticleEmitterInfoData, ParticleEmitterObject, PathNode, PathRequest, PowerupItem, RayRequest,
  RedOmniLight, TreeGridStack, VehicleSpawn::VehicleTracker, Weapon).
- Status: re-pinned with user approval 2026-11-12.

### C-INIT-ORDER-42

- No digest row moved.
- Functions: the `.CRT$XCA..XCZ` initializer table and the dynamic initializers in the map
  (`capture-vs2013.ps1 -Map`, `artifacts/p42-branch/map` against `artifacts/p42-base/map`).
- Gametoast changes:
  - 0c1bbd3e ("Reorder: Render constructors") added `#pragma init_seg(lib)` to `PblMath.cpp`,
    `pcFLRender.cpp`, `pcRedAutoSettings.cpp`, `pcRedShader.cpp` and 23 of the 26 `Shaders\*.cpp`, and
    removed it from `pcPblMatrix.cpp` (`_IdentityMatrix` became a per-TU static in
    `pcPblMatrix.inl`; e7791bfa and 3f03ca22 moved it there).
  - 1310337d ("RTTI Inheritance") and 65b0b893 made the `rttiHash*` statics private through the
    `PBL_DECLARE_*_RTTI` macros, 0c1bbd3e made the shaders' `s_Factory`, `s_shaderDef` and
    `s_uiRenderTypeHash` private, and 331568b7 and 0c1bbd3e made the GameLoop UI statics class
    members.
  - c7d933b7 moved `PlayerPrefs::mAllUserPrefs` to PlayerPreferences.cpp, `gVoiceChatTeamColors` to
    TeamDisplay.cpp and `sNegativeXRotation80` to HUDDirections.cpp; 6edcc0e7 moved
    `netAutoAssignTeamsOriginalSetting` to NetGame.cpp.
- Finding: the tables keep their sizes (898 initializers, 8 discarded, 903 dynamic initializers on
  both builds). 118 entries differ only in spelling (32 RTTI hashes and 78 shader statics now
  `private`, 8 GameLoop statics demangled as class members), 5 moved to another object (the moves above and the
  `_IdentityMatrix` copies now in NetGame.obj and NetSetup.obj instead of EntityTrap.obj and
  pcRefractionShader.obj), and none was added or removed. The order changed: PblMath, pcFLRender,
  pcRedShader, pcRedAutoSettings and 23 shader TUs now run first, after the CRT entries (the
  Refraction, Shield and Water shaders stay in the user section, after pcRedShader), and
  `gMatrixIdentity` moved from position 2 to position 746. `pcRedShader.cpp` still runs before every
  shader TU (C-SHADER-INIT). The only earlier initializer found that reads `gMatrixIdentity` is
  `trackDesc` (`NetGame.cpp`, position 515), which nothing reads (42-UPSTREAM-BUGS row 21).
- Review fix 42-09 WR-02 (bb00d88b) interacts with 0c1bbd3e's removal of `init_seg(lib)` from
  `pcPblMatrix.cpp`: retail runs `gMatrixIdentity`'s initializer at index 466, in the user section
  after RedEngineFL and before every Battlefront2 initializer, so the pragma's removal is retail and
  only our single-project link order was wrong. `BattlefrontII.vcxproj` now lists `pcPblMatrix.cpp`
  before the first Battlefront2 TU (guard row INITORDER-01). On 277bc98c (`capture-vs2013.ps1 -Map`,
  `artifacts/p42-10/map`) `gMatrixIdentity` is at position 125, the first user-section entry after
  the last lib-section shader initializer, and `trackDesc` is at 516, after it; in
  `initializers-di.txt` the `gMatrixIdentity` row moves to the first line. The other 897 rows keep
  0c1bbd3e's order, the sizes stay 898/8/903, and the factory logs, the 1600-turn digest traces and
  every table row other than C-FLYER-REACT are equal to e9a7cc62.
- Effect: none on the sim. Factory logs, digest traces and every digest except C-FLYER-REACT are
  equal. The memory report prints the pools in registration order, so the `MEMREPORT` lines of a run
  come out in a different order; sorted they are equal, and `mem-peaks.txt` is keyed, so no row
  moves for it.
- Moves: `artifacts/p42-base/map/initializers*.txt` against `artifacts/p42-10/map/initializers*.txt`.
  The pinned `tests/baseline/v143/initializers*.txt` were the Phase 20 capture (dce2810) and already
  differed from P41; no gate compares them. 42-10 re-captured them from the 277bc98c map.
- Status: re-pinned with user approval 2026-11-12.

### C-RTTI-42

- No digest row moved.
- Functions: the 31 classes of 42-D13-ITEMS.tsv `rtti-layout`, through `PBL_DECLARE_ROOT_RTTI` and
  `PBL_DECLARE_DERIVED_RTTI` (`src/PebbleFL/Common/PblRtti.h`).
- Gametoast change: 1310337d ("RTTI Inheritance") and 65b0b893 replaced the hand-written
  `IsRtti`/`GetDerivedRtti`/`GetDerivedRttiName` declarations by the macros.
- Finding: the derived macro only overrides the three virtuals, so it adds no slot. Of the eight
  root classes, AIGoal, Controller, Entity, Ordnance, OrdnanceClass and Weapon declare their
  virtuals in the same order as at P41. Navigator now declares the RTTI virtuals before
  `~Navigator`, but `~Navigator` overrides `Fsm::~Fsm`, so it has no new slot. WeaponClass now
  declares `Build` after the RTTI virtuals, but `Build` overrides `Factory::Build`, so the new slots
  are still `IsRtti`, `GetDerivedRtti`, `GetDerivedRttiName` in that order.
- Effect: none. The layout pins (328 `static_assert`s and `game object handle id stays at offset
  0x204`) pass, factory logs and traces are equal.
- Judgement: no move.

### C-FLOAT-42

- No digest row moved.
- Functions (42-D13-ITEMS.tsv `sim-digest`): `operator+(const PblVector3&, const PblVector3&)`
  (`0x0040ca30`) and `operator*(float, const PblVector3&)` (5196d9b2), `DirectionMatrix`
  (`0x0043cd40`, 3f03ca22), the four slip sites and the spin update of `EntityFlyer::Update`
  (`0x004ac460`) and `Chunk::Update` (`0x00455c90`) (5e0adcc0), `WeaponDetonator::NetAttachItem`
  (`0x006827d0`, 81d6a7b4) and `UnitFollowAgent::EnterState` (`0x006680d0`, 9fdad8a7).
- Finding: each new form computes the same IEEE operations on the same operands:
  - `D3DXVec3Add` and `D3DXVec3Scale` are the same per-component add and multiply; a two-operand
    float multiply is commutative.
  - `operator/(const PblVector3&, float)` is `lhs * (1.0f / rhs)` (`pcPblVector.inl`), so
    `v / (a * b)` is `v * (1.0f / (a * b))`, the old expression, and `v * 20.0f / 5.0f` is
    `(v * 20.0f) * (1.0f / 5.0f)`; `1.0f / 5.0f` rounds to `0x3E4CCCCD`, the bits of `0.2f`. Retail
    `UnitFollowAgent::EnterState` multiplies by the constant `0x3E4CCCCD` at `0x0066821A..0x00668228`.
  - The flyer spin is `((v * 0.015625f) * (1.0f - mFlightRatio)) * fDeltaTime` added to `mOmega`,
    per component, as before; addition of two terms is commutative.
  - `Chunk::Update` and `NetAttachItem` keep the same multiplies and sums through named temporaries.
- Effect: none. The flyer, chunk and AI follow paths run on hot1g_con, spa3c_ass and kas2c_uber; all
  traced turns are equal, and the only whole-game move (spa3c_ass) is C-FLYER-REACT.
- Judgement: no move.

### C-SLIDE-42

- No digest row moved.
- Function: `EntitySoldier::Render` (`// FUNCTION: SWBF2 0x004e23d0`), the foot-slide effects.
- Gametoast change: 5e0adcc0 removed `bSlide = false;` before the left foot and computes the speed as
  `SquaredLength((1.0f / fDeltaTime) * (...))`.
- Retail evidence: retail clears the flag once (`mov byte ptr [esp+0Fh],0` at `0x004E3E68`) before
  the right foot and sets it for the left foot only inside the grounded test (`setae` at
  `0x004E410B`), so a left foot that fails the test keeps the right foot's value; the speed is
  `divss` of 1 by the delta then `mulss` per component (`0x004E40BB..0x004E40F4`). Gametoast's new
  form is the retail one.
- Effect: render path only (foot-slide effect start and stop). A branch build with `bSlide = false;`
  restored gives the branch's spa3c_ass trace, not P41's, so it is not the cause of C-FLYER-REACT.
- Judgement: no move.

### C-SPECTATOR-42

- No digest row moved.
- Function: `NetGame::ClientUpdateSpectator` (`// FUNCTION: SWBF2 0x005c3d30`).
- Gametoast change: 52c5e8b2 ("Minor Net body updates") calls `Character::GetPtr(player)`
  (`0x00437510`) instead of `Character::FindPlayer(player)` (`0x00429470`).
- Retail evidence: retail calls `00437510` at `0x005C3D7E`, then `004293E0` (`GetPlayerId`) and
  `004533B0` (`AddPlayer`) when the id differs. Gametoast's form is the retail one. With
  `FindPlayer` the re-seat could never run.
- Effect: network client only; no single-player, host or dedicated digest runs it, and none moved.
- Judgement: no move in any pinned row (retail behaviour on a client).

### C-SHELL-INITALL-42

- No digest row moved.
- Function: `GameState::PreStateInit` (`// FUNCTION: SWBF2 0x0053af10`), the call of
  `FLEffect::InitAll` (`// FUNCTION: SWBF2 0x00526b20`).
- Gametoast change: 9fdad8a7 ("Reorder: Entity and weapon") calls `FLEffect::InitAll(true)` in
  every state; P41 passed `bMission`, so the shell did not activate the enabled effects.
- Retail evidence: retail `FLEffect::InitAll` stores the constant 1 in `s_bInitRender`
  (`mov byte ptr ds:[01E51E26h],1` at `0x00526BAA`) and activates every enabled effect with no test
  of an argument (`0x00526BDE..0x00526BE9`), and `PreStateInit` calls it at `0x0053B4BF` in both
  states. Gametoast's form is the retail one.
- Measurement: the `-shellfirst` geo1c_con case run with a P41 Test exe (built from e9a7cc62 in the
  NOSTAGE worktree `artifacts/wt-p42`) and with the branch Test exe (`artifacts/p42-08/shell-p41`,
  `shell-head`): the shell-exit heap reports (Runtime `peak=9567744 used=4311232 allocs=40313`, Lua,
  Application), the mission's turn-1500 and exit reports, every pool report, `HANDLESCAN` and the
  digest are equal line for line once sorted (only the pool print order differs, C-INIT-ORDER-42).
  No pinned table has a shell row; lua-bins (Test `-Smoke All`) is equal.
- Effect: none measured in headless runs.
- Judgement: no move.

### C-WIRE-ORDER-42

- No digest row moved.
- Functions: the 12 `DIFF` lines of `wire-order.ps1` against e9a7cc62 with compiler-resolved widths
  on both sides (`FILES 24`, `COMPARED 1557 base=1584 root=1558`, no `MISSING` and no `ADDED` line;
  `artifacts/p42/wire-order.txt`). Each is classified in the phase's `tools/wire-classes.tsv`.
- Gametoast changes: 6edcc0e7 (`NetGame::IsSkipped`, 0x005b7d70: the deleted `ReadNetIsLocalTurn`
  helper only matched the Read prefix), 52c5e8b2 (`MarkWriteAlign` 0x005bd440, `ReadDestMask`
  0x005d24f0, `WriteWaiting` 0x005d23a0, `WriteUploadAck` 0x005c4ff0, `WriteCommandPost` 0x005c2680,
  `AloNextGameInfo::Write` 0x005c70d0, `WritePingData` 0x005d2bb0), e92f57b2 (`ReadBootRequest`
  0x005d1240) and e672dc82 (`NetTrigger::Write` 0x005f60b0, `EntityDroideka::ReadObject` 0x005ede90,
  `EntityDroideka::ReadLateState` 0x005f4ba0).
- Finding: named locals, inverted branches and split flag writes; on every runtime path the same
  fields are written and read in the same order with the same widths.
- Effect: none. The corpus replays (recvgroup, recvpacket, chat-client, chat-server, upload, rcon,
  load, turnrate, netwire) pass.
- Judgement: no wire change.

### Phase 42 D-13 items (42-D13-ITEMS.tsv)

| Test | Cause |
|------|-------|
| sim-digest `operator+`, `operator*(float, PblVector3)` (5196d9b2) | C-FLOAT-42 |
| sim-digest `DirectionMatrix` (3f03ca22) | C-FLOAT-42 |
| net-client `NetGame::ClientUpdateSpectator` (52c5e8b2) | C-SPECTATOR-42 |
| sim-digest `EntityFlyer::Update` slip and spin, `Chunk::Update` (5e0adcc0) | C-FLOAT-42 |
| sim-digest `EntitySoldier::Render` `bSlide` (5e0adcc0) | C-SLIDE-42 |
| sim-digest `WeaponDetonator::NetAttachItem` (81d6a7b4) | C-FLOAT-42 |
| render `BirdParticleManager::SetPotentialScatterPoint` (81d6a7b4) | no move (effects only; pointer to reference, same callee) |
| render `LightsaberTrailEffect::RenderLightsaberTrail` (9ffe45cb) | no move (same vertex slots, render only) |
| sim-digest `UnitFollowAgent::EnterState` `/ 5.0f` (9fdad8a7) | C-FLOAT-42 |
| rtti-layout (1310337d/65b0b893) | C-RTTI-42 |
| shell-initall `GameState::PreStateInit` (9fdad8a7) | C-SHELL-INITALL-42 |

### Re-check of earlier causes

- C-SHADER-INIT, Phase 42 re-check: unchanged in effect. 0c1bbd3e put `#pragma init_seg(lib)` on
  every shader TU and on `pcRedShader.cpp` itself, so the order inside the lib section follows the
  project item order; `BattlefrontII.vcxproj` lists `pcRedShader.cpp` before `pcFLRender.cpp` and
  every `Shaders\` TU (42-02), and the map shows `s_ShaderFactoryByRenderType` ahead of every shader
  registration. The 42-07 Release Standard smoke (`artifacts/tests/20261112-020053`) and this
  plan's `-Smoke All` reach the first frame on every case with no startup access violation.
- Factory: Phase 42 re-check: all six factory logs are byte-identical between e9a7cc62 and the
  branch.

## Phase 43: static-library split and stick target against 0352ab66

Phase 43 split the single game project into the exe and six static libraries linked as object
inputs, made a collision object's death clear every stick and stuck mine that names it, and fixed
the client stick restore. The reference is 0352ab66, the Phase 42 result (P42). The measurement is
on the reviewed tree: two Release `-Smoke All` runs (`artifacts/tests/20261113-211209` and
`-212946`, tables byte-equal apart from the wall-clock `dedicated` row), `-Collect`, `-Map`,
`-Hardening`, Debug `-Smoke Twice`, Test `-Smoke All` (lua-bins, off-thread) and Test
`-Smoke Perturb`, each compared with this directory.

Digests, final scores, CLOTH, mem-peaks, pool-sizes, lua-bins, Perturb, Debug digests, the four
factory logs and the four 1600-turn digest traces are equal to P42 on every mission. The only moved
rows are in `initializers.txt` and `initializers-di.txt` (below). New rows (the `crash-stickdelete`
case, the `mimalloc-crt-hooks` hardening line and the off-thread rows of four crash cases) are
additions, not moves. The `crt-heap-ban` line that `hardening.txt` and `smoke-all.txt` also lack is
not from Phase 43: e9a7cc62 (Phase 41) added the check after f1090621 last captured both files, and
the P42 runs already print it.

### C-SPLIT-STEAM-INIT

- No digest row moved. Only `initializers.txt` and `initializers-di.txt` change; no check reads
  them.
- Cause: at P42 the single project listed `src\Steam\gfGalaxy.cpp`, `gfSteamAuth.cpp` and
  `src\lglcd\lgLcd.cpp` last, after the zlib items. With library objects linked as inputs every exe
  object precedes every library object, so `gfSteamAuth.obj` now initializes after `PblTime.obj`
  and before `RedWater.obj`, the first library initializer. `gfGalaxy.obj` and `lgLcd.obj` have no
  initializers; PebbleFL, GameSpyFL and Battlefront2 objects preceded every library object at P42
  too.
- `initializers.txt`: 898 rows on both, the same (symbol, object) multiset. The
  `gEncryptedAppTicket` and `gFriendConnectionStrings` initializer pointers of `gfSteamAuth.obj`
  move from rows 895-896 to 748-749, and the 147 rows between shift by 8 bytes. No spelling change.
- `initializers-di.txt`: 903 rows on both, the same multiset. The same two dynamic initializers
  move from rows 901-902 to 632-633; 272 rows change code address (the two moved functions and 270
  library functions whose `.text` now follows every exe object). No spelling change.
- Safety: `gEncryptedAppTicket` is a default `std::string` and allocates nothing.
  `gFriendConnectionStrings` is a default `std::unordered_map` that allocates through the global
  `operator new`. The heap is already built at both positions by `PblHeap.obj`'s `test`
  initializer (row 742 in both trees), whose first `operator new[]` runs `Startup_RedInitHeap`, and
  no library initializer or atexit destructor writes allocator state. mimalloc's hooks stay in
  `.CRT$XIB`/`.CRT$XLB`/`.CRT$XLY` at P42's addresses. The two globals are read only by Steam
  callbacks and shell queries after `WinMain`.
- Status: re-pinned with user approval 2026-11-13.

## Phase 44: test speed-up against 7bb63e16

### C-HARDEN-TESTS-EXE

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:aslr-pe-tests`; no
  other row moves.
- Cause: `run.ps1` printed `aslr-pe-tests` only when the same call also ran T1, so
  `run.ps1 -Configuration Release -NoBuild -Hardening` printed 38 lines and a call with T1 printed
  39. `compare-tables.ps1 -Hardening` now compares the lines exactly, so the check reads the tests
  exe at its path (`bin\Tests\<configuration>\OpenSWBF2Tests.exe`, or `OPENSWBF2_TEST_EXE`) whether
  or not T1 ran, the same way `flag-alignednew-tests` already reads the tests tlog, and prints a
  SKIP row when the tests exe is not built.
- Captured from `run.ps1 -Configuration Release -NoBuild -Hardening` with
  `capture-vs2013.ps1 -Tables -HardeningDir`.

### C-HARDEN-BASEREAD

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:baseline-readers`; no
  other row moves.
- Cause: a new source check. `baseline-readers` fails when a file under `tests\baseline\v143` has
  no entry in the reader map in `compare-tables.ps1`, or an entry names a reader that does not
  read it. The source checks print one line each, so the hardening output has one more line.
- Captured from `run.ps1 -Configuration Release -NoBuild -Hardening` with
  `capture-vs2013.ps1 -Tables -HardeningDir`.

### C-INIT-ADDR

- No digest row moved. `initializers.txt`, `initializers-di.txt` and `initializers-discarded.txt`
  lose their second column, the section:offset address; every other byte is unchanged.
- What moved: plan 44-18 adds the progress section (`pcProgress.cpp`). The Release exe now imports
  `CreateFileMappingA` and `MapViewOfFile`; the import address table sits at the start of `.rdata`,
  ahead of `.CRT$XCA..XCZ`, so every `initializers.txt` offset grew by 0xC. `.text` grew to
  0x45213A bytes, 0x13A past a page boundary, so `.rdata` starts one page later and every
  `initializers-discarded.txt` offset (`.text` symbols written relative to `.rdata`) fell by 0x1000.
  `initializers-di.txt` did not move only because the new code links after every `??__E` function.
- Proof: in all 898 `initializers.txt` rows and all 8 `initializers-discarded.txt` rows the index,
  symbol, demangled name and object are identical to the pinned files, and only the address
  differs (+0xC and -0x1000 on every row); the 903 `initializers-di.txt` rows were byte-identical.
  The re-capture (`capture-vs2013.ps1 -NoBuild -Map`) of all three files equals the previous pinned
  file with its second column removed, byte for byte.
- Why the address is not compared: what these tables protect is initializer order and membership,
  the property C-INIT-ORDER-42 and C-SPLIT-STEAM-INIT reason about (which objects initialize, in
  which order, and what runs before `gMatrixIdentity` or the heap). The address records link
  layout, which any new import or any `.text` growth across a page moves without changing a single
  initializer. The capture still sorts by address, so the row order is the initializer order, and
  `compare-tables.ps1 -Map` still compares the files byte for byte: a reordered, added, removed or
  renamed initializer fails (selftest fixture `map compare fails a reordered, added or removed
  initializer`). `tests/baseline/vs2013/initializers*.txt` keep their address column: no compare
  reads them, only `-Verify` checks their manifest hashes.
- Status: check change and re-capture approved by the user 2026-11-15.

### C-CASE-CRASHSTALL

- No digest row moved. `smoke-all-digests.txt` and `offthread.txt` each gain one row for the new
  Crash case `crash-stall`; no other row moves.
- Cause: a new smoke case. `crash-stall` runs the standard mission with `-crashtest hang` under the
  supervised watchdog, so the harness sees no progress, sets the stall flag and the game writes its
  own hang dump and exits 5. The hang starts at frame 1, before the first turn, so it logs no SIM
  DIGEST line (`none none`, the same as `crash-hang`) and no off-thread allocation (`0 0 0`).
- Captured from `run.ps1 -Configuration Release -NoBuild -Smoke All -Soak Short`
  (`smoke-all-digests.txt`) and `run.ps1 -Configuration Test -NoBuild -Smoke All -Soak Short`
  (`offthread.txt`); the pinned files equal those captures apart from the rows no compare reads
  (the wall-clock `dedicated` row, the `# commit` line and the `sound-on` section).
  - row: smoke-all-digests crash-stall
  - row: offthread crash-stall

## Phase 45: x64 build and pointer width

### C-HARDEN-X64VERDICT

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:allow-x64-verdict`; no
  other row moves.
- Cause: a new source check. `allow-x64-verdict` fails when a `SpacerAllow` or `OffsetAllow` entry
  in `tests\smoke\cases.ps1` does not end with `|x64:safe:<reason>` or `|x64:fixed:<what changed>`
  with a non-empty text. The source checks print one line each, so the hardening output has one
  more line.
- Captured from `run.ps1 -Configuration Release -Smoke None -Hardening` with
  `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch `-OutDir`; the
  capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under phase 45 decision D-53, 2026-11-17.

### C-HARDEN-DELAYIMPORTS

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:import-delay-modules`; no
  other row moves.
- Cause: the import check now splits `dumpbin /imports` at `Section contains the following delay
  load imports:` and judges the delay-import table against its own per-platform list in
  `tests\smoke\cases.ps1` (phase 46 D-22, VEND-07), so the hardening output has one more line.
- Captured from `run.ps1 -Configuration Release -Smoke None -Hardening` with
  `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch `-OutDir`; the
  capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under phase 46 decision D-02, 2026-11-18.

### C-HARDEN-GALAXYSEAM

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:galaxy-factory-seam`,
  after `galaxy-getter-cast`; no other row moves.
- Cause: a new source check (phase 46 D-07) fails on any `GalaxyFactory::` call under `src` outside
  `src\Steam\gfVendor.cpp`, so the hardening output has one more line. The same plan delay-loads
  `steam_api.dll` and `Galaxy.dll`; `import-modules`, `import-delay-modules` and `safeseh` keep
  their PASS rows because the allow-lists move those DLLs from the static to the delay list.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke None -Hardening`
  with `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch
  `-OutDir`; the capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under phase 46 decision D-07, 2026-11-18.

### C-HARDEN-UNICOWS

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:unicows-staged`, after
  `import-delay-modules`; no other row moves.
- Cause: a new check (phase 46 D-20). `unicows.dll` is no longer staged, the `StageVendor` target
  deletes a stale copy from the output folder, and `unicows-staged` fails when the file is in the
  output folder, in a `GameDataFile`, `VendorFile` or `Copy` item of `BattlefrontII.vcxproj`, in the
  worker file list, in either import section of the exe, or named under `src`. Nothing imports it,
  so the row passes and the hardening output has one more line.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -Smoke None -Hardening` with
  `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch `-OutDir`; the
  capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under phase 46 decision D-20, 2026-11-18.

### C-HARDEN-D3DX9

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:d3dx9-pins`, after
  `unicows-staged`; no other row moves.
- Cause: a new check (phase 46 D-17). `d3dx9_43.dll` now ships beside the exe from
  `Microsoft.DXSDK.D3DX` 9.29.952.8, pinned in `lib\vendor-pins.props`. `d3dx9-pins` reads build
  files only: it fails when a pin is not 64 lower-case hex digits, when `BattlefrontII.vcxproj` does
  not import the pins, or when `StageVendor` lacks the `VerifyFileHash` of the cached package or of
  the Win32 or x64 DLL against its pin. It prints the same line in staged, `SWBF2_NOSTAGE`, guard
  worker and CI runs, so it can be pinned.
- The check of the staged file itself is the separate line `vendor:d3dx9-staged`, printed only by
  `-Smoke Vendor` runs and never part of the hardening output: a Release hardening run without
  staging would print SKIP for it where the pinned table says PASS.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -Smoke None -Hardening` with
  `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch `-OutDir`; the
  capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under phase 46 decision D-17, 2026-11-18.

### C-HARDEN-STEAMID

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:steamid-abi`, after
  `galaxy-factory-seam`; no other row moves.
- Cause: a new source check (phase 46 D-38). `steamworks.h` declares `ISteamUser` methods that
  return or take `CSteamID` by value, whose x64 hidden-return and by-value convention against the
  real `steam_api64.dll` is not settled. `steamid-abi` fails on a call of `GetSteamID`,
  `InitiateGameConnection`, `BeginAuthSession`, `EndAuthSession`, `UserHasLicenseForApp` or
  `AdvertiseGame` anywhere under `src`, and on a by-value `CSteamID` outside `steamworks.h`. No
  code does either, so the row passes.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -Smoke Vendor` (its hardening step)
  with `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch `-OutDir`;
  the capture equals the previous pinned file plus this line and the `steam-api64-pins` line, byte
  for byte.
- Status: re-pinned under phase 46 decision D-38, 2026-11-18.

### C-HARDEN-STEAMPIN

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:steam-api64-pins`, after
  `d3dx9-pins`; no other row moves.
- Cause: a new check (phase 46 D-14, D-15). x64 builds stage `steam_api64.dll` from
  `SWBF2_VENDOR_X64`, pinned as `SteamApi64Sha256` in `lib\vendor-pins.props`. `steam-api64-pins`
  reads build files only: it fails when the pin is not 64 lower-case hex digits, when `StageVendor`
  has no `Copy` of the file, or when a `Copy` has no `VerifyFileHash` of the same file against the
  pin, under the same condition, before it. It prints the same line in staged, `SWBF2_NOSTAGE`,
  guard worker and CI runs, so it can be pinned. The staged file itself is checked by the x64-only
  `steam-api64-staged` row (hash and exports), which x64 prints and no pinned table holds.
- Captured with C-HARDEN-STEAMID, from the same run and capture.
- Status: re-pinned under phase 46 decisions D-14 and D-15, 2026-11-18.

### C-CASE-CRASHDELAYLOAD

- No digest row moved. `smoke-all-digests.txt` and `offthread.txt` each gain one row for the new
  Crash case `crash-delayload`; no other row moves.
- Cause: a new smoke case. `crash-delayload` runs the standard mission on Win32 with
  `steam_api.dll` removed from the worker and `-crashtest delayload`, which calls
  `SteamAPI_RunCallbacks` past the vendor seam, so the delay-load helper raises 0xC06D007E and the
  game writes its FATAL line naming `steam_api.dll` and one dump, then exits 3. The fault comes at
  frame 1, before the first turn, so it logs no SIM DIGEST line (`none none`, the same as the
  other `-crashtest` faults) and no off-thread allocation (`0 0 0`).
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke Crash`
  (`capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -SmokeAllDir`) and
  `run.ps1 -Platform Win32 -Configuration Test -Smoke Crash`
  (`capture-vs2013.ps1 -Configuration Test -NoBuild -Tables -Baseline v143 -OffThreadDir`), each
  into a scratch `-OutDir`. Every other Crash row in both captures equals the pinned row, byte for
  byte.
  - row: smoke-all-digests crash-delayload
  - row: offthread crash-delayload
- Status: added under phase 46 decision D-06, 2026-11-18.

## Phase 47: x64 headless boot and digest spike

### C-COMBO-BLOCKS

- No digest row moved. On x86 every Combo type that `ComboArrayAdd` grows (`State`, `Transition`,
  `Condition`, `Attack`, `DamageSample`) is a multiple of 4 bytes, so its pool block equals its
  size and the template makes the same `AllocateAt`, `AllocateRun` and `Free` calls on the same
  addresses as before. This is pinned by x86 static_asserts in `tests/unit/layout_pins.cpp` and by
  the T1 case `combo pools give each combo type a block equal to its size on x86`.
- What broke on x64: `MemoryPool` rounds every block to `sizeof(void*)`, so on x64
  `Combo::Condition` (20 bytes) gets a 24-byte block and `Combo::DamageSample` (28) a 32-byte
  block. `ComboArrayAdd` took block `i` for element `i` (`AllocateAt(raArray + uiCount, sizeof(T))`),
  so the array stride (`sizeof(T)`) and the block stride differed and the next element landed
  inside a block the pool still listed as free, which corrupted the free list. The x64 Asan exe
  reported it as `unknown-crash WRITE of size 20` in `Combo::Condition::_Init` from
  `WeaponMeleeClass::ReadCombo`.
- Why the pool block cannot shrink to `sizeof(T)`: each free block holds the `void*` link of the
  free list at its start (`MemoryPool::Create`, `Allocate` growth, `Free`, `PerturbShuffleChunk`),
  so a block must be at least pointer-sized and pointer-aligned. The rounding is a real invariant
  of the pool.
- Why the element stride cannot grow to the block size: every reader indexes the arrays at
  `sizeof(T)` (`m_aCondition[i]`, `m_aState + n` and the rest in `Combo.cpp` and
  `WeaponMelee.cpp`); a block-sized stride would mean changing every reader.
- Why not `alignas(alignof(void*))` on the Combo types: none of them needs pointer alignment
  (they hold `unsigned int`, `float` and `unsigned short` fields). Raising their alignment only to
  make the type size match the allocator is padding a type to fit an allocator, a workaround
  under the Root Cause rule, and it would also change `sizeof` of every Combo type on x64.
- Fix where the invariant broke: `ComboArrayAdd` keeps a contiguous `T` array and counts the
  whole blocks it holds, `(n * sizeof(T) + block - 1) / block`. A new element that fits in the
  held blocks needs no pool call; otherwise it takes the next block right after the held ones
  (`AllocateAt`, whole block) or moves the array to a run of whole blocks (`AllocateRun`), copies
  `n * sizeof(T)` bytes and frees each old block at block stride. No Combo type changed.
- Proven by the T1 cases `combo array packs elements across pool blocks` and `combo array keeps
  packed elements across a run move` (a 6-byte type with an 8-byte block on both platforms), RED
  on the old template on Win32, and by guard rows `COMBO-STRIDE` and `COMBO-STRIDE-X64`.
- Status: fixed under phase 47 decision D-28, 2026-11-18.

### C-CASE-CRASHEARLY

- No digest row moved. `smoke-all-digests.txt` and `offthread.txt` each gain two rows for the new
  Crash cases `crash-early` and `crash-heapinit`; no other row moves.
- Cause: two new smoke cases from 47-01. `crash-early` writes through a null pointer at the top of
  `GameWinMain`, before the vendor probe, and `crash-heapinit` does the same in
  `Startup_RedInitHeap` right after the crash handler is installed. Each writes its FATAL line to
  the case's `run.log` and one dump, then exits 3. Both faults come before the first turn, so they
  log no SIM DIGEST line (`none none`, the same as the other `-crashtest` faults) and no off-thread
  allocation (`0 0 0`).
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke All` (the tables
  `run.ps1` writes beside the run) and `run.ps1 -Platform Win32 -Configuration Test -Smoke Crash`
  (`capture-vs2013.ps1 -Configuration Test -NoBuild -Tables -Baseline v143 -OffThreadDir`, scratch
  `-OutDir`). Every other row in both captures equals the pinned row, byte for byte, except the
  wall-clock `dedicated` row, which is not compared.
  - row: smoke-all-digests crash-early
  - row: smoke-all-digests crash-heapinit
  - row: offthread crash-early
  - row: offthread crash-heapinit
- Status: added in 47-04, 2026-11-18.

### C-CASE-PARTICLESOUND

- No digest row moved. `smoke-all-digests.txt` and `offthread.txt` each gain one row for the new
  Crash case `crash-particlesound`; no other row moves.
- Cause: a new smoke case for the particle emitter ambient-sound fix in 47-04. On the standard
  mission, `-crashtest particlesound` finds the first loaded emitter class whose ambient sound
  plays, builds an emitter with one info block, updates it twice (the second update points the
  sound at the block), deletes it, then builds another and resets it. It logs
  `OPENSWBF2 PARTICLESOUND ... voice_after_reset=0`. Building the emitters draws from the global
  random stream, so the case's digest differs from `geo1c_con` and is pinned as its own row. The
  test runs on the main thread, so the off-thread row is `0 0 0`.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke Crash`
  (`capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -SmokeAllDir`) and
  `run.ps1 -Platform Win32 -Configuration Test -NoBuild -Smoke Crash`
  (`capture-vs2013.ps1 -Configuration Test -NoBuild -Tables -Baseline v143 -OffThreadDir`), each
  into a scratch `-OutDir`. Release and Test log the same SIM DIGEST line for the case. Every
  other Crash row in both captures equals the pinned row, byte for byte.
  - row: smoke-all-digests crash-particlesound
  - row: offthread crash-particlesound
- Status: added in 47-04, 2026-11-19.

### C-CASE-PARTICLEBURST

- No digest row moved. `smoke-all-digests.txt` and `offthread.txt` each gain one row for the new
  Crash case `crash-particleburst`; no other row moves.
- Cause: a new smoke case for the guarded burst-loop line in 47-04's particle emitter fix
  (`MoveAmbientSound(mAmbientSound, pNext);` in `ParticleEmitter::Update`). That line's only
  carrier was a natural read in x64 `tat2g_eli`, which went away when Phase 48 gave x64 the x86
  sim (48-25 branch gate). On the standard mission, `-crashtest particleburst` finds the first
  loaded emitter class whose ambient sound plays, builds an emitter with one info block and
  updates it twice so the sound points at that block, links a second block the way
  `ParticleEmitterObject::RegisterStep` does, then calls `Update` once past the first burst so
  the burst loop releases the first block. It logs
  `OPENSWBF2 PARTICLEBURST ... playing=1 released=1 voice_on_next=1`, where `voice_on_next`
  compares the voice position pointer with the second block. Building the emitter draws from the
  global random stream, so the case's digest differs from `geo1c_con` and is pinned as its own
  row. The test runs on the main thread, so the off-thread row is `0 0 0`.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke Crash`
  (`capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -SmokeAllDir`) and
  `run.ps1 -Platform Win32 -Configuration Test -Smoke Crash`
  (`capture-vs2013.ps1 -Configuration Test -NoBuild -Tables -Baseline v143 -OffThreadDir`), each
  into a scratch `-OutDir`. Release and Test log the same SIM DIGEST line for the case. Every
  other Crash row in both captures equals the pinned row, byte for byte.
  - row: smoke-all-digests crash-particleburst
  - row: offthread crash-particleburst
- Status: added in 48-26, 2026-11-20.

### C-HEAPCAP-X64-RULE

- No digest row moved. No x86 heap size, x86 cap or x86 table changed; every x86 size in
  `GameMemory.h` and the x86 arena keep their old value through `sizeof(void*) == 8 ? x64 : x86`.
- Rule for every x64 default heap cap (phase 47 decisions D-17, D-18, D-23, D-24): for a heap
  whose largest peak is above 0 in both `mem-peaks.txt` (x86) and `mem-peaks-x64.txt` (x64),
  `cap_x64 = ceil(peak_x64 * cap_x86 / peak_x86)`, rounded up to the next multiple of 65536.
  Each peak is that heap's largest `peak` over every mission and both sources (`turn1500`,
  `exit`) of its file; `cap_x86` is the x86 cap MEMREPORT logs. The x64 cap keeps the x86
  headroom ratio over the measured peak; nothing is doubled. A heap whose computed cap is not above
  its x86 cap keeps the x86 size.
- The cap MEMREPORT logs is the heap range `RedMiCreate` returns: start rounded up and end
  rounded down to the heap alignment. On both platforms Application and Gamespy lose 16 bytes to
  that rounding (`0xF40000` logs 15990768) and Lua loses none. The x64 sizes add the same 16
  bytes, so the logged x64 cap equals the derived cap exactly.
- Runtime is the rest of the arena, so its cap is set through `kArenaSize`. The x64 arena is the
  x86 layout with Application, Lua and Runtime at their x64 sizes and the x86 fixed overhead kept
  (the heap table, the 0x40 start alignment of Lua, Artist and Runtime); the 16 bytes added to
  Application fall inside the Lua start alignment. Result: `kArenaSize` 0x5B42160 (95691104).
- `-heapcap` and Lua `SetHeapCap` are unchanged. The x64 defaults are far below `kHeapCapMax`
  (0x20000000) each and `kHeapCapTotalMax` (0xC0000000) together.
- Captured: `run.ps1 -Platform x64 -Configuration Release -NoBuild -Smoke All -Soak Short
  -HogAddrSpace` on commit 322e6c84 after the 47-03 Weapon and Ordnance pool fix, run folder
  `artifacts\tests\20261119-034808`; its `tables\mem-peaks.txt` is pinned unchanged as
  `mem-peaks-x64.txt`. The x64 heaps in that run still had the x86 sizes, so every logged x64 cap
  equals the x86 cap. The run had no crash and no stall; its only failures are the six missions
  with an x86 digest row (dag1c_ctf, geo1c_con, hot1g_con, kas2c_uber, nab2g_c, spa3c_ass),
  because x64 simulation differs from x86 until Phase 48.
- `mem-peaks-x64.txt` and `mem-peaks.txt` have the same missions and the same heap rows. Three
  pool rows differ (x86 `dea1g_con exit pool Aimer` and `MountedTurret`, `kas2c_uber exit pool
  RayRequest`; x64 `dea1g_con turn1500 pool Aimer` and `kas2c_uber turn1500 pool RayRequest`): a
  pool gets a row once it has grown, and on x64 the simulation of those two missions grows them at
  another time (x64 dea1g_con builds its Aimers before turn 1500 and never a MountedTurret).
  `compare-tables.ps1 -MemPeaksX64` checks the missions and heap rows and counts the pool rows.
- Status: added in 47-09, 2026-11-19.

### C-HEAPCAP-X64-Application

- No digest row moved. x64 `kApplicationHeapSize` 0x1560010 (logged cap 22413312); x86 stays
  0xF40000 (logged cap 15990768).
- Peaks: x86 125040 (cor1c_c, turn1500), x64 175104 (cor1c_c, turn1500); x64/x86 1.400. x86
  cap/peak 127.885. ceil(175104 * 15990768 / 125040) = 22393214, rounded to 22413312.
- Cause: the measured x64 peak is above the x86 peak; the cap follows C-HEAPCAP-X64-RULE.
- Status: added in 47-09, 2026-11-19.

### C-HEAPCAP-X64-Lua

- No digest row moved. x64 `kLuaHeapSize` 0x1BE0000 (logged cap 29229056); x86 stays 0x1000000
  (16777216).
- Peaks: x86 2617536 (kam1c_c, turn1500), x64 4553172 (geo1c_c, turn1500); x64/x86 1.739. x86
  cap/peak 6.410. ceil(4553172 * 16777216 / 2617536) = 29183763, rounded to 29229056.
- Cause: the measured x64 peak is above the x86 peak; the cap follows C-HEAPCAP-X64-RULE.
- Status: added in 47-09, 2026-11-19. Superseded in 49-13 by C-HEAPCAP-X64-Lua-RETUNE in
  `tests\baseline\v143-x64\CAUSES.md` (0x1570000, `kArenaSize` 0x54D2160).

### C-HEAPCAP-X64-Runtime

- No digest row moved. x64 Runtime logged cap 41943040, set through `kArenaSize` 0x5B42160; x86
  stays at 32235168 from the 0x4000000 arena.
- Peaks: x86 19346544 (hot1g_uber, turn1500), x64 25157728 (kas2c_uber, exit); x64/x86 1.300.
  x86 cap/peak 1.666. ceil(25157728 * 32235168 / 19346544) = 41917750, rounded to 41943040.
- Cause: the measured x64 peak is above the x86 peak; the cap follows C-HEAPCAP-X64-RULE. The
  peak includes the x64 pool blocks after the 47-03 fix (Weapon 592, Ordnance 560 bytes).
- Status: added in 47-09, 2026-11-19. The cap is unchanged; since 49-13 it is set through
  `kArenaSize` 0x54D2160 (C-HEAPCAP-X64-Lua-RETUNE in `tests\baseline\v143-x64\CAUSES.md`).

### C-HEAPCAP-X64-KEPT

- No digest row moved. These heaps keep their x86 size on x64.
- Gamespy (logged cap 2097136), Artist (8192) and HUDEditHeap (1048560): peak 0 on every mission
  on both platforms, so no ratio exists (D-24).
- Debug (3145728): peak 1048592 on both platforms (one 1 MB block), ratio 1.000; the rule gives
  3145728, not above the x86 cap.
- The Temp load heap (0x1FFF00 inside a 0x200000 Runtime block) has no MEMREPORT row and is not
  covered by the rule; its block is part of the Runtime peak.
- Status: added in 47-09, 2026-11-19.

### C-X64-FMA3

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed.
- Change (phase 48 D-11, PAR-03): the x64 exe calls `_set_FMA3_enable(0)` in `PcMathStartup`,
  which `Startup_RedInitHeap` calls right after `pcCrash::Install`, before any game code runs. The
  call is in `pcMathStartupX64.cpp`, compiled only for x64; Win32 compiles `pcMathStartup.cpp`, an
  empty `PcMathStartup`. Both are chosen by a platform `Condition` on the `ClCompile` item, with no
  platform macro (D-02). The tests exe calls `PcMathStartup` first in `main`, so its CRT runs as the
  game's does.
- Cause: x64 UCRT math picks an FMA3 code path at run time when the CPU has FMA3, so the same x64
  exe could compute other bits on another x64 CPU. x86 has no FMA3 CRT path (SysWOW64 ucrtbase
  exports only `_set_SSE2_enable`) and does not change. Phase 48 research measured FMA3 off as
  neutral for the sim's float-in, double-routine, float-out forms; it is reproducibility across x64
  CPUs, not a parity fix.
- `_TheSinCosTable` (`init_seg(lib)`) is built before `Startup_RedInitHeap`. Research measured its
  bits equal on x86 and on x64 with FMA3 on and off; the T1 `sin cos table bits equal x86` pins its
  4096 floats (FNV-1a 32 over their bytes in table order) on both platforms.
- The x64 T1 `x64 start-up turns FMA3 off` checks `_get_FMA3_enable() == 0` and prints the CPU's
  FMA3 bit as information. On a CPU without FMA3 the state is 0 whatever the call does, so this T1
  is not a guard. The guards are the x64 hardening line `fma3-off`, judged on the x64 Release exe
  only (it needs `Startup_RedInitHeap` to call or jump to `PcMathStartup` and `PcMathStartup` to call
  or jump to `_set_FMA3_enable` with `ecx` last set to 0, read from `dumpbin /disasm` over the two
  functions' map ranges with PDB names), and the x64 guard rows FMA3-SET-X64 (argument 1) and
  FMA3-CALL-X64 (call removed). Their verdict reads the exe, never the CPU. No Win32 guard row: the
  Win32 `PcMathStartup` is empty, so there is no x86 behaviour to revert.
- Status: Fixed in 48-01, 2026-11-19.

### C-X64-D3DX

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. The Win32 game
  compiles the same TUs and imports the same 15 d3dx9 math functions as before.
- Cause: the x86 `d3dx9_43.dll` picks its math code path at run time from `cpuid` (a dispatch table
  at `0x101CA000`); on this CPU it runs SSE paths with `rsqrtss`/`rcpps` and its own summation
  orders, and generic x87 paths for RotationQuaternion and Scaling. The x64 DLL computes plain
  float math. Phase 48 research measured different output words on 11 of the 13 math functions
  the sim imports (131,072 random inputs each), the main source of x64 digest drift.
- Change (phase 48 D-15, approved by the user on 2026-11-19): the x64 game links an in-tree port of
  the x86 DLL's code paths, `PcD3dx::*` in `src/PebbleFL/pc/pcD3dxPort.cpp`, and
  `pcD3dxLinkX64.cpp` defines the `D3DX*` exports with their `WINAPI` signatures, forwarding to
  the port. Both TUs are x64-only `ClCompile` items chosen by a platform `Condition` in
  `BattlefrontII.vcxproj` (D-02); the linker resolves the calls to them before it searches
  `d3dx9.lib`, so the x64 exe no longer imports them. x86 keeps calling the real DLL. The
  definitions sit in an `extern "C"` block after `<d3dx9math.h>`, so a signature that drifts from
  the SDK declaration is error C2733 instead of a silent C++ overload (checked once by changing a
  parameter type). That is why the `d3dx-decl` hardening check, which bans hand-written D3DX
  declarations so the SDK header stays the only one, skips this one file; every other file under
  `src` is still scanned and the `hardening.txt` row does not change.
- Fixed here: MatrixMultiply (rows 0 and 3 `(t0+t1)+(t2+t3)`, rows 1 and 2 `((t0+t1)+t2)+t3`,
  row 3 products with the B row first), Vec3TransformCoord (`rcpps` plus one Newton step on w),
  Vec3TransformNormal, Vec3Normalize (`rsqrtss` plus Newton, zero below `0x28800000`),
  QuaternionNormalize (`rsqrtss` plus Newton), MatrixRotationQuaternion (x87: float products,
  `(1-a)-b` at 53 bits written as `double`) and MatrixScaling (x87 load and store) in 48-03;
  Vec4Transform (`(w*m3+z*m2)+(y*m1+x*m0)` with the vector first in every product, on both the
  aligned and the unaligned path) and MatrixInverse (the DLL's SSE cofactor sequence transcribed
  register by register, the determinant from the `addss` of the last shuffle, `NULL` returned and
  nothing written when the determinant compares equal to zero or is unordered, the determinant
  stored before the matrix, and `rcpss` plus one Newton step `2r-d*r*r` for the scale) in 48-04.
  The port keeps
  every input load before the first store, the DLL's aligned and unaligned operand orders in the
  two transforms (chosen by the matrix address, as the DLL does), the SSE rule that a NaN pair
  returns the first operand (written explicitly, so the compiler's operand order cannot change it),
  the x87 rule that a NaN pair returns the larger significand (and the positive one on a tie), and
  the x87 quieting of a signalling NaN on load.
- Proof: on Win32 the T1 `d3dx port equals the x86 d3dx9 dll` first checks that the loaded
  `d3dx9_43.dll` sits next to the tests exe and has the `D3dxWin32Sha256` pin from
  `lib/vendor-pins.props` (the tests project stages the pinned DLL the way `StageVendor` does),
  then compares the port with the DLL word for word over 262,144 inputs per function from
  `tests/unit/d3dx_corpus.cpp` (raw bit patterns over the whole float range, ±0, denormals,
  ±FLT_MAX, ±inf, quiet and signalling NaNs, identity and near-singular matrices, exactly
  singular matrices (a zero row, a repeated row, a column twice another) for MatrixInverse with and
  without a determinant pointer, zero and
  near-threshold vectors, aliased outputs, aligned and unaligned matrices). The results of
  `rsqrtss`, `rcpps` and the x87 transcendentals are CPU-specific, so this T1 compares with the DLL
  on the same machine instead of with a file. The five functions with no approximation
  instruction (MatrixMultiply, Vec3TransformNormal, MatrixRotationQuaternion, MatrixScaling,
  Vec4Transform) have
  the FNV-1a 32 of the DLL's outputs pinned in `tests/corpus/d3dx/pins.txt` (written by
  `--gen-corpus` from the DLL on Win32), and the T1 `d3dx port matches pinned x86 bits` checks the
  port against them on both platforms. Guard rows D3DX-MUL (Win32) and D3DX-MUL-X64 revert the
  row 1 and 2 summation order; D3DX-NORM (Win32) replaces the Newton step with a plain square root
  divide; D3DX-INV (Win32) replaces the MatrixInverse `rcpss` and Newton step with a plain divide.
- RotationX/Y/Z (48-06): the x86 DLL runs them as short x87 routines on every argument (`fld`,
  `fsincos`, the cosine and sine stored as float, then `fld1`/`fldz`/`fchs` and float stores in a
  fixed order). x87 `fsincos` reduces with a 66-bit pi below 2^63; at and above 2^63, and for NaN
  and Inf, it leaves the operand on the stack with C2 set, so the DLL stores the angle as the cosine
  and the x87 indefinite `0xFFC00000` from the empty-stack pop as the sine. No SSE or CRT code gives
  those bits. By the user's decision D-19 (2026-11-19) under the approval D-15, the x64 game runs
  ml64 transcriptions of the three routines, `PcD3dxMatrixRotationX/Y/Z` in
  `src/PebbleFL/pc/pcD3dxX87X64.asm`: every x87 instruction, operand width and order as in the
  DLL, the stack arguments replaced by `rcx` and `xmm1` spilled to the caller's shadow space, the
  matrix pointer returned in `rax`. Each runs its x87 part under control word 0x27F, the CW of
  headless x86, and restores the caller's CW when it differed; rendered x86 runs the DLL under PC24
  after `CreateDevice`, which this does not reproduce (D-18). The asm is a `MASM` item chosen for
  x64 by a platform `Condition` in `BattlefrontII.vcxproj` and the tests project (D-02), built by
  the VS `masm` build customization (`masm.props` in `OpenSWBF2.Game.props`, `masm.targets` in
  the projects); `pcD3dxLinkX64.cpp` defines `D3DXMatrixRotationX/Y/Z` and forwards to the procs,
  so the x64 exe no longer imports them. The Win32 build has no `MASM` item and runs no assembler.
  Proof: the three are x64-port-only entries of `tests/unit/d3dx_corpus.cpp`, 262,144 angles each
  (raw bit patterns over the whole float range, the special values above, both sides of ±2^63,
  multiples of pi/2 up to 2^24 with nearby floats, 94.25, and exponents from 2^16 to FLT_MAX),
  huge and ordinary angles interleaved so a leaked x87 stack or CW shows in the next result; the
  output includes the returned pointer. The x87 results are CPU-specific, so on x64 they are
  compared word for word with the DLL on the same machine through the Win32 child and never
  pinned; the Win32 run prints them as `[N/A]`. Guard rows D3DX-ROTX-X64 (sine and cosine stores
  exchanged) and D3DX-X87-HUGE-X64 (the angle reduced with `fprem1` by 2pi when `fsincos` sets C2,
  which changes only the |x| >= 2^63 results) go RED by bit mismatch; MASM-COND (Win32) replaces
  the item `Condition` with `ExcludedFromBuild` and fails `game MASM items are x64 only by item
  condition`.
- RotationAxis (48-07): the x86 DLL runs it as a generic x87 routine on every argument. It takes
  `fsincos` of the angle, stores the cosine and sine as float, keeps `1 - cos` as float, copies the
  axis and normalizes it in place through `D3DXVec3Normalize` (the SSE `rsqrtss` path on this CPU),
  then forms the matrix from 53-bit x87 chains of float operands that keep products and sums
  unrounded between stores. No C++ expression on x64 holds those intermediates in the same
  registers and order, and `fsincos` gives the huge-argument bits described above. By D-19 under
  D-15, the x64 game runs `PcD3dxMatrixRotationAxis` in `src/PebbleFL/pc/pcD3dxX87X64.asm`: every
  x87 instruction, operand width and order as in the DLL, the DLL's frame slots mapped to an 80-byte
  frame with unwind data (`PROC FRAME`, `rbx` saved), `xmm2` spilled to the caller's shadow space,
  the matrix pointer kept in `rbx` and returned in `rax`, and the normalize call made to
  `PcD3dxVec3Normalize`, the `extern "C"` entry of the 48-03 port of the same DLL path. The CW
  handling is the one of RotationX/Y/Z; the call runs under that CW, and the port's SSE code does
  not read it. The only encoding difference is `fmul st(0), st`, which ml64 encodes as `D8 C8`
  where the DLL has `DC C8`; both multiply ST(0) by itself into ST(0). `pcD3dxLinkX64.cpp` defines
  `D3DXMatrixRotationAxis` and forwards to the proc. Proof: the x64-port-only corpus entry
  `MatrixRotationAxis` (layout `xa`, two variants, the second with the axis inside the output
  matrix) compares 262,144 cases with the DLL through the Win32 child: raw bit patterns and the
  special values for all four words, and structured axes (unit, non-unit, signed zero, denormal,
  huge, one NaN or Inf component) crossed with the structured angles above. Guard row
  D3DX-X87-X64 turns one `fsub` of the chain into `fsubr` and goes RED by bit mismatch.
- Import check: the x64-only hardening row `d3dx-imports` (`Get-D3dxImportsCheck` in
  `tests/smoke.ps1`, run from `Get-ImportChecks`) reads `dumpbin /imports` of the x64 game exe and
  passes only if the `d3dx9_43.dll` import set is exactly `D3DXCheckTextureRequirements` and
  `D3DXLoadSurfaceFromSurface`, the two render functions, naming any extra or missing import. Guard
  row D3DX-LINK-X64 sets the `pcD3dxLinkX64.cpp` item `Condition` to false, so the math imports
  come back, and the row goes RED. The x64 Smoke row D3DX-LINK-X64S (48-12) makes the same edit
  and goes RED because the x64 `geo1c_con` digest at turn 1500 no longer equals its row. The Win32
  import rows and `hardening.txt` do not change.
- Status: all 13 functions Fixed, seven in 48-03, MatrixInverse and Vec4Transform in 48-04 (D-15),
  RotationX/Y/Z in 48-06 and RotationAxis in 48-07 (D-15, D-19), all 2026-11-19. The x64
  comparison with the x86 DLL for the approximation and x87 functions runs through the Win32 child
  from 48-05.

### C-X64-X87TRIG

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. No game TU includes
  the new header yet, so no x86 or x64 game object changed except the new x64 `MASM` object.
- Cause: x86 `ucrtbase.dll` 10.0.26100.9711 (SysWOW64, image base 0x10000000) runs
  `_libm_sse2_sin_precise` (0x10093A60), `_libm_sse2_cos_precise` (0x100905D0),
  `_libm_sse2_tan_precise` (0x100941E0) and plain `sin` (0x10093740) and `cos` (0x100902C0) through
  a state gate (MXCSR `& 7F80h == 1F80h` and x87 CW `& 7Fh == 7Fh`, else x87) and a top-16-bit
  test: sin and cos with |x| >= 90112.0 (`sub ax,3030h / cmp ax,10C5h / ja / jg`), tan with
  |x| >= 12800.0 (`sub ax,3820h / cmp ax,8A8h`), NaN and Inf go to x87. The x87 path loads CW 0x27F
  unless it is already 0x27F, runs `fsin`, `fcos` or `fptan` (popping the 1.0), and when C2 is set
  reduces with `fprem1` against the 80-bit constant at 0x10008DCA (pi x 2^62, exponent 0x403E, not
  2pi as 48-02 wrote) and runs the instruction again. Inf gives the x87 indefinite through the CRT
  error handler; a NaN is loaded as an 80-bit value with its payload and returned without x87
  arithmetic. Below 0x3030 (0x3820 for tan) in the top 16 bits a short SSE tiny-argument path runs.
  The error handler sets `errno` to EDOM (33) for +-inf at every caller CW; a NaN, and a finite
  argument on the x87 path at CW 0x27F, 0x7F or 0x37F, leave `errno` unchanged (measured in 48-19
  with the x86 CRT, `artifacts\gate48\p19\errno.cpp`).
  The x64 UCRT reduces huge arguments in SSE and gives other bits from 90112 up (the first float
  above the threshold already differs), and 48-02 showed that mod data (ODF, `.lvl` animation keys,
  Lua) reaches these arguments.
- Change (by the user's decision D-19, 2026-11-19): the x64 build runs an ml64 transcription of the
  three `_precise` routines, `PcCrtSinX64`, `PcCrtCosX64` and `PcCrtTanX64` in
  `src/PebbleFL/pc/pcCrtTrigX64.asm`: the same gate, thresholds and branches, the tiny paths with
  the constants copied from ucrtbase, and the x87 paths instruction for instruction with the
  reduction constant and the indefinite copied byte for byte. Where x86 continues in its in-range
  SSE2 polynomial the procs tail-jump to the x64 UCRT `sin`, `cos` or `tan` through their `__imp_`
  pointers; they define no CRT name. They are leaf procs that use only the caller's shadow space,
  leave the x87 stack empty and restore the caller's CW. When the caller's CW is not 0x27F and
  its precision exception is masked and raised, x86 stores the result as a double and reloads it,
  and the transcription does the same; x86 sets no `errno` there. For +-inf the procs tail-jump to
  `PcCrtTrigDomain`, a framed proc (unwind data, 16-byte aligned call) that sets `errno` to EDOM
  through the x64 UCRT `_errno` and returns the indefinite unchanged, as the x86 handler does
  (48-19). The asm is a `MASM` item chosen for x64 by a platform
  `Condition` in `BattlefrontII.vcxproj` and the tests project (D-02). `src/PebbleFL/Common/
  PblCrtTrig.h` gives `PblCrtSin`, `PblCrtCos` and `PblCrtTan`: `if constexpr (sizeof(void*) == 8)`
  calls the proc, else `sin`, `cos` or `tan`, so x86 compiles the same CRT call (D-02). x86 code
  does not change.
- Proof: the T1 `crt trig fallback equals the x86 crt` (`tests/unit/crt_trig.cpp`) compares
  `PblCrtSin`, `PblCrtCos` and `PblCrtTan` with the x86 CRT over about 183,000 inputs per function:
  every float and double within 64 ulps of ±90112 (±12800 for tan), of the tiny boundary and of
  ±2^63; 512 random floats per binade and sign from the threshold binade to FLT_MAX; 16 random
  doubles per binade and sign from 2^63 to DBL_MAX (several `fprem1` steps); ±0, denormals, ±inf,
  ±FLT_MAX, ±DBL_MAX, quiet and signalling NaNs of both signs with payloads in either word; the
  2048 SinCosTable arguments and `15.0`; 32,768 random floats below the threshold; 1,024 tiny
  doubles. Inputs x86 sends to x87 or to the tiny path are compared as double bits; in-range inputs
  as the float the sim keeps, `(float)f(x)`. The reference is two routes: the `_precise` routines
  (`tests/unit/crt_trig_ref_win32.cpp`, compiled `/O2 /Oi- /fp:precise` as the game, calls
  `__libm_sse2_{sin,cos,tan}_precise`) and plain `sin`/`cos` (`crt_trig_ref_strict_win32.cpp`,
  `/fp:strict` as `WeaponCannon.cpp`, calls `_sin`/`_cos`). x87 results are CPU-specific, so the
  T1 compares with the x86 CRT on the same machine, never with a file: on x64 through the Win32
  child `--crt-trig-ref=<file>` built from the same tree, on Win32 in process, where it also checks
  that the child's file equals the in-process results and that the two routes agree on every x87
  input. It prints the ucrtbase file version. A Windows update that changes SysWOW64 ucrtbase
  changes x86 itself; the T1 then fails until the transcription is redone. Guard rows
  X87TRIG-X64 (the sin top-16 test sends every argument to the x64 UCRT, the x64 behaviour before
  D-19) and X87TRIG-RED-X64 (the sin `fprem1` reduction dropped, which changes only |x| >= 2^63)
  go RED by bit mismatch. The T1 `crt trig sets errno like the x86 crt` (`tests/unit/crt_trig.cpp`)
  pins `errno` after `PblCrtSin`, `PblCrtCos`, `PblCrtTan` and their float forms on both platforms:
  EDOM for +-inf, unchanged for NaNs, huge, in-range, tiny and zero arguments; on Win32 it runs the
  x86 CRT itself. Row X87TRIG-ERRNO-X64 (the `PcCrtTrigDomain` jump replaced by a plain return, the
  48-08 behaviour) turns it RED on x64.
- Status: helper Fixed in 48-08, 2026-11-19; `errno` for +-inf Fixed in 48-19, 2026-11-20; the sim call sites are routed through `PblCrtTrig.h`
  in 48-09 and 48-10 (C-X64-CRTF).

### C-X64-CRTF

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Cause: on x86 `<corecrt_math.h>` defines every float CRT routine inline as the double routine
  rounded to float (`sinf(x)` is `(float)sin(x)`), and the C++ float overloads in `<cmath>` call
  those inlines, so x86 runs `__libm_sse2_{sin,cos,tan,asin,acos,atan,exp,log,log10,pow}_precise`,
  `_CIatan2` or `_CIfmod` and rounds the result. On x64 the same source calls the UCRT float
  routines `sinf`, `cosf`, `tanf`, `asinf`, `acosf`, `atanf`, `atan2f`, `expf`, `logf`, `log10f`,
  `powf` and `fmodf`, which use other algorithms: over 1,048,576 inputs per function the x86 and
  x64 float results differ in sin 908, cos 915, tan 174, asin 33,439, acos 10,240, atan 2, exp 72,
  log 124, log10 828 and pow 78 words (48-RESEARCH). x64 `(float)f((double)x)` gave 0 differences
  for all of them, with FMA3 on or off. One case is not covered by the route: fmod with an
  invalid operation (x = +-inf or y = +-0) gives the x87 indefinite 0xFFC00000 on x86 and
  0x7FC00000 on x64, and two NaN operands follow the x87 NaN rule on x86; Lua `math.mod`
  reaches these, and 48-11 adds those rules to `PblCrtFmodF` (C-X64-FMOD).
- Change: the game writes the x86 form. `src/PebbleFL/Common/PblCrtTrig.h` gives float-parameter
  helpers `PblCrtSinF`, `CosF`, `TanF`, `AsinF`, `AcosF`, `AtanF`, `Atan2F`, `ExpF`, `LogF`,
  `Log10F`, `PowF` and `FmodF`, each shaped like the x86 inline (`float f(float x) { return
  (float)g((double)x); }`), and every float-overload call in game code uses them: in 48-09 the sim
  TUs and `PblAngle.h`, in 48-10 the render, camera, HUD and sound TUs, `pcColorRamp.cpp`,
  `RedScreenShot.cpp` and Lua `lmathlib.cpp` (`lua_Number` is float, so `math.sin` and `^` called
  the float overloads). The trig helpers choose the x64 route in themselves with
  `if constexpr (sizeof(void*) == 8)` (D-02). The argument domain is the whole float range: sin,
  cos and tan, float or double argument, go through `PblCrtTrig.h`, which on x64 runs the ml64
  copy of the x86 fallback at and above its threshold (C-X64-X87TRIG, D-19), and mod data reaches
  that range (48-02 Reach). So the direct double-argument calls are routed too: `RepairHelper`
  `cos(15.0)`, the SinCosTable initializer in `PblMath.cpp`, `ParticleGeometry.cpp` and
  `pcHeatShimmerEffect.cpp` use `PblCrtSin`, `PblCrtCos` and `PblCrtTan`.
- Proof that x86 does not change: the `dumpbin /disasm` of every x86 Release object equals the
  one before the change, except `NetGame.obj` and `NetSetup.obj` (the two `/Od` TUs, where a new
  declaration in an included header renumbers the compiler's string-literal labels `$SG`; no
  instruction changes) and `ParticleGeometry.obj` (its three jump-table labels `$LN` are numbered
  4 higher because one more inline function is expanded; no instruction changes, and the linked
  x86 exe built with and without that edit has the same `.text` and `.data` bytes).
- Guards: because x86 code does not change there is no Win32 row and no call-site T1. The x64
  hardening row `crt-float-imports` fails if the x64 game imports any of the twelve float routines;
  `crt-trig-route` fails if any x64 game object other than `pcCrtTrigX64.obj` references `sin`,
  `cos` or `tan` (from the objects in the link tlog, `dumpbin /symbols`). Guard rows CRTF-X64 (one
  EntityDroideka site back to `cosf`) and CRTT-X64 (one EntityDroideka site to
  `(float)sin((double)x)`) turn them RED. The x64 Smoke row CRTF-X64S (48-12; the EntitySoldier
  in-air drag `PblCrtExpF` back to `expf`, which moves the x64 `geo1c_con` digest at turn 1500)
  and the `sim math corpus` T1 (48-11) complete the proof.
- Corpus (48-11): the T1 `sim math corpus` checks every sim CRT form in the form the sim calls
  it (float in, double routine, float out; sin, cos and tan through `PblCrtTrig.h`, also on the
  plain `/fp:strict` route of `WeaponCannon`) over the whole float range: per function 262,144
  fixed-seed raw float patterns (pairs for atan2, pow and fmod), 64 values in every binade of
  both signs (every exponent pair for the pair functions), the specials (+-0, denormals,
  +-FLT_MAX, +-inf, quiet and signalling NaNs with payloads) and +-64 ulps around each
  function's domain boundaries, the trig thresholds 90112.0 and 12800.0 included. Inputs that
  x86 computes without an x87 transcendental are pinned in `tests/corpus/simmath/crt.txt` (one
  line per function and block of 65,536 inputs: name, block, count, FNV-1a 32 of the result
  bits), generated from the raw x86 CRT by the Win32 tests exe. The sin, cos and tan inputs on
  the x87 fallback path, every atan2 input (`_CIatan2` always runs x87 `fpatan`) and the fmod
  inputs of two NaNs with equal significands (C-X64-FMOD) are CPU-specific, so they are compared
  with the x86 CRT on the same machine (Win32 in-process,
  x64 through the `--crt-trig-ref` child) and never pinned (D-19). It passes on Win32 and x64.
  `tests/corpus/mathfn/math.txt` stays a Win32-only toolchain corpus by the user's decision
  D-16: `T1NotForPlatform` in `tests/smoke.ps1` lists it and `math corpus x87 pc24` as not for
  x64, each with its reason.
- Status: Fixed, 48-10, 2026-11-19.

### C-X64-FMOD

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Cause: x86 `_CIfmod` (SysWOW64 `ucrtbase.dll` 10.0.26100.9711, 0x1008DC00) is
  `mov edx,100014B0h / jmp 1008D650`, the x87 intrinsic dispatcher. It sets the x87 control word
  0x133F, classifies both operands with `fxam` (class table at 0x1000867E: valid, zero, NaN,
  infinity) and jumps through the fmod table at 0x100014C0, index `class(y) + 4 * class(x)`:
  - valid or zero x with valid y: 0x1008DC10, the `fprem` loop (exact) or the signed zero;
  - x valid or zero with y infinite: 0x1008DAB0, which returns x;
  - y zero or x infinite, no NaN operand (the invalid operation): 0x1008DBA0, which loads the
    real indefinite (tbyte at 0x10008650, sign set, significand 0xC000000000000000) and reports
    `_DOMAIN` through 0x1006050B; with invalid masked (the game's control word 0x27F), the
    exception handler at 0x10060658 leaves the result, so the caller's `fstp dword` stores
    0xFFC00000;
  - one NaN operand: 0x1008DB30 (y) or 0x1008DB20 (x, after `fxch`), `faddp` of the NaN and the
    other operand, which returns that NaN quieted with its sign and payload;
  - two NaN operands: 0x1008DB60, `faddp` of both, which follows the x87 rule: the NaN with the
    larger significand; with equal significands the positive one unless both are negative
    (measured on this machine: `7FC00001,FFC00001` and `FFC00001,7FC00001` both give
    0x7FC00001).

  x64 `fmod` returns 0x7FC00000 for the invalid operation and the second operand for two NaNs.
  The sim widens floats with `cvtss2sd`/`cvtps2pd`, so a signalling float NaN arrives quiet.
- Change: on x64 `PblCrtFmodF` (`src/PebbleFL/Common/PblCrtTrig.h`) returns
  `PblCrtX87NanPair(x, y)` when an operand is a NaN, otherwise `fmod` rounded to float, and
  0xFFC00000 when that result is a NaN. On x86 it stays `(float)fmod((double)x, (double)y)`,
  chosen by `if constexpr (sizeof(void*) == 8)` (D-02).
- Reach: Lua `math.mod` (a mod script can pass y = 0, inf or NaN; 48-02). `SpaceAssault::
  GetRandomCritSys` passes y = 1 and never reaches it. The other users (GameLoop,
  pcWeatherEffect, HUDElementMap, HUDElementTarget) are render or HUD only and now match x86 too.
- Proof that x86 does not change: the x86 Release objects of lmathlib, SpaceAssault, GameLoop,
  pcWeatherEffect, HUDElementMap and HUDElementTarget have the same `dumpbin /disasm` as before;
  over all 1068 objects only `NetGame.obj` and `NetSetup.obj` (the `/Od` TUs) differ, in their
  `$SG` string-literal numbers and, in NetSetup, one byte of `.rdata` padding from the new
  string order (same strings).
- Tie inputs: the Intel SDM rule for two NaN operands on x87 is the larger significand and does
  not define equal significands, so the tie above is this CPU's behaviour, not an architectural
  rule. The 120 fmod inputs of two NaNs with equal significands (after quieting) are therefore
  compared with the x86 CRT on the same machine (Win32 in-process, x64 through the
  `--crt-trig-ref` child) and are not in `crt.txt` (48-19). On x64 that comparison also checks
  `PblCrtX87NanPair`'s tie against this CPU's `faddp`, the rule the `PblX87*` helpers share.
- Guards: T1 `sim math corpus` (its fmod pins and its same-machine fmod results come from the raw
  x86 CRT, never through the helper). Rows FMOD-NAN-X64 (the invalid-operation rule back to the plain rounded result) and
  FMOD-NANOP-X64 (the two-NaN rule back to the first operand) turn it RED on x64. No Win32 row:
  x86 code does not change.
- Status: Fixed, 48-11, 2026-11-20.

### C-X64-NANOP

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change (same proof as C-X64-FMOD).
- Cause, atan2: x86 `_CIatan2` (0x1008D950) is `mov edx,10001560h / jmp 1008D650`, the same
  dispatcher as `_CIfmod`; its table at 0x10001570 sends NaN operands to the same entries
  0x1008DB20, 0x1008DB30 and 0x1008DB60, so x86 atan2 follows the one-NaN and two-NaN rules of
  C-X64-FMOD. x64 `atan2` returns the second operand for two NaNs. The 48-11 corpus found 265
  inputs.
- Cause, pow: x86 `__libm_sse2_pow_precise` returns a NaN x quieted with its sign for every
  y other than +-0, and 1 for y = +-0. x64 `pow` clears the sign of a negative NaN x when y is
  an odd integer. Measured with the raw x86 and x64 CRTs over 4,781,280 NaN-x pairs and
  4,781,280 NaN-y pairs: NaN y with a non-NaN x and two NaNs already agree; NaN x with an odd
  integer y is the only difference. This rule is measured, not read from the disassembly; the
  corpus pins the x86 results, so another difference shows as a failing Win32-pinned block.
- Change: on x64 `PblCrtAtan2F` returns `PblCrtX87NanPair(y, x)` when an operand is a NaN, and
  `PblCrtPowF` returns x quieted when x is a NaN and y is not +-0. x86 is unchanged
  (`if constexpr (sizeof(void*) == 8)`, D-02).
- Reach: atan2 is called at 45 x86 sites, including Lua `math.atan2`; pow is Lua `math.pow` and
  `^`. Any mod script or mod data that makes a NaN reaches them.
- Guards: T1 `sim math corpus`. Rows ATAN2-NAN-X64 and POW-NAN-X64 (each helper back to the
  plain CRT call) turn it RED on x64; FMOD-NANOP-X64 covers the shared two-NaN rule. No Win32
  row: x86 code does not change.
- Status: Fixed, 48-11, 2026-11-20 (found by the corpus).

### C-X64-PATHROLL

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `EntityPathFollower::Roll` in `src/Battlefront2/Source/EntityPathFollower.cpp`,
  `// FUNCTION: SWBF2 0x004d6a10`, the weave target
  `fTarget = Sin(t * 0.43f) * (Cos(t * 2.33f) * (Sin(t * 1.17f) * fWeave)) + fBank`.
- Cause: float codegen at an ST0 return, the C3 shape. `PblMath::Sin` and `Cos` are `__cdecl`
  and return in ST0 on x86. The x86 v143 Release code keeps the last `Sin` return on the x87
  stack: `call Sin; fmul dword [tmp]; fadd dword [fBank]; fstp dword [fTarget]`. At the game's
  control word 0x27F the product of two floats is exact and the sum is rounded to 53 bits,
  then once to float. The two inner products are stored to float (`fstp`) before they are used.
  x64 computes `mulss` then `addss`, which rounds the product to float before the add.
- Proof chain (spa3c_ass, x64 Test against x86 Test, 48-13): the first state split is turn
  2927, character 47 (team 2, class 1) `vel.y`, `vel.z` 1 ulp, which is the crashing
  EntityFlyer 0x65F75027 (`CRASHING`, `mVelocity = Interpolate(mVelocity, vDrive, fDrift)`).
  `vDrive` differs because its axes differ; orientation is not hashed. Traced back per turn,
  the flyer's matrix first differs at turn 1494 after `EntityPathFollower::Update`, in
  `Rotate`, in the `Roll` result (`BC79C75C` against `BC79C75D`), and inside `Roll` in
  `fTarget` only: `Sin(t * 0.43f) = BF68E9F1`, `Cos(t * 2.33f) = 3F73D1F4`,
  `Sin(t * 1.17f) = 3DE94569`, `fWeave = 2`, `fBank = BAEC9DC0` are equal, and `fTarget` is
  `BE4BFC51` on x86 against `BE4BFC52` on x64. `(float)(s1 * t2 + b)` in double gives
  `BE4BFC51`, `(float)((float)(s1 * t2) + b)` gives `BE4BFC52`.
- Neutralising experiment: the x64 Test build with only this expression computed in explicit
  double equals x86 to game over on spa3c_ass (21247 turns) and hot1g_con (9014 turns), `state=`
  and `rand=`. With only that expression put back, spa3c_ass splits at 2927 and hot1g_con at
  8238 again. hot1g_con 8238 (static object hp 4 ulp) is therefore the same site.
- Change: `PblX87MulAddF(a, b, c)` (`src/PebbleFL/Common/PblX87Math.h`) is
  `(float)((double)a * (double)b + (double)c)`, which is the x87 53-bit result for float
  operands: the product is exact in both, the sum is rounded to 53 bits in both, and neither
  overflows nor underflows the double range. NaNs follow the x87 rules: an invalid product is
  0xFFC00000, two NaN operands give `PblCrtX87NanPair` in the multiply and in the add. `Roll`
  calls it on x64 and keeps the original expression on x86, chosen by
  `if constexpr (sizeof(void*) == 8)` (D-02).
- Proof that x86 does not change: the x86 Release `EntityPathFollower.obj` has the same
  `dumpbin /disasm` as before the edit (objcmp). Win32 Release `-Smoke All -Soak Short` passes
  with every digest row.
- Rendered x86 (D-18): after `CreateDevice` the x87 control word is 24-bit. The x86 sequence
  then rounds the product to 24 bits and the sum to 24 bits, which is the single-precision
  result x64 computed before this fix. Rendered x86 is unchanged by this fix and still differs
  from headless x86 at this step; x64 now equals headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins 40 results of the x86 x87
  sequence (`fld; fmul; fadd; fstp` at 0x27F, measured on this machine), the turn-1494 inputs
  among them, on both platforms. T1 `path follower roll target keeps the x86 rounding on x64`
  pins the call in `Roll`. Rows X87MA-X64 (the helper back to single precision), X87MA-NANMUL-X64
  and X87MA-NANADD-X64 (the two-NaN rules) and EPF-ROLL-X64 (`Roll` back to the single-precision
  expression) turn them RED on x64. No Win32 row: x86 code does not change.
- Status: Fixed, 48-13, 2026-11-20.

### C-X64-CONVOYDIST

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `UnitTrooperAgent::UpdateStatePostVision` in
  `src/Battlefront2/Source/UnitTrooperAgent.cpp`, state `STATE_Convoy`, the follow distance
  `fDist = (GetCollisionRadius(self) + GetCollisionRadius(parent)) * 2.5f`.
- Cause: float codegen at an ST0 return, the C3 shape. `AIUtil::GetCollisionRadius` is `__cdecl`
  and returns in ST0 on x86; every one of its return paths leaves a float-exact value (a loaded
  float, or a float times 0.5). The x86 v143 Release code stores the first return to float, then
  keeps the second on the x87 stack: `call GetCollisionRadius; fadd dword [r1]; fmul dword [2.5];
  fstp dword [fDist]`. At the game's control word 0x27F the sum and the product are rounded to
  53 bits, then once to float. x64 computes `addss` then `mulss`, which rounds the sum to float
  before the multiply.
- Proof chain (geo1c_con, x64 Test against x86 Test, 48-14): the first state split is turn 4534,
  character 34 (team 2, class 2, a soldier in `SPRINT`) `vel.y`, `vel.z` 2 ulp. Its matrix is
  equal; `Move` gets a different `fForward`, `fStrafe` because `mControlMove` from
  `AIUtil::UpdatePhysics` differs, because the steer destination `x` differs 1 ulp
  (`4133166B` against `4133166A`). The destination is the goal of a new two-node path from
  `NavigatorFollowTarget::UpdateFollowCrumbs`, a crumb built from
  `mFollowTarget->GetMatrix().ApplyTransform(mFollowOffset)`, and `mFollowOffset.z` already
  differs (`BF62AAB1` against `BF62AAB0`) from turn 4186, when the convoy follow starts.
  `mFollowOffset` is `ReserveOffset(FT_CONVOY) * fDist` with `z` scaled by 0.3; the radii
  `3EECD7F7` and `40884EF6` are equal, and `fDist` is `413CE393` on x86 against `413CE392` on
  x64. `(float)(((double)r2 + r1) * 2.5)` gives `413CE393`, `(float)((float)(r1 + r2) * 2.5f)`
  gives `413CE392`.
- Neutralising experiment: the x64 Test build with only this expression computed in explicit
  double moves geo1c_con's first split from 4534 to 5456 (C-X64-CYLRAY). shell-first, which
  split at 798 in 4 of 4 runs at the 48-13 tree, equals x86 to game over (9014 turns) with only
  this change and still splits at 798 with only the C-X64-CYLRAY change, so its 798 split is
  this site.
- Change: `PblX87AddMulF(a, b, c)` (`src/PebbleFL/Common/PblX87Math.h`) is
  `(float)(((double)a + (double)b) * (double)c)`, which is the x87 53-bit result for float
  operands: the sum and the product are rounded to 53 bits in both, and for float operands
  neither overflows nor underflows the double range (the sum is below 2^129 and, when not zero,
  at least 2^-149, so the product lies between 2^-298 and 2^257). NaNs follow the x87 rules: an
  invalid sum is 0xFFC00000, two NaN operands give `PblCrtX87NanPair` in the add and in the
  multiply. The convoy state calls it on x64 and keeps the original expression on x86, chosen
  by `if constexpr (sizeof(void*) == 8)` (D-02).
- Proof that x86 does not change: the x86 Release `UnitTrooperAgent.obj` has the same
  `dumpbin /disasm` as before the edit (objcmp). Win32 Release `-Smoke All -Soak Short` passes
  with every digest row.
- Rendered x86 (D-18): after `CreateDevice` the x87 control word is 24-bit. The x86 sequence
  then rounds the sum and the product to 24 bits, which is the single-precision result x64
  computed before this fix. Rendered x86 is unchanged by this fix and still differs from
  headless x86 at this step; x64 now equals headless x86.
- Guards: T1 `x87 add multiply keeps the x86 rounding` pins 41 results of the x86 x87 sequence
  (`fld; fadd; fmul; fstp` at 0x27F, measured on this machine), the turn-4186 radii among them,
  on both platforms. T1 `convoy follow distance keeps the x86 rounding on x64` pins the call.
  Rows X87AM-X64 (the helper back to single precision), X87AM-NANADD-X64 and X87AM-NANMUL-X64
  (the two-NaN rules) and UTA-CONVOY-X64 (the convoy state back to the single-precision
  expression) turn them RED on x64. No Win32 row: x86 code does not change.
- Status: Fixed, 48-14, 2026-11-20.

### C-X64-CYLRAY

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `CollisionUtil::RayHitAxisAlignedCylinder` in
  `src/Battlefront2/Source/CollisionUtil.cpp`, `// FUNCTION: SWBF2 0x0046b710`, the two cap
  re-entry returns `return fRest * fHit + t;`.
- Cause: float codegen at an ST0 return, the C3 shape. `RayHitAACylinder` returns in ST0; the
  x86 v143 Release code stores it to float, then returns `fld dword [fHit]; fmul dword [fRest];
  fadd dword [t]` in ST0 without a store. At 0x27F that is the 53-bit `a * b + c`. Every x86
  caller stores the return to float at once: `RayHitCylinder`, `CollisionPrimitive::RayTest`,
  and `CollisionModel::RayHit` (through `CollisionPrimitive::RayHit` and
  `CollisionBodyPrimitive::RayHit`, which pass ST0 on). So x86 rounds once to float. x64
  computes `mulss` then `addss`.
- Proof chain (geo1c_con with C-X64-CONVOYDIST neutralised, x64 Test against x86 Test, 48-14):
  the first state split is turn 5456, character 57 (team 3, class 0, an EntityWalker,
  object 0x5F4D4647) `pos.y` 1 ulp when it lands. Traced back per turn, the walker first
  differs at turn 5452 in `EntityWalker::UpdateOrientation`, in leg pair 1's `mPushValue`
  (`3EB24044` against `3EB24030`), from `WalkerLegPair::SetupGroundCollision` with equal
  position, feet and pitch. The second foot's ray (`CollisionManager::RayHit`, filter 0x90)
  hits an object, not the terrain (terrain hits equal); its `CollisionModel::RayHit` has equal
  matrix, inverse and local ray, and body 0, a `CYLINDER` primitive (radius 0.75, height 12.58),
  returns `3F372E64` on x86 against `3F372E65` on x64. The cap re-entry inputs
  `fHit = 3F368699`, `fRest = 3F7DB75D`, `t = 3C1228AB` are equal on both platforms;
  `(float)(fHit * fRest + t)` in double gives `3F372E64`, the single-precision form
  `3F372E65`. `RayHitAACylinder`'s own `fHit / fLength` is an x87 `fdiv` of two floats, which
  rounded once to float equals the single-precision quotient (53 >= 2 * 24 + 2), so it is not
  a difference.
- Neutralising experiment: with C-X64-CONVOYDIST neutralised, the x64 Test build with only
  these two returns computed in explicit double equals x86 to game over on geo1c_con
  (9014 turns, `state=` and `rand=`).
- Change: both returns call `PblX87MulAddF(fHit, fRest, t)` (C-X64-PATHROLL) on x64 and keep
  the original expression on x86, chosen by `if constexpr (sizeof(void*) == 8)` (D-02).
- Proof that x86 does not change: the x86 Release `CollisionUtil.obj` has the same
  `dumpbin /disasm` as before the edit (objcmp). Win32 Release `-Smoke All -Soak Short` passes
  with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the x86 sequence rounds the product and the
  sum to 24 bits, which is the single-precision result x64 computed before this fix. Rendered
  x86 is unchanged by this fix and still differs from headless x86 at this step; x64 now equals
  headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` now also pins the turn-5452 inputs
  (`3F368699 3F7DB75D 3C1228AB` to `3F372E64`, measured with the x87 sequence on this
  machine), on both platforms. T1 `cylinder ray hit return keeps the x86 rounding on x64` pins
  both returns. Rows CU-AACYL-TOP-X64 and CU-AACYL-BOT-X64 (each return back to the
  single-precision expression) turn it RED on x64. No Win32 row: x86 code does not change.
- Status: Fixed, 48-14, 2026-11-20.

### C-X64-SACHEAT

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `SpaceAssault_CheatLikeABastard` in `src/Battlefront2/Source/AIDifficulty.cpp`,
  `// FUNCTION: SWBF2 0x00439930`, the two timers `mNextFlyerKillTime` and
  `mNextCritSysDamageTime` = `fTime + SpaceAssault_GetRandom*Time() * fIntervalScale`.
- Cause: float codegen at an ST0 return, the C3 shape. Both `SpaceAssault_GetRandom*Time` are
  `__cdecl` and return the ST0 of `AIUtil::GetRandomFloat`, which is `fld` of a stored float on
  both paths, so the return is float-exact. x86 v143 Release: `call GetRandom*Time; fmul dword
  [fIntervalScale]; fadd dword [fTime]; fstp dword [timer]` (obj 0x88..0x97 and 0x16F..0x17E):
  one 53-bit `a * b + c` at 0x27F, rounded once to float. x64: `mulss`, `addss`.
- Inputs that differ: any `fIntervalScale` that is not a power of two (the auto-balance scale
  `-1 / (modifier - 1)`, 1/3, 1/5, ...): `429888AE * 3EAAAAAB + 43EC76D0` is `43F92CDF` on x86
  and `43F92CDE` in single precision.
- Change: each timer calls `PblX87MulAddF(GetRandom*Time(), fIntervalScale, fTime)` on x64 and
  keeps the original expression on x86, chosen by `if constexpr (sizeof(void*) == 8)` (D-02).
  The file keeps its `FloatingPointModel Strict` (C2).
- Proof that x86 does not change: the x86 Release `AIDifficulty.obj` has the same
  `dumpbin /disasm` as before the edit (objcmp). Win32 Release `-Smoke All -Soak Short` passes
  with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the x86 sequence rounds the product and the
  sum to 24 bits, which is the single-precision result x64 computed before this fix. Rendered
  x86 is unchanged by this fix and still differs from headless x86 at this step; x64 now equals
  headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins the inputs above and
  `42203205 * 3E4CCCCD + 426AFCC8 = 42858365`, measured with the x87 sequence on this machine.
  T1s `space assault flyer kill timer keeps the x86 rounding on x64` and `space assault
  critical system timer keeps the x86 rounding on x64` pin the two calls. Rows
  AID-FLYERKILL-X64 and AID-CRITSYS-X64 turn them RED on x64. No Win32 row: x86 code does not
  change.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-BARRAGE

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `AILowLevel::ProcessBarrageWpn` in `src/Battlefront2/Source/AILowLevel.cpp`, the
  barrage count `(int)(GetRandomFloat(0, iMax - iMin) * GetBarrageCountMultiplier() + iMin)`.
- Cause: float codegen at an ST0 return, the C3 shape. `GetRandomFloat` is stored to float;
  `AIDifficulty::GetBarrageCountMultiplier` returns `fld` of a stored float (float-exact). x86
  v143 Release: `call GetBarrageCountMultiplier; fmul dword [r]; fadd dword [(float)iMin];
  fstp dword; cvttss2si` (obj 0xEC..0x109): the 53-bit `a * b + c` rounded once to float, then
  truncated. x64: `mulss`, `addss`, `cvttss2si`.
- Inputs that differ: `3F21AF28 * 4119F4DD + 40800000` is `41213C55` on x86 and `41213C56` in
  single precision; the truncated count differs when the two results straddle an integer.
- Change: x64 stores the random first (the x86 call order) and computes
  `(int)PblX87MulAddF(GetBarrageCountMultiplier(), r, (float)iMin)`; x86 keeps the original
  expression, chosen by `if constexpr (sizeof(void*) == 8)` (D-02).
- Proof that x86 does not change: objcmp of the x86 Release `AILowLevel.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the x86 sequence equals the single-precision
  form x64 computed before this fix; rendered x86 is unchanged and still differs from headless
  x86 here; x64 now equals headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `barrage count keeps the x86 rounding on x64` pins the call.
  Row AILL-BARRAGE-X64 turns it RED on x64. No Win32 row: x86 code does not change.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-JETJUMP

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `UnitDeathmatchAgent::DoJetJumpStuff` in
  `src/Battlefront2/Source/UnitDeathmatchAgent.cpp`, `mNextJumpTime = fNow + 5.0f +
  GetGlobalFloat() * 10.0f`. x86 inlines it into `UnitDeathmatchAgent::UpdateStatePostVision`
  as well, so both inventory rows are this one source line.
- Cause: float codegen at an ST0 return. `PblRandom::GetGlobalFloat` returns `fld (1.m);
  fsub 1.0`, exact (k * 2^-23). `fNow + 5.0f` is an SSE `addss` stored to float on both
  platforms. x86: `call GetGlobalFloat; fmul dword [10.0]; fadd dword [t]; fstp` (obj
  0x17F..0x19F and 0x1EF..0x20F): the 53-bit `r * 10 + t` rounded once. x64: `mulss`, `addss`.
- Inputs that differ: ordinary; `3F66834C * 10 + 415E842F` is `41B74B1F` on x86 and `41B74B20`
  in single precision (26214 of the 2^23 random values differ at `t = 905`).
- Change: x64 calls `PblX87MulAddF(GetGlobalFloat(), 10.0f, fNow + 5.0f)`; x86 keeps the
  original expression (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `UnitDeathmatchAgent.obj` is
  `same`. Win32 Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `deathmatch jet jump time keeps the x86 rounding on x64`
  pins the call. Row UDA-JETJUMP-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-SOLDACTION

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `SoldierAnimator::SetAction` in `src/Battlefront2/Source/SoldierAnimator.cpp`, state
  `FLY`, `mRotSpeed = GetGlobalFloat() * 20.0f - 10.0f`.
- Cause: float codegen at an ST0 return. x86: `call GetGlobalFloat; fmul dword [20.0]; fsub
  dword [10.0]; fstp dword [mRotSpeed]` (obj 0x522..0x536): the 53-bit result rounded once.
  x64: `mulss`, `subss`. The exact random times 20 needs up to 26 bits.
- Inputs that differ: ordinary; 2516582 of the 2^23 random values differ, for example
  `3EEE80B4` gives `BF2EF8F8` on x86 and `BF2EF900` in single precision.
- Change: x64 calls `PblX87MulAddF(GetGlobalFloat(), 20.0f, -10.0f)`. The product is never a
  NaN and the subtracted constant is not a NaN, so `+ (-10)` equals the x87 `- 10` for every
  random value. x86 keeps the original expression (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `SoldierAnimator.obj` is `same`.
  Win32 Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `soldier fly spin keeps the x86 rounding on x64` pins the
  call. Row SA-FLYSPIN-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-FLYERRAY

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `EntityFlyer::Update` in `src/Battlefront2/Source/EntityFlyer.cpp`, state `LANDING`,
  `fDist = CollisionManager::RayHit(...) * 1024.0f - (fCenterY - fBodyY)`.
- Cause: float codegen at an ST0 return. `CollisionManager::RayHit` returns `fld` of a stored
  float or `fld1` (float-exact). x86: `call RayHit; fmul dword [1024.0]; fsub dword [d]; fstp`
  (obj 0x24B7..0x24E4), `d` the SSE difference stored to float. x64: `mulss`, `subss`.
- Inputs that differ: the product by 1024 is exact in single precision unless it overflows, so
  only a hit fraction at or above 2^118 differs: `7A800000 * 1024 - 7F7FFFFF` is `73800000`
  (2^104) on x86 and `7F800000` in single precision. A NaN `d` keeps its own sign in the x87
  `fsub`, so `PblX87MulAddF(r, 1024, -d)` is not exact here.
- Change: `PblX87MulSubF(a, b, c)` (`src/PebbleFL/Common/PblX87Math.h`) is
  `(float)((double)a * (double)b - (double)c)` when the product is not a NaN, otherwise
  `PblX87MulAddF(a, b, c)`, whose NaN path is the x87 rule for the product and for the second
  operand (the NaN rules do not depend on add or subtract). x64 calls
  `PblX87MulSubF(RayHit(...), 1024.0f, fCenterY - fBodyY)`; x86 keeps the original expression
  (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `EntityFlyer.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 at this step; x64 now equals headless
  x86.
- Guards: T1 `x87 multiply subtract keeps the x86 rounding` pins 38 results of the x87 sequence
  `fld; fmul; fsub; fstp` at 0x27F measured on this machine: 12 random inputs where the single
  form differs, the overflow above, signed zeros, exact cancellation, denormals, NaN `c` of
  both signs, inf times 0, inf minus inf and the two-NaN pairs. T1 `flyer landing ground
  distance keeps the x86 rounding on x64` pins the call. Rows X87MS-X64 (the helper back to
  single precision), X87MS-NAN-X64 (the NaN path back to the SSE subtract) and EF-LANDDIST-X64
  (the call back to the single-precision expression) turn them RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-TARGETPRIO

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `TargetInfo::GetTargetPrio` in `src/Battlefront2/Source/TargetInfo.cpp`,
  `// FUNCTION: SWBF2 0x006520a0`, `return fOff * 16.0f + fAlong;`.
- Cause: an unrounded ST0 return. x86: `fld dword [fOff]; fmul dword [16.0]; fadd dword
  [fAlong]; ret` (obj 0xB7..0xC4). Every caller stores the return to float at once:
  `EntitySoldier::TryBoard` (`fstp dword [ebp-70h]`) and the three calls in
  `PlayerController::Update` (`fstp dword [esp+34h]`, `fstp dword [esp+18h]`,
  `fstp dword [esp+5Ch]`). So x86 rounds once. x64: `mulss`, `addss`.
- Inputs that differ: the product by 16 is exact unless it overflows: `abs(fOff) >= 2^124` with
  a `fAlong` that brings the sum back below the float range (`7D800000 * 16 + FF7FFFFF` is
  `73800000` on x86, `7F800000` in single precision), and NaN pairs (`FFC00000 * 16 +
  7FC00001` is `7FC00001` on x86, `FFC00000` with SSE).
- Change: x64 returns `PblX87MulAddF(fOff, 16.0f, fAlong)`; x86 keeps the original expression
  (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `TargetInfo.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 multiply add keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `target priority keeps the x86 rounding on x64` pins the
  return. Row TI-PRIO-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-IDLEBOB

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `IdleHelper::UpdateState` in `src/Battlefront2/Source/IdleHelper.cpp`,
  `fWander = (Cos(mIdleTime / 5 * 0.3) + Sin(mIdleTime / 5)) * 0.2f`.
- Cause: float codegen at an ST0 return. `PblMath::Sin` and `Cos` return `fld` of a stored
  float. x86: `call Sin; fstp [s]; call Cos; fadd dword [s]; fmul dword [0.2]; fstp` (obj
  0xC9..0x101): the 53-bit `(c + s) * 0.2` rounded once. x64: `addss`, `mulss`.
- Inputs that differ: ordinary; `(BE15E8CB + BE22F024) * 0.2` is `BD7A4726` on x86 and
  `BD7A4727` in single precision.
- Change: x64 calls `PblX87AddMulF(Cos(...), Sin(...), 0.2f)`; x86 keeps the original
  expression (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `IdleHelper.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 add multiply keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `idle wander keeps the x86 rounding on x64` pins the call.
  Row IH-WANDER-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-FLYERTRICK

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `EntityFlyer::DoTrick` in `src/Battlefront2/Source/EntityFlyer.cpp`, the four flip
  cases `mGetSpeedSpeed >= (GetFlyerMinSpeed() + GetFlyerMidSpeed()) * 0.5f`.
- Cause: float codegen at an ST0 return. Both getters return `fld` of a stored float. x86, four
  times: `call GetFlyerMinSpeed; fstp [m]; call GetFlyerMidSpeed; fadd dword [m]; fmul dword
  [0.5]; fstp` (obj 0x1B2, 0x248, 0x2DE, 0x360). x64: `addss`, `mulss`.
- Inputs that differ: the half is exact unless the sum overflows (`7F7FFFFF + 7F7FFFFF` halves
  to `7F7FFFFF` on x86 and gives `7F800000` in single precision) or a NaN pair meets. A
  subnormal half equals the single form for 0.5 (the float sum is exact below 2^-125 and the
  scale commutes with rounding above it); the T1 pins one such case.
- Change: the switch holds one `float fTrickSpeed`; each flip case assigns it with
  `PblX87AddMulF(GetFlyerMidSpeed(), GetFlyerMinSpeed(), 0.5f)` on x64 and with the original
  expression on x86 (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `EntityFlyer.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 add multiply keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `flyer trick roll speed keeps the x86 rounding on x64`
  counts the block four times. Rows EF-TRICK-UR-X64, EF-TRICK-DR-X64, EF-TRICK-UL-X64 and
  EF-TRICK-DL-X64 (each case back to the single-precision expression) turn it RED on x64. No
  Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-DPS

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `CombatUtil::CalcDamagePerSecond` in `src/Battlefront2/Source/CombatUtil.cpp`,
  `fDamagePerSecond = (sCalcDamagePerSecond(top) + fDamagePerSecond) * 0.5f`.
- Cause: float codegen at an ST0 return. `sCalcDamagePerSecond` returns `fld` of a stored float
  or `fldz`. x86: `call sCalcDamagePerSecond; fadd dword [d]; fmul dword [0.5]; fstp` (obj
  0x176..0x187). x64: `addss`, `mulss`.
- Inputs that differ: the sum overflowing the float range (`7F7FFFFF + 7F000000` halves to
  `7F400000` on x86, `7F800000` in single precision) and NaN pairs; the T1 also pins a
  subnormal half, which equals the single form.
- Change: x64 calls `PblX87AddMulF(sCalcDamagePerSecond(...), fDamagePerSecond, 0.5f)`; x86
  keeps the original expression (`if constexpr (sizeof(void*) == 8)`, D-02).
- Proof that x86 does not change: objcmp of the x86 Release `CombatUtil.obj` is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 add multiply keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `charged damage per second keeps the x86 rounding on x64`
  pins the call. Row CU-DPS-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-RESERVE

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `ReserveManager::ReserveOffset` in `src/Battlefront2/Source/ReserveManager.cpp`,
  `FT_DOUBLEFILE_RAND`, the seed `(int)((GetMissionTime() + fBits) * -0.125f)`.
- Cause: float codegen at an ST0 return. `GameLoop::GetMissionTime` returns `fld` of a stored
  float or `fldz`. x86: `call GetMissionTime; fadd dword [fBits]; fmul dword [-0.125]; fstp
  dword; cvttss2si` (obj 0x3FC..0x416). x64: `addss`, `mulss`, `cvttss2si`.
- Inputs that differ: the sum overflowing the float range (`(7F7FFFFF + 7F7FFFFF) * -0.125` is
  `FE7FFFFF` on x86, `FF800000` in single precision), a subnormal result whose sum the float
  add rounds first (`(01800000 + 00000005) * -0.125` is `80400001` on x86, `80400000` in single
  precision), and NaN pairs.
- Change: the case body is in both branches of `if constexpr (sizeof(void*) == 8)` (D-02); the
  x64 copy computes the seed with `(int)PblX87AddMulF(GetMissionTime(), fBits, -0.125f)`, the
  x86 copy is the original text. A local for the product or the integer moved x86 register
  allocation in the vector code after it (objcmp `DIFF`); the whole-body form keeps the x86
  object identical.
- Proof that x86 does not change: objcmp of the x86 Release `ReserveManager.obj` is `same`.
  Win32 Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at 24 bits the x86 sequence equals the single-precision form; rendered
  x86 is unchanged and still differs from headless x86 here; x64 now equals headless x86.
- Guards: T1 `x87 add multiply keeps the x86 rounding` pins the inputs above (measured with the
  x87 sequence on this machine); T1 `double file jitter phase keeps the x86 rounding on x64`
  pins the case body. Row RM-JITTER-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-21, 2026-11-20.

### C-X64-SNIPEHINT

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `AIUtil::IsSnipeHintNearby` in `src/Battlefront2/Source/AIUtil.cpp`, the next check
  time `GetRandomFloat(0.0f, 4.0f) + (GameLoop::GetMissionTime() + 8.0f)`.
- Cause: float codegen at an ST0 return. The random is an SSE float stored first;
  `GameLoop::GetMissionTime` returns `fld` of a stored float or `fldz`. x86: `call
  GetMissionTime; fadd dword [8.0]; fadd dword [r]; fstp dword` (obj 0x7F..0x90), the two adds
  rounded once to float from the 53-bit sum. x64: `addss 8.0`, `addss r`.
- Inputs that differ: ordinary mission times, for example `(42750181 + 41000000) + 4043A16E` is
  `42909DCC` on x86 and `42909DCB` in single precision, plus a sum overflowing the float range
  and NaN pairs.
- Change: x64 stores the random first and calls `PblX87AddAddF(GetMissionTime(), 8.0f, fJitter)`
  under `if constexpr (sizeof(void*) == 8)` (D-02); x86 keeps the original expression.
  `PblX87AddAddF` (new, `src/PebbleFL/Common/PblX87Math.h`) is `(a + b) + c` in double, which
  is the x87 53-bit chain for every float input, with the x87 NaN rule per step through
  `PblX87NanStep`.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the x86 sequence rounds each sum to 24 bits,
  which is the single-precision result x64 computed before this fix for ordinary inputs.
  Rendered x86 is unchanged by this fix and still differs from headless x86 at this step; x64
  now equals headless x86.
- Guards: T1 `x87 add add keeps the x86 rounding` pins the inputs above and the helper's
  overflow, subnormal, signed-zero, invalid and NaN cases, measured with the x87 sequence on
  this machine; T1 `snipe hint check time keeps the x86 rounding on x64` pins the call. Rows
  X87AA-X64, X87AA-NAN1-X64, X87AA-NAN2-X64, X87NS-X64 and AIU-SNIPE-X64 turn them RED on x64.
  No Win32 row: x86 code does not change.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-COMBATRANGE

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Functions: `CombatUtil::GetCombatRange` in `src/Battlefront2/Source/CombatUtil.cpp` and its
  chaining caller `CombatHelper::UpdateState` in `src/Battlefront2/Source/CombatHelper.cpp`,
  `GetCombatRange(GetMe()) + 10.0f`.
- Cause: an unrounded ST0 return chained by the caller. x86 `GetCombatRange` inlines
  `sGetCombatRange`: `GetMaxSpeed() * 5.0f` is stored (`fstp`), then `call sGetWpnMaxRange;
  fadd dword [fMove]; ret` (obj 0x2E..0x3C) returns the sum unrounded. `sGetWpnMaxRange`
  returns `fld` of a stored float on both paths. `UpdateState` adds `fadd dword [10.0]` and
  stores (obj 0x654..0x674), so x86 rounds `(W + M) + 10` once. x64: `addss` in
  `GetCombatRange`, `addss 10.0` in `UpdateState`. The other caller,
  `UnitAgent::Handle_Audible_CanHearEnemy`, stores the return at once (`fstp [ebp-2Ch]`), which
  is the single add x64 already computes.
- Inputs that differ: ordinary ranges, for example `(41B5C77B + 3F5D00F4) + 41200000` is
  `420657C1` on x86 and `420657C2` in single precision.
- Change: in `UpdateState`, x64 computes the two operands as `GetCombatRange` does
  (`GetCurWpn()->GetClass()`, `GetMaxSpeed() * 5.0f`, `CombatUtil::sGetWpnMaxRange`) and calls
  `PblX87AddAddF(W, M, 10.0f)` under `if constexpr (sizeof(void*) == 8)`; x86 keeps the
  original call. `GetCombatRange` itself is not changed: its storing caller is exact. No
  signature changes. The receiver of `GetMaxSpeed` is the controllable's own object, never an
  `Asteroid`.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word both adds round to 24 bits, the
  single-precision result; rendered x86 is unchanged and still differs from headless x86 here;
  x64 now equals headless x86.
- Guards: T1 `x87 add add keeps the x86 rounding` pins the inputs above; T1 `combat engage
  range keeps the x86 rounding on x64` pins the branch. Row CH-RANGE-X64 turns it RED on x64.
  No Win32 row.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-SPAWNTIME

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `PlayerController::Update` in `src/Battlefront2/Source/PlayerController.cpp`, the
  invincibility cancel `SetSpawnTime(NetComm::GetTime() - (float)gSpawnInvicibility - 0.1f)`.
- Cause: float codegen at an ST0 return. `NetComm::GetTime` is `fld dword [netTime]`;
  `(float)gSpawnInvicibility` is an SSE float stored first. x86: `call GetTime; fsub dword [s];
  fsub dword [0.1]; fstp dword [mSpawnTime]` (obj 0x91A..0x92A). x64: `subss`, `subss`.
- Inputs that differ: ordinary times, for example `(3F04F6A7 - 40400000) - 3DCCCCCD` is
  `C02528BD` on x86 and `C02528BC` in single precision, and NaN pairs. The x87 `fsub` keeps a
  NaN subtrahend's own sign (`(1 - 1) - FFC00001` is `FFC00001`), so the helper subtracts and
  never adds a negated operand.
- Change: x64 calls `PblX87SubSubF(NetComm::GetTime(), (float)gSpawnInvicibility, 0.1f)` under
  `if constexpr (sizeof(void*) == 8)`; x86 keeps the original expression. `PblX87SubSubF` (new)
  is `(a - b) - c` in double with the x87 NaN rule per step.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word both subtractions round to 24 bits, the
  single-precision result; rendered x86 is unchanged and still differs from headless x86 here;
  x64 now equals headless x86.
- Guards: T1 `x87 sub sub keeps the x86 rounding` pins the inputs above and the helper's edge
  and NaN cases; T1 `spawn invincibility cancel time keeps the x86 rounding on x64` pins the
  call. Rows X87SS-X64, X87SS-NAN1-X64, X87SS-NAN2-X64 and PC-SPAWN-X64 turn them RED on x64.
  No Win32 row.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-ADRENALINE

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `Adrenaline::Update` in `src/Battlefront2/Source/Adrenaline.cpp`, the speed factor
  `1.0f - Sqrt(v . v) / pObject->GetMaxSpeed()`.
- Cause: float codegen at an ST0 return. The speed is an SSE float (`sqrtsd; cvtsd2ss`, stored).
  x86: `call [vtable+60h] (GetMaxSpeed); fdivr dword [speed]; fld1; fsubrp st(1),st; fstp
  dword` (obj 0x94..0xBD), the quotient kept at 53 bits. x64: `divss`, `subss`. The receiver
  is `pOwner->GetGameObject()` or `mOwner->GetGameObject()` of a controllable
  (`ControllerManager.cpp`, `UnitController.cpp`): the controllable's own entity or the parent
  of a turret or passenger slot (buildings and vehicles). None is an `Asteroid`, so
  `GetMaxSpeed` returns a loaded float or 0 (the overrides listed for
  `AIUtil::CalculateBubbleOffset`).
- Inputs that differ: ordinary speeds, for example `1 - 4083E185 / 4103D944` is `3EFFEFF9` on
  x86 and `3EFFEFFA` in single precision.
- Change: x64 stores the speed and calls `PblX87SubDivF(1.0f, fSpeed, GetMaxSpeed())` under
  `if constexpr (sizeof(void*) == 8)`; x86 keeps the original expression. `PblX87SubDivF`
  (new) is `c - a / b` in double with the x87 NaN rule per step; a quotient of two floats lies
  in the double normal range, `x / 0` gives the signed infinity and `0 / 0`, `inf / inf` the
  x87 indefinite `FFC00000`.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the quotient and the difference round to 24
  bits, the single-precision result; rendered x86 is unchanged and still differs from headless
  x86 here; x64 now equals headless x86.
- Guards: T1 `x87 sub div keeps the x86 rounding` pins the inputs above and the helper's edge
  and NaN cases; T1 `adrenaline speed factor keeps the x86 rounding on x64` pins the branch.
  Rows X87SD-X64, X87SD-NAN1-X64, X87SD-NAN2-X64 and ADR-SPEED-X64 turn them RED on x64. No
  Win32 row.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-CATAPULT

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `WeaponCatapultClass::SetProperty` in `src/Battlefront2/Source/WeaponCatapult.cpp`,
  `CockedAngle`: `mCockedAngle = (float)atof(v) * 3.1415927f / 180.0f`.
- Cause: x87 chain on an ODF value. x86 rounds the `atof` double to float first (`call atof;
  fstp dword; fld dword`), then `fmul dword [3.1415927]; fdiv dword [180.0]; fstp dword`
  (obj 0x66..0x81), one rounding of the 53-bit quotient. x64: `cvtsd2ss`, `mulss`, `divss`.
- Inputs that differ: ordinary angles, for example `C297F629 * 40490FDB / 43340000` is
  `BFA9BE22` on x86 and `BFA9BE21` in single precision.
- Change: x64 calls `PblX87MulDivF((float)atof(v), 3.1415927f, 180.0f)` under
  `if constexpr (sizeof(void*) == 8)`; x86 keeps the original expression. The first operand is
  the float, as x86 rounds it before the `fmul`. `PblX87MulDivF` (new) is `a * b / c` in double
  with the x87 NaN rule per step.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word the product and the quotient round to 24
  bits, the single-precision result; rendered x86 is unchanged by this fix and still differs
  from headless x86 here when the class loads after `CreateDevice`; x64 now equals headless
  x86.
- Guards: T1 `x87 mul div keeps the x86 rounding` pins the inputs above and the helper's edge
  and NaN cases; T1 `catapult cocked angle keeps the x86 rounding on x64` pins the call. Rows
  X87MD-X64, X87MD-NAN1-X64, X87MD-NAN2-X64 and WC-COCKED-X64 turn them RED on x64. No Win32
  row.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-VISIONDIST

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Functions: `VisionManager::MaxVisibleDist` and `VisionManager::UpdatePotentiallyVisible` in
  `src/Battlefront2/Source/VisionManager.cpp`, and the chaining caller `AttackPatterns::GetFar`
  in `src/Battlefront2/Source/UnitAttackPatterns.cpp`.
- Cause: an unrounded ST0 return with three different x86 uses. The terms are SSE floats
  (`fDist = radius * 20`, `fModifier`). Called `MaxVisibleDist` ends `fld dword [fDist]; fadd
  dword [30.0]; fmul dword [fModifier]; ret` (obj 0x109..0x119), unrounded.
  `UnitAgent::HandleDamagedEvent` stores the return at once (one rounding of
  `(d + 30) * f`). `GetFar` subtracts `fsub dword [10.0]` and stores (obj 0x4F..0x71), one
  rounding of `(d + 30) * f - 10`. `UpdatePotentiallyVisible` inlines the function as `addss
  30.0; mulss` (two roundings). x64 before: `addss; mulss` in all three, then `subss 10.0` in
  `GetFar`.
- Inputs that differ: ordinary distances, for example `(427F2693 + 41F00000) * 41EB2532` is
  `452C4B75` on x86 and `452C4B76` in single precision, and
  `(422C9F81 + 41F00000) * 3FA1CDD0 - 41200000` is `42A4F392` on x86 and `42A4F391` in single
  precision.
- Change: the terms move into the inline companion `VisionManager::MaxVisibleDistTerms` in the
  new header `src/Battlefront2/Source/VisionManagerDist.h` (x86 never calls it, so it emits no
  x86 code). Under `if constexpr (sizeof(void*) == 8)`: `MaxVisibleDist` returns
  `PblX87AddMulF(fDist, 30.0f, fModifier)` (one rounding, for the storing caller);
  `UpdatePotentiallyVisible` computes `(fDist + 30.0f) * fModifier` in float (the inlined x86
  form); `GetFar` computes `PblX87AddMulAddF(fDist, 30.0f, fModifier, -10.0f)`. The x86 branches
  are the original text. Adding `-10` equals the x87 `fsub 10` for every value, including NaN
  and zero results (checked with the x87 tool for the site inputs). `PblX87AddMulAddF` (new)
  is `(a + b) * c + d` in double with the x87 NaN rule per step. No signature, vtable or struct
  changes.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects is `same`. Win32
  Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word each step rounds to 24 bits, the
  single-precision result; rendered x86 is unchanged and still differs from headless x86 at the
  called function; x64 now equals headless x86 at all three uses.
- Guards: T1s `x87 add multiply keeps the x86 rounding` and `x87 add mul add keeps the x86
  rounding` pin the inputs above and the helper's edge and NaN cases; T1s `max visible distance
  keeps the x86 rounding on x64`, `potentially visible distance keeps the x86 rounding on x64`,
  `attack far range keeps the x86 rounding on x64` and `max visible distance terms keep the x86
  operands on x64` pin the branches and the companion. Rows X87AMA-X64, X87AMA-NAN1-X64,
  X87AMA-NAN2-X64, X87AMA-NAN3-X64, VM-DIST-X64, VM-PVDIST-X64, UAP-FAR-X64 and VMD-TERMS-X64
  turn them RED on x64. No Win32 row.
- Status: Fixed, 48-22, 2026-11-20.

### C-X64-BOARDRANGE

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `UnitBoardAgent::UpdateStatePostVision` in
  `src/Battlefront2/Source/UnitBoardAgent.cpp`, the boarding range
  `pMe->GetCollisionRadius() + pVehicleCol->GetCollisionRadius() + 1.5f`.
- Cause: float codegen at an ST0 return. `TreeGridObject::GetCollisionRadius` is `movss; fld`
  of a stored float on both paths. x86: `call GetCollisionRadius (me); fstp [r]; call
  GetCollisionRadius (vehicle); fadd dword [r]; fadd dword [1.5]; fstp` (obj 0x19F..0x1B9), the
  two adds rounded once from the 53-bit sum. x64: `addss`, `addss 1.5`. The gap list named this
  row with the `AIUtil::GetCollisionRadius` family, but it calls the `TreeGridObject` getter and
  differs for ordinary radii.
- Inputs that differ: ordinary radii, for example `(40C79056 + 3F4AF587) + 3FC00000` is
  `41087783` on x86 and `41087784` in single precision, plus a sum overflowing the float range
  and NaN pairs.
- Change: x64 calls `PblX87AddAddF(pVehicleCol->GetCollisionRadius(), pMe->GetCollisionRadius(),
  1.5f)` (operands in the x86 order) under `if constexpr (sizeof(void*) == 8)`; x86 keeps the
  original expression.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects; only `NetGame.obj`
  (`$SG` numbers) and `EntityLight.obj` (one `$LN` label number, see C-X64-LIGHTFMOD) differ in
  name only. Win32 Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word both adds round to 24 bits, the
  single-precision result; rendered x86 is unchanged and still differs from headless x86 here;
  x64 now equals headless x86.
- Guards: T1 `x87 add add keeps the x86 rounding` pins the inputs above (measured with the x87
  sequence on this machine); T1 `boarding range keeps the x86 rounding on x64` pins the branch.
  Row UBA-RANGE-X64 turns it RED on x64. No Win32 row.
- Status: Fixed, 48-23, 2026-11-20.

### C-X64-DOT3

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Functions: the out-of-line `DotProduct(const PblVector3&, const PblVector3&)` COMDAT
  (`src/PebbleFL/pc/pcPblVector.inl`) at its nine x86 call sites, and `Asteroid::GetMaxSpeed`
  (`src/Battlefront2/Source/Asteroid.cpp`).
- Cause: a three-term dot product kept at 53 bits. The x86 exe links the `EntitySoldier.obj`
  copy of the COMDAT (map: `00503cd0 f i EntitySoldier.obj`): `fld [a.y]; fmul [b.y]; fld [a.x];
  fmul [b.x]; faddp; fld [a.z]; fmul [b.z]; faddp; ret`, unrounded. The `/Od` `NetGame.obj` copy
  (`call D3DXVec3Dot`) is not linked. Its callers are `EntitySoldier::Update` (2 calls,
  `EntitySoldier.cpp` sprint start and sprint end) and seven in `NetGame.cpp` (`FrontDist`,
  `FarToNear` twice, `GetClosestOrdnancePos`, `GetObjectsInScope`, `AddSend` twice); each
  stores ST0 at once (`fstp`), so x86 is the 53-bit dot rounded once. Every other `DotProduct`
  use is inlined on x86 as SSE single and is not changed. `Asteroid::GetMaxSpeed` (vtable +60h)
  is the same sequence on `mVelocity` and the matrix z row, unrounded. x64: `mulss` and `addss`
  per step.
- GetMaxSpeed callers: every `[vtable+60h]` call in the x86 objects of the 14 source files that
  call `GetMaxSpeed` stores the return at once (`fstp`) except `AIUtil::CalculateBubbleOffset`
  (`fmul 0.95`), the first call in `AIUtil::IsNearPost` (`fmul 0.5`),
  `CombatUtil::GetCombatRange`, `GetMaxCombatRange`, `sGetCombatRange` and
  `sSelectBestWeaponType` (`fmul 5.0`) and `NavigatorSlide::NextVehicleSlidePos`
  (`fmul 1.5`). Each of those chains on the controllable's own object (`GetMe()`,
  `pControllable->GetGameObject()`, the navigator's object), which is never an `Asteroid`
  (48-22), so a once-rounded `Asteroid::GetMaxSpeed` is exact for every caller that can
  receive one.
- Inputs that differ: ordinary vectors, for example the dot of `(40BDE018, 42023A53, BD986428)`
  and `(432779BF, 4357DCE1, C3502233)` is `45FB2875` on x86 and `45FB2876` in single precision;
  products that overflow float but cancel (`(2^127, 2^127, 1) . (-2^127, 2^127, 1)` is
  `3F800000` on x86, the indefinite in single precision); subnormal products; NaN pairs.
- Change: `PblX87Dot3F(ax, ay, az, bx, by, bz)` (new, `src/PebbleFL/Common/PblX87Math.h`)
  computes `(ay * by + ax * bx) + az * bz` in double and rounds once, with the x87 NaN rule per
  step through `PblX87NanStep`. Float products are exact in double and every partial sum is a
  normal double, so this is the x87 53-bit sequence for every float input. Each call site has
  an x64 branch under `if constexpr (sizeof(void*) == 8)`; the x86 text is the original. The
  `DotProduct` inline body is not changed. Shape of each branch:
  - `EntitySoldier::Update`: the whole `if (mPlayerId >= 0 ...)` sprint group is in both
    branches (a conditional operator inside the condition moved x86 register allocation).
  - `NetGame.cpp` is `/Od`, where a conditional operator compiles both arms. `FrontDist`,
    `FarToNear`, `GetClosestOrdnancePos` and `GetObjectsInScope` use statement branches.
    `AddSend` holds a static local (`sSideLimit`) whose decorated name carries a scope number,
    and any block before it renumbers it; so x86 `AddSend` is unchanged and x64 replaces the
    priority after the original block with `AddSendPriorityX87`, an inline function that
    recomputes the offset and the priority with `PblX87Dot3F` (never emitted on x86).
    `NetGame.cpp` includes `PblX87Math.h` after its last `ensure` use, because `ensure` pushes
    `__LINE__`, and the earlier include position reordered the `/Od` `.data` globals.
  - `Asteroid::GetMaxSpeed` moved from the class body in `Asteroid.h` to `Asteroid.cpp` with
    its `// FUNCTION:` marker; the header now only declares it. Keeping the body in the header
    would have put `PblX87Math.h` into `NetGame.cpp` through `Asteroid.h`, which reordered the
    `/Od` `.data` globals. The x86 code of `Asteroid.obj` is the same.
- Proof that x86 does not change: objcmp of all 1068 x86 Release objects; `NetGame.obj` differs
  only in `$SG` numbers (section sizes equal, `sgcmp.py`), `EntityLight.obj` only in one `$LN`
  label number. Win32 Release `-Smoke All -Soak Short` passes with every digest row.
- Rendered x86 (D-18): at the 24-bit control word each step rounds to 24 bits and the products
  are no longer exact, so rendered x86 differs from both; rendered x86 is unchanged by this
  fix; x64 now equals headless x86.
- Guards: T1 `x87 dot keeps the x86 rounding` pins 105 cases measured with the x87 sequence on
  this machine (12 ordinary inputs where single precision differs, overflow and subnormal
  products, signed zeros, `inf * 0` and `inf - inf` in every step, NaNs in each operand, NaN
  pairs in every step, ties); site T1s `soldier sprint start`, `soldier sprint end`, `net front
  distance`, `net far to near order`, `net closest ordnance position`, `net scope distance`,
  `net send priority` (each `... keeps the x86 rounding on x64`), `net send priority is
  replaced on x64` and `asteroid max speed keeps the x86 rounding on x64` pin the branches. Rows
  X87DOT-X64, X87DOT-NAN1-X64 to X87DOT-NAN5-X64, ES-SPRINT-X64, ES-SPRINTEND-X64,
  NG-FRONT-X64, NG-FARNEAR1-X64, NG-FARNEAR2-X64, NG-ORDPOS-X64, NG-SCOPE-X64,
  NG-SENDFACE-X64, NG-SENDSIDE-X64, NG-SENDPRIO-X64 and AST-SPEED-X64 turn them RED on x64. No
  Win32 row.
- Status: Fixed, 48-23, 2026-11-20.

### C-X64-LIGHTFMOD

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Functions: `EntityLight::Update` in `src/Battlefront2/Source/EntityLight.cpp` (the StrobeFade
  and Pulse flicker timers) and the ground effect timer in
  `src/Battlefront2/Source/WeatherEffect.cpp`.
- Cause: a direct CRT `fmod((double)t, (double)p)` stored to float, not routed through
  `PblCrtFmodF`. x86 goes through `_CIfmod` and gives the x87 indefinite `FFC00000` for
  `fmod(t, 0)` or an infinite timer and the x87 NaN-pair rule for NaN operands (C-X64-FMOD);
  x64 UCRT `fmod` gives `7FC00000` and the second NaN. Valid inputs are exact on both.
- Change: the three sites call `PblCrtFmodF(t, p)`, whose x64 branch applies the C-X64-FMOD
  rule and whose x86 branch is the same `(float)fmod((double)t, (double)p)`.
  `PblCrtTrig.h` is included in both files.
- Proof that x86 does not change: `EntityLight.obj` has the same bytes in every non-debug
  section as the HEAD build (scratch compile of both, `dumpbin /rawdata`); the disassembly
  differs only in the name of the switch jump table label (`$LN38` to `$LN42`), from the inline
  function's labels. `WeatherEffect.obj` is `same`. Win32 Release `-Smoke All -Soak Short`
  passes with every digest row.
- Rendered x86 (D-18): `fmod` is exact or a NaN at every precision; no rendered change.
- Guards: T1 `sim math corpus` pins `PblCrtFmodF` against the raw x86 CRT for the zero,
  infinite and NaN operands (48-11); T1 `light and weather timers keep the x86 fmod rule on
  x64` pins the three calls and that no direct `fmod(` remains. Rows EL-FMOD-FADE-X64,
  EL-FMOD-PULSE-X64 and WE-FMOD-X64 (each back to the direct `fmod`) turn it RED on x64. No
  Win32 row.
- Status: Fixed, 48-23, 2026-11-20.

### C-X64-CANNONSPREAD

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Functions: `WeaponCannon::Fire` (`// FUNCTION: SWBF2 0x0067f320`), `WeaponCannon::GetYawSpread`
  (`0x0067e880`) and `WeaponCannon::GetPitchSpread` (`0x0067ea60`) in
  `src/Battlefront2/Source/WeaponCannon.cpp`.
- Relation to C3: C3 made v143 x86 emit the VS2013 sequence for this chain through the per-file
  `<FloatingPointModel>Strict</FloatingPointModel>` item, which stays. It never covered x64,
  where `/fp:strict` uses SSE and rounds every step. This cause carries the same chain to x64.
- x86 sequence: `GetPitchSpread` is `maxss kick, 0`, then `fld [mStanceSpread]; fadd [kick];
  fadd [class mPitchSpread]; ret`, and `GetYawSpread` ends the same way with the yaw term. Both
  return the 53-bit sum in ST0. Fire draws two Sim randoms, `subss` rounds their difference to
  float (exact for two values in [0, 1)), calls the spread through the vtable (`[vtable+44h]` for
  pitch, `[vtable+40h]` for yaw), then `fmul [diff]; fadd [pitch or yaw]; fstp`, one rounding.
  No class overrides the two spreads below `WeaponCannon` (WeaponLauncher, WeaponTowCable and
  WeaponGrapplingHook do not), so the call always reaches these bodies.
- x64 sequence before the fix: `maxss, addss, addss` in the spread, then `mulss, addss` in Fire,
  four roundings.
- Inputs that differ: ordinary spreads, for example `(3DADC107 + 3D3C5B81 + 3D8CDEE3) * 3F475682
  + 3E968F72` is `3EE623F3` on x87 and `3EE623F4` on SSE (117 of 446 tool inputs), sums above
  `FLT_MAX` that come back in range (`7F7FFFFF + 7F7FFFFF - 7F7FFFFF` times 0.5 is `7EFFFFFF`, SSE
  `7F800000`) and NaN pairs after an `inf * 0` step.
- Change: `PblX87AddAddMulAddF(a, b, c, d, e)` in `PblX87Math.h` computes `((a + b) + c) * d + e`
  in double and rounds once; every partial result is a normal double or zero (float sums are
  multiples of 2^-149 and `abs(d) >= 2^-23` when nonzero), so each step equals the x87 PC53 step.
  NaN results replay the steps through `PblX87NanStep`. Fire's x64 branch draws the four randoms
  in the x86 order and passes `mStanceSpread`, the kick, the class pitch spread or
  `GetYawSpreadTerm()`, the difference and the pattern angle. `GetYawSpreadTerm` is a new
  non-virtual inline member holding the yaw term that `GetYawSpread` used inline; no vtable,
  layout or signature changes. The x64 branches of `GetYawSpread` and `GetPitchSpread` return
  `PblX87AddAddF` of their operands.
- Proof that x86 does not change: `WeaponCannon.obj` and the three subclass objects are `same`;
  `objcmp.ps1 -All` reports only `NetGame.obj`, whose disassembly is equal after `$SG`
  normalising (`sgcmp`), with equal section sizes and `.data` layout.
- Rendered x86 (D-18): the spreads are sim code; headless and rendered x86 run the same x86
  sequence, so nothing changes.
- Guards: T1 `x87 add add mul add keeps the x86 rounding` (70 cases from the real x87 sequence,
  `cw 027F`); T1s `cannon spread keeps the x86 rounding on x64` and `cannon spread returns keep the
  x86 rounding on x64` pin the source blocks. Rows X87AAMA-X64, X87AAMA-NAN1-X64 to NAN4-X64,
  WC-FIREPITCH-X64, WC-FIREYAW-X64, WC-YAWRET-X64, WC-PITCHRET-X64 and WC-YAWTERM-X64 turn them RED
  on x64. No Win32 row: the x86 text is unchanged.
- Status: Fixed, 48-24, 2026-11-20.

### C-X64-TERRAINRAY

- No digest row moved. No x86 table and no Win32 `hardening.txt` row changed. x86 object code
  does not change.
- Function: `RedTerrain::_RayHit(vStart, vEnd, fA, fB, bAbove)` in
  `src/RedEngineFL/Application/RedTerrain.cpp` (`// FUNCTION: SWBF2 0x006c32f0`), reached through
  `RedTerrain::RayHit`.
- x86 sequence: the first-half call is stored (`fstp [ebp+8]`), compared, and returned as
  `fld [fFirst]; fmul 0.5`. The second-half return is `call; fld 0.5; fmul st(1), st; faddp`, so
  each nested second half adds `* 0.5 + 0.5` on the 53-bit inner value. Leaves return a stored
  float (`fT`, the inlined three-argument `_RayHit`) or `fld1`. `RayHit` returns the ST0 value
  unchanged, and its four callers (EntityTerrain::RayHit, EntityWalker::PostCollisionUpdate twice,
  RedShadowProxy::BuildData) each `fstp` it at once.
- x64 sequence before the fix: every level rounds to float, so a chain of nested second halves
  rounds once per level.
- Inputs that differ: leaves with many significant bits under three or more nested second halves,
  for example leaf `3F555555` at depth 3 gives `3F7AAAAB` on x87 and `3F7AAAAA` on SSE (52 of 576
  tool inputs over depths 1 to 24). The stored `fFirst * 0.5` path is exact on x87 and in double
  and rounds once when stored, as `mulss` does (13 subnormal and edge inputs, no difference).
- Change: the x64 branch of `_RayHit`, inside the subdivide block, returns
  `(float)_RayHitX87(...)`. `_RayHitX87` is a private static inline member that follows the same
  control flow in double: the first half is rounded to the float `fFirst` and compared as x86 does,
  returns `(double)fFirst * 0.5` or `PblX87HalfStepD(inner)`, and at a leaf calls the original
  `_RayHit`, which takes the same leaf path. Recursion depth and `bAbove` updates equal x86's.
  All values are in [0, 1], so each double step equals the x87 PC53 step.
- Proof that x86 does not change: `RedTerrain.obj` disassembly is `same`, and a scratch compile
  shows only `.debug$S` differs. A first version whose twin called the three-argument `_RayHit`
  stopped x86 from inlining it into the five-argument function, although the twin emits nothing
  on x86; the twin now reaches the leaf through the five-argument function.
- Rendered x86 (D-18): the ray chain stays in [0, 1] with no NaN, and rendered x87 at 24 bits
  rounds every step; this change touches x64 only, so rendered x86 is unchanged.
- Guards: T1 `x87 terrain ray recursion keeps the x86 rounding` (123 leaf and depth cases and 13
  half cases from the real x87 loop); T1 `terrain ray recursion keeps the x86 rounding on x64` pins
  the twin and the branch. Rows X87HALF-X64, RT-RAYSTEP-X64, RT-RAYFIRST-X64 and RT-RAYBRANCH-X64
  turn them RED on x64. No Win32 row.
- Status: Fixed, 48-24, 2026-11-20.

### C-MEMPEAKS-X64-PARITY

- No digest row moved. No x86 table, x86 heap size or x64 heap size changed.
- Change (phase 48 D-13): `mem-peaks-x64.txt` is re-captured now that x64 runs the x86
  simulation, and `compare-tables.ps1 -MemPeaksX64` requires every key, pool rows included, to
  be in both `mem-peaks-x64.txt` and `mem-peaks.txt` with the same row count. This closes the
  47-09 workaround of C-HEAPCAP-X64-RULE, whose last point (pool rows only counted) no longer
  holds.
- Captured: `run.ps1 -Platform x64 -Configuration Release -Smoke All -Soak Short
  -HogAddrSpace` on commit 2e0803ec, run folder `artifacts\tests\20261120-073109`: 207 PASS,
  0 FAIL, 0 STALL, 0 CRASH, no `refused size` line. Its `tables\mem-peaks.txt` is pinned
  unchanged as `mem-peaks-x64.txt`.
- Both files have 109 missions and the same 13718 keys; the 12955 pool keys are equal, so the
  three 47-09 pool differences (`dea1g_con` Aimer and MountedTurret, `kas2c_uber` RayRequest)
  are gone.
- Since 49-13 the x64 table is `tests\baseline\v143-x64\mem-peaks.txt`, re-captured after the
  Lua bin re-tune (C-X64-TABLES there).
- Pool block sizes still differ by pointer width for 100 pool types, for example Weapon 448 and
  592, Ordnance 384 and 560, EntitySoldier 4048 and 6112, RayRequest 20 and 40 (x86 and x64);
  blocks per page differ with them. These are layout sizes, not a simulation difference; the
  reader compares keys, not sizes or counts.
- Heap caps: the x64 peaks equal the 47-09 ones except Runtime, 25162848 (kas2c_uber, exit)
  instead of 25157728. ceil(25162848 * 32235168 / 19346544) = 41926281 rounds to 41943040, the
  current cap, so no derived x64 cap moved and `GameMemory.h` is unchanged.
- Guard: the selftest fixture `compare-tables -MemPeaksX64 ... pool keys` passes the pinned pair
  and fails a copy of `mem-peaks-x64.txt` without one pool row, naming its key; the strict reader
  fails the 47-09 table with those three pool keys.
- Status: Fixed, 48-18, 2026-11-20.

### C-HOSTFAULT-CHECKPOINT

- No digest row moved. `hardening.txt` gains two rows, `[PASS] hardening:host-fault-capture` and
  `[PASS] hardening:all-checkpoints`, after `baseline-readers`; no other row moves.
- Cause: two new harness source checks (phase 48, gap plan 48-25, after the one-off x64
  `kas2c_uber` split in the 48-18 branch gate). `run.ps1` writes the count of System log events
  from WHEA-Logger, Kernel-Power 41 and EventLog 6008 at the start and end of each run into
  `run-params.txt` (`whea_before`, `whea_after`, `whea_delta`), and a run with a failure and new
  events gets a `Suspected hardware:` summary line that changes no result, count or exit code.
  `host-fault-capture` runs the capture with a stubbed count and finds the calls in `run.ps1`.
  Every All mission case now runs with the existing `-digesttrace 1500` switch and non-Fatal
  cases require the turn-100 DIGEST TRACE line (digest cases also turn 1500);
  `all-checkpoints` builds the All cases on Win32 and x64 and fails a mission case without them.
  Neither check reads the game or the host event log, so both print the same line on every run.
- No game source, project file or object changed.
- Captured: `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Hardening`, run folder
  `artifacts\tests\20261120-101518`, then `capture-vs2013.ps1 -Tables -HardeningDir`.
- Guards: rows HOSTFAULT-END, HOSTFAULT-NOTE and CHECKPOINT-ALL turn the checks RED.
- Status: re-pinned, 48-25, 2026-11-20.

### C-X86-NO-COMMIT-TABLE

- No digest row moved. No table in this directory changed, and there is no `commit.txt` here.
- Decision (phase 49, plan 49-05): x64 `commit-flat` is also judged against pinned rows in
  `tests\baseline\v143-x64\commit.txt` (C-X64-COMMIT-TABLE there). x86 keeps its self-relative
  rule in this phase: settled `arena` and `rest = gamevm + gamecrt` must not rise above the peak
  of cycles 1-8. No new x86 gating check is added. The row check runs only where the platform's
  baseline folder holds `commit.txt`, so x86 judging is unchanged.
- Status: decided, 49-05, 2026-11-21.

### C-LUA-HIST

- No digest row moved. `lua-hist.txt` is a new table; no other table changed.
- What it counts: the Lua demand per 4-byte size bin. Test builds count every block in
  `Lua_Red_realloc` (the new size, and the old size as freed when the block moves) and in
  `Lua_Red_Free`, whether the block came from a pool or the Lua heap. For each bin
  `(size + 3) >> 2` up to 640 (twice the largest bin of `gShellPoolInfo` and `gGamePoolInfo`),
  plus one `over` row for larger sizes, the table holds the peak number of live blocks and the
  total allocation calls since `LuaMemory::CreatePools`, read from the
  `OPENSWBF2 MEMREPORT luahist` lines of the turn1500 block (rows with no calls are not logged).
- It does not depend on the bin table: the counters see the requested size before a pool is
  chosen, so a change to `gShellPoolInfo` or `gGamePoolInfo` changes `lua-bins.txt` but not
  this table. It is the demand signal a bin re-tune starts from (the spill column of
  `lua-bins.txt` only counts sizes that already have a bin).
- Exact: two Test `-Smoke All -Soak Short` runs at 502047e0 gave byte-identical captures below
  `# commit`: `artifacts\tests\20261121-072049` and `artifacts\tests\20261121-072442`. Both
  runs passed `tables:lua-bins` against the unchanged `lua-bins.txt`.
- No Release object changed: the counting calls are inline no-ops outside Test, and the
  Win32 Release objects compare equal before and after (`coffcmp.py`, 1068 same, 0 diff).
- Guards: rows LUAHIST-FREE and LUAHIST-FREE-X64 break the free-side decrement and turn the
  T1 `lua demand histogram counts peak and calls per bin` RED.
- Status: pinned, 49-07, 2026-11-21.

### C-SAVE-LIGHTUD-WIDTH

- No digest row moved. No table in this directory changed.
- Defect: `LuaSaveItems` wrote a Lua light userdata as `sizeof(void*)` bytes and skipped
  `sizeof(void*)` bytes on read, so an x64 metagame, campaign or mission setup payload was 4
  bytes longer per light userdata than the x86 one, and each build misparsed the other's file.
  The on-disk format is the x86 format (D-12).
- Fix (phase 49, plan 49-09): both builds write the low 32 bits as exactly 4 bytes and skip
  exactly 4 bytes on read; the value is still read back as nil. Every load of a Lua save
  payload goes through `LuaSaveItems::Load`, which logs one Warning line
  `OPENSWBF2 SAVE rejected <type> save "<name>"` when the payload is rejected (D-14).
- x86 bytes are unchanged: `WriteLuaItem` and `ReadLuaItem` disassemble identically before and
  after in the Win32 Test and Release objects. The only changed x86 functions are the new
  `LuaSaveItems::Load`, `LoadLuaItems` and its three callers (two extra arguments).
- Retail `.gc` saves hold no light userdata (7 files decoded in 49-RESEARCH), so retail data
  never hit the defect; a mod script could.
- Proof: the T1 `lua save items match pinned x86 bytes` writes a fixed item set (string and
  number keys only, so traversal order is the same on both builds) and requires the 89 bytes
  the unchanged Win32 build wrote; it failed on x64 before the fix (101 bytes) and passes on
  both after. Guards: LHND-39, LSIW-WRITE-X64, LSIW-SKIP-X64 and LSIW-LOG.
- Status: fixed, 49-09, 2026-11-21.

### C-DIGEST-TRACE-49

- No digest row moved. `digest-trace-dag1c_ctf.txt` and `digest-trace-kas2c_uber.txt` are new
  reference traces; no other file in this directory changed.
- What they hold: the `OPENSWBF2 DIGEST TRACE` lines of turns 1 to 1600 for the Standard
  digest cases `digest-dag1c_ctf` and `digest-kas2c_uber`, written by `capture-vs2013.ps1
  -Collect` with the Standard arguments. Turn 1500 of each equals the state and rand values of
  that case's row in `tests\smoke\digest-expected.txt`.
- Why (phase 49, plan 49-16): every digest smoke case runs with `-digesttrace 1500`, and a
  digest mismatch prints the first turn that differs from `digest-trace-<mission>.txt`. These
  two missions had no pinned trace, so a split in them could not name its turn.
- Exact: two Win32 `-Collect` captures, `artifacts\gate49\p16\collect-1` and `collect-2`, are
  byte-identical in every file, and the x64 capture `artifacts\gate49\p16\collect-x64` gives
  the same bytes for both traces. Every other `-Collect` file equals its pinned copy.
- Reader: `compare-tables.ps1 -Collect`; both files are in `KeyComparedTables`.
- Status: pinned, 49-16, 2026-11-21.

### C-WAVEFORMAT-CBSIZE

- No digest row moved. No table in this directory changed.
- Defect: `DSBuffer::WaveFormat::WaveFormat` (`snddsbuffer.cpp`, 0x00743bb0) set `cbSize = 40`
  for `WAVE_FORMAT_EXTENSIBLE`. DirectSound copies `sizeof(WAVEFORMATEX) + cbSize` = 58 bytes,
  but the struct is a 40-byte `WAVEFORMATEXTENSIBLE` on the stack of `DSBuffer::Create`, so 18
  bytes past it were read. x64 ASan reported it as `stack-buffer-overflow READ of size 58` in
  `DSBuffer::Create` via `DSOUND.dll` (D-16).
- Retail: the retail exe (GOG XPLAY Release, 2017-10-23, SHA-256
  `3bfdb0931885de776126957aa4883b691fc391b58126ff639e36592309d75e90`) stores `mov edx,28h` (40) at 0x00743c24, so the over-read is retail behaviour. The PCM
  branch stores 14 (0x0E); DirectSound ignores `cbSize` for PCM and 18 + 14 fits in the struct,
  so it is unchanged.
- Fix (phase 50, plan 50-04): `cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)`
  (22), the documented value.
- x86 change: one function, the `WaveFormat` constructor. In the Win32 Release and Test
  `snddsbuffer.obj` the only difference is the immediate at +0x77, `mov ecx,28h` to
  `mov ecx,16h`; coffcmp reports one section of the same size, and nothing else moved.
- Guards: rows CBSIZE-PIN and CBSIZE-PIN-X64 restore `cbSize = 40` and turn the T1
  `dsound wave format cbSize matches the struct passed (source pin)` RED.
- Status: fixed, 50-04, 2026-11-22.

### C-DSOUND-LFQUEUE-ASAN

- No digest row moved. No table in this directory changed.
- Defect (Windows, not the game): `dsound.dll` 10.0.26100.1 (x64, SHA-256
  `5aa7fa9e6017b88d19a4d9e99b9e9ecd12cb2bb83e02e137e88bc672909588c0`) keeps its lock-free queues
  (`LFQueuePut`, `LFQueueGet`, the `CGraphManager` command queue) as tagged words
  `(node << 21) + ((seq + 1) & 0xFFFFFF)` and decodes `(hdr >> 21) & 0x7FFFFFFFFF8`, so a node
  pointer keeps 43 bits. The nodes come from `_aligned_malloc` (`CMFRTFifo<Command*>::MemoryAlloc`).
- Under MSVC x64 ASan that call is intercepted. The shadow is `[off, off + 2^44)` and the ASan
  heap lies above it, so every node is at or above 2^44. Its decoded value `node mod 2^43` falls
  in the shadow, which reads as zero. `CGraphManager::ThreadProc` (`DSOUND.dll+0x1466d`) then reads
  a null command and faults on DSOUND's own thread, created in the first `IDirectSoundBuffer::Play`.
- Proof (50-05): a standalone DirectSound `Play` program faults at `DSOUND.dll+0x1466d` 3 of 3 with
  `/fsanitize=address` and exits 0 3 of 3 without it. Under cdb, every node is at or above 2^43 and
  every decode lands in the shadow with ASan. Without ASan, every node is below 2^43 and every
  decode equals the pushed node.
- No ASan runtime option (`help=1`, 14.44), `/HIGHENTROPYVA:NO` or `/MT` keeps the nodes below
  2^43. x86 pointers are 32 bits and are not affected.
- Effect: `boot-sound-on` is excluded from x64 Asan only (`$script:SmokeExcludedPlatform` in
  `tests/smoke/cases.ps1`). Win32 Asan still runs `sound-on`; x64 Release and Test still run
  `boot-sound-on` and `sound-on`.
- Owner: replacing the DirectSound output (BACKLOG Tier 5: audio, XAudio2 or WASAPI). Phase 52
  HARDX-02 (x64 Asan Standard) is also affected.
- Status: carried, 50-05, 2026-11-22.

### C-FFMPEG-VENDOR-SCOPE

- No digest row moved. No table in this directory changed.
- What: `src/FFmpegFL/` holds the Bink-only subset of FFmpeg 9.0.2 (179 `.c`, the headers they
  include and the generated config files), written by `tools/update-ffmpeg.ps1` and checked with
  its `-Verify` mode. It is upstream vendor code: the files stay byte for byte as in the release
  tarball plus the recorded patch (`src/FFmpegFL/changes.diff`).
- Cause: the game-code source rules (allocator bans such as `crt-heap-ban`, handle and spacer
  rules, logging through `RedLog.h`) describe our own code and the recovered game. They don't
  apply to upstream FFmpeg, which calls the CRT heap names inside `libavutil/mem.c` by design.
  So `src\FFmpegFL\` gets the same exclusion as `src\LuaFL\`, `src\ZlibFL\` and `src\mimalloc\`:
  the vendor regex in `Get-X64SizeLiteralCheck` and `Get-SourceChecks` (`tests/smoke.ps1`) and
  the `redlog-vendor` directory list (`tests/run.ps1`), which also keeps `RedLog.h` out of it.
- Our own file inside the tree (`src/FFmpegFL/openswbf2/ossf2_alloc.c`, from 51-02) only forwards
  to hooks the exe registers. The source exclusion covers it too, so the binary check
  `ffmpeg-crt-heap` (51-03: the DLL imports nothing from `api-ms-win-crt-heap-l1-1-0.dll`) binds
  it instead.
- Proof: the `crt-heap-ban` selftest fixture puts the same `_aligned_malloc` line under
  `src\FFmpegFL\` and `src\RedEngineFL\`; only the `RedEngineFL` file is reported. With the
  regex reverted it fails (51-01).
- Status: in force, 51-01, 2026-11-23.

### C-FFMPEG-ALLOC-CONFIG

- No digest row moved. No table in this directory changed.
- What: `tools/update-ffmpeg.ps1` edits each generated `config.h` (`src/FFmpegFL/config/Win32/`
  and `x64/`): `HAVE_ALIGNED_MALLOC 1 -> 0` and `HAVE_POSIX_MEMALIGN 0 -> 1`. The edit is in
  `src/FFmpegFL/changes.diff`.
- Cause: configure is run with `--malloc-prefix=ossf2_`, which renames `malloc`, `realloc`,
  `free`, `memalign` and `posix_memalign` in `libavutil/mem.c`. On MSVC configure also finds
  `_aligned_malloc` and sets `HAVE_ALIGNED_MALLOC 1`, and `mem.c` then calls `_aligned_malloc`,
  `_aligned_realloc` and `_aligned_free` unprefixed, so the prefix routes nothing (measured in
  51-RESEARCH: those three are the DLL's only CRT heap imports). `HAVE_POSIX_MEMALIGN` is 0 only
  because configure can't link the prefixed name. With the edit, `av_malloc` calls
  `ossf2_posix_memalign(&p, ALIGN, size)`, `av_realloc` calls `ossf2_realloc` and `av_free` calls
  `ossf2_free`, which forward to the Red heap that is current at call time, the heap
  `binkw32` used through `BinkSetMemory` (D-15).
- `ALIGN` is 16 with asm off, and Red heap user pointers are 16-aligned (`kHeapAlignment` 0x10).
- Guard: the script fails unless configure generated exactly `HAVE_ALIGNED_MALLOC 1` and
  `HAVE_POSIX_MEMALIGN 0`, naming the value it found, so the edit can't go stale on an update.
- Status: in force, 51-01, 2026-11-23.

### C-FFMPEG-WD4146

- No digest row moved. No table in this directory changed.
- What: `src/FFmpegFL/FFmpegFL.vcxproj` adds `/wd4146` to the `AdditionalOptions` of every
  FFmpegFL C source. MSBuild emits `AdditionalOptions` after `/sdl`, and the cl tlog shows
  `/sdl` before `/wd4146` on every FFmpegFL line.
- Cause: upstream applies unary minus to unsigned values by design (`libavutil/intmath.h:120,136`,
  pulled into about 104 files, `libavcodec/bink.c` x8, `libavutil/crc.c`,
  `libavutil/mathematics.c`, `libavcodec/golomb.h`, `libavcodec/dirac.c`). `/sdl` turns C4146
  into an error, 229 hits. Upstream's own MSVC flags disable it (`-wd4146`). The order matters:
  `/sdl /wd4146` disables it and `/wd4146 /sdl` doesn't, so a wrong order fails the build.
- Every other warning stays enabled, and `/sdl`, `/GS` and every other props flag stay on.
- Status: in force, 51-02, 2026-11-23.

### C-FFMPEG-WD4703

- No digest row moved. No table in this directory changed.
- What: `/wd4703` per file on five FFmpegFL sources, and on no other file:
  `libavutil/encryption_info.c` (275, `info`), `libavformat/url.c` (294, `base_path_end`),
  `libavformat/avio.c` (179, `key`), `libavformat/av1.c` (491, `seq`) and `libavcodec/raw.c`
  (94, `tags`).
- Cause: `/sdl` turns C4703 (potentially uninitialized local pointer) into an error, and all five
  are false positives:
  - `encryption_info.c`: `info` is set on the first loop pass (`i == 0`) before any read.
  - `url.c`: `base_path_end` is set under `if (use_base_path)` (273-274), and every read is
    under `if (use_base_path)` too (293-294, 304-305).
  - `avio.c`: `p != key` is read only when `ret >= 0`, and then the loop condition has assigned
    `key`.
  - `av1.c`: `seq` is read only when `write_seq_header` is set, after the `!nb_seq` check, and
    `nb_seq > 1` fails earlier, so the one sequence header assigned it.
  - `raw.c`: the switch in `avpriv_pix_fmt_find` covers every `PixelFormatTagLists` enumerator,
    and its only caller in the subset (`libavformat/demux.c:3014`) passes `PIX_FMT_LIST_RAW`.
- 51-RESEARCH named only the first two. 51-02 compiled each of the 179 subset sources alone
  with `/sdl` at `/O2` and `/Od` on x86 and x64 and found these five on every combination.
  An MSBuild compile stops at the first failing file of its `/MP` batch (51-02 saw `raw.c`,
  then `av1.c`, one per build), so a survey read from build output undercounts.
- Status: in force, 51-02, 2026-11-23.

### C-FFMPEG-DEBUG-O1

- No digest row moved. No table in this directory changed.
- What: in Debug only, `libavcodec/avcodec.c` compiles with `Optimization` `MinSpace` (`/O1`);
  every other Debug FFmpegFL file stays at `/Od`.
- Cause: `avcodec.c` calls `ff_thread_init`, `ff_thread_flush` and `ff_thread_free` under
  `if (HAVE_THREADS && ...)`. With threads off (`HAVE_THREADS 0`) those functions aren't in the
  subset, and at `/Od` the dead calls aren't eliminated, so the Debug link has three unresolved
  symbols. Release, Test and Asan build at `/O2` and need nothing.
- Status: in force, 51-02, 2026-11-23.

### C-FFMPEG-BCRYPT

- No digest row moved. No table in this directory changed.
- What: `openswbf2-ffmpeg.dll` imports `bcrypt.dll` (`BCryptOpenAlgorithmProvider`,
  `BCryptGenRandom`, `BCryptCloseAlgorithmProvider`) in every configuration on both platforms,
  and links `bcrypt.lib`.
- Cause: a direct call chain from an export the decoder needs. `avformat_alloc_context`
  (`libavformat/options.c:180`) calls `av_opt_set_defaults`, then `av_opt_set_defaults2`, which
  calls `set_string_color` for colour options (`libavutil/opt.c:1732`), which calls
  `av_parse_color` (`opt.c:533`), whose `"random"` colour calls `av_get_random_seed`
  (`libavutil/parseutils.c:381`), which calls the three `BCrypt*` functions
  (`libavutil/random_seed.c`). `avformat_open_input` reaches `set_string_color` too, through
  `av_opt_set_dict` and `av_opt_set`. These are static calls, so `/OPT:REF` can't drop them while
  `avformat_alloc_context` is exported.
- Evidence (51-02): every DLL links with `/OPT:REF /OPT:ICF` and `/Gy`. The `/VERBOSE:REF` link
  doesn't discard `av_get_random_seed`, `av_parse_color`, `set_string_color` or
  `av_opt_set_defaults2`. Measured links that export only `avformat_alloc_context`,
  `avformat_open_input`, `avcodec_open2` or `avutil_version` each import the three `BCrypt*`
  names too. The other caller, `av_aes_ctr_set_random_iv`, is discarded.
- Runtime: no Bink option is a colour, so the `"random"` branch never runs; the import is static.
- Status: in force, 51-02, 2026-11-23.

### C-FFMPEG-HARDENING-ROWS

- No digest row moved. `hardening.txt` gains five rows, `[PASS] hardening:ffmpeg-license`,
  `ffmpeg-flags`, `ffmpeg-dll`, `ffmpeg-imports` and `ffmpeg-crt-heap`, after `aslr-pe-tests`; no
  other row moves.
- Cause: a new binary ships. `openswbf2-ffmpeg.dll` comes from its own DLL project, which
  `Get-GameProjects` doesn't scan, so `run.ps1` holds it to its own rows on both platforms:
  - `ffmpeg-license`: the committed `config.h` and `config_components.h` set no `CONFIG_GPL`,
    `CONFIG_NONFREE`, `CONFIG_GPLV3`, `CONFIG_VERSION3` or `CONFIG_LGPLV3`, the configure line has
    no `--enable-gpl`, `--enable-nonfree` or `--enable-version3`, and no component outside the four
    Bink ones is enabled. The `CheckFFmpegLicence` target in `FFmpegFL.vcxproj` applies the same
    rules and fails the build.
  - `ffmpeg-flags`: every line of the FFmpegFL cl tlog has `/sdl`, `/GS`, `/Gy` and `/wd4146` after
    `/sdl` (C-FFMPEG-WD4146); `/wd4703` is on exactly the five C-FFMPEG-WD4703 files and no other
    `/wd` appears; `/O1` is on `avcodec.c` in Debug only (C-FFMPEG-DEBUG-O1), every other Debug
    line has `/Od` and every other configuration `/O2`; no `/GS-`, `/GL`, `/fp:fast`,
    `/fp:contract` or `/Zc:throwingNew`; on x64 every line has `/we4311 /we4312 /we4302 /we4477`.
  - `ffmpeg-dll`: the DLL has Dynamic base, NX compatible, relocations, a security cookie and, on
    Win32, a SafeSEH table.
  - `ffmpeg-imports`: the import modules are KERNEL32, VCRUNTIME140 and the api-ms CRT sets
    runtime, math, stdio, string, time, convert, environment and utility, plus `bcrypt.dll`
    (C-FFMPEG-BCRYPT) and, in Asan only, the C-FFMPEG-ASAN-IMPORTS modules.
  - `ffmpeg-crt-heap`: nothing from `api-ms-win-crt-heap-l1-1-0.dll` and no `malloc`, `realloc`,
    `free`, `calloc` or `_aligned_*` from any module (C-FFMPEG-ALLOC-CONFIG,
    C-FFMPEG-VENDOR-SCOPE); in Asan, only the heap names `vcasan.obj` imports
    (C-FFMPEG-ASAN-IMPORTS).
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke None -Hardening`
  with `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch
  `-OutDir`; the capture equals the pinned file plus those five lines, byte for byte.
- Status: in force, 51-03, 2026-11-23.

### C-FFMPEG-ASAN-IMPORTS

- No digest row moved. No table in this directory changed.
- What: the Asan `openswbf2-ffmpeg.dll` imports `clang_rt.asan_dynamic-i386.dll` on Win32 and
  `clang_rt.asan_dynamic-x86_64.dll` on x64, and `calloc`, `free` and `malloc` from
  `api-ms-win-crt-heap-l1-1-0.dll`. The x64 Asan DLL also imports `__CxxFrameHandler4` from
  `VCRUNTIME140_1.dll`. The Debug, Release and Test DLLs import none of these.
- Cause: `/fsanitize=address` links MSVC's ASan support library `vcasan.lib` into every image,
  the Asan `BattlefrontII.exe` too. Its `vcasan.obj` has `UNDEF` `__imp_calloc`, `__imp_free` and
  `__imp_malloc` (`__imp__` on x86) and, on x64, `__CxxFrameHandler4`. The 51-02 `/VERBOSE` link
  shows `__imp__calloc` and `__imp__malloc` referenced only from `vcasan.obj`, and `__imp__free`
  from `vcasan.obj` and `FILE_OPEN.OBJ`, whose only caller `ff_tempfile` `/OPT:REF` discards.
  FFmpegFL is C and calls none of them, and the same sources import none of them in Release and
  Test.
- Check: in Asan, `ffmpeg-crt-heap` reads the linked `vcasan.lib` path from the FFmpegFL link
  tlog, lists its `UNDEF __imp_` heap names with dumpbin, and fails when the DLL imports a heap
  name outside that list or when the list can't be read. `ffmpeg-imports` allows the three
  modules only in Asan.
- Status: in force, 51-03, 2026-11-23.

### C-MOVIE-GOLDENS

- No digest row moved. No table in this directory changed; `movie-goldens.txt` is new.
- What: `movie-goldens.txt` archives the binkw32 frame hashes and the deterministic binkw32 facts
  that the FFmpeg player is checked against (D-10, D-11). It holds the FNV-1a 64 hash of the
  B, G and R bytes of every decoded frame of three retail segments: `shell.mvs` 0xB4A21ED9
  (1349 frames, 48 kHz), `ingame.mvs` 0x2AB9757F (1369 frames, 48 kHz, the segment `nab2g_c`
  plays) and `ingame.mvs` 0x0B78EBBC (223 frames, 44.1 kHz), 2941 lines in all. A check set
  repeats frames 1, 2, 3, 30, 300 and the last frame of each segment; frames 1 to 3 and the last
  frame are black fades, so frames 30 and 300 are the content pins. The facts are the texture
  format, size, frame rate, audio layout, Bink heap use, the `BinkSetVolume` arguments and the
  notify sequence of one shell loop.
- Cause: binkw32 is deleted in 51-15, so its output has to be captured before the switch. The
  hashes come from the Test-only `-movietest` path (Win32 Test, `BinkCopyToBuffer` into the
  `D3DFMT_X8R8G8B8` texture, hashed before `UnlockRect`); two runs per segment gave byte-identical
  frame lines, frame numbers ran 1 to Frames with no gap, and a `-nosound` run gave the same
  hashes. The file holds no retail bytes, only hashes and counts; the raw frames stay local.
- Check: the T1 `baseline movie goldens hold every binkw32 frame of the three segments and pin the
  content frames` (`tests/unit/memory_baseline.cpp`) reads the file.
- Colour (51-05): the `## colour` section records `model=exact`. binkw32's X8R8G8B8 blitter is an
  MMX routine with nearest 2x2 chroma and fixed `pmulhw` constants; that model applied to the
  YUV420P frames FFmpeg decodes from the same segment bytes gives all 2941 archived hashes, with
  both the Win32 and the x64 `openswbf2-ffmpeg.dll`, and every pixel of the 17 local check-set
  frames. So FFmpeg's YUV equals Bink's own and no error bound is needed. The same T1 pins the
  model line and the match count.
- Status: in force, 51-04, 2026-11-23; colour section 51-05, 2026-11-23.

### C-MOVIE-INGAME-DIGEST

- No digest row moved. `digest-expected.txt` and `smoke-all-digests.txt` are unchanged; the
  headless `nab2g_c` row never opens a decoder (`-norender` turns movies off).
- What: the `## binkw32 ingame digest` section of `movie-goldens.txt` holds one line,
  `ingame-movie-digest` and the `OPENSWBF2 SIM DIGEST` line that the rendered fixed-rate
  `nab2g_c` run gives with binkw32 (MOVIE-09). The Test case `ingame-movie-digest` names it with
  `GoldenDigest`, and the smoke judge requires its digest line to equal it. The FFmpeg player
  must give the same line after the switch.
- Cause: before 51-06 two binkw32 runs of the case gave different digests. The first differing
  turn was turn 1 and the only differing field was the mission time; characters and both random
  seeds were equal. Under `-fixedrate` every frame advances the game clock by 1/30 s, and while
  the in-game movie plays the sim is paused but frames run unthrottled, so the movie's length in
  frames set the mission time. That length came from wall time in two places in
  `pcRedMoviePlayer.cpp`: the Bink open waited for the async read buffer (open frame 51 to 63),
  and each frame step waited for `BinkWait`, Bink's real-time clock (close frame 164613 to
  171343). Under fixed rate the player now fills the read buffer before the open and steps a
  frame when the game's system clock reaches it, the clock `StopInGameMoviePlay` already uses
  under fixed rate. Without fixed rate it calls `BinkWait` as before.
- Measured after the fix: two sound-on runs and one `-nosound` run gave the same digest line
  and the same 1500-turn trace; the movie opens on frame 6 and closes on frame 1377 in each,
  and frames 30 and 300 keep their archived hashes.
- Check: the case itself, and two guard rows that turn it RED with a moved mission time:
  `MOVIE-FIXED-CLOCK` restores `BinkWait` under fixed rate, and `MOVIE-FIXED-OPEN` drops the
  wait for a full read buffer before the open. The movie goldens T1 pins the section's shape.
- Status: in force, 51-06, 2026-11-23.

### C-MOVIE-HEADER-CAPS

- No digest row moved. No table in this directory changed; the header check is not called by the
  game yet (51-12 wires it in front of the decoder).
- What: `CheckMovieHeader` (`src/RedEngineFL/movie/pc/pcRedMovieHeader.cpp`) reads a Bink
  header and its whole frame index from bytes and rejects a stream with a named reason before
  FFmpeg sees a byte (D-18). The caps and their retail values (MOVIE-10 measured set, 24 `.mvs`,
  281 streams, all BIKi 800x600, one track):

  | Cap | Value | Retail max |
  |-----|-------|------------|
  | width | 1024 | 800 |
  | height | 768 | 600 |
  | frames | 16000 (32000 until 51-17) | 7314 (`trailer.mvs` 0x816FEDE0) |
  | largest frame and every index span | 262144 (0x40000) | 135192 (`shell*.mvs` 0xA62393D8) |
  | tracks | 6 | 1 |
  | rate | 8000 to 48000 | 44100 (12 streams) and 48000 (269) |
  | audio flags | only 0x4000, 0x2000, 0x1000 (`BINK_AUD_*`) | 0x7000 |
  | revision | b, f, g, h, i, k (FFmpeg `bink.c` probe for BIK) | i |
  | frame rate | 1 to 120 fps (`fpsNum >= fpsDen` and `fpsNum <= 120 x fpsDen`, 64-bit) | 29.97 (5000000/166833, 261 streams; 10000000/333667, 8) and 30.0 (10000000/333333, 12) |
  | largest frame against the file | largest <= file size + 8 | below its own file size in all 281 (a frame lies inside its file) |
  | every index span against the tracks | span >= 4 x tracks | 124 (1 track) |

- Heap budget (D-15 routes every FFmpeg allocation to the Red heap the movie opens from; every
  caller opens on Runtime). The smallest headroom is x86: Runtime cap 32,235,168 B minus the
  largest Runtime peak of any measured mission, 19,332,704 B (`digest-kas2c_uber`, Win32 Test
  gate 20261123-092349), leaves 12,902,464 B. x64 leaves 41,943,040 - 25,162,848 = 16,780,192 B.
- Decoder cost, measured with a counting allocator on the in-tree `openswbf2-ffmpeg.dll`
  (Release, video and audio decoders open, every packet decoded) on retail 800x600 streams:
  peak 6,495,684 B x86 and 6,499,552 B x64 on 0xA62393D8 (254 frames, largest 135192),
  6,570,507 B x86 on 0x816FEDE0 (7314 frames). From those runs: 10 B per pixel of bundles
  (`init_bundles`, 10 x 64 B per 8x8 block), about 3.075 B per pixel of decoded frames,
  24.65 B per frame of index (`AVIndexEntry`), one packet of the largest frame and about
  78 KB of fixed state.
- At the caps, with the index doubled for a realloc copy and the packet doubled for the
  decoder's own reference: 77,829 + 13.075 x 786,432 (10,282,598) + 49.3 x 16,000 (788,800)
  + 2 x 262,144 (524,288) + the 0x40000 read ring and its pointer arrays (262,240) + the
  decoder's audio conversion block, 2048 frames x 2 channels x 2 B (8,192) + `MovieAudio`'s
  packets and hold, (16 x 4800 + 48000) x 2 channels x 2 B (499,200) = 12,443,147 B, inside the
  x86 headroom of 12,902,464 B with 459,317 B to spare. A 1280x720 cap (921,600 pixels) adds
  1.77 MB and does not fit.
- Audio (51-17, review CR-01): the decoder kept every sample of a take in a buffer that grew up
  to `kMovieAllocCap`, so one hostile frame of minimal DCT blocks could hold 11.8 MB of Runtime at
  the realloc peak, and neither that buffer nor `MovieAudio`'s 499,200 B block was in this budget.
  Both had to be counted: with 32000 frames the caps came to 12,724,555 + 499,200 = 13,223,755 B
  before any decoder buffer, 321,291 B over. The decoder now converts each decoded audio frame
  (Bink gives at most 1920 sample frames per frame) into one fixed block allocated at open and
  hands it straight to `MovieAudio::Feed`, which keeps what it holds and counts the rest as
  dropped, so audio costs a constant 8,192 B in the decoder. The frame cap went from 32000 to
  16000, still above twice the retail maximum of 7314, to make room for `MovieAudio`'s block.
  The largest retail take is 36,480 sample frames in one frame (`crawl.mvs` 0x72EA78A3 frame 0,
  measured over all 281 streams), the same samples now reach the player in the same order, and
  no frame pin, golden or digest moved. `movie header caps fit the x86 runtime heap budget`
  (`tests/unit/movie_decoder.cpp`) recomputes this sum from the cap constants and fails above the
  headroom, so a cap can't move without the budget.
- Textures: the player's 3 `X8R8G8B8` textures are `D3DPOOL_MANAGED` and are created by D3D
  (`pcRedTexture.cpp` `pcInit`), outside the Red arena, so they are not in the heap budget.
  At the caps they take 3 x 1024 x 768 x 4 = 9,437,184 B of managed memory, against 5,760,000 B
  for retail. Counted against the heap as well, even retail `trailer.mvs` would not fit the x86
  headroom above, so that model is wrong rather than the caps.
- Frames: the header plus the whole index has to fit the first read the player makes, 0x20000
  bytes (`RedMovie::Open` default read size, used by every caller). At 6 tracks and BIKk the
  header end is 44 + 4 + 72 + 4 x (16000 + 1) = 64,124 B, inside 131,072.
- Largest frame: 0x40000 is the player's read ring (`RedMovie::Open` default buffer size), so
  one whole frame always fits the ring.
- Tracks: retail ships one locale per file (`crawl`, `crawluk`, `crawlfr`, `crawlgr`,
  `crawlit`, `crawlsp`), so a single stream carrying every locale holds 6 tracks.
- Rate: 48000 Hz is the Snd engine's mix rate (`sndsoftoutput.cpp`), and 8000 Hz is the lowest
  standard rate; a zero rate would divide the audio clock by zero.
- Index: the first entry with bit 0 cleared must equal the header end, 44 + 12 x tracks +
  4 x (frames + 1), plus 4 for BIKk; it holds in all 281 retail streams and removes the
  demuxer's only seek. Entries must rise strictly, each span at most the largest cap, and the
  last entry must equal the file size + 8 with bit 0 cleared, because FFmpeg ends the last frame
  at the file size rather than at the last entry. All 281 retail streams have it equal. The
  file size + 8 must not exceed the segment length.
- Check: `tests/unit/movie_header.cpp` (retail shapes pass, each cap passes, each field one past
  its cap rejects with its reason, every truncation returns NeedMore at a guard page), the
  guard rows `MOVIE-HDR-*` and their `-X64` twins, and the MOVIE-10 T2
  `tests/unit/movie_retail.cpp`, which fails on any retail stream outside the measured set.
- Frame rate (51-17): the retail set has three rates, measured over all 281 streams with
  `artifacts\gate51\p17\scan_audio.py` (the 51-07 note of one rate was wrong). Zero was the only
  rate rejected before, so a rate of 1/0xFFFFFFFF fps passed and froze playback: the shell
  background never advanced and an in-game movie stayed in state 3. Below 1 fps a frame is held
  longer than a second, which no movie needs; above 120 fps a frame is due faster than any
  display shows it, and the player decodes at most one frame per update. Both bounds are
  compared as 64-bit products, so `120 x fpsDen` can't wrap.
- Clock arithmetic (51-17): `MovieFrameDue` and `MovieFrameDueTicks` (`pcRedMovieClock.h`)
  compare exact 128-bit products built by `MovieMulWide`. `(frame - 1) x fpsDen` is a product of
  two 32-bit values and fits 64 bits for any header; times the 32-bit sample rate or the 64-bit
  QPC frequency it fits 128 bits, as does the 64-bit played count or tick count times the 32-bit
  `fpsNum`. So no compare wraps at any header value or counter frequency. Before, a 10 MHz
  counter, 32000 frames (the cap then) and an `fpsDen` near 2^32 gave about 1.4e21 and wrapped. The timer
  fallback now restarts from the frame it starts at (`muTimerFrame`) instead of rebasing with a
  division of the same product.
- Largest frame against the file (51-17): FFmpeg's `read_header` rejects `largest frame size
  greater than file size`; the check now rejects it as `largest` too, so an accepted header is
  one FFmpeg's `read_header` accepts.
- Index span against the tracks (51-17): every Bink frame starts with one 4-byte audio size per
  track, and upstream `read_packet` tests `audio_size > remain_packet_size - 4` in `uint32_t`,
  which wraps on a shorter frame. The check rejects a span below `4 x tracks` as `order`.
- Status: in force, 51-07, 2026-11-23; frame rate, largest and span bounds added 51-17,
  2026-11-24.

### C-MOVIE-FRAMES

- No digest row moved. No table in this directory changed; `movie-frames.txt` is new.
- What: `movie-frames.txt` pins the FFmpeg path's frames exactly (D-11). Its `## ffmpeg frames`
  section holds the FNV-1a 64 hash of the B, G and R bytes of every frame of the three
  `movie-goldens.txt` segments, 2941 lines, decoded by `MovieDecoder` from block reads of the
  `.mvs` and converted by `ConvertYuv420ToXrgb` into a pitch-padded buffer.
- Cause: the FFmpeg decoder replaces binkw32, so its own output needs an exact pin that does not
  depend on binkw32 staying in the tree. Two Win32 Test runs gave byte-identical hashes and the
  x64 Test run gave the same bytes. The path is integer-only (FFmpeg's Bink decoder with asm off
  and the integer converter), so one table is shared by both platforms and there is no
  `v143-x64` sibling. Under the exact colour model (`model=exact` in `movie-goldens.txt`) every
  line equals the binkw32 hash of the same frame.
- Check: the data-gated T2 `movie frame hashes match the pinned FFmpeg path`
  (`tests/unit/movie_retail.cpp`) decodes the three segments on Win32 and x64 and requires every
  hash to equal its pin and, under `model=exact`, its binkw32 golden. The guard row
  `MOVIE-FRAME-SWAPUV` and its `-X64` twin swap the decoder's U and V planes and turn it RED.
- Status: in force, 51-10, 2026-11-23.

### C-FFMPEG-TX-LEAK

- No digest row moved. No table in this directory changed.
- What: `src/FFmpegFL/patches/tx-subtx-free.patch`, a second local FFmpeg patch, applied by
  `tools/update-ffmpeg.ps1` after the upstream bink fix and pinned by its SHA-256
  (`$TxPatchSha256`). In `libavutil/tx.c` `ff_tx_init_subtx`, when no codelet initialised
  (`!s->nb_sub`), it frees `s->sub[0].sub` before `s->sub`. It is not upstream: FFmpeg master
  has the same code.
- Cause: a codelet init such as `ff_tx_dct_init` or `ff_tx_rdft_init` first calls
  `ff_tx_init_subtx` on its own context, which allocates that context's `sub` array, and can
  then fail its own `exp` allocation. The parent's failure path `reset_ctx(sctx, 0)` resets the
  children but keeps `sctx->sub` for the next attempt, so when no attempt succeeds the array is
  lost when `s->sub` is freed. Measured in 51-12 by failing each FFmpeg allocation of a
  `binkaudio_dct` decoder open in turn: failing allocation 71 (`ff_tx_rdft_init_float_c`,
  `tx_template.c:1622`) or 72 (`ff_tx_dct_init_float_c`, `tx_template.c:1853`) rejected the open
  and left one 416-byte block, `TX_MAX_SUB` (4) x `sizeof(AVTXContext)` (104 on x86), allocated
  at 67 or 64 by the nested `ff_tx_init_subtx`. A hostile or oversized movie under memory
  pressure could repeat it on every open.
- Check: the T1s in `tests/unit/movie_oom.cpp` fail every FFmpeg allocation of a decoder open and
  of a decode in turn and require zero bytes left after `Close` at every point. The guard row
  `MOVIE-TX-LEAK` and its `-X64` twin drop the added free and turn them RED.
- Status: in force, 51-12, 2026-11-23.

### C-BINK-GONE-ROW

- No digest row moved. `hardening.txt` gains one row, `[PASS] hardening:bink-gone`, after
  `unicows-staged`; no other row moves.
- Cause: a new check (MOVIE-07). binkw32 is deleted: `lib\Win32\binkw32.lib`, its link input,
  its `GameDataFile` staging, the `BINK`, `BINKFILE` and `BINKIO` mirrors and the Test switch
  `-moviedump` are gone, and `StageVendor` deletes a stale `binkw32.dll` from the output folder.
  `bink-gone` keeps it out on both platforms. It fails on `binkw32.dll` or `binkw64.dll` in either
  import section of the exe, a `binkw32.dll` in the output folder, a `binkw*` file under `lib`, a
  `.vcxproj`, `.vcxproj.filters` or `.props` element other than a `Delete` that names binkw32, a
  `StageVendor` without the `Delete` of `$(OutDir)binkw32.dll`, binkw32 in `tests\smoke\cases.ps1`
  or `tests\common.ps1` or in either platform's worker file list, and a `Bink<Upper>...(` call or
  a `BINK`, `BINKIO` or `BINKFILE` identifier in `src` (outside `LuaFL`, `ZlibFL`, `mimalloc` and
  `FFmpegFL`) or `tests\unit`, and (51-17) a `binkw32` or `binkw64` name in `src` outside
  those trees, which catches a run-time `LoadLibrary` the import checks can not see. The FFmpeg
  Bink decoder and demuxer keep their format name, and
  `tests\baseline\census` and the `## binkw32 ...` sections of `movie-goldens.txt` are historical
  records the check does not read. Nothing names binkw32, so the row passes and the hardening
  output has one more line.
- Captured from `run.ps1 -Platform Win32 -Configuration Release -NoBuild -Smoke None -Hardening`
  with `capture-vs2013.ps1 -NoBuild -Tables -Baseline v143 -HardeningDir` into a scratch
  `-OutDir`; the capture equals the pinned file plus that one line, byte for byte.
- Status: re-pinned under MOVIE-07, 51-15, 2026-11-24.

### C-FFMPEG-CONFIG-FIXED

- No digest row moved. No table in this directory changed.
- What: `tools/update-ffmpeg.ps1` (`Edit-ConfigH`) writes `CONFIG_THIS_YEAR 2026` and the
  `CC_IDENT` of cl 19.44.35230 for the platform ("... for x86" or "... for x64") into each
  generated `config.h` in place of the values configure produced, and records the edit in
  `src/FFmpegFL/UPSTREAM.md`; `changes.diff` holds the diff against configure's output.
- Cause: configure writes the current year (`date`) and the installed compiler's banner into
  `config.h`, and `-Verify` byte-compares the regenerated file. So from 2027-01-01, or after any
  Visual Studio update, `-Verify` failed although the subset was byte-exact, a verdict that
  depended on the calendar and on another installed program. No compiled source reads either
  name (a grep of the vendored `.c` and `.h` files finds neither; only `fftools`, which is not
  vendored, reads them), so the fixed values change no compiled byte.
- Guard: the script fails unless configure generated exactly one definition of each, in the
  expected form (four digits; the cl banner for x86 or x64), and fails if any vendored `.c` or
  `.h` uses either name, so the "unused" fact stays proven on every update.
- Check: a selftest fixture runs `Edit-ConfigH` on synthetic `config.h` files with another year
  and another cl version and gets the committed bytes, and shows that the subset compare passes
  on equal trees and fails on a one-byte change.
- Status: in force, 51-17, 2026-11-24.

### C-MOVIE-HWGAIN

- No digest row moved. No table in this directory changed.
- What: `MovieAudioRenderer::UpdateObject` (`pcRedMovieAudio.cpp`) sets `DSBufferRenderer` flag
  0x10 before every call of the base `UpdateObject`, so the base applies the movie gain as
  DirectSound volume (19004 -> -473 mB, Bink's level, 51-11).
- Cause: no retail code sets flag 0x10, and `DSBufferRenderer::Flush` keeps only bit 8
  (`mFlags & 8u`), so every flush clears it; the subclass sets it again on the sound thread
  before each tick. Within the tick whose stopped-state `Flush` clears it, and in `SetFormat`
  during `Open` (which resets the DirectSound volume to full), the renderer briefly runs in the
  other gain mode, and the next tick restores it. The design is correct only by that ordering,
  so it is pinned.
- Played count: bytes are counted only when the tick did not start with a desired stop, because
  a stop `Flush` zeroes `mLeftToPlay` without playing it. A cursor that advances more than one
  whole buffer between ticks is counted modulo the buffer size, the same arithmetic the base
  uses for `mLeftToPlay`: DirectSound exposes only the cursor, and with the 500 ms buffer the
  case needs the sound thread to stall for more than half a second.
- Check: `tests/unit/movie_audio.cpp` (a source pin that the flag is set before the base call
  and that `Flush` keeps only bit 8; the played step across a wrap, at the end and on a stop),
  with the guard rows `MOVIE-AUDIO-HWGAIN` and `MOVIE-AUDIO-STOPCOUNT` and their `-X64` twins.
- Status: in force, 51-17, 2026-11-24.

### C-X87-INVENTORY-RESYNC

- No digest row moved. `x87-inventory.tsv` gains three rows, all `match:exact` from the rule:
  `NetGame.obj ?FinishSend@@...`, `NetGame.obj ?Remove@@YAXPAVNetObj@@_N@Z` and
  `pcRedMovieConvert.obj ?ConvertAudioToS16@@...`. No other row moved.
- Cause, FinishSend and Remove: `x87-inventory.ps1` took function boundaries from the labels of
  dumpbin's linear disassembly. MSVC places a switch jump table after the function's code, and
  `NetGame.cpp` builds with optimization off. When the table's bytes decode into an instruction
  that runs past the next function's first byte, dumpbin loses sync, prints no label for that
  function, and the inventory dropped its code (the predecessor's walk stops at its own table).
  On main (e2577761) `FinishSend` (0x0061D240, after `AddSend`'s table), `Remove`,
  `SentStats`, `GetLastObjState` and `SpawnDisplay::Read` started inside a decoded instruction;
  the 51 code moved `NetGame.obj` by 0x460, so `FinishSend` and `SentStats` came back and the
  branch gate's `x87-inventory` step failed with `new ... FinishSend`. Both functions are
  unchanged since main; `FinishSend` stores `GetMaxHealth()` and `Remove` stores one float
  return, each a single `fstp`.
- Fix: the inventory decodes again with `dumpbin /range` from every map function (`f`) start
  that is not an instruction boundary of the linear decode, so the rows no longer depend on
  layout; `-Disasm` has nothing to decode again with and fails naming the function. With the
  fix, main gains exactly the `FinishSend` and `Remove` rows and main and 51 differ by one row.
- Cause, ConvertAudioToS16: new 51 code (`pcRedMovieConvert.cpp`, the decoder's float to S16
  conversion); its x87 use is loads and stores only.
- Check: a selftest fixture feeds a disassembly whose map start falls inside a decoded
  instruction and requires the failure; the 51-17 tool exits 0 on it and folds the lost
  function's `fmul`/`fstp` into its predecessor. Regenerating with `-Out` over the pinned file
  gives the same bytes, and `-Check` passes on the Release exe.
- Status: re-pinned, 51-18, 2026-11-24.

## Policy

The v143 files in `tests/corpus/` and this directory are the reference from now on. The digest
must stay identical; a change needs a new written cause.
