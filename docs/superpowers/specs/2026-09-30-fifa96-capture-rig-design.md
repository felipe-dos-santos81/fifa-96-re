# FIFA 96 — P0 Capture Rig (FU-2 file trace + FU-3 codec buffers) — Design

Date: 2026-09-30. Status: approved in chat (approach A, "single tagged-stream TSR").
Parent program: full playable SDL port; P0 is the first sub-project because the
FU-2 (CRC/filename attribution) and FU-3 (codec-funnel) gaps gate every later
visual/audible claim. This spec covers the rig only; FU-2/FU-3 closeouts that
*consume* traces are follow-on slices.

## Goal

A capture rig that produces one ordered, per-session binary trace of (a) every
INT-21 file operation the original `FIFA96.EXE` performs (filename, params,
bytes-read, buffer hash) and (b) register/buffer snapshots at the
map-cited codec-funnel sites, delivered to the host via the DOSBox-X serial
port as a file. The trace is parsed by an in-repo C tool and becomes
citable ground truth ("runtime-captured" evidence class) for the map and FU
closeouts.

## Non-goals

- No video (INT-10/VRAM) or audio (SB DMA) capture — deferred until this rig
  lands FU-2/FU-3.
- No game-logic porting, no decoder implementations in this slice. The rig
  *observes*; decoding is the next sub-project's job, driven by captured bytes.
- No in-VM session automation. Sessions are played by a human; the rig logs.
- Nothing in `captures/` is ever committed (derived from copyrighted content);
  only synthetic parser fixtures and verbatim parsed lines in docs are.

## Hard constraints (inherited)

- Never write to `game/FIFAPCCD96.iso` (read-only convention). The TSR lives
  in `game/hdd/` (writable emulated state), copied there by the run script.
- Honesty rules extend to captured evidence: a map/FU citation quotes parsed
  trace output verbatim; "runtime-captured" is labeled as its own evidence
  class, never blended with static cites; no address is patched that the map
  does not cite with instruction bytes.
- Existing 10 golden CTests stay green; no edits to `src/`, `include/`,
  `tests/golden/` behavior baselines.

## System dependency (approved)

`nasm` (apt package) to assemble the 16-bit `.COM`. If unavailable at build
time, `make tsr` fails with a clear message; no hand-emitted blob fallback.

## Architecture

### Guest: `tsr/fifa96_capture.asm` → `FIFACAP.COM`

16-bit real-mode COM, assembled `nasm -f bin`. Installs resident via
`INT 27h` (AL=0, DX=paragraphs). Components:

1. **COM1 sink.** `out` to `0x3F8` after bounded spin on THRE (`in` `0x3FD`,
   bit 5). Init probe at install: if THRE never sets (bad DOSBox config),
   print an error and do NOT go resident. Dropped records are detectable
   downstream, not prevented by backpressure.
2. **Frame format.** Every record: `type:u8, seq:u16 (per-type), len:u16,
   payload[len]`. Types: `0x01 HEADER` (magic `FCAP`, version u8, patch-count
   u8), `0x02 FILE`, `0x03 CODEC`, `0x04 HEARTBEAT` (file-record counter u32),
   `0x05 PATCH_SKIP` (site id u8, reason u8, 0=bad-signature 1=already-patched
   2=site-out-of-range), `0x06 END`, `0x07 PATCH_OK` (site id u8, computed
   target linear u32, signature length matched u8). Stream is byte-transparent
   (parser does resync by scanning for plausible frame headers; seq gaps ⇒
   flagged).
