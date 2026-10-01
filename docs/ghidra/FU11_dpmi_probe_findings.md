# FU-11: in-guest DPMI probe — first result (decompressor entry)

First end-to-end result of the FU-10 plan, run by `tools/trace_probe.sh`.
Target: `FUN_0009e718`, the kVGT command-stream decompressor (FU-5); it is on
the load path for every compressed asset (FU-8/FU-9), so it fires early in a
headless run. The run captured the caller chain through the DPMI probe with a
measured relocation delta and no assumed addresses.

## Mechanism

```
game code                    cave (link 0x6728D, 102 B)            TSR (real mode)
call FUN_0009e718  ---->  pushad/pushfd/push es
  (entry replaced by      save caller ret [esp+44], resume [esp+40]+(ow-5)
   `call cave`)           zero 48-byte DPMI real-mode call struct at SS:ESP
                          ESI=site, EBP=resume, EBX/EDX=caller lo/hi
                          ES:EDI = struct, AX=0300h, BX=0061h, CX=0
                          int 31h  --------------------->  INT 61h vector
                                                          handler reads
                                                          ESI/EBX/EDX/EBP,
                                                          sends T_PROBE frame
                                                          over COM1, iret
                          pop es/popfd/popad
                          add esp,4   (drop `call cave`'s return slot)
                          replay 7 displaced bytes of the prologue
                          jmp 0x9E71F
```

* **Patch** (`tools/fifa96_patch.py`): `call cave` (5 bytes) at `0x9E718`; the
  whole-instruction overwrite prefix is 7 bytes
  (`56 57 55 8B 44 24 10` = `push esi; push edi; push ebp; mov eax,[esp+0x10]`);
  the cave replays those bytes and jumps to `0x9E71F`. Program runs from a copy
  (`build/fifa96-trace-1.iso`); the retail ISO is never written.
* **DPMI notification**: `INT 31h AX=0300h` (simulate real-mode interrupt) with
  `BL=0x61`. DOSBox-X has no DPMI host; the service is provided by the game's
  extender (DOS/4GW — its real-mode reflection stub is the obj2 code at
  `0xE0000` described in FU-9). The PM cave builds the real-mode call structure
  per DPMI 0.9 and the 16-bit handler reads the full 32-bit slots with 32-bit
  operand sizes.
* **Wire format**: `type:u8=0x08, seq:u16=0, len:u16=16` + four LE u32:
  `site, caller_lo, caller_hi, target_ret`. The handler is
  `tsr/fifa96_capture.asm` (`build/FIFACAP.COM`, sha256
  `83dd6a1de341e0bbd53ccc0a47ebf5071818efe9351ffa4155f21e775b89a30b`).
* **Normalization** (`tools/fifa96_probe.py`): runtime addresses are relocated
  by a per-run delta measured from the frame itself:
  `delta = target_ret - (target_link + overwrite)`,
  `caller_link = caller_ret - delta`.
* **Capture termination**: `run-fifa96-capture.sh` runs DOSBox-X under
  `timeout -s TERM -k 5 "$TIMEOUT" ...` — SIGTERM first, then SIGKILL after a
  5 s grace period — so a live-guest capture always terminates (DOSBox-X
  ignores SIGTERM on this host) and the decoder always sees a closed trace.
  `tools/trace_probe.sh` passes `TIMEOUT` through (`${TIMEOUT:-120}`) and
  deletes any stale `captures/session-$SESSION/trace.bin` before capturing.

## The run

```
make tsr
sh tools/trace_probe.sh 0x9E718 1 probe-9e718
```

Patch summary: `target 0x9e718 overwrite: 7 bytes`, `cave 0x6728d: 102 bytes`,
`file-size delta: 0`. The guest kept running (no `T_END` frame) until the
capture was stopped; 22 probe frames arrived in the first ~14 s.

## Decoded result (verbatim decoder stdout)

```
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
T_PROBE site=1 caller=0x0029a871 delta=0x1fc000 caller_link=0x9e871 target_link=0x9e71f
probe_frames=22
expect_site=0x1 hit=True
```

All 22 frames are byte-identical: one site, one caller, one delta. The trace
holds 946 complete frames total (HEADER ×1, POK ×5, FILE ×918,
T_PROBE ×22) and no `T_END`.

## Measured delta (not assumed)

`delta = 0x1FC000` for this run. It is derived from the wire data:
`target_ret 0x0029A71F - (target_link 0x9E718 + overwrite 7 = 0x9E71F) =
0x1FC000`, and it is identical in all 22 frames. This **supersedes the
`0x2D1000` figure quoted in FU-8** for this configuration: the runtime is
authoritative, and the TSR-resident rig places the extender's image at a
different linear base. Any absolute game address in a follow-up analysis must
use a delta measured from a frame of that same run.

## Cross-check against the static direct-call census

