# FIFA 96 — FU351-L2c-1 computed-call closure

Goal: replace the pose feed's `(0,0)` handler arguments with the native
`FUN_000505D0 0x51056` EBX/ECX values and the caller guard.

## Evidence

- `get_xrefs_to 0x505D0` = 3: `0x4D3D9`, `0x4CF51`, `0x4D009`.
- `0x4D3D4 EAX=1`; `0x4CF4D EAX=0` (ECX not set; caller-supplied);
  `0x4D002 EAX=0, ECX=0xA`.
- `0x4B64C`: `[EAX] = dword[0x1577BE] SAR 16` (lead x),
  `[EDX] = dword[0x1577C0] SAR 16` (lead z).
- `0x4B8D4`: `ac2 > 3 -> -1`; otherwise `FUN_0004BA00` slot plus the
  `track_z - hist_z` sign branch; latch `[0x1074A8]`.
- `0x4BA00`/`0x4BE44`: `[0x157A83]` event record -> slot; load port
  `fifa96_camera_target_slot`.
- `0x51047 TEST ECX,ECX`; `0x5104B` reloads ECX from `[ESP+0x14]` (lead z).
- ECX preservers: `0x4B64C`, `0x4B8D4`, `0x4B6FC`, `0x4B7D0`; `0x53D50`
  and `0x6401C` do not write it.

## Tasks

1. Add axis/ac2/event/latch/lead fields to the handler state.
2. Port `FUN_0004B8D4` as `fifa96_camera_pose_axis_a`.
3. Compute axes before the pose switch; feed them to `camera_pan_update`.
4. Add `handler_skip` for the zero-guard case (`FUN_0004CEF4` inherits ECX).
5. Keep the live `FUN_0004D908` `(0,0)` pair.
6. Tests, `make check`, M1/M2, smoke, FU-351 §16, commit/push.

## Remaining

- `FUN_0004CF7C` replay driver (`[0x107DD8]=1`, `FUN_0004F8C8`).
- Alternate `[0x105FC4]` target sources.
- Non-live row-0x17 action-camera installer.
