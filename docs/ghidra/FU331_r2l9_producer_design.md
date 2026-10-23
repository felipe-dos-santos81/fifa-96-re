# FU-331 — the port audit and the placement-limit producer design (phase-35 R2-L9)

**Phase/Provenance:** M2 phase-35 Stage-1 recon-ahead wave, **slice 2 of 4** —
the port audit + the producer design (`docs/superpowers/plans/2026-10-23-fifa96-m2-phase35-recon-ahead.md`
Task 2), serving `docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`
§3 (Scope), §4 (Native authority), §8 (Decisions 1/2/4) and §9 item 2.
**Repo-static design: no Ghidra read this session, no probe, no build, no
measurement.** Every native fact is **carried** with its owner doc: `FU328`
§3.3/§5.6/§8 L1–L3/L8 + erratum 2 (the counter, the diagnostic, the golden
exposure), `FU84` §6 (the `FUN_0007997C` store and the `0x10F394` table),
`FU178` §3.3 (`0x80B3D..0x80B43`, the row-09 `+0x7B = +0x79*2`), `FU142` §2
(the row-26 table read, the `+0xD` descriptor byte), `FU188` §4 (the same
`+0xD` byte), `FU295` §2 (the unported `0x741C4` driver), `FU151` §2.8 (the
`FUN_0008C33C → FUN_0007997C` pass), `FU253` §2 (OL-82's scope), `FU324`
§3/§5 (the `+0x7B` producer set). Every port/test claim is a first-hand
`file:line` read this session (§11). **FU330's census draft is not on disk at
the time of writing:** the table bytes and the per-record values below are
written as **"the FU330 values"**, forward-cited — the design is exact in
*shape* and honest about *value*. The only repo write is this draft; no git;
the ISO untouched; `build/`, `tests/golden/` and the census `60/80` untouched;
nothing was built or run.

---

## 1. The question

> Every `timer79` (`+0x79`) and `timer7b` (`+0x7B`) site in the engine: which
> is the producer's target, which are correct native copies that merely
> *inherit* the missing zero, which are the row-09 velocity staging, and which
> are the other rows' independent native writers? What exactly does the live
> `+0x79` do to the row-09 staging (the OL-82 `r->timer7b = 0`), and what is the
> native shape it must become? And what is the **exact producer**: the cell, the
> writer site(s), the timing, the value expression, the interaction with the
> `+0x7B` install reset (`entities.c:94`), with the RED/GREEN unit pins that
> prove it and the FU328-L1/L2/L3/L8 mapping — asserting no value FU330 owns?

## 2. The one-page answer

**The port has no producer and no writer for `+0x79`; it has 23 engine sites and
exactly one stale assumption.** Six classes: the cell and its declarations (3),
five copies/transport sites (all *correct native copies* that inherit 0 — not
"assumptions"), two `+0x79` reads (both inside the exported-but-NULL row-03
body), the three row-09 staging lines (the one genuine assumption), nine other
rows' independent `+0x7B` writers, and one mover consumer. The spec §2/§3
framing that the `timer7b = timer79` copies "assume `+0x79 = 0`" is audited and
**corrected**: they do not assume anything — they faithfully copy a cell that
nothing produces. The only literal zero is `handlers.c:5034`.

**The row-09 interplay is a value change, not a shape change.** Native
`0x80B43` stores `word[+0x7B] = word[+0x79] * 2` (FU178 §3.3, fresh
`0x80B3D MOV AX,[EBP+0x79]; 0x80B41 ADD EAX,EAX; 0x80B43 MOV [EBP+0x7B],AX`).
The port's `r->timer7b = 0` is honest only while `+0x79 == 0` (the OL-82
staged-cell convention, FU178 §10:382-384); OL-82 is **retired for this cell**
and only for this cell. With the producer, the same line becomes
`r->timer7b = (uint16_t)((uint32_t)e->timer79 * 2u)`; the two consumers
(`handlers.c:5036/5037`) then emit `vel73 = mul(2·limit, sine)`,
`vel75 = mul(2·limit, t1)`, `speed71 = distance(vel73, vel75)` and
`target = pos + vel<<3`. The face/sine/position path is **untouched**: the
`dword[+0x7B]>>16` reads at `0x80AC3/0x80AE8` are `word[+0x7D]` = `face7d`
(the AGENTS alias trap), and the port already stages `face7d` there.

**The producer is three elements, not a guessed number.** (1) The **cell**:
`fifa96_match_entity.timer79` (+0x79, `entities.h:129`) plus two new declared
staging cells — `desc_d` (native `byte[[rec+4]+0xD]`, the roster descriptor)
and `byte9d` (native `byte[+0x9D]`), both producers unported (OL-72 / FU301-L3
/ FU328-L3). (2) The **value**: `word[+0x79] = byte[0x10F394 + desc_d] | byte9d`
(the FU330 values; FU84 §6 carries the 48 contiguous bytes `0x10F394..0x10F3C3`).
(3) The **writers**, in the native order (`0x79A4D` store, then `0x79A86` copy):
`fifa96_match_entities_reset_lane` — the modelled `FUN_0007997C`, exact but
dead live (FU295-L1) — **and** `fifa96_match_entities_init`, the derived seam
for the unported `0x741C4 → FUN_0008C33C` caller, which is what makes the value
live from frame 0 on the path `match_run_begin → entities_init`
(`match_run.c:2378`). Both write `+0x79` **and** `+0x7B` (the native pass writes
both), so no record starts with a produced limit and a zero counter.

**The blast radius is the point of the phase, and it is two-sided.** Every row
install copies the limit into `+0x7B` (`entities.c:94`, native `0x7DA9B`), so
the mover's stride nibble (`match_run.c:549`) rises from 0 to the table value —
the whole formation moves (FU328-L8). And the row-03 idle core would compute
`min((move_attr>>22)+1, limit) = min(1, limit) = 1`, **lower** than the install
value: wiring row 03 (FU328-L2) before staging the real `move_attr` is a
regression, not a second win (FU331-L4).

