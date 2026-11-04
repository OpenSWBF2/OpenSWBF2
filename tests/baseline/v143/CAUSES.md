# v143 against the VS2013 baseline

Every difference between the v143 Release build and `tests/baseline/vs2013/`, with its cause.

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

## Policy

The v143 files in `tests/corpus/` and this directory are the reference from now on. The digest
must stay identical; a change needs a new written cause.
