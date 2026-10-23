# FU-332 — the measured blast radius of the `+0x79` placement-limit producer (phase-35 R2-L9)

**Phase/Provenance:** phase-35 recon-ahead wave, Stage-1 **slice 3 of 4 — the
measured whole-formation / M2 blast radius** (`docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`
§3/§6/§9 item 3; `docs/superpowers/plans/2026-10-23-fifa96-m2-phase35-recon-ahead.md`
Task 3), 2026-10-23, read-only. Question: **when the native placement limit
`+0x79` is produced, what moves — which records, from which frame, how far, what
does the M2 golden do, and does the natural no-slot pan arrive?** Method: an
**out-of-tree patched build** (FU323 §3's probe precedent; the repo's archives
linked in `build/CMakeFiles/test_engine_m2.dir/link.txt` order, only the patched
TUs compiled here and placed before the archives) whose **unpatched** probe
reproduces the M2 golden **`d5446144…`** (295 087 B, `cmp` byte-identical) and
the M1 golden **`09b726b7…`** (22 004 B, `cmp` byte-identical); five variants
(unpatched, producer@init 6, producer@init 6 + row-09, producer@init 15 +
row-09, producer@the native reset-lane site) measured over the verbatim natural
window (12 000 steps, 3 598 granted frames) and the full M2 transcript (6 522
presented frames). Fresh Ghidra this session: `/FIFA96.EXE`, `0x79A30..0x79A5F`
(the producer store block) and `0x80B30..0x80B7F` (the row-09 staging), plus a
fresh `get_xrefs_to 0x7997C`. Carried as named: FU84 §6 (the `0x10F394` table and
`FUN_0007997C`), FU328 §5.4/§5.6/L1–L3/L8 (the diagnostic ladder and the
NO-GO), FU326 §4.2/§6 (the `|w| ≤ 15` domain and the seed parity), FU323
§3/§4 (the probe method and the silent bind), FU321 §3.4 (the closed form),
FU327 §2 (the all-record `|v| ≤ 1` cap), the phase-33 register
(`docs/ENGINE.md:6784-6786`, the 5 671 hash-only / contiguous-852 precedent).
Scratch, patch, build and logs live
under `/tmp/opencode/fu332/`. **No repo file was created or changed except this
draft; no git; the ISO read only; `build/` and `tests/golden/` untouched;
census `60/80` unchanged.**

## 1. The question

> The phase-34 verdict named one missing producer (`+0x79`, FU328-L1) and left
> its blast radius **predicted** ("the M2 golden will move wholesale",
> FU328-L8). Measured, what is that radius? (a) Every record's `+0x79`/`+0x7B`/
> stride/attr/velocity over the natural window, and the first frame any of them
> moves. (b) The M2 golden's first differing frame, moved-line count, `state=`
> suffixes and strata, each attributable to the producer's chain. (c) Does the
> natural no-slot band entry arrive **moving** — the gate tuple, `has_slot 0`,
> `|v0| ≥ 1`, the event, the place delta `T(s)`, the pan — and at what
> frequency? (d) Does the row-09 `timer7b` staging (the OL-82 zero) move with
> it?

## 2. The one-page answer

**The producer moves the whole formation, the M2 golden and the natural pan —
and the pan is real.** Measured on the undriven natural tape with **no staged
slot/seed/face/timer cell** (`+0x79` is a production cell, set from the
`0x10F394` table word at the record-creation site):

- **Movement.** With `+0x79 = 6` (the port-reachable table floor):
  `+0x7B = 6` on **every** record-frame, the mover's stride nibble **6** (was
  **0**), `attr = min(dist>>3, stride_len) ≤ 6` (was ≤ 1), the formation-wide
  `max |vel|` **6** (was **1**), and per-record displacement up to **2 488**
  units over the window. The first record to move is **id 10** (the slot
  record, the kickoff taker) at granted **f = 1, step 12**; all **20** active
  records have diverged by **f = 3**. The velocity at the first divergence goes
  `−1 → −2` (all 20). With `+0x79 = 15` (the `0x10F394` table max):
  `+0x7B = 15`, nibble **15**, `max |vel|` **15**, displacement up to **2 872**,
  and all 20 diverge at **f = 1** (`−1 → −6`).
- **The M2 golden.** First differing line **frame 852** (`state=1/1-0`);
  **5 671 of 6 522 lines moved (86.95 %)**, as a **single contiguous tail**
  852…6522 — no scattered or earlier moved line. Length unchanged (295 087 B);
  sha `d5446144…` → **`6aa2703b…`** (value 6) / **`bd971062…`** (value 15). The
  **stratum set is intact**: the same 10 `state=` tags, none added, none lost;
  the moved lines fall in `19/1-0` (2 620/2 620), `2/1-0` (2 100/2 173) and
  `1/1-0` (951/953), and the cutscene/forced strata `0/1-0`, `4/1-0`,
  `21/1-0`, `12/1-0`, `19/0-0`, `2/0-0`, `20/0-0` and the pre-match lines are
  **0 moved**.