**Verdict: GO on the producer design and on P1 as a separately pinned,
golden-moving fidelity commit (spec §8.1); NO-GO on asserting any native
per-record value (FU330) or any witness (FU332/FU333).** Three RED/GREEN pins
(the producer, the counter, the row-09 consumer), one known moved existing pin
(`test_action_09_stage0_set_branch`'s `target_x == pos_x`, which the native
`0x80B91..0x80B9D` add contradicts once the velocity is live), and one
must-not-fold item (row 03).

---

## 3. The port audit (`timer79` / `timer7b`, every site)

### 3.1 The classes

| class | what it is | count |
|---|---|---|
| **T** the target | the pool cell and its two declarations | 3 |
| **C** the copies | `+0x79 → +0x7B`, plus the pool↔run transport | 5 |
| **R** the reads | `+0x79` read as a row-03 limit/copy source | 2 |
| **S** the row-09 staging | the OL-82 literal and its two consumers | 3 |
| **W** other native writers | independent `+0x7B` producers of other rows | 9 |
| **M** the mover consumer | the stride nibble | 1 |

**23 engine sites.** Plus **7 loader/arm-mirror sites** (§3.3), which are a
different record type and outside the producer's blast radius. The table's last
two rows (class **I**) are the producer's two **unported inputs**, not
`timer7b` sites, and are outside the 23.

### 3.2 The audit table

| # | class | site | native | today | under the producer | verdict |
|---|---|---|---|---|---|---|
| 1 | T | `include/fifa96_engine/fifa96_match_entities.h:129` | word `+0x79` | declared, read-only | written by P1 | **the producer's target** |
| 2 | T | `include/fifa96_engine/fifa96_match_entities.h:128` | word `+0x7B` | declared | unchanged | the counter |
| 3 | T | `include/fifa96_engine/fifa96_match_run.h:456` | word `+0x7B` (run copy) | staged from the pool | carries the value | transport |
| 4 | C | `src/fifa96_engine/fifa96_match_entities.c:94` | `0x7DA9B` row install | `timer7b = timer79` (= 0) | **= 6** on every install | LIVE — the main blast radius |
| 5 | C | `src/fifa96_engine/fifa96_match_entities.c:209` | `0x79A86` reset-lane | `= timer79` (= 0) | the seed site's own copy | exact native site; **dead live** (FU295-L1) |
| 6 | C | `src/fifa96_engine/fifa96_match_entities.c:744` | `0x7836A/0x78374` record-0 keeper machine | `= timer79` (= 0) | **= 6 every frame, record 0** | LIVE; order before the dispatch is native |
| 7 | C | `src/fifa96_engine/fifa96_match_run.c:1377` | the staging load | copies `+0x7B` | unchanged | transport |
| 8 | C | `src/fifa96_engine/fifa96_match_run.c:1594` | the repack store | copies `+0x7B` back | unchanged | transport |
| 9 | R | `src/fifa96_engine/fifa96_match_handlers.c:5785` | `0x7E27C` row-03 slot branch B | `= e->timer79` (= 0) | **= 6** | body exported, row NULL (FU328-L2) |
| 10 | R | `src/fifa96_engine/fifa96_match_handlers.c:5821` | `0x7E2E0..0x7E2F4` row-03 idle core | `min(1, 0) = 0` | `min(1, 6) = 1` — **not 6** | body exported, row NULL; FU331-L4 |
| 11 | S | `src/fifa96_engine/fifa96_match_handlers.c:5034` | `0x80B43` `= +0x79*2` | **literal 0** (OL-82) | `= 2*+0x79` = 12 | **the one false assumption — P1 fixes it** |
| 12 | S | `src/fifa96_engine/fifa96_match_handlers.c:5036` | `0x80B51..0x80B5E` | `vel73 = mul(0, sin1) = 0` | `= 12` at face 0x100 | the row-09 consumer (pin P3) |
| 13 | S | `src/fifa96_engine/fifa96_match_handlers.c:5037` | `0x80B62..0x80B6F` | `vel75 = mul(0, t1) = 0` | `= 0` at speed 0 | the row-09 consumer (pin P3) |
| 14 | M | `src/fifa96_engine/fifa96_match_run.c:549` | the mover stride nibble | `timer7b & 0xF = 0` | `= 6` (or 12 after row 09) | **the whole-formation movement** |
| 15 | W | `src/fifa96_engine/fifa96_match_handlers.c:497` | keeper claim | `= s->timer7b` | unchanged | keeper machine's own 3/2 |
| 16 | W | `src/fifa96_engine/fifa96_match_handlers.c:737` / `:749` | row-26 plumbing | in → body → out | unchanged | value-preserving |
| 17 | W | `src/fifa96_engine/fifa96_match_handlers.c:848` / `:866` | row-2A plumbing | in → body → out | unchanged | value-preserving |
| 18 | W | `src/fifa96_engine/fifa96_match_handlers.c:2344` | row 18 | `= 2` | unchanged | independent |
| 19 | W | `src/fifa96_engine/fifa96_match_handlers.c:4861` | row 1x | `= 1` (human phase) | unchanged | independent |
| 20 | W | `src/fifa96_engine/fifa96_match_handlers.c:5822` | row-03 idle core | `= min(1, limit)` | `= 1` | independent (see #10) |
| 21 | W | `src/fifa96_engine/fifa96_match_handlers.c:5846` | `0x7E510` phase 0xC | `= 2` | unchanged | independent |
| 22 | I | `src/fifa96_engine/fifa96_match_handlers.c:2629` / `:3122` | `byte9d = 0` (OL-72/OL-69) | staged 0 | the OR term stays 0 | **the producer's second input** |
| 23 | I | `src/fifa96_engine/fifa96_match_run.c:1443` | `player_d = 0` | the `+0xD` byte staged 0 | `desc_d`'s twin | **the producer's first input** |

Every line was read this session; the native column is the carried owner doc
(`FU328` §3.3 for `0x7E27C`/`0x7E2E0..0x7E2F4`, `FU178` §3.3 for
`0x80B3D..0x80B6F`, `FU84` §6 for `0x79A86`/`0x7836A`/`0x78374` and
`0x7DA9B`, `FU142` §2 for the `+0xD`/`0x10F394` identity).

### 3.3 The loader/arm mirrors (outside the blast radius)

`include/fifa96_loader/fifa96_arm_helpers.h:31`, `fifa96_arm_bodies.c:52`
(`timer7b = table[player_d] >> 1`), `:449` (`= 2`), `:594` (`= 4`),
`include/fifa96_loader/fifa96_keeper.h:203`, `fifa96_keeper.c:502` (`= 3`),
`:562` (`= 2`). These are **separate record structs** (`fifa96_arm_record`,
keeper state) with their own native `+0x7B` writes; none has a `+0x79` field,
none sees the pool cell, and none is touched by P1. They are listed so a future
reader does not mistake the arm-body `timer7b` for a second consumer of the
producer.

### 3.4 The audit's four findings

1. **The zero is inherited, never asserted.** Three sites copy `+0x79` and two
   read it as a row-03 limit; the only literal `0` is `handlers.c:5034`. A
   producer fixes the copies for free — the
   spec §2's "the port assumes `+0x79 = 0` in places" holds *only* for the
   row-09 line.
2. **The row-09 literal is the one thing that must be fixed in the same
   commit.** Leaving it would make the producer introduce a divergence (the
   record would carry `+0x7B = 0` where the native has 12).
3. **The row-03 body is not a producer route today and would be a
   counter-*lowering* route.** `move_attr` is staged 0 (`handlers.c:5816-5821`),
   so the body computes 1; the dispatch row is NULL (`handlers.c:6346-6347`,
   FU328-L2). The live counter route with P1 is the **install copy** (#4) and
   **record 0's per-frame copy** (#6).
4. **The value is multimodal by design.** `+0x7B` is 6 (install), 12 (row 09),
   1/2/4 (rows 1x/18/26), 3/2 (keeper). FU324-L5's metric re-measure (the
   spec §8.5 clause) rides exactly this distribution — cited, not measured
   here.

---

## 4. The row-09 interplay and the OL-82 retirement

### 4.1 The native shape (carried, FU178 §3.3, `docs/ghidra/FU178_row_09_sequence.md:175`)

```
0x80B3D  MOV AX,[EBP+0x79]   ; word[+0x79]
0x80B41  ADD EAX,EAX         ; * 2
0x80B43  MOV [EBP+0x7B],AX   ; word[+0x7B] = 2 * word[+0x79]
0x80B47  MOV EAX,EBP
0x80B4C  CALL 0x79B58        ; the receiver timer (+0x93 = 0x10)
0x80B51..0x80B6F  two CALL 0x795A4 -> word[+0x73], word[+0x75]
```

The store is a **word** store: it writes `+0x7B` only, never the `+0x79` word
and never the `+0x7D` facing. `ADD EAX,EAX` doubles the zero-extended word
(`AX`), so the C form is `(uint16_t)((uint32_t)timer79 * 2u)` (wrapping for
`+0x79 ≥ 0x8000`, unreachable for the live 6..0x40 domain).

### 4.2 What the port does (fresh reads)

`handlers.c:5034` stores the literal 0; `:5036/5037` immediately multiply it by
the two sines through `match_row09_mul` (`(a*b + 0x8000) >> 16`, the `0x795A4`
port at `:4972-4979`); `:5038` derives `speed71`; `:5039-5043` copies
`pos → target` and adds `vel << 3`. So **the literal feeds three observable
outputs**: the velocity pair, the speed, and the target offset. The comment at
`:4966-4969` states why the zero was honest: "`word[+0x79]` (the `+0x7B`
source)" is one of the staged-zero unmodelled cells (FU178 §10:382-384).

