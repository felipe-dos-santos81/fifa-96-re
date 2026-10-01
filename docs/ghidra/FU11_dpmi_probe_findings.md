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
holds 946 complete frames total (HEADER ×1, PATCH_OK ×5, FILE ×918,
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
