# P12 — match camera type 0 and the goal mesh (2026-11-07)

**Spec:** `../specs/2026-11-05-fifa96-playable-sdl-design.md` (P1/P5).

## Symptom

After the P11 playable build, the windowed match was fast, but the view still
looked like a placeholder: a wide side-on empty pitch, a single flat red/blue
stand, no goal net, and tiny players. The menu was native.

## Root-cause evidence

- The live renderer used camera object 0's **image defaults** at `0x107508`
  (`-1920,160,30`, yaw `0xBF9C`, pitch `200`, ratio `0x1500`).
- Ghidra `disassemble_function 0x4D7E8` (the camera-type setup) copies
  `PTR_DAT_00108B64[type] + 0x48 + index*0x18` into the camera object. The
  type-0 entry table at `0x1089B4` points at `0x107E2C`; `read_memory 0x107E2C`
  = `-100, 488, 1100, 33900, 3900, 4608`. The image defaults are the attract
  pose, not the match pose.
- The DOSBox reference match is a high, close follow view with the goal net
  visible; the port drew a single far wall and no net.
- Native `Net.fsh` is GAMEART0 entry 36, five frames; frame 4 is a 64x64 gray
  mesh (`187..255`, background 187, white cords >=200).

## Tasks

1. **Live camera type 0.** Set the live `yaw/pitch/view_ratio` from the
   `0x107E2C` entry and use its position as the eye offset from the follow
   focus. Keep the image defaults for non-live/tape runs. *Check:*
   `test_engine_match_render` asserts the type-0 eye and angles; M1/M2 exact.
2. **Goal mesh.** Stage `Net.fsh` frame 4 by name, inverse-project the
   goal-line plane `z=±0xC1E` through the same camera matrix as the pitch, fill
   the mouth dark, draw the `>=200` white cords and white posts/crossbar.
   *Check:* the live fixture asserts the net frame is staged; the release
   screenshot shows a goal.
3. **Far end stand.** Add the derived goal-end wall at
   `z=-(0xC1E+64)` using the same stand/crowd/ad banks; this removes the
   black void behind the goal. *Check:* the live wall hash changes when the
   banks are removed; no goldens move.
4. **Verify.** `make check` **119/119**, M1/M2 exact, release play-test and
   screenshot `docs/screens/p12-match-goal-net.png`; the type-0 eye/angles and
   the 64x64 net frame are test-pinned.

## Legs

The native 3dwpts point/quad bowl (`FUN_000610C4`, `0x15438C`/`0x1543D4`) and
the full 3D side/top net remain **P10-L2**. The native camera director
`FUN_0004E834` and lens remain **P10-L6**; the type-0 pose is the native start,
the follow is derived. HUD art remains **P10-L5**.