### 4.3 What a live `+0x79` does

With the FU330-shaped index staging (`desc_d = 0` → `table[0] = 6`):

| cell | before | after | mechanism |
|---|---|---|---|
| `+0x7B` | 0 | **12** | `2 * 6` (0x80B43) |
| `vel73` | 0 | **12** at `face7d = 0x100` | `mul(12, sine(0x100) = 0x10000)` |
| `vel75` | 0 | 0 at `speed = 0` | `mul(12, t1 = 0)` |
| `speed71` | 0 | **12** | `distance(12, 0) = 12` (axis case, `entity_update.c:211-229`) |
| `target_x` | `pos_x` | **`pos_x + 96`** | `+= vel73 * 8` (0x80B91..0x80B9D) |
| `face7d` / `type` / `pos` | — | **unchanged** | the face path is `+0x7D`, not `+0x7B` |
| the *next* mover tick | stride 0 | **stride 12** | the `+0x7B` write-back (`match_run.c:1594`) → `:549` |

The last row is the one that matters for the formation: row 09's store leaks
into the **stride nibble**, so a row-09 record is faster for as long as the
value survives (until the next install or the record-0 copy). That is native
behaviour and must not be "sanitised".

### 4.4 The OL-82 correction (quoted, this doc's finding)

> `handlers.c:5034`: `r->timer7b = 0; /* 0x80B43 word[+0x79]*2, +0x79 zero (OL-82) */`

**Corrected:** the native is `word[+0x7B] = word[+0x79] * 2`; the zero was a
**staged-input convention** (OL-82 is FU253 §2's row-08 unmodeled-inputs class,
borrowed here for the `+0x79` source), valid only while `+0x79` had no producer.
P1 **retires the instance**, not OL-82: the line becomes
`r->timer7b = (uint16_t)((uint32_t)e->timer79 * 2u);` and the comment becomes a
citation of the FU330 census. The convention itself continues to govern
`0x1577C0/C2` and `[0x15877C]` in the same body.

### 4.5 The alias note a future reader needs

