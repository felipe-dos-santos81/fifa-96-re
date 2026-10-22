# FIFA 96 M2 phase-32 recon-ahead plan (FU318–FU321)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce four frozen evidence docs — FU318–FU321 — that decide the phase-32 route to FU314-L1a: a natural **non-zero-delta** row-04 pan via the west-side drive (FU314 R2) or via the live face producers (FU314-L5/L3), with the exact port shape, drive, M2 exposure, and GO/NO-GO verdicts.

**Architecture:** Four independent read-only slices, one agent each, each derived from fresh Ghidra MCP reads of `/FIFA96.EXE` plus the repo sources; drafts land under `docs/ghidra/`, receive a whole-wave review + quoted errata, and are promoted one-per-commit. No production code, no test/golden change.

**Tech Stack:** Ghidra MCP (standard tools only), rea MCP where useful, the repo's C sources + docs, out-of-tree measurement probes linking `build/libfifa96_*.a`.

**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md`

## Global Constraints

- **Read-only wave:** no repo source/test/golden change; the only repo writes are the four draft docs. Measurement probes are out-of-tree (`/tmp/opencode/`) copies; the ISO is read-only.
- **Ghidra:** the program argument is exactly `/FIFA96.EXE` (case-sensitive); LE-image globals sit at `+0x100000`; **scripts are disabled**; standard MCP tools only; never objdump a stale copy; caller counts need a fresh `get_xrefs_to` in the same session; verify CALL bytes before trusting a target; the alias traps (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte `addr+3`).
- **Evidence rule:** every address/byte claim cites a fresh disassembly or raw-byte read; anything not statically provable becomes a **numbered leg** with its native owner — never asserted, never faked. Prior doc sections are never rewritten; corrections land as quoted **errata**.
- **Doc format:** single H1; `**Phase/Provenance:**` line; the question; the evidence table (claim → fresh evidence); port shape; numbered legs; GO/NO-GO verdict; known limits.
- **Goldens:** M1 `09b726b7…` immutable; M2 `8434cfab…` is the baseline — the wave may not re-pin; an expected move is analyzed, never applied.
- **Scope:** the census holds `60/80`; the armer/follow (FU316) stay out.

## Review Focus

1. **The gate algebra for an offset record** — a "non-zero-delta" witness that quietly re-proves the zero-delta case (the bearing folding to `0x80`).
2. **The `+0x7D` producer census** — a "ported" claim that is actually octant-only (`fifa96_arm_face` writes the sector, not the face).
3. **The `direct_face` staging** — a partial turn that reads a point the port never stages, changing the sim without the native input.
4. **The drive** — a staged period/clock or a tuned hold that masquerades as natural play.
5. **The M2 exposure** — a face change that moves frames beyond the intended gate consequence.

---

### Task 1: FU318 — the `+0x7D` producer census (FU314-L3)

**Files:**
- Create: `docs/ghidra/FU318_r2l6_face_producers.md`

**Interfaces:**
- Consumes: the spec §2/§3.1; FU314 §2.3 + §7.3 + the review erratum; the port sources below.
- Produces: the census table, the natural-producer verdict, the minimal landing shape, GO/NO-GO; legs `FU318-L1…` for FU319/FU321.

- [ ] **Step 1: Re-derive the native helper and its callers.** `disassemble_bytes 0x79C50/0x60`; `get_xrefs_to 0x79C50` fresh (record the count and every site); classify each site: which cell(s) it can write (`+0x7D`/`+0x8E`), and whether the `(0,0)` early return (`0x79C59..0x79C68`) makes it a no-op.
- [ ] **Step 2: Re-derive the per-frame arm.** `decompile_function 0x8E008`; verify the `0x8E1CB` call and its `≥0x70` row-byte gate; `read_memory 0x10F334/0x40` + `0x10F33C/0x40` (the `−off[+0x8E]` tables); `disassemble_bytes 0x8E1A0/0x60`.
- [ ] **Step 3: Re-derive the locomotion turn.** `disassemble_bytes 0x8E240/0x90`; the `0x8E29C CMP byte[+0x43],0` and the turn block; cross-read `src/fifa96_loader/fifa96_action_handlers.c:202-300`.
- [ ] **Step 4: Audit the port sites.** Fresh search for `fifa96_arm_face` and `fifa96_entity_face`; for each call site record what it writes (`r->type` vs `r->face7d`+sector); confirm the split against the native both-cell contract. Name the sites by file:line.
- [ ] **Step 5: Decide the natural producer.** Which producer runs for a walking code-4 record in the measured natural window (the phase-31 T1 run, 3,598 granted frames), and which one the non-zero-delta route needs (R1 vs R2). State the evidence and the counter-evidence.
- [ ] **Step 6: Write the doc** to `docs/ghidra/FU318_r2l6_face_producers.md` in the required format, with the GO/NO-GO and the M2 risk (name every consumer of `+0x7D`: row 04 `0x7EE5C`/`fifa96_outfield.c:632`, the defender decision `fifa96_outfield.c:384`, the ball pairing, and any others found fresh).
- [ ] **Step 7: Self-audit** every claim against a fresh read; convert anything unproven into a numbered leg; keep the evidence rule.

---

### Task 2: FU319 — the mover's `direct_face` (FU314-L5)

**Files:**
- Create: `docs/ghidra/FU319_r2l6_direct_face.md`

**Interfaces:**
- Consumes: the spec §2/§3.2; FU314 §2.3 + §7.5; FU318's turn evidence if it lands.
- Produces: the `+0x43` producer/semantics, the port staging shape, the expected sim diff, GO/NO-GO; legs `FU319-L1…`.

- [ ] **Step 1: Find the native `+0x43` producers.** Fresh `get_xrefs_to` on the mover family (`0x8E244`/`0x7BF20`) and the action rows; disassemble/decompile every write to `byte[rec+0x43]` you can prove; record the semantics (who sets it, to what, when it clears).
- [ ] **Step 2: Re-derive the turn's other inputs.** The `0x57A73` point (what writes it: the camera focus/ball?), the `face_x/face_z` staging, `move_attr >> 16 > 0x60`, the `speed`/`has_slot` turn limit; `read_memory`/xrefs as needed.
- [ ] **Step 3: Audit the port.** `src/fifa96_engine/fifa96_match_run.c:489-548` (the mover, `direct_face = 0`, the write at `:532`); the record struct (does it model `+0x43`/`face_x/face_z`?); the locomotion struct/header; the test surface that can observe the face.
- [ ] **Step 4: Specify the minimal faithful staging.** The exact field(s), the source cell(s), the call shape, and what it changes (the face, the gates that read it, the ball pairing, the AI). If the focus point is required, name it and say whether it is in the minimal slice or a leg.
- [ ] **Step 5: Predict the M2 diff.** Which match frames change and why (the intended gate consequence vs collateral); the exact re-pin procedure the Stage-2 plan must use.
- [ ] **Step 6: Write the doc** in the required format with GO/NO-GO and known limits.
- [ ] **Step 7: Self-audit** as Task 1.

---

### Task 3: FU320 — the non-zero-delta natural drive (R1/R2)

**Files:**
- Create: `docs/ghidra/FU320_r2l6_nonzero_drive.md`

**Interfaces:**
- Consumes: the spec §2/§3.3; FU314 §6 + the review erratum (the measured tuple); FU317's drive; the T1 witness.
- Produces: the drive(s) that produce the offset gate tuple, the exact frames/pins, GO/NO-GO; if NO-GO, the minimal producer change that creates one; legs `FU320-L1…`.

- [ ] **Step 1: Re-derive the gate algebra for an offset record.** From `fifa96_entity_angle` (`src/fifa96_loader/fifa96_entity_update.c:75-119`) and `src/fifa96_loader/fifa96_outfield.c:626-635`: the pass set for `face 0x100` and for a live face; the west-side cone; the zero-delta special case. Write the algebra in the doc with the exact fold.
- [ ] **Step 2: Build the probe.** An out-of-tree copy of the natural pass (the FU314 §4 method: a `/tmp` copy of `tests/test_engine_m2.c` with a per-granted-frame dump linking `build/libfifa96_*.a`) or a test-side instrumented run; sample the post-frame gate tuple (record id, pos, lane, bound, `face7d`, bearing, Δ, `controlled`, `tracked`, camera cells) for the candidate drives. Do not modify the repo.
- [ ] **Step 3: Sweep the drives.** The T1 hold start/length variants around 1810/1820; the west-side crossing (a longer/earlier hold, a second hold); the mirrored side/second half; a restart; an over-running defender. For each: the band-entry tuple, the pan (place/pan/tracked), and the negative result.
- [ ] **Step 4: Nominate the witness.** The exact drive (keys + frame indices), the gate frame, the offset magnitude, the expected claim/pan cells, the delta-run shape; the no-code R2 verdict.
- [ ] **Step 5: If no R2 witness exists**, name the minimal producer change (from FU318/FU319) that would create one and the drive that would then exercise it; mark the GO/NO-GO accordingly.
- [ ] **Step 6: Write the doc** in the required format with the measured table, the nominated drive, GO/NO-GO, known limits (the probe's sampling, the ISO-only run, the timing coupling).
- [ ] **Step 7: Re-run the nominated drive once** to confirm the tuple is stable before reporting.

---

### Task 4: FU321 — the acceptance and the golden risk

**Files:**
- Create: `docs/ghidra/FU321_r2l6_nonzero_acceptance.md`

**Interfaces:**
- Consumes: the spec §5/§6; the phase-31 T3 acceptance (`docs/screens/p31-l2-notes.md`, the register); the camera walk (`src/fifa96_loader/fifa96_camera.c`, `src/fifa96_engine/fifa96_match_run.c`); FU320's nominated drive.
- Produces: the Layer-2 procedure, the expected place delta/trace cells, the M2 policy, the hazards, GO/NO-GO; legs `FU321-L1…`.

- [ ] **Step 1: Derive the expected pan.** From the gate tuple and the seed (`word[+0x73]<<2`, `word[+0x75]<<2` at `src/fifa96_loader/fifa96_outfield.c:692-701`) through `fifa96_camera_event_set`/`update_walk`: the expected place delta for a non-zero-delta pan vs the zero-delta `−0x90`; the Y/Z expectations.
- [ ] **Step 2: Design the acceptance.** The `make game` drive (FU320's keys + the tape-equivalent timing), the same-clock pan/still pairs, the trace cells (place/pan/tracked/Y/Z/follow), the AE relative, the bbox caveat; the trace-primary rule.
- [ ] **Step 3: Design the M2 accounting.** The faithful-move policy: the frame-diff format, the reason, the register pin, the golden `cmp`; the m-42/fixture hazards; the census.
- [ ] **Step 4: Enumerate the hazards.** The controlled fallback (`is_ball_track`), the claim ordering, the one-shot/knife-edge, the drive timing, the sprite-only bbox, the period staging if used.
- [ ] **Step 5: Write the doc** in the required format with GO/NO-GO and known limits.
- [ ] **Step 6: Self-audit** as Task 1.

---

### Task 5: The whole-wave review + promotion

**Files:**
- Modify: the four drafts (errata only: append `## Review erratum (wave review, YYYY-MM-DD)`).
- Commits: one per doc (`docs(fu318): …` … `docs(fu321): …`).

**Interfaces:**
- Consumes: the four drafts; the spec; FU314–FU317.
- Produces: the frozen wave the Stage-2 producer plan argues from.

- [ ] **Step 1: Read the four drafts against the spec and FU314–FU317.** Check: every claim has fresh evidence; the caller counts are fresh; the alias traps are respected; the unproven items are legs; the verdicts are honest; no production claim is smuggled.
- [ ] **Step 2: Cross-check the slices.** FU318's producer verdict vs FU319's staging vs FU320's drive vs FU321's acceptance — no circular evidence, no contradiction left unrecorded.
- [ ] **Step 3: Append quoted errata** for every correction (prior text stands; the erratum quotes it and corrects it).
- [ ] **Step 4: Commit one doc per commit**, pushing each; verify the docs-only change and that M1/M2 are unmoved.
- [ ] **Step 5: Report the wave verdicts** and hand off to the Stage-2 producer plan.