The census values are call-site addresses; the measured `caller_link` is the
return address, i.e. call site + 5 (verified on the retail image: each address
holds `E8 rel32` to `0x9E718`).

| static census (call site) | E8 bytes (retail image) | expected return (site + 5) | measured `caller_link` |
| --- | --- | --- | --- |
| `0x9E86C` | `e8 a7 fe ff ff` | `0x9E871` | **`0x9E871` (22/22 frames)** |
| `0x9E884` | `e8 8f fe ff ff` | `0x9E889` | not seen in this window |
| `0xCAE32` | `e8 e1 38 fd ff` | `0xCAE37` | not seen in this window |

The first decompressor caller is the `0x9E86C` site; it fired 22 times in the
first ~14 s of the headless run. The other two sites are later/rarer and were
not reached before the capture was stopped.

## Provenance

* Session: `captures/session-probe-9e718/trace.bin`, **71585 bytes** (946
  complete frames; the 22 T_PROBE frames above).
* Patched image: `build/fifa96-trace-1.iso` (copy; retail ISO untouched).
* TSR: `build/FIFACAP.COM`, sha256 `83dd6a1d…` (full value above).
* Runner: `tools/trace_probe.sh` (patch → headless capture → decode with the
  measured overwrite).

## Limitations

* Only the `0x9E86C` caller has been observed so far; the other two census
  sites need a longer/more specific run (Task 5).
* The probe adds a DPMI round-trip and a serial frame per decompressor call;
  it is a measurement rig, not a shippable mode.
* The call structure's `SS` field sits at offset 0x30, outside the 48-byte
  zeroed frame; DOS/4GW tolerated it (identical behavior with `SS=0` in a
  control run), but zeroing it would be spec hygiene if the cave is reused.

---

## Task 5 batch — decompressor wrapper and loader entries (sites 2–4)

Five further `tools/trace_probe.sh` runner invocations across three sessions
(sites 2–4; the two zero-frame targets were each re-run once), run sequentially
(one DOSBox-X at a time); each runner self-terminated, and no external kill was
used. The probe reports **return addresses**; the FU-8/FU-9 census integers are
**call-opcode** addresses, so a frame matches a census site `S` when
`caller_link == S + 5`. Every census site for these three targets was
re-verified in the retail image first: each address holds `E8 rel32` whose
destination is the target (bytes shown only for the exercised rows below).

| target | site | session | frames | delta | final trace |
| --- | --- | --- | --- | --- | --- |
| `FUN_0009e860` | 2 | `probe-9e860` | 140 | `0x1FC000` | 316997 B |
| `FUN_000c9d10` | 3 | `probe-c9d10` | 0 in both runs | n/a | 450724 B (re-run) |
| `FUN_000cad30` | 4 | `probe-cad30` | 0 in both runs | n/a | 450376 B (re-run) |

### Site 2 — `FUN_0009e860` (VGT decompressor, `src, dst`)

**Run:** `sh tools/trace_probe.sh 0x9E860 2 probe-9e860`

Patch summary (verbatim):

```
target 0x9e860 overwrite: 6 bytes
cave 0x6728d: 101 bytes
file-size delta: 0
```

* **Overwrite length:** 6 bytes (resume link `0x9E866`).
* **Measured delta:** `0x1FC000`, identical in all 140 frames (same value as
  the Task 4 site-1 run, i.e. stable placement for this rig).
* **Frames:** 140 `T_PROBE`. Trace mix: HEADER ×1, POK ×5, FILE ×3714,
  HB ×3, no `T_END` (guest ran until the capture was stopped).

Decoder stdout (verbatim):

```
T_PROBE site=2 caller=0x00214ced delta=0x1fc000 caller_link=0x18ced target_link=0x9e866
T_PROBE site=2 caller=0x00214ced delta=0x1fc000 caller_link=0x18ced target_link=0x9e866
T_PROBE site=2 caller=0x0024637b delta=0x1fc000 caller_link=0x4a37b target_link=0x9e866
T_PROBE site=2 caller=0x0024637b delta=0x1fc000 caller_link=0x4a37b target_link=0x9e866
T_PROBE site=2 caller=0x00214ced delta=0x1fc000 caller_link=0x18ced target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
T_PROBE site=2 caller=0x002aa479 delta=0x1fc000 caller_link=0xae479 target_link=0x9e866
probe_frames=140
expect_site=0x2 hit=True
```

Cross-check against the FU-9 census (site + 5):

| census site | `E8` bytes (retail image) | expected `S+5` | measured `caller_link` | frames | verdict |
| --- | --- | --- | --- | --- | --- |
| `0x18CE8` | `e8 73 5b 08 00` | `0x18CED` | **`0x18CED`** | 3 | matched |
| `0x4A376` | `e8 e5 44 05 00` | `0x4A37B` | **`0x4A37B`** | 2 | matched |
| *not in census* | `0xAE474`: `e8 e7 03 ff ff` | `0xAE479` | **`0xAE479`** | 135 | **UNMATCHED** |