FU178 §3.3's prose calls `+0x7B` "the heading word"; the underlying reads are
`MOV EAX,[EBP+0x7B]; SAR EAX,0x10` (`0x80AC3`, `0x80AE8`), which is
**`word[+0x7D]`** — `face7d` — per the AGENTS alias rule
(`dword[addr]>>16` = the word at `addr+2`). FU178 §10 already erratum-corrects
the *second* multiply's operand; this is a second, independent erratum
candidate (FU331-L5). The port is **right** (`face = r->face7d`, `:5017/5021`)
and a future "fix" that stages the fold from `timer7b` would be a regression.

---

## 5. The producer design

### 5.1 The cell, the inputs and the value function

```
word[+0x79] = byte[0x10F394 + desc_d] | byte[+0x9D]        (0x79A45..0x79A4D)
word[+0x7B] = word[+0x79]                                  (0x79A86)
```

- `timer79` — the pool word `+0x79` (`entities.h:129`), unchanged in type.
- `desc_d` — **new** pool byte: the native `byte[[rec+4]+0xD]`, the roster
  descriptor index (FU84 §6 renders the read as `+0xA` with a `>>24`; the alias
  resolves it to `+0xD` — FU331-L2, FU330 confirms). The native reads it as a
  signed dword byte
  (`MOV EAX,[EAX+0xA]; SAR 0x18` — FU142 §2:1029) and indexes the table
  **unmasked** (`MOV AL,[EDX+EAX]; AND 0xFF`). The port keeps it a `uint8_t`
  and indexes the quoted 48-byte region; the negative/out-of-range class is a
  leg (FU331-L3). *Same native byte* as the run record's `player_d`
  (`fifa96_match_run.h:467`, `match_run.c:1443`) and FU188 §4's row-24 index —
  one producer, three consumers, when the roster descriptor lands.
- `byte9d` — **new** pool byte: the native `byte[+0x9D]` (the OR term; the
  same record byte the row-0x0B install threshold reads,
  `fifa96_outfield.h:247`). Staged 0 today (`handlers.c:2629`, `:3122`), which
  makes the OR a **written no-op** — the shape is exact, the value is not
  invented.

Both inputs default to 0 (`entities_init`'s `memset`), so the produced value is
`table[0] = 6` for every record until the descriptor lands. **That 6 is
`0x10F394[0]`, not a native per-record value** (FU328-L3); the per-record
census is FU330's.

### 5.2 The table (carried, FU84 §6:322-326; FU330 re-reads and owns it)

`0x10F394` is 48 contiguous quoted bytes — the 32-byte table and `0x10F3B4` at
`+0x20`:

```
idx  0.. 7: 06 06 06 06 06 06 06 06
idx  8..15: 07 07 07 07 07 07 07 07
idx 16..23: 0F 0F 0F 0E 0D 0D 0C 0B
idx 24..31: 0B 0A 09 09 08 07 07 06
idx 32..39: 10 13 16 1A 1D 20 23 26      (0x10F3B4 = 0x10F394+0x20)
idx 40..47: 2A 2D 30 33 36 3A 3D 40
```

The port carries it as a `static const uint8_t fifa96_match_placement_table[0x30]`
in `fifa96_match_entities.c` with the FU84 cite on the array. FU330's census
confirms or corrects each byte; a correction lands **in the table and in the
pins together**. Note the table is *not* capped at 15: indices 32..47 read
16..0x40, so "6..15" is a range of the first 32 entries, not a cap (FU328
erratum 2, restated).

### 5.3 The writer sites and the timing

```c
/* fifa96_match_entities.c */
/* Contract: desc_d < 0x30. The native read is unmasked and the region past
 * 0x10F3C3 is not quoted, so the port refuses to invent it; the out-of-range
 * class keeps the pre-producer value 0 and is FU331-L3's (unreachable while
 * desc_d is staged 0). */
uint16_t fifa96_match_entities_placement_limit(uint8_t desc_d, uint8_t byte9d) {
  if (desc_d >= 0x30u) return 0;   /* FU331-L3 */
  return (uint16_t)((uint16_t)fifa96_match_placement_table[desc_d] | (uint16_t)byte9d);
}

void fifa96_match_entities_placement_seed(struct fifa96_match_entity *e) {
  e->timer79 = fifa96_match_entities_placement_limit(e->desc_d, e->byte9d); /* 0x79A45..0x79A4D */
  e->timer7b = e->timer79;                                                  /* 0x79A86 */
}
```

Two call sites, the native order preserved inside the helper:

1. **`fifa96_match_entities_reset_lane`** — replace `entities.c:209`
   (`e->timer7b = e->timer79;`) with `fifa96_match_entities_placement_seed(e);`
   at the same position in the per-record loop. This is the exact native site.
   It is **dead on the live engine path** (FU295-L1: the `0x741C4` driver and
   `FUN_0008C33C` are unported), so it alone would change nothing.
2. **`fifa96_match_entities_init`** — call the seed for each record after the
   per-record seeding loop (`entities.c:40-52`, after `e->active = i` at `:51`). This is
   the **derived seam** for the unported caller: the native limit is established
   by the reset pass at match begin, and `init` is the port's match-begin
   construction (`match_run_begin → entities_init`, `match_run.c:2378`). It is
   the same "derived subset at the port's begin" pattern the tree already uses
   for unported drivers, and it is what makes the producer **live**.

**Timing honesty:** the native store re-runs on every team reset pass; if the
caller census (FU330) shows it runs per-frame, the value is a per-record
descriptor **constant** and the port's init seed is observationally identical.
If FU330 shows a mid-match re-seed with a *changed* descriptor, the port needs a
re-seed at that event — **FU331-L1** owns the question; the design's helper makes
the re-seed a one-line call, so the shape survives either answer.

Ordering requirements (both native): the `+0x79` store **precedes** the `+0x7B`
copy in the same pass (0x79A4D < 0x79A86), and the `+0x7B` install copy
(`entities.c:94`) is a *later, independent* site that re-copies the same value on
every row change. The reset-lane port's write order of `+0x7B` vs `+0x77`
(`bound`) differs from the native byte order, but the cells are disjoint — no
observable effect; noted so a future byte-order audit does not re-open it.

### 5.4 The `+0x7B` interaction (`entities.c:94` and friends)

| route | native | with the producer | live? |
|---|---|---|---|
| row install `entities.c:94` | `0x7DA9B` `= +0x79` | `+0x7B = 6` on every code change | **yes — the main route** |
| reset-lane `entities.c:209` | `0x79A86` | `= limit`, then install again at `:230` | no (FU295-L1) |
| record-0 machine `entities.c:744` | `0x7836A/0x78374` | `= 6` every frame, record 0, **before** the dispatch | yes |
| row-03 slot B `handlers.c:5785` | `0x7E27C` | `= 6` | no (row NULL) |
| row-03 idle core `handlers.c:5821` | `0x7E2E0..0x7E2F4` | `min(1, 6) = 1` | no (row NULL) |
| row 09 `handlers.c:5034` | `0x80B43` | `= 12` | only for a code-09 record |

No feedback loop: none of the copies writes `+0x79`. The record-0 route runs
before the dispatch in both the native walk and the port (`entities.c:737-752`
documents the single `CALL dword[rec+0x18]` after the machine), so a row-09
record 0 write would win for the frame and be re-copied next frame — native.

### 5.5 What P1 must *not* do

- **Do not wire row 03.** `handlers.c:6346-6347` stays NULL; the counter route
  via `min(1, limit)` would *lower* the install value (FU331-L4).
- **Do not stage a measured per-record value.** `desc_d`/`byte9d` stay 0 until
  the roster descriptor (FU301-L3 / OL-72) or FU330's census supplies real
  data; staging, say, entity 12's value would be the spec §7 trap.
- **Do not touch the loader/arm mirrors** (§3.3) or the other rows' writers.
- **Do not fold the golden re-pin into the acceptance commit** (spec §8.1).

---

## 6. The pins (RED / GREEN)

All three are unit pins (no ISO, no golden). "RED" = the exact first failing
assert on the current tree; "GREEN" = the exact expected value after P1.

### 6.1 P1 — the producer (`tests/test_engine_match_entities.c`, new
`test_placement_limit_producer`, registered near `:1220`)

