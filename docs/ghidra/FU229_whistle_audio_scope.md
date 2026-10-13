# FU-229 — the whistle/audio scope verdict: `0x974DC(0x1E)` cannot move the null-backend hash (phase-21 recon, slice W21-7)

Provenance: phase-21 recon slice **W21-7**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, **15239** symbols, **2 757 479** bytes,
`.image` overlay present). Every address/byte/instruction/xref-count below is a
**fresh** `read_memory` / `disassemble_bytes` / `disassemble_function` /
`get_xrefs_to` / `get_function_by_address` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); LE globals sit
at `+0x100000` from the file offset; addresses are `/FIFA96.EXE` runtime
addresses. Engine source read first-hand. **No writes**: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; the ISO /
`game/hdd/` untouched; no git commands.

Scope (W21-7 brief): give a definitive **in/out-of-scope verdict** on whether the
`0x974DC(0x1E)` whistle can change the **null-backend hash** pinned by the M1/M2
goldens — the phase-21 tape re-pin's one open audio question. Paths read: (1) the
native whistle call chain `0x974DC → 0x64EC0 → 0x64EF0` (the `0x155C8C` ring) and
`0x974DC → 0x65CC0 → 0xA7705` (the voice trigger, FU221-L1), as fired by the
phase-`0x1D`/`0x21`/`0x22` bodies (all staged as `ref_whistle` in the port);
(2) `src/fifa96_engine/platform_null.c` — exactly what it hashes and how staged
requests reach it; (3) the verdict. **Extends FU-113 (§ the `0x974DC` site map),
FU-195, FU-211, FU-220, FU-221 and the phase-20 sit-8 register.**

**Answer in one line.** **Out of scope: the live `0x974DC(0x1E)` whistle cannot
change either null-backend hash.** The pinned M1/M2 golden hash is
`present_hash` = FNV-1a over the four Mode-X **planes** + the 768-byte **palette**
only (`platform_null.c:39..44`); the native whistle chain is **audio-only** (it
writes the `0x155C8C` ring, its indices and the voice handle — no plane or
palette byte), the port stages it as the **write-only** `mr->ref_whistle`
(nothing in `src/` reads it), and the PCM sink `audio_submit` has **no call site
anywhere in the tree**, so the separate `audio_hash` is never populated either.
**The phase-21 re-pin forces no audio work.** The audio ring/voice remainder
stays the honest leg **FU-221-L1**.

---

## 1. The native whistle call chain (fresh)

### 1.1 `FUN_000974DC` — a 7-instruction audio trampoline

Fresh `disassemble_function 0x974DC` (7 instructions):

```
0x974DC PUSH EDX
0x974DD MOV  EDX,EAX        ; EDX = event id (the caller loads 0x1E)
0x974DF CALL 0x00064EC0     ; ring push (guarded)
0x974E4 MOV  EAX,EDX
0x974E6 CALL 0x00065CC0     ; voice-trigger notify
0x974EB POP  EDX
0x974EC RET
```

The body **writes no memory of its own**: every effect is in its two callees. It
is the shared "play sound id" entry (FU-113: `FUN_000974dc`, which notifies
`FUN_00065cc0`/`FUN_000651f0`). Fresh `get_xrefs_to 0x974DC` = **42** callers;
the phase-`0x1D`/`0x21`/`0x22` fire sites (§1.4) are six of them.

### 1.2 The ring push: `FUN_00064EC0` → `FUN_00064EF0` (base `0x155C8C`)

Fresh `disassemble_function 0x64EC0` (12 instructions):

```
0x64EC0 PUSH EDX
0x64EC1 MOV  EDX,EAX
0x64EC3 MOV  EAX,[0x00109A98]   ; replay-family flags
0x64EC8 AND  EAX,0x80
0x64ECD JNZ  0x64EE2            ; replay -> skip (no push)
0x64ECF CALL 0x00037AE4
0x64ED4 TEST EAX,EAX
0x64ED6 JNZ  0x64EE2            ; blocked -> skip
0x64ED8 MOV  EAX,0x1
0x64EDD CALL 0x00064EF0         ; push (mode 1)
0x64EE2 POP  EDX
0x64EE3 RET
```

`FUN_00064EF0` is the 20-slot ring (`disassemble_function 0x64EF0`, 45
instructions). The single ring store is

```
0x64F21 MOV dword ptr [EDI*0x4 + 0x00155c8c],EDX
```

