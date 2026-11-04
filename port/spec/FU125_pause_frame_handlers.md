# FU-125 — the pause frame handlers and the tick freeze

Follow-on to FU-124 §2/§5 leg 1 (the `EDX` stores to `[0x7300]`). The two
orphan writers are small per-frame handlers created and repaired this slice;
they deliberately freeze the tick.

Result in one line: **`FUN_00049a54` and `FUN_00049a98` are pause/overlay frame
handlers: each calls `FUN_00049280(0)` and stores the returned `EDX` — the
restored entry value the callers zero — into `[0x7300]`, then runs the
pause-UI helpers and the `FUN_00047878`/`FUN_00047880` show/hide pair, so the
zero store is the pause freeze, not a mis-decode.**

## 1. `FUN_00049a54` (created, body `0x49A54..0x49A97`)

```
FUN_00049280(0);              // timer delta (EAX); EDX restored
[0x7300] = EDX;               // = 0 -> freeze the shared tick
FUN_00063900(); FUN_000510c0(); FUN_00053f08();
if (FUN_00047878() != 0) { [0x7330] = 1; return; }
if (FUN_00047880() != 0) FUN_000478fc();
```

## 2. `FUN_00049a98` (created, body `0x49A98..0x49ACC`)

```
FUN_00049280(0);
[0x7300] = EDX;               // freeze
FUN_00053f90();
if ([0x7330] != 0) { FUN_000478fc(); return; }
if (FUN_00047880() != 0) FUN_000478ac();
```

Both are indirect-entry frame states (no callers); `FUN_00047878` sets the
`[0x7330]` flag that the second handler and the `0x49B28` match body check
(the `DAT_00007330 = 1` arm in FUN_00049b28, FU-111 §1). `FUN_00047880` /
`FUN_000478FC` / `FUN_000478AC` are the pause-menu show/hide/refresh trio
already seen in FUN_00049b28's paths.

With the delta in `EAX` and the caller's `EDX` restored (FU-124 §1), the two
`MOV [0x7300],EDX` stores write zero **by design**: paused frames do not
advance the shared tick consumed by the replay control timeline and the view
blend (FU-116 §1, FU-122 §2).

The third writer, `0x496A7`, stores `EAX` (the true delta) inside a long
orphan per-frame body that starts before `0x495CC`; its entry is still open.

## 3. Closures / errata

* **FU-124 §2 — closed.** The `EDX` stores are the pause freeze; the
  `EAX` store is the live delta.
* **FU-124 §5 leg 1 — mostly closed.** The two handlers are functions
  now; only the `0x496xx` body's entry remains.
* **FU-111 §1 — refined.** `[0x7330]` is the pause/overlay flag the
  `0x49B28` match body and these handlers share.
* Ghidra-project change: created `FUN_00049a54`/`FUN_00049a98`; cleared two
  hidden data items (`0x49A6E`, `0x49A73`) that masked `CALL 0x53F08` and
  `CALL 0x47878` — same artifact class as `0x49FD8` (FU-111 §5).

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `create_function` 0x49A54 (body repaired to
`0x49A54..0x49A97`) / 0x49A98; `decompile_function` 0x49A54, 0x49A98;
`clearListing` `0x49A6E..0x49A74`, `0x49A73..0x49A7B`; `read_memory`
`0x49A73` (revealed `CALL 0x47878`); instruction context
`0x49600..0x496C0`. Address mapping as FU-88.

## 5. Open legs

1. The `0x496xx` per-frame body's entry (the `EAX` tick writer).
2. `FUN_00047878`/`0x47880`/`0x478FC`/`0x478AC` (the pause-menu trio) and
   `FUN_00063900`/`0x510C0`/`0x53F08`/`0x53F90`.
3. `[0x7330]`'s other consumers.