```c
struct fifa96_match_entities pool;
assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
/* RED: timer79 == 0 (no producer). GREEN: 0x10F394[0] = 6 */
assert(pool.team[TEAM0].records[3].timer79 == 6);
assert(pool.team[TEAM0].records[3].timer7b == 6);      /* 0x79A86 */
/* the table index and the OR are both exercised */
pool.team[TEAM0].records[4].desc_d = 22;               /* 0x10F394[22] = 0x0C */
assert(fifa96_match_entities_reset_lane(&pool, TEAM0, 0, 0, 0) == FIFA96_OK);
assert(pool.team[TEAM0].records[4].timer79 == 0x0C);
assert(pool.team[TEAM0].records[4].timer7b == 0x0C);
pool.team[TEAM0].records[5].desc_d = 16;               /* 0x0F */
pool.team[TEAM0].records[5].byte9d = 0x10;             /* OR -> 0x1F (discriminates) */
pool.team[TEAM0].records[6].desc_d = 0;
pool.team[TEAM0].records[6].byte9d = 0x80;             /* 6 | 0x80 = 0x86 */
assert(fifa96_match_entities_reset_lane(&pool, TEAM0, 0, 0, 0) == FIFA96_OK);
assert(pool.team[TEAM0].records[5].timer79 == 0x1F);
assert(pool.team[TEAM0].records[6].timer79 == 0x86);
```

RED today: the first `timer79 == 6` (it is 0). GREEN: all pass. The `0x0C` /
`0x1F` / `0x86` literals are the carried table + the OR and move with the table
if FU330 corrects it.

### 6.2 P2 — the counter (`tests/test_engine_match_entities.c`, extend
`test_install_stages` at `:157-174`, or the new test)

```c
struct fifa96_match_entity e;
memset(&e, 0, sizeof e);
e.active = 1;
e.desc_d = 22;                                  /* limit 0x0C */
fifa96_match_entities_placement_seed(&e);
assert(e.timer79 == 0x0C && e.timer7b == 0x0C);
assert(fifa96_match_entities_install(&e, 2, 0x0B, 0) == 1);
assert(e.timer7b == 0x0C);                      /* 0x7DA9B re-copies */
assert((e.timer7b & 0x0F) >= 3);                /* FU328 §3.3 clause 1 */
```

RED today: `placement_seed` does not exist (compile-RED), and the
behavioural RED — `install` with `timer79 = 0` leaves `+0x7B = 0`, so
`(timer7b & 0x0F) >= 3` fails. GREEN: 0x0C, 12 ≥ 3.

### 6.3 P3 — the row-09 consumer (`tests/test_engine_match_handlers.c`, new
`test_action_09_placement_limit_velocity` beside `:3595`)

```c
struct fixture f; make_fixture(&f);
struct fifa96_match_entity *e = &f.mr.entities.team[0].records[0];
f.mr.record.entity_id = 0;
f.mr.state.phase = 2;
f.mr.record.active = 1;
f.mr.record.stage92 = 0;
f.mr.record.lane_x = 0;            /* speed 0: the position path stays put */
f.mr.record.pos_x = 0x100;
f.mr.record.pos_y = 0x10;
f.mr.record.pos_z = 0x200;
f.mr.record.face7d = 0x100;        /* sine(0x100) = 0x10000 exactly */
e->lane_z = 0;                     /* dx == dz == 0 -> the face is not recomputed */
e->cam_dz6f = 0;
e->timer79 = 6;
assert(fifa96_match_dispatch_action(&f.mr, 0x09) == FIFA96_OK);
assert(f.mr.record.timer7b == 12);         /* 0x80B43: 2 * 6     RED: 0 */
assert(f.mr.record.vel73 == 12);           /* mul(12, 0x10000)   RED: 0 */
assert(f.mr.record.vel75 == 0);            /* invariant: mul(12, 0) and mul(0, 0) */
assert(f.mr.record.speed71 == 12);         /* distance(12,0)     RED: 0 */
assert(f.mr.record.target_x == 0x160);     /* 0x100 + 12*8       RED: 0x100 */
assert(f.mr.record.target_z == 0x200);     /* invariant */
assert(f.mr.record.stage92 == 1);          /* invariant */
```