with index registers `[0x109AEC]` (count), `[0x109AF0]` (write index EDI),
`[0x109AF4]` (read index ECX) and the wrap test `CMP EDI,0x14` (`0x64F28`,
0x14 = 20 slots). Fresh `read_memory 0x109AEC` (16 B) = **all zero** at
file-load (the three cells are runtime-built); the cells are named in the fresh
`FUN_00064EF0` disassembly — loads at `0x64EF4` (`[0x109AEC]`), `0x64EFA`
(`[0x109AF4]`), `0x64F00` (`[0x109AF0]`) and stores at `0x64F4F`/`0x64F55`/`0x64F5B`. **Ring base `0x155C8C`, 20 dwords = 80 bytes →
`0x155C8C..0x155CDC`** — matching the brief's "`0x155C8C` ring".

### 1.3 The voice trigger: `FUN_00065CC0` → `FUN_000A7705`

Fresh `disassemble_function 0x65CC0` (17 instructions):

```
0x65CC0 PUSH ECX / PUSH EDX
0x65CC2 CMP  dword ptr [0x00155CE0],0x0 ; JZ skip
0x65CCB CMP  dword ptr [0x00155D30],0x0 ; JZ skip
0x65CD4 MOV  ECX,dword ptr [0x00155D00]
0x65CDA TEST ECX,ECX                    ; JLE skip
0x65CDE CMP  dword ptr [0x0010A250],0x0 ; JZ skip
0x65CE7 MOV  EDX,ECX
0x65CE9 CALL 0x000A7705                 ; arm the voice engine
0x65CEE MOV  [0x0010A254],EAX           ; store the voice handle
0x65CF3 POP EDX / POP ECX / RET
```

and fresh `disassemble_function 0xA7705` (9 instructions): `PUSH EBX/ECX; MOV
ECX,0x40; MOV EBX,EDX; MOV EDX,-1; CALL 0x000A7728; POP ECX/EBX; RET` — the
FU-43/FU-49 id-arm wrapper into the unported voice/mixer engine `0xA7728`.

