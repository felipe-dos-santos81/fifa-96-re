# FU-336 — phase `0x1B` / act 5: the pure act-slot reset

**Scope.** The native act-5 handler `0x890EC`. Phase-36 wave, 2026-11-04.

## 1. The body `0x890EC..0x8910D` `[M]`

Raw bytes (34): `52 31d2 30e4 66891518 881500 882528881500 31d2
882529881500 891508881500 5a c3`; `RET` at `0x8910D`, alignment `0x8910E`,
the act-6 body starts at `0x89110`.

| addr | instruction | cell |
|---|---|---|
| `0x890EC` | `PUSH EDX` | — |
| `0x890ED/EF` | `XOR EDX,EDX` / `XOR AH,AH` | — |
| `0x890F1` | `MOV word [0x158818],DX` | act timer := 0 |
| `0x890F8` | `MOV byte [0x158828],AH` | act id := 0 |
| `0x89100` | `MOV byte [0x158829],AH` | act stage := 0 |
| `0x89106` | `MOV dword [0x158808],EDX` | act handler := 0 |
| `0x8910C/D` | `POP EDX` / `RET` | — |

Four stores, all in the act slot. No CALL, no phase write; `[0x15881A]`,
`[0x15882A]`, `[0x15882B/C]` and the timeline timer are untouched. EDX is
saved/restored; AH is clobbered and not restored (no engine cell mirrors it).

**Erratum to FU-83 §2:** the timer clear is a **word** store
(`66 89 15 ...`), not a dword.

## 2. Fresh invoker census `[M]`

- `get_xrefs_to 0x888FC` = 11 calls, all `UNCONDITIONAL_CALL`; the EAX arguments
  are `{1,2,3,4,6,7,8,9,0xA,0xB,0xC}`. **No EAX=5**, and the only
  register-derived site (`0x8ABCF MOV EAX,EBX`) is forced to 1 by
  `0x8ABC0`. No indirect/data reference.
- `get_xrefs_to 0x740A0` = 27 calls; the phase arguments are
  `{0,1,2,3,4,5,6,7,8,9,0xA,0xB,0xC,0xD,0xF,0x10,0x11,0x13,0x14,0x15}`. **No
  0x1B.** The phase byte `0x157A4D` has only three writers (`0x740AC`,
  `0x73E4E`, `0x73D2C`); the last two write 0.
- `read_memory 0x1107EC[5] = 0x890EC`; `get_xrefs_to 0x890EC` = one DATA
  reference (`0x110800`, the phase slot). `search_byte_patterns "ec 90 08 00"`
  = exactly one hit. No other selector.
- The engine dispatches acts `{1,2,3,4,7,8,9,0xA,0xB,0xC}` and never 5.

**Verdict:** act 5 is unreachable from the native and engine act machines. It
is plausibly a vestigial "cancel the act slot" primitive (inference, not
evidence). The row is **ported-but-dormant**; the phase slot stays NULL.

## 3. The port

`fifa96_match_action_phase_1B` in `src/fifa96_engine/fifa96_match_run.c`,
declared in `fifa96_match_run.h`. It clears `act.timer` (word), `act.id`,
`act.stage`; the handler dword is derived from `act.id` in the port, so no
separate cell is needed.

Fixture `tests/test_engine_match_act.c::test_phase_1B_body_reset`: stage
`id 5 / stage 2 / timer 0x2A`, call, assert all three cleared and
`state.phase` unchanged; NULL -> `-FIFA96_ERR_INVALID`. The existing
`test_phase_1B_leg_no_invoker` keeps the NULL-slot pin and the act-5
install-but-no-invoke behavior.

**Legs:** R8-L1 (no engine cell mirrors the handler dword; a future port
needing an independently-set handler must add one); R8-L2 (act 5's original
purpose is unproven).