3. **INT-21 hook.** Saves original vector at install. On entry (no reentrancy
   guard needed: the TSR never calls INT-21 itself; logging goes to COM1):
   if `AH in {3Dh open, 3Eh close, 3Fh read, 40h write}` → snapshot
   `AX,BX,CX,DS:DX`, capture DS:DX filename (≤13 bytes, NUL-terminated; for
   3Eh/3Fh/40h DS:DX is the read/write buffer — the filename is only taken
   for `AH=3Dh`), chain to the original handler
   (`jmp far [orig_vec]`), then on return snapshot `AX`
   (bytes read/written or handle) and, for `AH=3Fh` with `CF=0`, FNV-1a 32
   hash of the first `min(CX,AX)` bytes of the buffer + those bytes (≤64 B) —
   full buffers are re-harvested offline from the ISO by matching hash+size,
   so the trace itself stays small. Emit `FILE` record
   (`ah,bx,cx,ds,dx,ax_after,flags(hash|len),payload`).
   Special case `AH=4Bh`: after a successful chain, read child-PSP via the
   caller's PSP (`INT 21h/AH=51h` snapshot at hook entry is impossible —
   instead save PSP once at install and trust single-program sessions;
   documented assumption) at `[PSP+0x5Ch]`, compute
   `game_linear_base = ((child_psp+16)<<4)`, and run the patch pass (§4).
   On any `AH=4Ch` → emit `END`.
4. **Deferred patch table.** Static table built at plan time from cited map
   rows. Each entry: `site_id:u8`, `image_offset:u16:u16` (the map's
   segment:offset), `first_byte:u8`, `orig_len:u8 (≤6)`,
   `orig_bytes[6]`, `signature[4]` (exact bytes at the site from the map's
   cited disasm). Derivation rule recorded in the plan:
   `image_offset` = map linear (`seg<<4 + off`) minus the image's Ghidra
   linear base; runtime target = `game_linear_base + image_offset`; the
   signature must match the 4 bytes at the target BEFORE patching, else
   `PATCH_SKIP` (reason 0). `first_byte` slot is overwritten with `0xCD60`
   (`INT 0x60`) after signature match. Already-`CD60` site ⇒ `PATCH_SKIP`
   (reason 1).
5. **INT-60 handler (site hit).** Emits `CODEC` record: site_id u8, all GPRs,
   DS/ES/SS/SP, IP/CS as they were at the site, plus the site's per-layout
   buffer dump declared in the plan (e.g. `load_mf_object`: DS:BX block,
   ≤64 B + FNV hash). Then the trampoline dance, one scratch slot (16 B) per
   site: copy the full `orig_len` bytes into the slot, append near-jmp `E9
   rel16` back to `site+orig_len`; rewrite the IRET frame's `IP` to the slot;
   set TF in the saved flags; `iret`. The site's original instruction thus
   executes exactly once per hit.
6. **INT-1 handler (re-patch trap).** Chained to the original INT-1 at
   install. Consumes the step only when the IP lies inside one of the scratch
   slots (pending-site table); on consume: rewrite `CD60` at that site, clear
   the pending flag, `iret`. Otherwise chain to the original. (TF is
   auto-cleared by the CPU on INT-1 delivery.)
7. **HEARTBEAT** every 1024th `FILE` record — the liveness signal for
   crash/self-checksum diagnosis.

### Host

- `run-fifa96-capture.sh`: copy of `run-fifa96.sh` + `serial1=file
  file:$CAP_FILE multiplier:100` (default `captures/session-<UTC>/trace.bin`,
  `mkdir -p`), autoexec: mount C, imgmount D, `C:\FIFACAP.COM` (copied into
  `game/hdd/`), then `FIFA96.EXE`. Exit of DOSBox finalizes the file.
  Note: the serial file sink form is `serial1=file file:<path>` (verified
  against `dosbox-x.reference.conf`); no `mode:file`/`filename=` and no
  `timeout:` — DOSBox-X closes/flushes the file on exit, which the rig relies on.
  Throughput: the emulated UART idles at 9600 baud, so without intervention
  trace emission throttles the whole game to ~960 B/s and the intro appears
  hung. `multiplier:` alone does not fix it: `CSerial::Init_Registers` computes
  `bytetime` before `CSerialPORTS` assigns `baud_multiplier`, and
  `changeLineProperties` (which applies it) only runs on a guest UART
  reconfiguration — so the TSR programs COM1 LCR/DLL/DLM to divisor 1
  (115200 raw) at install, before emitting anything.
