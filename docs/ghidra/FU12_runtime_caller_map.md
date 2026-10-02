# FU-12: runtime caller map II — file layer and screen entries

Follow-up to `FU11_dpmi_probe_findings.md` using the same DPMI probe
(`tools/trace_probe.sh`). Four further entry points were probed to close the
static census gap left by FU-8/FU-9.

Tooling added with this slice: `tools/fifa96_callers.py` (static `E8 rel32`
direct-call census over the extracted LE image; `tests/test_callers.py`,
CTest `test_callers`, suite 17/17). The probe reports RETURN addresses, so
each runtime `caller_link` is cross-checked against `opcode + 5` from this
census. The measured linear delta is `0x1FC000` in all four runs, consistent
with FU-11; no delta is hardcoded.

## Target 5 — `FUN_0009a0a8` whole-file loader (`0x9A0A8`)

* Overwrite 5 bytes; `target_link=0x9A0AD`.
* Static census: five direct callers, opcodes `0x9A02C`, `0x9A048`,
  `0x9A067`, `0x9A084`, `0x9A09C` (returns `+5`).
* Runtime: 7 frames, all identical:

```
T_PROBE site=5 caller=0x0029604d delta=0x1fc000 caller_link=0x9a04d target_link=0x9a0ad
probe_frames=7
```

* Matched: `0x9A04D = 0x9A048 + 5` (7/7 frames).
* Not exercised: the other four sibling call sites.
* Provenance: `captures/session-probe-9a0a8/trace.bin`, 314,378 bytes.

## Target 6 — `FUN_0009777c` VFS read (`0x9777C`)

* Overwrite 7 bytes; `target_link=0x97783`.
* Static census: two direct callers, opcodes `0x977F1`, `0x9780D`.
* Runtime: 3,082 frames, all identical to the line below:

```
T_PROBE site=6 caller=0x002937f6 delta=0x1fc000 caller_link=0x977f6 target_link=0x97783
probe_frames=3082
```

* Matched: `0x977F6 = 0x977F1 + 5` (3,082/3,082 frames).
* Not exercised: `0x9780D`.
* Provenance: `captures/session-probe-9777c/trace.bin`, 378,866 bytes.

## Target 7 — `FUN_00014c18` surface/still loader (`0x14C18`)

* Static census: two direct callers, opcodes `0x25D64`, `0x27B42`.
* Runtime: **zero** probe frames in both runs — 120 s
  (`captures/session-probe-14c18/trace.bin`, 314,318 bytes) and the 180 s
  retry (`captures/session-probe-14c18-r2/trace.bin`, 450,637 bytes).
* Verdict: not exercised in the unattended startup window. The guest stayed
  live (rig SIGKILLed it at timeout, no `T_END`), so this is a reachability
  result, not a crash. Both static callers remain untested at runtime.

## Target 8 — `FUN_00023b38` table-frame loader (`0x23B38`)

* Static census: twelve direct callers, opcodes `0x2405D`, `0x240BC`,
  `0x24222`, `0x2427D`, `0x242BA`, `0x24317`, `0x2436E`, `0x246D5`,
  `0x24784`, `0x2488A`, `0x24933`, `0x25DD5`.
* Runtime: **zero** probe frames in both runs — 120 s
  (`captures/session-probe-23b38/trace.bin`, 314,753 bytes) and the 180 s
  retry (`captures/session-probe-23b38-r2/trace.bin`, 449,158 bytes).
* Verdict: not exercised in the unattended startup window; guest live.
  All twelve static callers remain untested at runtime.

## Interpretation

* Both generic file-layer entries are startup-hot and each is driven through
  a **single** caller among its static sites: the whole-file loader through
  `0x9A048`, the VFS read through `0x977F1` (its sibling `0x9780D` never
  fired). These are the first runtime-confirmed call chains for the FU-8
  loader family.
* The two screen consumers sit outside the unattended startup path despite
  the game displaying screens: their callers are menu/gameplay-gated (or the
  startup stills bypass these routines). Reaching them needs scripted input;
  that is the architectural follow-up (headless menu input), not more static
  analysis.
* Unmatched callers: none in this campaign — every measured `caller_link`
  matched its static census, in contrast to FU-11's `0xAE479` census gap.