* **UNMATCHED `0xAE479`** (135/140 frames): its return address minus 5 is
  `0xAE474`, which in the retail image holds `e8 e7 03 ff ff` (`call
  0x9E860`). It is therefore a real direct caller that the 12-entry FU-9
  census omitted, not a runtime-built dispatch. A full-image `E8` scan finds
  three direct call sites outside the census: `0x78EAB`, `0x78EB7`,
  `0xAE474`; only `0xAE474` fired in this window.
* Census sites not exercised (10 of 12): `0x14C65`, `0x14D76`, `0x23C0C`,
  `0x24DE6`, `0x26E47`, `0x4A32A`, `0x4A42B`, `0x4A5DC`, `0x4B000`,
  `0x78E5E`.

Provenance: `captures/session-probe-9e860/trace.bin`, **316997 bytes**
(HEADER ×1 + POK ×5 + FILE ×3714 + HB ×3 + T_PROBE ×140 complete frames).
Patched copy `build/fifa96-trace-2.iso`; retail ISO untouched.

### Site 3 — `FUN_000c9d10` (resource loader entry, `name, type, flags`) — zero frames

**Run (default window):** `sh tools/trace_probe.sh 0xC9D10 3 probe-c9d10`
**Re-run (the one permitted retry):**
`TIMEOUT=180 sh tools/trace_probe.sh 0xC9D10 3 probe-c9d10`

Patch summary (verbatim):

```
target 0xc9d10 overwrite: 6 bytes
cave 0x6728d: 101 bytes
file-size delta: 0
```

* **Overwrite length:** 6 bytes. **Measured delta:** not measurable — zero
  probe frames in both runs.
* **Frames:** 0 in each run. Decoder stdout (verbatim):

```
probe_frames=0
expect_site=0x3 hit=False
```

* Both runs kept a live guest: the re-run trace holds HEADER ×1, POK ×5,
  FILE ×5286, HB ×5 and no `T_END`.
* Both census sites are genuine direct calls in the retail image (`0xC571A`:
  `e8 f1 45 00 00`, `0xC5742`: `e8 c9 45 00 00`, both `call 0xC9D10`), and a
  full-image `E8` scan finds no other direct caller. The zero-frame result is
  therefore "loader entry not reached in the headless window", not a
  patch/probe failure.
* Census sites not exercised (2 of 2): `0xC571A`, `0xC5742`.

Provenance (final re-run): `captures/session-probe-c9d10/trace.bin`,
**450724 bytes** (HEADER ×1 + POK ×5 + FILE ×5286 + HB ×5). The first 120 s
run produced 314231 bytes, also zero probe frames; its trace was deleted by
the runner's stale-trace cleanup before the re-run.

### Site 4 — `FUN_000cad30` (load + decode worker, `buf, type, flags`) — zero frames

**Run (default window):** `sh tools/trace_probe.sh 0xCAD30 4 probe-cad30`
**Re-run (the one permitted retry):**
`TIMEOUT=180 sh tools/trace_probe.sh 0xCAD30 4 probe-cad30`

Patch summary (verbatim):

```
target 0xcad30 overwrite: 6 bytes
cave 0x6728d: 101 bytes
file-size delta: 0
```

* **Overwrite length:** 6 bytes. **Measured delta:** not measurable — zero
  probe frames in both runs.
* **Frames:** 0 in each run. Decoder stdout (verbatim):

```
probe_frames=0
expect_site=0x4 hit=False
```

* The re-run kept a live guest: HEADER ×1, POK ×5, FILE ×5282, HB ×5, no
  `T_END`.
* All seven census sites are genuine direct calls in the retail image
  (`0xC9D75`: `e8 b6 0f 00 00`, `0xC9DC2`: `e8 69 0f 00 00`,
  `0xCACB4`: `e8 77 00 00 00`, `0xCACD0`: `e8 5b 00 00 00`,
  `0xCACEF`: `e8 3c 00 00 00`, `0xCAD0C`: `e8 1f 00 00 00`,
  `0xCAD24`: `e8 07 00 00 00`, all `call 0xCAD30`), and the full-image `E8`
  scan finds exactly these seven, no others. The zero-frame result is
  "decode worker not reached in the headless window".
* Census sites not exercised (7 of 7): `0xC9D75`, `0xC9DC2`, `0xCACB4`,
  `0xCACD0`, `0xCACEF`, `0xCAD0C`, `0xCAD24`.

Provenance (final re-run): `captures/session-probe-cad30/trace.bin`,
**450376 bytes** (HEADER ×1 + POK ×5 + FILE ×5282 + HB ×5). The first 120 s
run produced 314927 bytes, also zero probe frames; its trace was deleted by
the runner's stale-trace cleanup before the re-run.