- **Attribution.** The pool's position fingerprint diverges at **step 12**
  (the kickoff install copies the produced limit into `+0x7B`, the mover then
  strides at 6 instead of 1); the transcript is byte-identical for the next
  **840** presented frames; frame 852 is inside the **first `1/1-0` window**
  (that tag's first line is 850). So the first diff is a *render* consequence
  of a pool divergence that starts 840 frames earlier — later than, not before,
  the first producer-attributable frame. Why exactly 852 (which render input
  first reads the diverged positions) is not isolated here (FU332-L4); the
  *fact* — contiguous tail, nothing earlier — is measured.
- **The natural pan.** At `+0x79 = 6`: band entry f 703, **event f 704**
  (step 2355), entity **12**, `code 4`, **`has_slot 0`**, `lane 56`
  (`≤ 0x40`, `bound 68`), `vel73 −6`, `face 0xFF00`, `cam (0,0,0)`,
  `seed (−0x18,0)`, **arm 0**, **`v0 = −2`**, **`T_x = −0x26`** (−38), the place
  moving **f 705…717, stop 718** (**13** frames, speeds `2,1`, `pan_counter 2`),
  place `(0,0,0) → (−38,0,0)`. At `+0x79 = 15`: f 606 entry, **event f 607**,
  `lane 40`, `vel73 −13`, `seed (−0x34,0)`, `v0 = −4`, **`T_x = −0x84`** (−132),
  **27** frames (608…634, stop 635, speeds `4,3,2,1`, `pan_counter 4`), place
  `→ (−132,0,0)`. These are **exactly** the FU328 §5.6 diagnostic rows (same
  event frames 704/607, same `T`) — now reproduced **without staging**.
- **The silent bind is gone.** The FU323-L1 zero-seed bind (f 1517, `seed −4`,
  `v0 = 0`) does not occur: the same record's gate passes earlier with a live
  speed; there is **one** no-slot event, not a zero-seed one plus a pan.
- **Frequency.** 1 no-slot gate pass and **1 pan per 3 598 granted frames
  (0.028 %)** — the same entry rate FU328 measured, now panning; over the
  24 000-step window still **1** pan (0.024 % of 4 138 first-half frames), then
  the half-time re-entry resets the place to `(0,0,0)` at f 4140 (the FU328-L4
  stall).
- **Row-09.** The faithful `timer7b = 2*word[+0x79]` correction (fresh
  `0x80B3D MOV AX,[EBP+0x79]; 0x80B41 ADD EAX,EAX; 0x80B43 MOV [EBP+0x7B],AX`)
  is **inert on both vehicles**: row 09 is never dispatched — **0** code-9
  record-frames in 71 960 natural record-frames and **0** handler calls across
  the 6 522-step M2 transcript (instrumented run). The OL-82 zero is dead code
  here; its retirement is still owed for fidelity, with **zero** measured blast
  radius.
- **The site matters.** The native store's only caller is `FUN_0008C33C`
  (fresh `get_xrefs_to 0x7997C`: **1** reference, `0x8C34A`), whose port
  (`fifa96_match_entities_reset_lane`) has **no live engine caller**
  (FU295-L1). A producer placed **at that native site** moves **nothing**
  (variant v4: 0 differing lines, the golden `d5446144…` unchanged). The
  measured producer therefore lives at the pool init, the live record-creation
  site the port actually has (FU332-L1).
- **M1 is untouched:** `09b726b7…`, `cmp` byte-identical under the producer
  (values 6 and 15), so the M1 immutability the spec pins holds.

**Verdict: GO for the producer as a faithfully pinned, golden-moving change,
and GO for the natural no-slot pan as a *producer-conditioned* witness —
NO-GO only on naming the measured tuple the *native* witness while the native
per-record values are still unmeasured (FU330-L1).** The table's **minimum** is
6 and 6 already pans (`v0 = −2`), so the witness survives every in-range table
row; the residual unknown is the roster index alone (the `+0x9D` OR is measured
zero on these pools and could only raise the value), i.e. a leg, not a blocker.

## 3. The producer that was measured (the exact patch and build line)

### 3.1 The native shape (fresh bytes, this session)

```
0x79a33  8b5a0a          MOV EBX,dword[EDX+0xA]      ; the record's descriptor dword
0x79a36  8b15387a1500    MOV EDX,dword[0x157A38]     ; -> 0x10F394
0x79a3c  c1fb18          SAR EBX,0x18                ; byte[[rec+4]+0xD] (the index)
0x79a3f  01da            ADD EDX,EBX
0x79a41  30ff            XOR BH,BH
0x79a43  8a1a            MOV BL,byte[EDX]            ; table[index]
0x79a45  8a959d000000    MOV DL,byte[EBP+0x9D]       ; the record's +0x9D byte (OL-72)
0x79a4b  08d3            OR BL,DL
0x79a4d  66895d79        MOV word[EBP+0x79],BX       ; *** the store ***
```

(`FUN_0007997C`, the reset/placement entry; FU84 §6 carried, bytes re-read
here. The OR means the native value is **≥** the table byte.)

### 3.2 The three patch hunks (all out-of-tree)

The probe implements **FU331 §5.3's two-site design** (now on disk): the exact
native site (the control) and the `entities_init` seam (the live route). The
producer (**P**), at the pool init — the live match-begin construction
(`match_run_begin → entities_init`):

```diff
--- src/fifa96_engine/fifa96_match_entities.c
+++ fifa96_match_entities.c
@@ e->active = (uint8_t)i;
+      e->timer79 = (uint16_t)(FU332_T79);   /* table[player_d=0] | +0x9D(=0) */
```

The producer at the **native** site (variant v4 only, the control):

```diff
@@ e->stage92 = 0;            /* +0x92 (0x79a2c) */
+    e->timer79 = (uint16_t)(FU332_T79);
     e->timer7b = e->timer79;  /* word +0x7B := +0x79 (0x79a86) */
```

The row-09 staging (**R**), at `handlers.c:5034`:

```diff
-    r->timer7b = 0;                             /* 0x80B43 word[+0x79]*2, +0x79 zero (OL-82) */
+    r->timer7b = (uint16_t)(2u * e->timer79);   /* 0x80B3D..0x80B43, fresh */
```

(`r` is the staging record, which has **no** `+0x79` cell; the native reads the
record's own word, so the faithful port must read the pool entity's — a
staging-shape item for P1, FU332-L3.)

### 3.3 Variants, build line, goldens

| variant | producer | `+0x79` | row-09 fixed | M2 sha | lines moved |
|---|---|---|---|---|---|
| **v0** | — (unpatched) | 0 | no | `d5446144…` (golden, `cmp`) | — |
| **v1** | pool init | 6 | no | `6aa2703b03444b99…` | 5 671 / 6 522 |
| **v2** | pool init | 6 | yes | `6aa2703b03444b99…` (identical to v1) | 5 671 |
| **v3** | pool init | 15 | yes | `bd971062d4acdc8f…` | 5 671 |
| **v4** | `reset_lane` (native site) | 6 | yes | `d5446144…` (unchanged) | 0 |

Build (published, `build.sh`): the repo flags verbatim
(`-std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all -Iinclude -Isrc`), the patched TUs compiled here (the
producer TU with `-DFU332_T79=6|15`), and

```
cc -fsanitize=address,undefined obj/e_init6.o obj/h_row09.o probe.o \
   -o probe_v2  build/libfifa96_engine.a build/libfifa96_iso9660.a … \
   (build/CMakeFiles/test_engine_m2.dir/link.txt order, 43 archives)
```

No struct layout is touched, so the FU323 §3 ABI hazard (mixed-header builds)
does not arise; the patched objects precede the archives, so the same-named
archive member is never pulled. The probe is a copy of `tests/test_engine_m2.c`
whose `main` carries one env-gated block (`FU332_TRANSCRIPT=1`) that runs
`run_tape` and writes the transcript — the golden producer without the assert
suite, so a patched build still yields the full 6 522-line transcript. The
unpatched build of exactly that probe prints `d5446144…`, `cmp`-identical to
the golden: the equivalence argument for the whole measurement.

## 4. The whole-formation movement (natural window, 3 598 granted frames)

The window is FU328's: `phase2_step 1802` (f 538), `granted 3 598`,
`step_ns 10 ms`, ISO present, `M2_KEYS` verbatim. 20 of the 22 record slots
carry active record-frames (ids 0 and 11, the two keepers, do not), i.e.
**71 960 active record-frames**.

| quantity | v0 (unpatched) | v1 (`+0x79` = 6) | v3 (`+0x79` = 15) |
|---|---|---|---|
| `word[+0x79]` over the window | 0 | **6** (every record-frame) | **15** |
| `word[+0x7B]` | 0 | **6** | **15** |
| mover stride nibble (`timer7b & 0xF`) | 0 (clamped to `stride_len = 1`) | **6** | **15** |
| `attr = min(dist>>3, stride_len)` | ≤ 1 | **≤ 6** | **≤ 15** |
| formation-wide `max |vel|` | **1** | **6** | **15** |
| velocity histogram `[|v|=0, 1–2, 3–5, 6–8, 9–15]` (record-frames) | `[44 622, 27 338, 0, 0, 0]` | `[64 430, 2 296, 3 510, 1 724, 0]` | `[66 242, 1 490, 1 952, 162, 2 114]` |
| first record to move / frame | id 10, f 1 (step 12) | id 10, **f 1** | all 20, **f 1** |
| all 20 records diverged by | — | **f 3** | **f 1** |
| velocity at the first divergence | `−1` | **`−2`** | **`−6`** |

Per record (v1; `first div` = the first frame the record's position differs
from v0; "moving frames" = frames whose own position changed; `max |Δpos|` =
the largest divergence from v0 over the window):

| id | slot | first div f | `vel73` v0 → v1 | stride | max abs v | moving frames | max \|Δpos\| | final Δ (x,z) |
|---:|---:|---:|---|---:|---:|---:|---:|---|
| 1 | 0 | 3 | −1 → −2 | 6 | 6 | 272 | 1 644 | (−40,−4) |
| 2 | 0 | 3 | −1 → −2 | 6 | 6 | 100 | 432 | (−248,0) |
| 3 | 0 | 3 | −1 → −2 | 6 | 6 | 314 | 1 368 | (−8,−12) |
| 4 | 0 | 3 | −1 → −2 | 6 | 6 | 376 | 2 488 | (−8,−4) |
| 5 | 0 | 3 | −1 → −2 | 6 | 6 | 374 | 2 092 | (−8,−4) |
| 6 | 0 | 3 | −1 → −2 | 6 | 6 | 300 | 1 608 | (0,0) |
| 7 | 0 | 3 | −1 → −2 | 6 | 6 | 264 | 1 088 | (−8,−8) |
| 8 | 0 | 3 | −1 → −2 | 6 | 6 | 302 | 1 252 | (−4,−4) |
| 9 | 0 | 3 | −1 → −2 | 6 | 6 | 2 172 | 1 696 | (−72,36) |
| 10 | **1** | **1** | −1 → −2 | 6 | 6 | 226 | 1 544 | (−8,0) |
| 12 | 0 | 3 | −1 → −2 | 6 | 6 | 236 | 1 704 | (−44,0) |
| 13 | 0 | 3 | −1 → −2 | 6 | 6 | 312 | 1 288 | (0,4) |
| 14 | 0 | 3 | −1 → −2 | 6 | 6 | 334 | 1 624 | (−8,4) |
| 15 | 0 | 3 | −1 → −2 | 6 | 6 | 256 | 916 | (0,8) |
| 16 | 0 | 3 | −1 → −2 | 6 | 6 | 306 | 1 052 | (0,12) |
| 17 | 0 | 3 | −1 → −2 | 6 | 6 | 298 | 1 468 | (−8,8) |
| 18 | 0 | 3 | −1 → −2 | 6 | 6 | 324 | 2 032 | (−8,8) |
| 19 | 0 | 3 | −1 → −2 | 6 | 6 | 360 | 1 992 | (−12,8) |
| 20 | 0 | 3 | −1 → −2 | 6 | 6 | 154 | 536 | (4,8) |
| 21 | 0 | 3 | −1 → −2 | 6 | 6 | 250 | 1 804 | (−8,4) |

At `+0x79 = 15` the same 20 records diverge at f 1 (`−1 → −6`), `max |vel|` is
**14–15** per heading octant (the stride table's entry caps the nibble, FU326
§4.2), the largest displacement is **2 872**, and the longest moving record is
id 1 (3 100 frames). The first divergence is inside code **0x24** (the kickoff
cutscene) on every record: the produced limit is copied by the *first* code
install (`0x7DA9B`, `entities.c:94`) at the kickoff placement, so the cutscene
walk-in itself runs at the produced stride.

The `+0x7B` copy is live on every install, and the row-03 counter maintenance
stays unwired (FU328-L2): the port therefore stages the produced **limit**
(6/15) as the counter on every record, not the native
`min(([+0x63]>>22)+1, limit)` — the measured radius is an upper-bound shape of
the counter, not the native maintenance.

## 5. The M2 diff accounting

The M2 transcript is 6 522 presented frames (`st.presents`, one per step) /
295 087 B / the golden's own producer.

| | v1 (6) | v3 (15) |
|---|---|---|
| first differing line | **852** (`frame=852 hash=964282c7… → e04df34d… state=1/1-0`) | **852** (`… → a07b8bff…`, same tag) |
| moved lines | **5 671 / 6 522 (86.95 %)** | 5 671 / 6 522 |
| moved shape | **contiguous tail 852…6522** (no earlier or scattered line) | same |
| last differing line | 6 522 | 6 522 |
| length | 295 087 B (unchanged) | 295 087 B |
| `state=` tags | 10 distinct, **set unchanged** (none added, none lost) | same |
| myers/`cmp` | `cmp` differs from line 852 | same |

Moved lines by stratum (golden tag; identical for both values):

| `state=` | moved / total | reading |
|---|---|---|
| `1/1-0` | **951 / 953** | the phase-1 restart — the 2 unmoved are lines 850–851 |
| `2/1-0` | **2 100 / 2 173** | the 73 unmoved are all lines 773–845 (before 852) |
| `19/1-0` | **2 620 / 2 620** | the ET-over/hold tail — all moved |
| `0/1-0` | 0 / 304 | the goal cutscene |
| `4/1-0` | 0 / 303 | the corner window |
| `21/1-0` | 0 / 100 | the act-9 window |
| `12/1-0` | 0 / 4 | the half-time chooser |
| `19/0-0` `2/0-0` `20/0-0` | 0 / 20 each | the pre-goal scripted frames |
| (no tag, pre-match) | 0 / 5 | the front-end frames |

**Attribution.** A per-step FNV fingerprint of the 22 records' positions
diverges at **step 12** (the kickoff install + first mover step); the transcript
is byte-identical from step 12 through step 851. The first differing line is
852, inside the first `1/1-0` window (tag first seen at 850). So no line moves
before the producer-attributable frame; the first moved line is a *render* of
the already-diverged pool. The moved strata are exactly those where the
formation is drawn from its own integrated positions; the cutscene/forced
strata are byte-identical. Which render input first reads the divergence at 852
is not isolated (FU332-L4) — the fact that nothing before 852 moved is the
stop-and-report evidence the spec asks for.

**The shape is the phase-33 precedent, and the count is arithmetic.** The
phase-33 re-pin (`docs/ENGINE.md:6784-6786`) records "**5 671 moved hash-only
lines** (contiguous 852..6522) … frames 1..851 byte-identical, the line count
stays 6 522" — the same shape measured here. 5 671 = 6 522 − 851: any
render-visible change from frame 852 moves exactly the tail, so the count is
not an independent coincidence but the tail length, and the discriminating
facts are the **first-diff frame (852)**, the **contiguity** and the
**unchanged 10-tag stratum set** — all three measured.

## 6. The natural pan

The band record is the FU328 entity **12** (team 1 record 1, the code-4
outfield record with no slot). No drive, no staged cell: the only change is the
produced limit.

| | v1 (`+0x79` = 6) | v3 (`+0x79` = 15) | FU328 §5.6 diagnostic row (staged; per its post-review erratum the "gate f" labels there are **event** frames) |
|---|---|---|---|
| band entry (filter) f / step | 703 / 2352 | 606 / 2029 | 704 / 607 |
| **event f** / step | **704** / 2355 | **607** / 2032 | **704** / **607** |
| entity / code / active / `has_slot` | 12 / 4 / 1 / **0** | 12 / 4 / 1 / **0** | same |
| `lane` / `lane_z` / `cam_dz6f` / `bound` / `timer81` | 56 / −56 / 0 / 68 / 0 | 40 / −40 / 0 / 66 / 0 | 56 / 40 |
| `dir` / `face` / `pos` / `cam` | (0,0) / `0xFF00` / (56,0) / (0,0,0) | (0,0) / `0xFF00` / (40,0) / (0,0,0) | same |
| `vel73` / `vel75` | **−6** / 0 | **−13** / 0 | −6 / −13 |
| `seed` | (−0x18, 0) | (−0x34, 0) | same |
| arm (classifier) | **0** (no-slot seed match) | **0** | arm 0 |
| **`v0`** (camera) | **−2** | **−4** | −2 / −4 |
| **`T(s)` per axis** | **(−0x26, 0)** = (−38, 0) | **(−0x84, 0)** = (−132, 0) | −0x26 / −0x84 |
| motion / stop f | **705…717 / 718** (**13** frames) | **608…634 / 635** (**27**) | 13 / 27 frames |
| speed sequence | `2,1` | `4,3,2,1` | same |
| `pan_counter` max | 2 | 4 | — |
| place | `(0,0,0) → (−38,0,0)` | `(0,0,0) → (−132,0,0)` | — |

The closed form holds exactly (`T = 2·Σ n·v`, `v0 = trunc(seed/0xC)`,
`v ← trunc(3v/4)`): `v0 = −2` → `−38` over 13; `v0 = −4` → `−132` over 27
(FU321 §3.4 / FU328 §3.2). **The silent bind's status: retired on this
vehicle** — the natural window has **one** event (the moving no-slot pan) and
no second, zero-seed bind; the FU323-L1 `f 1517 / seed −4 / v0 0` tuple does not
exist once the record is fast (the trajectory reaches the gate much earlier).

**Frequency.** 1 band entry, 1 event, **1 pan** per **3 598 granted frames =
0.028 %**; **1 / 4 138 (0.024 %)** of the 24 000-step window's first-half
play. The 24 000-step run: the same single pan (event f 704), then a place
snap back to `(0,0,0)` at f 4140…4141 (the phase-1 re-entry / half-time reset of
the FU328-L4 stall). The pan is a first-half, undriven, no-slot, `Y = 0` pan;
`Y` stays 0 (the arm is the height-0 arm), the carried FU322 Y-less caveat
applies.

**The value question.** 6 is the port-reachable floor (`table[0]` with
`player_d = 0` and `+0x9D` unported); 15 is the table's max. Both pan, and the
FU328 diagnostic pans at 3. **FU330 (now on disk) pins the same basis**: the
index byte is per-player roster data, the port stages `player_d = 0`, the
`+0x9D` pool is BSS (64 zero bytes), so the faithful kickoff value is **6 on
all 22 records** — exactly the measured floor — and the native per-record
values stay unmeasured (FU330-L1 = FU328-L3). FU330 also proves `t | 0x9D ≤ 15`
for an in-range index (the OR's producer emits only `0x00`/`0x0F`), so the
table rows are 6..15 and every one of them is ≥ 3, the pan's seed threshold
(`w ≥ 3` → seed `0xC` → `v0 ≥ 1`; at `+0x79 = 2` the seed would be 8 and
`v0 = 0` — predicted by the same chain, not measured). The witness therefore
does not depend on which **table** row lands.

**The spec §7 staged-cell rule, applied.** This is **not** the §5.6 diagnostic
re-run: the `+0x79` store is *production code* at the record-creation site,
sourced from the native `0x10F394` table word, and the tape is `M2_KEYS`
verbatim with no input hold and no test-side write to the slot, seed, face or
timer cells. What is still not natural is the **value**: the port-reachable
floor stands in for the unmeasured native per-record value (FU332-L2). The
tuple is therefore labelled **producer-conditioned**: it is the witness the P1
change produces as designed, and its exact numbers must be re-read once the
roster index (FU330-L1) is pinned — the `+0x9D` OR is already measured zero on
these pools (FU330). A "witness" that needed a *test knob* or a staged
slot/seed/face cell would still be NO-GO — none is used here.

## 7. The row-09 staging effect (measured zero)

The native row-09 body writes `word[+0x7B] = 2 * word[+0x79]` (fresh bytes
above) and then feeds `+0x7B` into the two velocity multiplies
(`0x80B51..0x80B6F`). The port hard-coded 0 under the OL-82 note. Patched to
the native form, the measurement is:

- **Natural window:** the v1 and v2 dumps are **byte-identical** (71 990
  record lines); the M2 transcripts are **byte-identical** (`6aa2703b…` both).
- **Cause (measured, not assumed):** row 09 is never dispatched — `code 9`
  occurs **0** times in 71 960 natural record-frames, and an instrumented run of
  the 6 522-step M2 scenario records **0** calls to `fifa96_match_action_09`.
- The correction is therefore **required for fidelity and inert on these
  vehicles**; a tape that dispenses row 09 (a code-9 install) would see
  `timer7b = 12` (at 6) / `30` (at 15) and a very different staging — untested
  here (FU332-L3).

## 8. Numbered legs (FU332-L1…)

1. **FU332-L1 — the native producer site is not live in the port.** Fresh
   `get_xrefs_to 0x7997C` gives exactly **one** caller, `0x8C34A` in
   `FUN_0008C33C`; the port's mirror (`fifa96_match_entities_reset_lane`,
   `entities.c:187`) has **no engine caller** (only tests; the header's FU295-L1
   note says the native `0x741C4`/`FUN_0007417C` callers are unported).
   Measured: a producer at that site changes **0** M2 lines (variant v4). The
   measured radius uses the pool-init seam. **The P1 design must name the live
   site** (FU331 §5.3 does: the same two writers, the init route live); the
   probe implements exactly those two. Owner: FU295-L1 + FU331/P1.
2. **FU332-L2 — the measured value is the port-reachable kickoff value, not
   the native per-record value.** `table[player_d = 0] = 6`
   (`arm_bodies.c:32-38`; the pool's `player_d` is staged 0, FU-142b App. C) and
   `+0x9D` has no pool cell → OR 0. FU330 agrees on the same basis (the roster
   index is per-player data not present in the image; the `+0x9D` pool is BSS
   zero) and proves `t | 0x9D ≤ 15` for an in-range index. The native
   per-record values are unmeasured (FU330-L1); the measured values 6 and 15
   bracket the **table** rows. Owner: FU330 (the native values) + FU331 (the
   descriptor producer).
3. **FU332-L3 — the staging record has no `+0x79`.** The row-09 body's `r`
   (`fifa96_match_run_record`) carries `timer7b` but not `timer79`; the native
   reads the record's own word, so the faithful port must read the pool
   entity's (`e->timer79`) or grow the staging record. Measured effect zero on
   both vehicles (row 09 never dispatched). Owner: the P1 design.
4. **FU332-L4 — the 840-frame render lag.** The pool positions diverge at
   step 12 but the presented hash is byte-identical until 852. The pre-852
   frames therefore do not render the diverged formation positions; the
   *mechanism* (camera pose, draw list, cutscene override) is not isolated
   here. Owner: a presentation-side measure if the first-diff frame ever needs
   a tighter attribution.
5. **FU332-L5 — the out-of-range index, not the OR, is the `+0x79 > 15` path.**
   The mover stages `timer7b & 0x0F` (`match_run.c:549`), so a value ≥ 16 would
   wrap the stride (22 → 6). For an **in-range** table index that cannot happen
   (FU330: the table's max byte is `0x0F` and the OR emits only `0x00`/`0x0F`,
   so `t | 0x9D ≤ 15`); the only >15 path is the **unguarded index** (`≥ 0x20`
   walking into the `0x10F3B4` table / `0x10F3C4` ramp / the `0x0FF4xx` wrap,
   FU330 §2), which the port does not model (`player_d` is a constant 0).
   Unmeasured here; the 15 bound is the in-range table max. Owner: the P1
   hazard list / FU330's out-of-range census.
6. **FU332-L6 — the A1–A3 window re-pins with this pacing.** The phase-33
   inversion's window constants (`tests/test_engine_m2.c:3972-3979`) ride the
   produced stride; FU332 did not run them (the patched build aborts the assert
   suite by design). The measured pan (f 704 at 6, f 607 at 15) is the pacing
   the re-pin must meet. Owner: P2 (FU329's Branch B concretized).
7. **FU332-L7 — the pan's place does not survive the half-time re-entry on the
   null tape.** The 24 000-step run resets the place to `(0,0,0)` at f 4140
   (the FU328-L4 stall). Whether the native half-time resets the camera place
   is unmeasured; carried, not asserted.

## 9. GO / NO-GO

**GO — the producer's blast radius is measured and accounted.** The whole
formation moves (20/20 records; the first at f 1, all 20 by f 3;
`max |v| 1 → 6` at the floor value), the M2 golden moves **5 671/6 522 lines as
one contiguous tail from frame 852** with the **10-tag stratum set intact** and
the length unchanged, and M1 stays byte-identical. Every moved line is
downstream of a pool divergence at step 12; nothing moved earlier.

**GO — the natural no-slot pan.** With the producer and **no staged
slot/seed/face/timer cell**, the undriven tape's band record passes the row-04
no-slot gate **moving** (`|v0| = 2` at 6, `4` at 15), the event binds, and the
place travels `T(s)` over the exact decay (13 / 27 frames) — the spec §6 tuple,
measured. The silent zero-seed bind is replaced, not accompanied.

**NO-GO (narrow, and only this) — calling the measured tuple the *native*
witness while the per-record native value remains unmeasured (FU330-L1).** The
value is the port-reachable kickoff value (6) / the table max (15); the roster
index is per-player data not present in the image, the `+0x9D` OR is measured
zero on these pools, and the table's minimum (6) already pans. So the witness
is robust, but the *tuple's exact numbers* will follow the native value. The
golden re-pin must not be sold as "the native golden" until that value lands
(FU330-L1; FU332-L1/L2).

**Census / goldens.** Census `60/80 = 75%` (no dispatch row is wired). M1
`09b726b7…` verified byte-identical under the producer (built and `cmp`ed);
M2 `d5446144…` verified on the unpatched probe and **not** re-pinned (the wave
may not). The patched transcripts are evidence only.

## 10. Known limits

- The probe is an out-of-tree **copy**; the unpatched build of the same probe
  reproduces both goldens byte-for-byte, which is the equivalence argument. The
  patched build is measured through the transcript-only main (the assert suite
  is not run; it aborts at the moved golden pins by design).
- Two values (6, 15) are measured; the interior rows 7…14 are **predicted** by
  the same monotone seed chain (and 3 is carried from FU328 §5.6), not measured.
- The native per-record value (the roster index) is unmeasured (FU330-L1 =
  FU328-L3); the `+0x9D` byte is measured zero on these pools (FU330) but its
  producer is unported (OL-72).
- The natural window is one deterministic tape; the live `make game` rate is
  unmeasured (FU328-L5). The pan is the height-0, `Y = 0` arm (FU322's caveat).
- The M2 diff is line-level on the transcript; the hash is the pixel authority.
- The row-09 correction is measured inert **because row 09 never dispatches**;
  a row-09-bearing tape is untested (FU332-L3).
- The probe seeds only the **match-begin** route (`entities_init`); the native
  also re-stores at each kickoff and period transition (FU330 §2 "Timing";
  FU331 §5.3's second writer). With a constant value of 6 and a persisting
  `+0x79`, the re-store is value-identical on this tape (every install already
  copies `+0x7B = 6`); it was not separately measured.
- The 24 000-step window's second half is still the FU328-L4 stall, so the pan
  is a first-half phenomenon on this vehicle (FU332-L7).

## 11. Provenance and self-audit

**Fresh Ghidra this session** (`/FIFA96.EXE`, standard MCP tools only; no
scripts/saves): `disassemble_bytes 0x79A30` (0x30 B — the `0x79A33..0x79A4D`
store block: the `0x10F394` lookup, `SAR 0x18`, the `+0x9D` OR, the
`MOV word[EBP+0x79],BX`); `disassemble_bytes 0x80B30` (0x50 B — the
`0x80B3D MOV AX,[EBP+0x79] / 0x80B41 ADD EAX,EAX / 0x80B43 MOV [EBP+0x7B],AX`
staging and the `0x80B51..0x80B6F` velocity multiplies);
`get_xrefs_to 0x7997C` (1 reference: `0x8C34A` in `FUN_0008C33C`). **Carried**
(named): FU84 §6 (the table and the function), FU328 §5.4/§5.6 (the diagnostic
ladder) and §5.1/§5.6/L1–L3/L8, FU326 §4.2/§6, FU323 §3/§4, FU321 §3.4, FU327
§2, the phase-33 register (`docs/ENGINE.md:6784-6786`), the FU295-L1 note
(`fifa96_match_entities.h:456-465`).

**Fresh source reads:** `src/fifa96_engine/fifa96_match_entities.c` (28-60,
76-100, 187-235, 700-760), `fifa96_match_handlers.c` (4985-5060, 5340-5390),
`fifa96_match_run.c` (490-560, 1420-1450), `include/fifa96_engine/fifa96_match_entities.h`
(60-180, 430-470), `src/fifa96_loader/fifa96_action_handlers.c` (240-290),
`src/fifa96_loader/fifa96_arm_bodies.c` (28-60), `tests/test_engine_m2.c`
(6527-6560, 6990-7105), `build/CMakeFiles/test_engine_m2.dir/{link.txt,flags.make}`.

**Measurements (out of tree, `/tmp/opencode/fu332/`):**
`probe_v0…v4` + `sweep79_v0…v4` (the FU328 harness extended with
`timer79`/stride/attr + the whole-formation census + `--dump`), `m1_v0/v2/v3`,
`m2_v0.txt` (`d5446144…`, 295 087 B, `cmp` the golden), `m2_v1/v2.txt`
(`6aa2703b…`), `m2_v3.txt` (`bd971062…`), `m2_v4.txt` (`d5446144…`),
`logs/map_v*.err` (the per-step pool fingerprint: positions diverge at
step 12),
`dump_n0/n1/n2/n3.txt` (71 990 REC lines each; v1 ≡ v2), `row09_trace.err`
(0 handler calls in 6 522 steps), `m1_v0/v2/v3.txt` (`09b726b7…`, `cmp` the
golden), the 12 000/24 000-step sweeps, and the stability re-runs (all
transcripts and sweeps byte-identical after label normalisation). The ISO was
read only; no repo file was written except this draft; no git; `build/`,
`tests/golden/` and `fifa96.rep/` untouched.

**Self-audit.** (a) Every address/byte claim is a fresh read or a named carried
doc. (b) The FU328 §5.6 ladder was a *staged* causal diagnostic; this doc's
numbers come from a **producer cell** (the table store at the creation site),
and the coincidence of event frames/`T` is reported as a cross-check, not as
the evidence — the staged-cell ban is respected (no slot/seed/face/timer cell
is touched in the witness). (c) The port-reachable value and the native value
are separated at every appearance (FU332-L2). (d) The producer's **site** is
declared, and the native site is measured dead (v4) rather than described.
(e) No invented drive; the tape is `M2_KEYS` verbatim. (f) The goldens are
verified (M1 `cmp`, M2 `cmp` on the unpatched build) and not re-pinned; the
census is unchanged; no repo write but this draft; no git.

## Review erratum (wave review, 2026-10-23)

**1. The patch is a value probe, not FU331's expression.** Quote (§3.2): "The probe implements **FU331 §5.3's two-site design** … The producer (**P**), at the pool init"; quote (§6): "the `+0x79` store is *production code* at the record-creation site, sourced from the native `0x10F394` table word". **Corrected:** the init hunk writes the constant `FU332_T79` (6/15 = `table[0]`) and **only `timer79`**; the first install copies it to `+0x7B`. It does not execute FU331's `placement_limit()` (table index + `+0x9D` OR) or its two-cell `placement_seed()` at the init. With `desc_d = byte9d = 0` the values are observationally equal, so the measured radius stands; but the measurement proves the **cell/seam/value 6**, not the table lookup/OR — FU331's P1 pins own the expression. Label the patch "constant `table[0]` at the init seam", not "the producer expression".

**2. `+0x9D` is not measured zero.** Quote (§2/§6): "the `+0x9D` OR is measured zero on these pools" / "the `+0x9D` byte is measured zero on these pools (FU330)". **Corrected:** the engine record has no `+0x9D` cell (FU330-L8); the probe cannot measure it and hard-codes 0. FU330's evidence is the BSS image read plus the P2-only writer census (FU330 §5), not a runtime measurement. Say "the port's OR term is 0 by construction; the native `+0x9D` is unmeasured (FU330-L1/FU328-L3)".

**3. The residual is not the roster index alone.** Quote (§2): "the residual unknown is the roster index alone (the `+0x9D` OR is measured zero on these pools and could only raise the value)". **Corrected (FU330 §6.1):** the live native domain is **2..15**; P2 (lineup rebind, `+0x79 >>= 1`, floor 2) and P3 (code-0x0C no-replacement, `max(+0x79>>1, 3)`) lower the register. A composed P2-after-P3 gives **2 or 3**: at 3 the pan survives (`|v0| = 1`, `T = −0xC`, 6 frames); at 2, on the doc's own predicted seed (`w = −2` → seed 8), `v0 = 0` and no pan. The measured tape has no such event (both writers unported), so the tuple stands, but the witness survives **table rows 6..15 and P2/P3 values 3..7**, not the whole native domain. Add P2/P3 (and the composed 2/3) to the residual and to the acceptance's re-measure rule (FU333-L15).

**4. The 840-frame gap is explained, not open.** Quote (§2/§9 L4): "Why exactly 852 (which render input first reads the diverged positions) is not isolated here (FU332-L4)" / "The *mechanism* (camera pose, draw list, cutscene override) is not isolated here." **Corrected (FU323 §6; `docs/ENGINE.md:6787-6791`):** the phase-33 register already isolates the first hash-visible frame **852** as the sit-1 half-time kickoff's camera reset to `(0,0,0)` revealing the since-372–379 face-driven walk-in sprite positions. The producer's pool diverges at **step 12**; the pre-852 transcript is the scripted/held-camera window. The first diff at 852 is the same reveal, so the attribution is *explained* (the per-sprite pixel proof stays FU323-L2's class). L4 should be narrowed, not left open; this also gives the Stage-2 tripwire its lower bound (a diff in 13..851 is a stop).

**5. The dump identity and count.** Quote (§7): "the v1 and v2 dumps are **byte-identical** (71 990 record lines)". **Corrected (the artefacts):** the raw dumps differ only in the `label=d1`/`d2` prefix; after label normalisation they are identical (`cmp`). The **REC** lines are **71 960** (20 records × 3 598); the 71 990/71 991 totals include the PH/RUN/PLACE/FILTER/MOVE summary lines. The load-bearing facts — the transcripts `6aa2703b…` both, the 71 960 record-frames, 0 code-9 — stand.

**6. The per-record `max |Δpos|` column is x-only.** Quote (§4): "`max |Δpos|` = the largest divergence from v0 over the window". **Corrected (the dumps):** the tabulated values are the max **x-axis** divergence (id 3 1 368, id 4 2 488, …); the largest Euclidean divergence is **2 894** (id 4), and the largest `|Δz|` is **1 968** (id 13/16). The overall **2 488** is the max per-axis; label the column `max |Δx|` or report both. (The v3 "2 872" is likewise x-only; the Euclidean max at 15 is 3 417.)