- `tools/fifa96_trace.c`: parser — CLI `fifa96_trace [--raw] FILE`: validates
  header, walks frames, resyncs with flagged `LOST-SYNC`/`SEQ-GAP` lines,
  prints one text line per record (`FILE ah=3F h=0x03 x=1024 got=1024
  name=FW1.QFS hash=8a3f21b0 len=1024 head=hex…`, `CODEC site=5dd2 ax=…
  ds:bx=… hash=… head=…`, skips/heartbeats/END), then summary stats
  (files-by-name, opens-per-session, patch results, codec hits per site).
- `Makefile` targets: `tsr` (nasm), `capture` (script), `trace FILE=…`
  (parser). CMake builds the parser alongside `fifa96_dump`.
- `.gitignore`: add `captures/`.

## Data flow → follow-on evidence use

Session (human plays boot→intro→asset load) → `trace.bin` → `fifa96_trace`
→ FU-2 closeout cites parsed lines to bind filenames↔golden container bytes
(hash+size matched against `game/FIFAPCCD96.iso` reads performed by an
in-repo matcher) → FU-3 closeout cites CODEC in/out records to bind buffers
to container payload offsets → next map slice records these as
runtime-captured cites under the same quote-only rules.

## Error handling (design decisions)

- COM not ready at install → visible error, not resident, script aborts
  (TSR returns non-zero via `INT 21h/AH=4Ch` AL≠0 after printing).
- Any site signature mismatch → `PATCH_SKIP` record; session still produces
  the full file trace (FU-2 leg never depends on codec patches).
- Game crash mid-session → truncated stream is normal-parseable; missing END
  + last heartbeat timestamp bound the loss window. A crash within one
  heartbeat interval of a newly-patched site is reported in the plan's
  acceptance notes as *suspected patch interaction* — investigation slice,
  never speculation in the map.
- Trace bytes unparseable at EOF → flagged LOST-SYNC tail, kept.

## Testing

1. CTests (new): synthetic frame streams → expected parser output
   (happy path, seq gap, resync after garbage, missing END, all skip
   reasons). Parser fixture bytes are hand-authored, not game-derived.
2. TSR verification is live-session-only (documented): assembly is checked by
   `make tsr` + a size/hash printout; behavior is accepted by the session
   checklist below.
3. Live acceptance checklist (manual, in the plan):
   a. `make capture` → reach game intro → quit → trace must contain HEADER,
      ≥1 FILE records for CD files, END or flagged truncation.
   b. Patch pass records: every mapped site emits exactly one result —
      `PATCH_OK` (site id + computed target linear) or `PATCH_SKIP` with a
      reason — all five accounted for.
   c. Reaching an asset load emits ≥1 CODEC record for the responsible site.
   d. Game behaves as with the plain `run-fifa96.sh` (no new hangs/corruption)
      — the self-checksum assumption, stated as an assumption until observed.

## Assumptions (explicit)

- Single-program sessions: PSP of COMMAND.COM stable while TSR resident
  (true for the `run-fifa96` flow: COMMAND → FIFACAP → FIFA96.EXE via AH=4B).
- `FIFA96.EXE` is execed through INT-21/AH=4B by COMMAND.COM (DOSBox-X
  autoexec). If the game is launched another way, the patch pass simply never
  fires — file trace unaffected.
- The game does not checksum the five patch-site bytes (no evidence either
  way; acceptance 3d tests it empirically).

## Decomposition handoff

On rig acceptance: slice "FU-2 closeout via trace" and slice "FU-3 codec
binding via trace" become the following plans under the standing SDD cycle;
P1 (SDL viewer) is specced only after FU-3 produces decodable-pixel evidence.