**§1 verdict:** the whole chain writes only **audio state** — the `0x155C8C` ring
(§1.2), its indices `0x109AEC/0x109AF0/0x109AF4`, the voice handle `0x10A254`,
and reads the gates `0x155CE0/0x155D30/0x155D00/0x10A250`. **No Mode-X plane byte
and no palette byte is written by any function in the direct chain.** (A
whole-callee-tree audit of `0xA7728`'s descendants was *not* performed — FU229-L3.)

### 1.4 The fire sites — phase `0x1D`/`0x21`/`0x22` (fresh disasm)

Fresh `disassemble_bytes` of the phase bodies:

**Phase `0x1D`** (`0x89A34`, functionless raw body):

```
0x89A1C MOV EAX,[0x00158816] / SAR EAX,0x10 / CMP EAX,0x78 / JL 0x89EA6
0x89A2D MOV EAX,0x1E
0x89A32 MOV DL,0x1
0x89A34 CALL 0x000974DC          ; <<< the whistle
0x89A39 MOV byte ptr [0x0015882A],DL   ; [0x15882A] := 1
```

**Phase `0x21`** (native act-0xB timer `0x8B874`, functionless raw body) — stage 0:

```
0x8B89E MOV EAX,0x1E
0x8B8A3 CALL 0x000974DC          ; <<< stage 0
```

stage 1 (`[0x158816]>>16 >= 0x3C`):

```
0x8B8D0 MOV EAX,0x1E
0x8B8D5 CALL 0x000974DC          ; <<< stage 1
```

**Phase `0x22`** (native act-0xC timer `0x8B900`, functionless raw body) — stages
0/1/2:

```
0x8B936 MOV EAX,0x1E / 0x8B93B CALL 0x000974DC   ; stage 0
0x8B969 MOV EAX,0x1E / 0x8B96E CALL 0x000974DC   ; stage 1
0x8B99C MOV EAX,0x1E / 0x8B9A1 CALL 0x000974DC   ; stage 2
```

Every site loads `EAX = 0x1E` and calls the §1.1 trampoline. `get_function_by_address`
returns **none** for `0x89A34`, `0x8B874` and `0x8B900` — the three bodies are
raw disassembly-only (the FU196-L6 / FU215-L3 no-function family). All six sites
write only the act-timer cells (`0x158818`/`0x158829`) around the call — **no
plane or palette write**.

---

## 2. The null backend's hash — exactly what it hashes (engine source, first-hand)

`src/fifa96_engine/platform_null.c` maintains **two independent FNV-1a 64
hashes** (`include/fifa96_engine/fifa96_platform_null.h:13..19`):

### 2.1 `present_hash` — Mode-X planes + palette (the pinned golden)

`n_present` (`platform_null.c:35..46`):

```
s->st.present_hash = fnv1a(present_hash ?: offset_basis, f->planes[0], f->stride * 240);
for (p = 1; p < 4; p++)  present_hash = fnv1a(present_hash, f->planes[p], f->stride * 240);
present_hash = fnv1a(present_hash, f->palette, 768);
```

So `present_hash` covers **only** the four `fifa96_platform_frame.planes[]`
(`stride * 240` bytes each; `stride = 80`, `fifa96_platform.h:6..8`) and the
768-byte `palette` (`fifa96_platform.h:8`). **No audio input enters
`present_hash`.**

### 2.2 `audio_hash` — PCM only, and it is never reached

`n_audio_submit` (`platform_null.c:50..55`) folds PCM bytes into a **separate**
`st.audio_hash`. It is invoked only through the `fifa96_platform.audio_submit`
function pointer (`fifa96_platform.h:24`):

```
p->audio_submit = n_audio_submit;   /* platform_null.c:72 */
```

**There is no caller of `audio_submit` anywhere in the repository.** A fresh
whole-tree grep for `audio_submit` returns only the struct field, the two backend
definitions (`platform_null.c:50/72`, `platform_sdl3.c:290/323`) and the
assignment expressions — **no `->audio_submit(...)` / `.audio_submit(...)` call
site** (and `fifa96_engine.c:138` calls only `audio_open`). Therefore
`n_audio_submit` is never executed, `st.audio_frames` stays 0, and `audio_hash`
stays 0.

### 2.3 How the staged request reaches the backend: it does not

The engine's only representation of the whistle is `mr->ref_whistle`
(`include/fifa96_engine/fifa96_match_run.h:886`). Every occurrence in `src/` is a
**write** (`fifa96_match_run.c:2114, 2247, 5725, 5803, 6167, 6275, 6282, 6308,
6315, 6322, 6502`); no rendering, platform or audio code ever **reads** it
(reads exist only in `tests/`). The phase-`0x1D` body (`fifa96_match_run.c:6121`)
and the phase-`0x21`/`0x22` bodies (`:6271`, `:6304`) set `mr->ref_whistle = 0x1Eu`
and nothing else audio-related. So the staged request has **no path to either
hash**: not to `present_hash` (no plane/palette read), not to `audio_hash` (no
`audio_submit` call).

### 2.4 What the golden actually pins

`test_engine_m2.c:2684` records `st.present_hash` only (`frame=%llu
hash=%016llx`); the golden `tests/golden/engine/m2-frames.txt` (sha
`441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506`) is a list of
those `present_hash` values. `audio_hash`/`audio_frames` are **never asserted in
any test or tool** (fresh grep of `tests/` + `tools/`). M1 is the same
(`test_engine_m1.c:133/187`, `golden/engine/m1-frames.txt`, immutable sha
`09b726b7…`).

---

## 3. Verdict — in/out of scope

**Out of scope. The live `0x974DC(0x1E)` whistle cannot change either null-backend
hash.**

| hash | what it folds | can the whistle move it? | why |
|---|---|---|---|
| `present_hash` (**the M1/M2 golden**) | `planes[0..3]` + `palette[768]` | **No** | the native chain is audio-only (§1); the port stages only the write-only `mr->ref_whistle` (§2.3); nothing in the render path reads it |
| `audio_hash` (the literal "PCM hash") | submitted PCM | **No** | `audio_submit` has **no call site** (§2.2) — the audio sink is unported; `n_audio_submit` never runs |

Two independent reasons converge. Even if the whistle were native-exact, its
effect lands in the audio ring / voice engine — neither of which the port runs —
and the pinned golden hashes the framebuffer + palette, into which the whistle
never writes.

**Consequence for the phase-21 re-pin:** no audio work is required. The tape
re-pin (if any) is driven by render/state, not by the whistle. `FU-221-L1` (the
`0x974DC(0x1E)` audio subsystem) stays **parked**, in scope for neither the
re-pin nor the census.

**Erratum note (naming).** The W21-7 brief and the phase-21 design doc
(`docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md:70`)
call the target "the null-backend **PCM** hash". The golden the re-pin actually
protects is `present_hash` (**planes + palette**, §2.1/§2.4); `audio_hash` (PCM)
is a separate, never-populated counter. Both are unaffected, but promotion of
this doc should keep the two hashes named distinctly so a future audio port does
not mistake one for the other.

---

## 4. Census / golden impact

**None.** No engine code, table row or golden changes:
`fifa96_match_handlers.c` registrations are untouched, and the live headline
`55/80` dispatch rows → `floor(100 × 55/80) = 68` (`docs/ENGINE.md:8`;
`README.md:1`) is unmoved by this slice. M2 `441415fc…` and M1 `09b726b7…` are
also unmoved (docs/finding only).

---

## 5. Residual legs

1. **FU229-L1 — the voice/mixer engine `0xA7728` is unported.** The whistle's
   actual audio output (PCM synthesis / mixer / DMA) lives behind
   `0xA7728` (reached via `0xA7705`), which no port code models. This is the same
   subsystem as **FU221-L1**, restated with the fresh call sites. **Not required
   for the re-pin.** Owner: FU-221-L1.
2. **FU229-L2 — the three fire sites have no Ghidra function.** Fresh
   `get_function_by_address` = **none** for `0x89A34`, `0x8B874`, `0x8B900`: the
   phase-`0x1D`/`0x21`/`0x22` bodies are raw disassembly-only, so their
   phase→body attribution is disassembly-level, not function-attributed. Owner:
   FU196-L6 / FU215-L3.
3. **FU229-L3 — no whole-callee-tree framebuffer audit.** "Audio-only" is
   asserted from the **direct** chain's own stores (§1.1–§1.3) and the FU-113
   site map; a full descendant audit of `0xA7728` (the voice engine) for any
   plane/palette write was **not** performed. It does not affect the verdict (§3
   holds even if the native wrote planes, because the port never runs the chain).
   Owner: FU-221-L1.
4. **FU229-L4 — the audio port will move `audio_hash`.** When the audio path is
   eventually ported and `audio_submit` gains a call site, the whistle will start
   feeding PCM and `audio_hash` becomes live; any future golden that folds
   `audio_hash` would then need its own pin. No such golden exists today. Owner:
   the audio-port slice (FU-221-L1).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `disassemble_function` — `0x974DC` (7 insns), `0x64EC0` (12), `0x64EF0` (45),
  `0x65CC0` (17), `0xA7705` (9).
* `disassemble_bytes` — `0x8B874` (320 B: the phase-`0x21`/`0x22` bodies),
  `0x89A00` (80 B: the phase-`0x1D` fire).
* `get_xrefs_to` — `0x974DC` (**42**).
* `get_function_by_address` — `0x8B874` = none, `0x89A34` = none (also
  `0x8B900` = none).

Engine/loader source read first-hand:
`src/fifa96_engine/platform_null.c` (`:35..46` `n_present`/`present_hash`,
`:50..55` `n_audio_submit`/`audio_hash`, `:72` the pointer install);
`include/fifa96_engine/fifa96_platform.h` (`:6..8` frame planes/stride/palette,
`:24` `audio_submit`); `include/fifa96_engine/fifa96_platform_null.h`
(`:13..19` stats); `include/fifa96_engine/fifa96_match_run.h` (`:886`
`ref_whistle`); `src/fifa96_engine/fifa96_match_run.c` (`:6121` phase-`0x1D`,
`:6271` phase-`0x21`, `:6304` phase-`0x22`; the `ref_whistle` write set);
`src/fifa96_engine/fifa96_engine.c` (`:138` the only `audio_*` call =
`audio_open`); `tests/test_engine_m2.c` (`:2684` the `present_hash` transcript
line). Repo greps: `audio_submit` (no call site), `ref_whistle` (write-only in
`src/`), `audio_hash`/`audio_frames` (never asserted). Docs cross-cited: FU-113,
FU-195, FU-211, FU-220, FU-221, FU-214/FU-215 (the phase-20 register). No writes
beyond this slice file; no rename/label/comment/function/script/project save;
`/FIFA96.EXE` and the ISO untouched; no git commands.

---

## Landing note

This slice **closes the phase-21 "does the whistle touch the null-backend hash?"
question as OUT OF SCOPE**: the pinned goldens hash planes + palette
(`present_hash`), the port stages the whistle as the write-only `mr->ref_whistle`,
and the PCM sink `audio_submit` has no caller (`audio_hash` never populated). No
engine change, no golden re-pin, **no audio work** for the phase-21 re-pin.
**FU-221-L1** (the audio subsystem) and the four FU229 legs above remain the
honest record.