RED today: `timer7b == 12` (it is 0). GREEN: all pass. The sine/distance
identities are pinned by the table's last entry (`65536`) and
`entity_update.c:211-229`; if either changes, this pin moves (FU331-L7).
**Zero-value companion (the retirement is conditional):** the same call with
`e->timer79 = 0` must give `timer7b == 0, vel73 == 0, speed71 == 0,
target_x == 0x100` — the current behaviour stays reachable by staging, which is
what keeps the pin a *producer* pin and not a rewrite.

### 6.4 The existing pins that must stay green — and the one that moves

Stay green (fresh reads this session): `test_engine_match_entities.c:166-173`
(the install copy with a staged `timer79 = 0x1234` — the copy is unchanged),
`:587-590` (the record-0 per-frame copy with `0x2222`),
`test_engine_match_handlers.c:1189` (the row-03 body with an explicit
`e->timer79 = 0` — the staging still wins over init, so the zero case stays
pinned), `test_engine_match_frame.c:417/424` (row-26 `player_d = 0` → 3),
`tests/test_arm_bodies.c` (the loader mirrors — untouched).

**Moves: `test_action_09_stage0_set_branch` (`:3613-3614`).** It asserts
`target_x == 0x100` / `target_z == 0x200` ("target = pos"), which is only the
first half of `0x80B8E..0x80B9D`: the native then adds `vel << 3`. With
`timer79 = 6` from `make_fixture`'s `entities_init`, `vel73` is non-zero for its
staged face (`lane_z 0x40`, `cam_dz6f 0x20`) and `target_x` becomes
`0x100 + 8*vel73`. The P1 commit must **re-derive** the assert (cite
`0x80B91..0x80B9D`), not silently re-pin it; `target_z` stays `0x200` because
`speed = 0` forces `t1 = 0` → `vel75 = 0`.

**Fixture baseline:** `make_fixture` runs `begin` (→ `entities_init`) and then
`entities_init` again (`:153`), so **every handler test's baseline
`+0x79/+0x7B` shifts 0 → 6**. The P1 RED run must enumerate every test that
moves; a moved test is re-derived against the native or reported, never
re-pinned to the new output.

### 6.5 The golden

P1 is a **golden-moving** fidelity commit by construction (FU328-L8): the first
record install or mover tick after `init` already carries `+0x7B = 6`, so the
M2 transcript's first encoded formation state must differ. The pins:
M1 `09b726b7…` must stay byte-identical (`cmp`; the producer lives in the
match-entity TU, not the m1 camera path — a prediction, the `cmp` is the pin);
M2 `d5446144…` re-pins **only** in P1 with the written reason, the first
differing line, the moved-line count and the `state=` suffix check (spec §6,
FU333's recipe). The A3 window (`tests/test_engine_m2.c:3972-3979`) re-pins in
P2 with FU332's measured pacing, not here. Any diff before the first
producer-attributable frame is a stop-and-report. **The numbers are FU332's;
this doc asserts none.**

---

## 7. The FU328 mapping

| leg | status after the design | owner of the remainder |
|---|---|---|
| **FU328-L1** the `+0x79` producer gap | **closed by P1** — two writers, exact order, `+0x7B` copied in the same pass; the live route is the init seed | — |
| **FU328-L2** row-03 NULL + staged `move_attr 0` | **not closed, and now provably a counter-*lowering* route** (`min(1, limit) = 1 < 6`); P1 must leave the row NULL | FU177-L1 (FU331-L4) |
| **FU328-L3** the native value for entity 12 unmeasured | **not closed** — the port stages `desc_d = 0` → `table[0] = 6` uniformly; the per-record census and the `+0x9D` producer are the missing inputs | FU330 + the roster descriptor (FU331-L2/L3) |
| **FU328-L8** the fix's M2 exposure | **confirmed by construction** (every install → `+0x7B = 6` → stride 6); the measured diff is not this doc's | FU332 (the numbers), FU333 (the governance) |

The spec §8.2 rule is honoured: the port's value comes from the native
**table** and the native **expression**, with the two unported inputs declared
and zero-staged; the diagnostic ladder (3/6/15) is never used as a value.

---

## 8. Numbered legs (FU331-L1…)

1. **FU331-L1 — the native call-site cadence.** The live caller of
   `FUN_0008C33C` is the unported `0x741C4..0x744CE` driver (FU295 §2:24,
   FU295-L1); the port seeds at `entities_init` (the begin construction). If
   FU330's caller census shows the reset pass runs **per frame**, the value is a
   descriptor constant and the seed is exact; if it shows a mid-match re-seed
   with a changed descriptor, a re-seed call is needed at that event (the
   helper makes it one line). **Not asserted either way.**
2. **FU331-L2 — the descriptor index is unported.** `desc_d` =
   `byte[[rec+4]+0xD]` (reconciled from FU84 §6's `byte[[rec+4]+0xA]>>24`
   through the `>>24` = `+3` alias rule; the same byte as FU142 §2:1029 and
   FU188 §4:218 and §8:432). The `[rec+4]` binding and its producer are the
   FU167/FU168/FU176/FU204/FU301 leg class (FU301-L3); the port stages 0. With
   `desc_d = 0` every record gets `table[0] = 6` — **declared, uniform, not
   native per record**.
3. **FU331-L3 — the `+0x9D` OR and the index domain.** `byte[+0x9D]` has no
   producer (OL-72; staged 0 at `handlers.c:2629`/`:3122`). The native read is
   unmasked (`AND 0xFF` on the byte, no index mask) and sign-extends the
   descriptor byte (`SAR 0x18`), so a descriptor ≥ 0x30 or negative reads
   outside the quoted region; the port's 48-byte array covers 0..0x2F and the
   out-of-range class is unobservable while `desc_d = 0`. Also unconfirmed:
   whether `BX`'s high byte is cleared before `0x79A4D MOV [EBP+0x79],BX`
   (FU84 §6 renders a word assignment; FU328 erratum 2 gives `OR BL,DL` only).
   **FU330's fresh read owns both.**
4. **FU331-L4 — row 03 must not be wired by P1.** With `move_attr` staged 0 the
   idle core computes `min((0>>22)+1, limit) = 1` (`handlers.c:5816-5822`), i.e.
   *below* the install value 6. Wiring the row before staging the real
   `[rec+0x63]` (FU324 §3.1, its low half has no writer) would slow the
   formation and fake a regression. Owner: FU177-L1 / FU328-L2.
5. **FU331-L5 — FU178 §3.3's "heading word `+0x7B`" is `word[+0x7D]`.** The
   two reads are `dword[+0x7B] >> 16`; the port correctly stages `face7d`
   (`handlers.c:5017/5021`). FU178 is append-only; the erratum is recorded here
   and should be quoted in FU333's register. A future reader must not "fix" the
   fold onto `timer7b`.
6. **FU331-L6 — the `+0x7B` value distribution is multimodal.** 6 (install),
   12 (row 09), 1/2/4 (rows 1x/18/26), 3/2 (keeper). FU324-L5's metric
   sensitivity (the spec §8.5 clause) rides this distribution; the re-measure is
   its own clause, cited not performed.
7. **FU331-L7 — the P3 pin's arithmetic rests on two carried identities:**
   `fifa96_entity_sine(0x100) = 0x10000` (the 257-entry table's last value) and
   `fifa96_entity_distance(12, 0) = 12` (the axis case). A refactor of either
   moves the literal, not the mechanism.
8. **FU331-L8 — the golden attribution is unmeasured here.** P1's first M2
   differing line must be the first encoded record state after `init`; a
   difference earlier, or a `state=`/stratum change, is a stop-and-report
   (FU332 measures, FU333 governs). M1's invariance is a prediction until the
   `cmp` runs.

---

## 9. GO / NO-GO

**GO — the producer design and P1.** The audit found exactly one false
assumption (`handlers.c:5034`), zero writers of `+0x79`, and a native expression
whose two inputs are individually named and individually staged. The design
adds one value function, two writers (the exact native site + the derived
live seam), two declared staging bytes, one table, and the row-09 retirement —
all citeable, none invented. Three RED/GREEN pins exist with exact literals.

**NO-GO — any claim beyond the design.** Not this doc's to make: the native
per-record values (FU330), the `+0x9D` producer (OL-72), the descriptor
producer (FU301-L3), the M2 diff and the natural band entry (FU332), and the
witness/A3/Layer-2 governance (FU333). **P1 must not wire row 03, must not
stage a measured value, and must not fold the golden re-pin into the acceptance
commit.**

**Census/README.** The producer is a state-fidelity change, not a new action
row: `60/80 = 75%` stands; no README recompute.

---

## 10. Known limits

- **No Ghidra, no probe, no build this slice.** The native bytes are carried
  (FU178/FU84/FU142/FU328); the port/test lines are fresh reads. FU330 may
  correct the table or the `BX` word form; the doc's shape does not depend on
  those corrections, only the literals do.
- **The value is a declared staging.** `table[0] = 6` is *not* the native value
  for any record; it is what the native expression yields with both unported
  inputs at zero. The design's fidelity claim is the **machinery** (cell,
  expression, order, rows), not the per-record number.
- **The row-09 interplay is pin-verified, not golden-verified.** Whether the M2
  tape ever dispatches a code-09 record is a measurement (FU332); if it does
  not, the row-09 change is invisible to the golden and only P3 proves it.
- **The fixture-baseline shift is a test-surface risk.** Every handler test's
  `+0x79/+0x7B` becomes 6; the P1 RED run must enumerate the moved tests.
  This doc names the one it can prove (`test_action_09_stage0_set_branch`) and
  predicts no others.
- **The init seed is a derived seam for an unported caller** (FU331-L1). It is
  the honest minimum, not a proof of the native cadence.
- **The `desc_d`/`byte9d` additions change the pool struct's size.** No test
  pins `sizeof (struct fifa96_match_entity)` (grep this session); if a future
  serialization appears, it must carry the two bytes.

---

## 11. Provenance and self-audit

**Fresh reads this session** (repo-static): `include/fifa96_engine/fifa96_match_entities.h`
(60-189, 430-470), `include/fifa96_engine/fifa96_match_run.h` (405-494, 985-1010),
`src/fifa96_engine/fifa96_match_entities.c` (1-60, 60-244, 720-759),
`src/fifa96_engine/fifa96_match_handlers.c` (470-589, 700-889, 2300-2369,
4835-4879, 4960-5079, 5760-5859, 6330-6360),
`src/fifa96_engine/fifa96_match_run.c` (490-569, 1360-1466, 1583-1612,
2353-2610, 3491-3527), `src/fifa96_loader/fifa96_action_handlers.c` (200-329,
362-370), `src/fifa96_loader/fifa96_outfield.c` (688-706),
`src/fifa96_loader/fifa96_camera.c` (748-780),
`src/fifa96_loader/fifa96_entity_update.c` (20-73, 211-229),
`tests/test_engine_match_entities.c` (120-180, 280-337, 580-595),
`tests/test_engine_match_handlers.c` (1175-1225, 3590-3669),
`tests/test_engine_match_frame.c` (393-424).

**Carried (named, not re-derived):** FU328 §3.3 (the `0x7E2E0..0x7E2F4`
counter), §5.6 (the diagnostic, used only as a causal label), §8 L1-L3/L8, §9,
errata 2/3; FU84 §6:322-336 (the `0x10F394` family and `FUN_0007997C`); FU178
§3.3:175/196-202 and §10:382-384; FU142 §2:1029 (the row-26 `+0xD` read);
FU188 §4:218 and §8:432 (the same byte); FU295:24 + L1 (the unported driver); FU151
§2.8 (the reset-lane pass); FU253 §2 (OL-82's row-08 scope); FU324 §3.1/§5
(the `+0x7B` producers and the unported `+0x63` low half); FU329 §2 (the
Branch-B framing, forward-cited); the phase-35 spec §3/§4/§8/§9.2.

**Literal corrections recorded here.** (a) The spec §2/§3's "the port assumes
`+0x79 = 0` in places" is narrowed to the single literal at
`handlers.c:5034`; the copies inherit, they do not assume (§3.4 finding 1).
(b) The spec §4's "FU328 §4.3" for the counter has no §4.3 in FU328 — the fresh
`0x7E2E0..0x7E2F4` read lives in **FU328 §3.3**; the spec cite is read as that
section. (c) FU84 §6's index expression `byte[[rec+4]+0xA]>>24` is
`byte[[rec+4]+0xD]` under the `>>24` = `+3` alias rule, which makes the
producer's index and row 26's/FU188's index **one byte**, not two (FU331-L2).

**Self-audit.** (a) No native byte is asserted beyond its owner doc; the table
is FU84's quote, flagged for FU330. (b) No value is taken from the diagnostic
or the wave's seen range. (c) Every port/test line was read this session;
every expectation is arithmetic shown in the doc. (d) The RED forms are
runnable on the current tree (compile-RED only where a new symbol is
introduced, declared as such). (e) The M2/M1 goldens are named, never moved
here; no push, no commit, no build, no ISO write. (f) Census and README
untouched.

## Review erratum (wave review, 2026-10-23)

**1. The table is not a new `fifa96_match_placement_table`.** Quote (§5.2): "The port carries it as a `static const uint8_t fifa96_match_placement_table[0x30]` in `fifa96_match_entities.c` with the FU84 cite on the array." **Corrected (fresh source read):** no such symbol exists; the port carries the 32-byte table once, as `fifa96_arm_26_timer_table[32]` (`fifa96_arm_bodies.c:32-37`). The proposed `[0x30]` array is **new** and would duplicate those 32 bytes. P1 should reuse/share/extend the existing table (or move it to a shared header), not add a second copy; the 48-byte region's rows 32..47 are the `0x10F3B4` continuation (FU330 §4), not a second tier table.

**2. The tier boundary is `0x20`, not `0x30`.** Quote (§5.3): "`if (desc_d >= 0x30u) return 0;` … the out-of-range class keeps the pre-producer value 0". **Corrected (FU330 §2/§4):** the tier table is 32 bytes (indices 0..31, values 6..15); `0x20..0x2F` is the `0x10F3B4` continuation (16..64), the unguarded-index class of FU330-L2, not a tier row. A `desc_d >= 0x20` model should either stop at 0x20 or explicitly label 0x20..0x2F as the continuation; and the fallback must **not write 0** into a live cell (0 is below the native floor and re-creates the silent bind). Leave the cell unchanged (or clamp) on the out-of-range class, and pin the choice.

**3. P1 is one of three `+0x79` writers, not the closed leg.** Quote (§7): "**FU328-L1** the `+0x79` producer gap | **closed by P1**". **Corrected (FU330 §2/§3.2/§3.3):** P1 lands the placement-reset writer (`FUN_0007997C`); the **lineup rebind** P2 (`0x8C850`, halving floor 2 at `0x8C939/0x8C942`) and the **code-0x0C no-replacement** P3 (`0x822E7/0x822F0`, floor 3) remain unported. FU328-L1 is narrowed, not closed; FU333-L15 owns the remainder. The port's placement value is therefore the *reset* value, not the full native three-writer set.

**4. The cadence is answered by FU330.** Quote (§5.3): "if FU330 shows a mid-match re-seed with a changed descriptor, the port needs a re-seed at that event — FU331-L1 owns the question." **Corrected (FU330 §6.2):** the reset pass is prelude-only (match start / kickoff / period transition, four callers), not per-frame; but P2 is a mid-match lineup rebind and P3 a mid-match contact arm. The init-only seed is exact while the value is constant and neither writer runs; the kickoff/period re-store and P2/P3 re-seeds are residuals, not a per-frame question. Add the re-seed hooks (or a numbered leg) and drop the "per-frame" conditional.

**5. The index form and the site count.** Quote (§5.1): "indexes the table **unmasked** (`MOV AL,[EDX+EAX]; AND 0xFF`)"; quote (§3.1): "**23 engine sites** … The table's last two rows (class **I**) … are outside the 23." **Corrected:** the fresh P1 has no `AND` on the index — `0x79A3F ADD EDX,EBX` uses the sign-extended index and `0x79A41 XOR BH,BH` clears the **store** high byte (FU330 §3.1); the port's `uint8_t`/guard is a model. The 23 are timer-cell **lines** (the W class is 9 lines in 7 rows); the audit table's 23 numbered rows include the two input rows (#22/#23), so it lists 25 lines. Make the count and the quoted form explicit so the completeness claim is auditable.

**6. "Not capped at 15" needs the exact qualification.** Quote (§5.2): "Note the table is *not* capped at 15: indices 32..47 read 16..0x40, so '6..15' is a range of the first 32 entries, not a cap (FU328 erratum 2, restated)." **Corrected (FU330 §2/§6.1):** 15 **is** the proven in-range maximum (table max 15, OR 0/0x0F); 6 is **not** a floor (P2/P3 lower to 3, composed to 2). The range fails as a *cap* only because the index is unguarded (32..47 and the 0x80+ wrap) and because P2/P3 can lower the register. Restate it that way; the 32..47 bytes are the unguarded continuation, not the producer's tier.
