# Loader rename map

INT 21h file-wrapper pass (project `fifa96`, program `/fifa96.exe`).
All wrappers share the carry-flag error idiom (`SBB BX,BX` / `OR AX,BX`).
Single open handle cell at `11bd:0e70` (word). Verified 2026-09-28.

| Ghidra FUN | Address | INT 21h AH | New name | C counterpart |
|------------|---------|------------|----------|---------------|
| FUN_11bd_5fb8 | 11bd:5fb8 | 3D | file_open_dos | fifa96_file_read (open path) |
| FUN_11bd_5fca | 11bd:5fca | 42 | file_seek_dos | fifa96_file_read_chunk (seek path) |
| FUN_11bd_5fe2 | 11bd:5fe2 | 3F | file_read_dos | fifa96_file_read (read path) |
| FUN_11bd_5ff7 | 11bd:5ff7 | 3E | file_close_dos | fifa96_file_free (close path) |
| FUN_11bd_6003 | 11bd:6003 | 3F | file_read_far_dos | fifa96_file_read_chunk (far-buffer read) |

Notes:
- `file_open_dos` (`MOV AX,0x3d00`, AL=0 open-readonly) stores the FD at `[0xe70]`.
- `file_seek_dos` (`MOV AX,0x4200`, AL=0 SEEK_SET) takes offset params off SP.
- `file_read_dos` (`MOV AH,0x3f`) reads CX bytes via the stored handle.
- `file_close_dos` (`MOV AH,0x3e`) swaps `0xffff` into `[0xe70]` (XCHG) then closes.
- `file_read_far_dos` (`MOV AH,0x3f`) takes a far `DS:DX` buffer + length (segment-split reader).
- Non-loader INT 21h hits excluded: AH=40h console write (FUN_11bd_1497),
  AH=4Ah realloc (FUN_11bd_1e9f), AH=48h alloc (FUN_11bd_4b80), AX=FF80h (FUN_11bd_479a).
- Top callers: FUN_11bd_30d8 (open/seek/read trio), FUN_11bd_5992 (loader core),
  FUN_11bd_304f, FUN_11bd_32c6, FUN_11bd_5bdb, FUN_11bd_5dd2.

## Codec funnel (verified 2026-09-28, program `/fifa96.exe`)

Single-object MF loader orchestrated by `load_mf_object` (ex-`FUN_11bd_5dd2`,
body `11bd:5dd2..11bd:5faa`, 194 insns). Caller `FUN_11bd_5992` reads one word
via `file_read_dos` and calls `load_mf_object` once iff the word is `0x4d`
(`'M'`; decompile `if (*(short *)(puVar3 + -0x10c) == 0x4d)`); an earlier
`'M'/'F'` magic check sits in the same caller. The funnel writes an `0x4d`
marker (`MOV byte ptr [SI],0x4d` at `11bd:5dde`), allocates via DOS `AH=48h`,
far-loads via `file_read_far_dos`, and may grow/relocate memory, far-copy, and
transfer control to the loaded image. No byte-decode transform was observed in
any member, so no `decode_*` name was assigned; no format tag (`SHPI`, `PCNX`,
`kVGT`, `BNK`) or envelope field reference was observed in any member, so the
funnel is NOT attributed to any single game-asset decoder (see header notes).

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_5dd2 | 11bd:5dd2 | callees: file_read_dos, file_read_far_dos, 5d79, 5db2, 6102, 6907, 26d0, 1000:0b12; `CALLF 0x1000:0b12` at 11bd:5f6b; `MOV byte ptr [SI],0x4d` at 11bd:5dde | load_mf_object | none — behavioral (single-object MF load orchestrator) |
| FUN_1000_0b12 | 1000:0b12 | `SHR AX,CL` at 1000:0b33; `INT 0x21` (`AH=4Ah` realloc) at 1000:0b56; `SHR CX,1` + `MOVSW.REP` at 1000:0b89..0b8b; table stores `MOV ES:[0x28b9],AX` / `MOV [0x9f1],AX` / `MOV [0x9b6],AX` / `MOV [0x20],AX` at 1000:0b96..0ba0 | mem_grow_relocate | none — behavioral (DOS realloc + backward slide + segment-table retarget) |
| FUN_11bd_6907 | 11bd:6907 | `MOV SS,DX` at 11bd:694c + `MOV SP,[BX+0x10]` at 11bd:694e + `RETF` at 11bd:6955 | exec_loaded_image | none — behavioral (SS/SP switch + far transfer to loaded image) |
| FUN_11bd_5d79 | 11bd:5d79 | `MOV byte ptr [BP-0xd],0x48` (AH=48h alloc probe) at 11bd:5d85; `CALL int21_dispatch-thunk` at 11bd:5d8e; `CMP word ptr [BP-0xe],0x8` retry loop at 11bd:5d9e | alloc_retry_loop | none — behavioral (DOS alloc probe retry loop) |
| FUN_11bd_6102 | 11bd:6102 | `MOVSB.REP ES:DI,SI` at 11bd:6117 (17-insn body is pure far copy) | copy_bytes_far | none — behavioral (far byte copy, no transform) |
| FUN_11bd_26d0 | 11bd:26d0 | `INT 0x21` at 11bd:26e9 with AX/BX/CX/DX/SI/DI loaded from + stored back to a register block; `JC` error path at 11bd:2700 | int21_dispatch | none — behavioral (generic INT 21h register-block dispatcher) |

Notes:
- `get_xrefs_to` on `0x28b9`/`0x9f1`/`0x9b6`/`0x20` returned 0 refs: the
  table-update stores in `mem_grow_relocate` are segment-relative (`ES:`) writes
  inside the function body, not absolute xrefs Ghidra indexes. The
  table-update claim is sourced to disassembly lines `1000:0b96..1000:0ba0`
  (plus the 0x27-entry retarget loop at `1000:0bce..0be7`) instead.
- `mem_grow_relocate` is shared infrastructure, not funnel-private: 7 callers
  (`11bd:3844`, `11bd:3986`, `11bd:3ed8`, `11bd:5c8b`, `11bd:5dd2`,
  `11bd:7290`, `11bd:76db`); it takes no byte-decode role.
- `mem_free_dos` (ex-`FUN_11bd_5db2`, renamed in delimitation pass) is called
  3× inside `load_mf_object` (cleanup/fail paths; see Load-path delimitation).
- Caller `dispatch_object_load` (ex-`FUN_11bd_5992`, renamed in delimitation
  pass) calls `load_mf_object` at most once per invocation
  (single-shot branch, no loop back) — see Block-walk verdict.

## Load-path delimitation (verified 2026-09-28, program `/fifa96.exe`)

`dispatch_object_load` (ex-`FUN_11bd_5992`, body `11bd:5992..11bd:5ad5`,
127 insns) is the load-path dispatcher: it checks the `'M'/'F'` magic
(`CMP byte ptr [BP+0xfefc],0x4d` at `11bd:5a2d`,
`CMP byte ptr [BP+0xfefd],0x46` at `11bd:5a34`), reads a type word via
`file_read_dos`, and dispatches on word `== 0x4d`
(`CMP word ptr [BP+0xfef6],0x4d` at `11bd:5aa1`): MF branch calls
`load_mf_object` once (`CALL 0x1000:79a2` at `11bd:5aa8`), else-branch calls
`parse_script_text` then `build_word_table`
(`CALL 0x1000:77ab` at `11bd:5ab1`, `CALL 0x1000:785b` at `11bd:5ab9`).
`mem_free_dos` (ex-`FUN_11bd_5db2`, 15 insns) is the putative AH=49h free-memory
sibling of `alloc_retry_loop`'s AH=48h probe (inferred by structural parallel:
same `FUN_11bd_2718` INT 21h register-block dispatcher, `INT 0x21` at
`11bd:273a`). Register-block offset supports the AH reading: the dispatcher
loads `AX` from block+0 (`MOV AX,word ptr [DI]` at `11bd:2720`), and `5db2`
passes block base `[BP-0xe]` (`LEA AX,[BP-0xe]` at `11bd:5dc6`) while storing
`0x49` at `[BP-0xd]` (= block+1 = AH on little-endian x86); it is called 3×
inside `load_mf_object` (at `11bd:5f5a`, `11bd:5f61`, `11bd:5fa1`, all on
cleanup/fail paths). No byte-decode transform was observed in any of the
four members, so no `decode_*` name was assigned.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_5db2 | 11bd:5db2 | `MOV byte ptr [BP-0xd],0x49` at 11bd:5db8 (= block+1 = AH: block base [BP-0xe] via `LEA` at 11bd:5dc6, dispatcher loads AX from block+0 at 11bd:2720) + `CALL 0x1000:42e8` (=2718 dispatcher) at 11bd:5dcb; called 3× inside load_mf_object (11bd:5f5a, 5f61, 5fa1); free-memory role inferred by structural parallel with alloc_retry_loop's 0x48 probe | mem_free_dos | none — behavioral (putative DOS free-memory wrapper, inferred) |
| FUN_11bd_5bdb | 11bd:5bdb | `CALL 0x1000:76a6` (=5ad6 char reader) at 11bd:5be3; `SUB AX,0x3c` (';' comment) at 11bd:5be9; `CALL file_close_dos` at 11bd:5c47 + `CALL file_open_dos` at 11bd:5c5a (include reopen); returns 0/1 | parse_script_text | none — behavioral (char-driven script/config text parser) |
| FUN_11bd_5c8b | 11bd:5c8b | `SUB AX,0x23` (quote 0x22/0x27) at 11bd:5ca2; C/E/M dispatch (0x43/0x45/0x4d) at 11bd:5cb7/5cbd/5cc0; `MOV SI,0x15e8` at 11bd:5c96 + `CALLF 0x1000:0b12` (mem_grow_relocate) at 11bd:5d55; `MOV word ptr [SI],0xffff` at 11bd:5d18 + [0xf22]/[0xcde] publish at 11bd:5d1e/5d26 | build_word_table | none — behavioral (script word-table builder) |
| FUN_11bd_5992 | 11bd:5992 | `CMP [BP+0xfefc],0x4d` at 11bd:5a2d + `CMP [BP+0xfefd],0x46` at 11bd:5a34 ('M'/'F'); `CMP [BP+0xfef6],0x4d` at 11bd:5aa1; `CALL load_mf_object` at 11bd:5aa8 else `CALL 5bdb` at 11bd:5ab1 + `CALL 5c8b` at 11bd:5ab9 | dispatch_object_load | none — behavioral (MF-magic + type-word load dispatcher) |

### Block-walk

Verdict: DEFER (becomes Task 3 FU-3 input). No deterministic next-tag rule was
observed in the funnel: `load_mf_object` runs once and returns (`RET` at
`11bd:5faa`, 194 insns, no loop back to a header read); its caller
`FUN_11bd_5992` invokes it at most once per call (branch on word `== 0x4d`, no
iteration). Missing for an ADOPT verdict: (a) who re-invokes `5992`/`5dd2` for
the next object (outer driver loop), (b) the role of `FUN_11bd_5db2` (3 calls
inside `5dd2`), (c) what the else-branch (`FUN_11bd_5bdb`/`FUN_11bd_5c8b`)
parses, and (d) any in-tail offset/stride field that locates the next tag.

### Driver loop (verdict: EXHAUSTED)

Ascent path walked with `get_function_callers`: `load_mf_object`
(`11bd:5dd2`) ← `dispatch_object_load` (`11bd:5992`, sole caller) ←
`FUN_11bd_2d9c` (sole caller) ← `entry` (`11bd:2382`, zero callers —
program entry, chain terminates). No back-edge re-invoking `5992`/`5dd2`
exists anywhere on the path: `FUN_11bd_2d9c` (body `11bd:2d9c..11bd:2ec8`)
calls `dispatch_object_load` at most twice per run — once unconditionally
(`CALL 0x1000:7562` = `11bd:5992` thunk, at `11bd:2e80`) and once
conditionally (`CALL 0x1000:7562` at `11bd:2e98`, guarded by `OR AX,AX` at
`11bd:2e91` + `JNZ` at `11bd:2e93` over the `FUN_11bd_6028` result from
`CALL 0x1000:7bf8` at `11bd:2e8c`) — then falls through to
`FUN_11bd_627f` (`CALL 0x1000:7e4f` at `11bd:2ec2`) + `RET` at `11bd:2ec8`,
or the `FUN_11bd_22ad` error path (`CALL 0x1000:3e7d` at `11bd:2ea6`,
noreturn per the decompiler's "Subroutine does not return" warning with dead
fall-through bytes after the call). `FUN_11bd_2d9c` left unrenamed (role beyond the two
dispatch sites involves unread callees `2f7f`/`614a`/`627f`/`6028`).
Consequence for the trace: at most TWO `load_mf_object` invocations per run
bound the capture window; there is no per-object driver loop to break on.

## Script semantics (verified 2026-09-28, program `/fifa96.exe`)

Attribution: NOT-ATTRIBUTABLE: no static filename bytes exist for any script
input. Include reopen at `11bd:5c5a` takes `LEA AX,[BP-0x42]` (`11bd:5c56`) +
`PUSH` (`11bd:5c59`) — a stack buffer filled at runtime from stream chars
(`LEA DI,[BP-0x42]` at `11bd:5c38`, `CALL script_read_char` at `11bd:5c3e`,
store at `11bd:5c41`, collect-while `CMP AL,0x20` / `JG` at `11bd:5c43/5c45`,
NUL at `11bd:5c53`). One caller up, `dispatch_object_load` (sole caller of
`parse_script_text` per `get_function_callers`) passes only a token word
(`PUSH word ptr [BP+0xfefa]` at `11bd:5aad`, consumed at `CMP SI,[BP+0x4]`,
`11bd:5c16`) — no filename. Dispatch's own opens are pre-dispatch setup shared
by both branches, likewise dynamic: `11bd:59d9` (stack buffer `[BP-0x100]`
built by the callee at `CALL 0x1000:6235`, `11bd:59cb`, with `0xf14`),
`11bd:59f8`/`11bd:5a69` (`PUSH 0x1190`, DS-relative static; bytes at
`11bd:1190`/`1991:1190`/`1000:1190` are jump-table/code/zeros, not a string —
DS base not statically known). Defined-strings list holds only DOS-extender
strings (`RUN.COM`, `LOADER3.EXE`, ...), no script names.

| Item | Verdict | Evidence |
|------|---------|----------|
| `;` rule | CONFIRMED-skip (both functions) | 5bdb: `SUB AX,0x3c` at `11bd:5be9` (c-0x3b after `INC` at `5be6`) + `JNZ` at `5bec` / `JMP 5c72` at `5bee`; second check `CMP SI,0x3b` at `11bd:5c2e` + `JZ 5c72` at `5c31`; skip loop `CALL` at `5c72`, `CMP SI,0xa` at `5c77`, `JNZ` at `5c7a` (keep skipping), `JMP 5be3` at `5c7c` (resume past newline), EOF `OR SI,SI` at `5c7f` / `JGE 5c72` at `5c81` / `JMP return-0` at `5c83`. 5c8b: SUB-chain reaches c==0x3b at `5caf/5cb2`, `JMP 5d62` at `5cb4`; loop `CALL` at `5d62`, `CMP AX,0xa` at `5d68`, `JNZ 5d70` at `5d6b` (keep skipping), `JMP 5c99` at `5d6d` (resume), EOF (AX<0) `OR` at `5d70` / `JGE 5d62` at `5d72` falls to epilogue return. |
| quote rule | CONFIRMED-shared collector | Entry: `SUB AX,0x23` at `11bd:5ca2` (tests c==0x22 given `INC` at `5c9f`) + `JNZ` at `5ca5` / `JMP 5d2b` at `5ca7`; `SUB AX,0x5` at `11bd:5caa` (tests c==0x27) + `JZ 5d2b` at `5cad`. Body at `5d2b`: `CMP [BP-2],0` + `JNZ 5d32` / `DEC SI` at `5d31`; flag=0 at `5d32`; `CALL` reader at `5d36`; `CMP AX,[BP-0x4]` (vs opening quote) at `5d39`; `JNZ 5d42` (store path) else `INC SI` at `5d3e` + `JMP 5c99` at `5d3f` (close, resume). Both quote bytes behave identically: raw bytes accumulate to `[SI]` until the matching close quote. |
| C/E/M branches | CONFIRMED per-branch | Dispatch: `SUB AX,0x8` at `5cb7` + `JZ 5ce8` (C==0x43); `DEC/DEC` at `5cbc/5cbd` + `JZ 5d0c` (E==0x45); `SUB AX,0x8` at `5cc0` + `JNZ 5c99` (M==0x4d, else rescan). C body at `5ce8`: `PUSH 0xf30` + `CALL match_script_keyword`; mismatch `JZ 5c99` at `5cf2`; match + flag==0 `DEC SI` at `5cfa`, store CR/LF/NUL (`MOV [SI],0xd` at `5cfb`, `MOV [SI],0xa` at `5cff`, `MOV [SI],AL=0` at `5d08`), resume via `5d3e`. E body at `5d0c`: `PUSH 0xf38` + `CALL`; mismatch resume; match falls into `5d18` terminator + publish + return. M body at `5cc5`: `PUSH 0xf29` + `CALL`; mismatch resume; match: flag!=0 NUL store (`MOV [SI],0x0` at `5cd7` + `INC SI` at `5cda`), `CALL script_read_number` at `5cdb`, `MOV [SI],AX` word store at `5cde`, `SI+=2`, flag=1, resume. |
| E/R meaning | CONFIRMED-chars (tails OPEN) | 5bdb ER pre-scan is literal ASCII: `SUB AX,0xa` at `5bf1` isolates `E` (0x45), `JZ 5bfd` at `5bf4`; `SUB AX,0xd` at `5bf6` isolates `R` (0x52), next-char `CALL` at `5bfd` + `CMP AX,0x52` at `5c00`, `JZ 5c05` on `R` else `JMP 5be3` reloop at `5bfb`; then `PUSH 0xf24` + `CALL match_script_keyword` at `5c05/5c08`, `OR AX,AX` + `JZ 5be3` at `5c0d/5c0f`. In 5c8b `E` (0x45) names the terminator branch above. E/R are chars, not flags. OPEN gap: the full NUL-terminated tails at DS-relative `0xf24`/`0xf29`/`0xf30`/`0xf38` are not in the file image (`11bd:0f24`/`1991:0f24`/`1000:0f24` probe as code/extender strings; DS base unknown) — keyword full spellings unconfirmed. |
| include reopen | CONFIRMED-mechanism | Trigger: `script_read_number` returns `0xfffc` on `@` (`CMP SI,0x40` at `5b76` + `MOV AX,0xfffc` at `5b7b`); `CMP SI,-0x4` at `11bd:5c33` + `JNZ` at `5c36` enters the path at `5c38`. Sequence: collect name chars (`5c38..5c45`), `CALL file_close_dos` at `5c47`, reset stream state (`MOV BX,[0x1188]` at `5c4a` + `MOV [BX+2],0x0` at `5c4e`), NUL at `5c53`, `CALL file_open_dos` at `5c5a`, `CMP [0xe70],0x0` at `5c5e` + `JL return-0` at `5c63/5c68` (fail shut). Name itself NOT-ATTRIBUTABLE (see Attribution). |
| 5bdb return rule | CONFIRMED | Returns 1: token==caller word (`CMP SI,[BP+0x4]` at `5c16` + `JZ 5c20` at `5c19`, `MOV AX,1` at `5c20`) or `*` sentinel (`CMP SI,-0x3` at `5c1b` + `JNZ 5c25` at `5c1e`, falls to `5c20`). Returns 0: reader EOF (`JZ 5c2a` at `5be7`, `JMP 5c2a` at `5c68`/`5c83`) or -1 token (`CMP SI,-0x1` at `5c25` + `JNZ 5c2e` at `5c28`, falls to `SUB AX,AX` at `5c2a`). Epilogue `5c85..5c8a`. LF sentinel -2 reloops (`CMP SI,-0x2` at `5c6a` + `JNZ 5c11` re-read at `5c6d` else `JMP 5be3`). |
| table append/limit/grow/terminator/publish | CONFIRMED | append: `SUB AH,AH` at `5d42` + `MOV [SI],AX` at `5d44` (zero-extended byte store, cursor SI from `MOV SI,0x15e8` at `5c96`). limit: `MOV AX,[0xce6]` at `5d46` + `SUB AX,0x10` at `5d49` + `INC SI` at `5d4c` + `CMP AX,SI` at `5d4d` + `JNC` at `5d4f`. grow: `MOV AX,0x200` at `5d51` + `PUSH` at `5d54` + `CALLF mem_grow_relocate` at `5d55` + `OR AX,AX` at `5d5b` + `JNZ` continue at `5d5d`, fail `INC SI` at `5d5f` + `JMP 5d18` at `5d60`. terminator: `MOV [SI],0xffff` at `5d18` (+`SI+=2` at `5d1c/5d1d`). publish: `MOV [0xf22],SI` at `5d1e`, `MOV AX,SI` at `5d22` + `AND AL,0xfe` at `5d24` + `MOV [0xcde],AX` at `5d26`. |

Newcomer renames (this pass, all CONFIRMED above, then `save_program` on
`/fifa96.exe` — success): `FUN_11bd_5ad6` → `script_read_char` (buffered char
reader: cursor block at `[0x1188]`, `INC [SI]` at `5adb`, refill `0x100` bytes
via `file_read_dos` at `5aec` when `[SI+2]` reached at `5adf`, EOF `-1` via
`MOV AX,0xffff` at `5af8`, byte+`CBW` at `5b03/5b06`); `FUN_11bd_5b51` →
`script_read_number` (decimal accumulator `IMUL` at `5b8b`, sentinels LF→-2 at
`5b67`, `*`→-3 at `5b71`, `@`→-4 at `5b7b`); `FUN_11bd_5bab` →
`match_script_keyword` (keyword pointer at `[BP+4]` per `5bcc`, NUL→return-1 at
`5bcf/5bd4`, per-char compare at `5bb9/5bc1/5bc4`, mismatch return-0 at `5bc8`).
No `decode_*` (no byte transform observed in any member). OPEN items got no
renames (keyword tail spellings, static `0x1190`/`0xfxx` targets).

Gate for fifa96_script lib: PASS with confirmed list (table append 5d44, limit 5d4d, grow 5d55, terminator 5d18, publish 5d1e/5d26; ';'-skip, quote collector, C/E/M branches, E/R-as-chars, include close/reopen, 5bdb 0/1 return rule, helpers script_read_char/script_read_number/match_script_keyword).

## Boot preamble (verified 2026-09-28, program `/fifa96.exe`)

`entry` (`11bd:2382..11bd:23fc`, 51 insns, zero callers — program entry)
establishes before `CALL 0x1000:496c` (= `FUN_11bd_2d9c`, verified by thunk
lookup) at `11bd:23f9`: stack/segments (`MOV SS,DI` with DI=0x1000 at
`11bd:2385`, `ADD SP,0x120e` at `11bd:2387`, `PUSH SS`/`POP ES` at
`11bd:23b7/23b8`, `PUSH SS`/`POP DS` at `11bd:23c6/23c7`, `CLD` at `11bd:23b9`);
paragraph-delta + SP snapshot (`SUB SI,DI` at `11bd:238e`, `SHL AX,CL` at
`11bd:2394`, `MOV SS:[0xce6],AX` at `11bd:2397`, `MOV SS:[0xcdc],SP` at
`11bd:239b`); DOS shrink (`MOV AH,0x4a` + `INT 0x21` at `11bd:23ae/23b0`);
DS snapshot (`MOV SS:[0xcec],DS` at `11bd:23b2`); BSS zero (`MOV DI,0x1186` +
`STOSB.REP` at `11bd:23ba/23c4`); DOS version (`MOV AH,0x30` + `INT 0x21` at
`11bd:23c8/23ca`, stored to `[0xcee]` at `11bd:23cc`); console device-flag
loop (`MOV AX,0x4400` + `INT 0x21` at `11bd:23d2/23d5`, `TEST DL,0x80` at
`11bd:23d9`, `OR byte ptr [BX+0xcfa],0x40` at `11bd:23de`, `DEC BX`/`JNS` at
`11bd:23e3/23e4` over BX=4..0); `CALL 0x1000:4030` at `11bd:23e6`; then
`XOR BP,BP` at `11bd:23e9` and three argument-word pushes (`PUSH [0xcf6]` /
`[0xcf4]` / `[0xcf2]` at `11bd:23ed/23f1/23f5`) before the call, `RET` at
`11bd:23fc`. No argv/env string decoding beyond the three pushed words —
full CRT argument parsing left OPEN.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_2f7f | 11bd:2f7f | `CMP [BP+0x4],0x20` at 11bd:2f82 + `CMP ...,0x9` at 2f88 + `CMP ...,0xa` at 2f8e + `CMP ...,0xd` at 2f94; `MOV AX,0x1` at 2f9a else `SUB AX,AX` at 2f9f; called 2× in 2d9c char loops (11bd:2e3a, 11bd:2e55 via thunk 0x1000:4b4f) | test_space_char | none — behavioral (whitespace classifier: space/tab/LF/CR test) |
| FUN_11bd_614a | 11bd:614a | `SCASB.REPNE` strlen prologues at 11bd:615c/6171; `JCXZ` empty-needle out at 6161; `JBE` length guard at 617b; `LODSB`+`SCASB.REPNE` scan at 6182/6187; `CMPSB.REPE` verify at 6194; miss `XOR AX,AX` at 61a2, hit `LEA AX,[BX-1]` at 6198; returns char *; called 2× in 2d9c prologue with NULL checks (11bd:2dcf, 11bd:2de0 via thunk 0x1000:7d1a) | find_substring | none — behavioral (substring search, strstr-like) |
| FUN_11bd_6028 | 11bd:6028 | `SCASB.REPNE` strlen at 11bd:603e; `CMPSB.REPE` compare at 6044; `SBB AX,AX` + `SBB AX,-1` sign pattern at 6048/604a (returns -1/0/1 short); called at 11bd:2e8c (thunk 0x1000:7bf8), result zero-gated in 2d9c (`if (sVar5 == 0)` → 2d43 + second dispatch) | compare_strings | none — behavioral (string compare, strcmp-like sign result) |
| FUN_11bd_627f | 11bd:627f | `MOV SP,[0xcdc]` at 11bd:627f; `SMSW [0xf88]` at 6283; 11 init callees (3844, 0c0d, 2fa3, 199a, 64b7, 641d, 016c, 65a6, 1280, 0251, 7290); `[0x2e]`-gated blocks at 6299/62a3/62ad/62e2; called once at 2d9c tail (11bd:2ec2 via thunk 0x1000:7e4f) before RET | run_postload_init | none — behavioral (post-load tail init sequence) |
| FUN_11bd_2d43 | 11bd:2d43 | `MOV [0x11d4],0x7` at 11bd:2d43; `MOV AX,0x15` + `PUSH AX` at 2d49/2d4c; `CALL 0x1000:3e7d` (=22ad, noreturn) at 2d4d; called at 11bd:2e95 (thunk 0x1000:4913) on the 6028==0 branch | raise_boot_error | none — behavioral (boot error raise: code store + noreturn call) |
| FUN_11bd_6a2d | 11bd:6a2d | `MOV DI,0xa2c` dest at 11bd:6a3c/6a48; `CMP AL,0x5c` at 6a53 + `MOV BX,DI` last-backslash track at 6a57; `MOV DI,BX` truncate at 6a5b; `MOV SI,0xa7c` + `LODSB`/`STOSB` append loop at 6a5d/6a60-6a64; called at 11bd:2ebd (thunk 0x1000:85fd) on the doubly-nested `[0x2f]`>2 path | splice_path_tail | none — behavioral (path splice: truncate at last backslash + append tail) |

Notes:
- All six renames verb-led; no `decode_*` (no byte-decode transform observed
  in any member — 614a/6028 are compare/search, 6a2d copies bytes verbatim via
  `LODSB`/`STOSB`).
- `FUN_11bd_2d9c` (body `11bd:2d9c..11bd:2ec8`) left unrenamed: orchestrator
  role spans unread callees beyond these six (65e1, 66e1, 191d, 18ba, 2fa5,
  3ed8, 2d99); revisit when those resolve.

### 3ed8 verdict: NEEDS-OWN-SLICE (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_3ed8` (body `11bd:3ed8..11bd:4586`, 667 insns, `RET` at `11bd:4586`;
adjacent `FUN_11bd_4587` is a callee, confirming the boundary) left unrenamed:
its role does not fit one paragraph, so no bound row and no rename.

Entry conditions: two word args (decompile
`void __fastcall FUN_11bd_3ed8(undefined2 param_1, undefined2 param_2)`,
called after the second-dispatch check in `2d9c`); frame `PUSH BP` /
`MOV BP,SP` / `SUB SP,0xec` at `11bd:3ed8..3edb`; first call
`CALL 0x1000:7ec8` (= `62f8`) at `11bd:3ef2` forwarding the args; BIOS
equipment-word probe (`MOV SI,word ptr ES:[BX]` at `11bd:3f1b` with
ES=0xF000/BX=0xFFFE, `CMP SI,0xfb` at `3f22` / `CMP SI,0xfd` at `3f28`,
`INC [0x11f0]` at `3f2e`); status cells `MOV [0x11d4],0x2` at `3f0d`,
`CALL 68c2` at `3f32`, `CALL 2d40` at `3f38`, `MOV [0x11d4],0x3` at `3f3d`
(`2d40` NOT conflated with Task-1 `raise_boot_error` at `2d43`).

Exit shape: tail publishes a mode byte (`CMP [0x10ee],0xff` at `11bd:451e`,
`MOV [0x10ee],AL` / `MOV [0x2e],AL` at `4527/452c`), `CALL 6250` at `4536`,
`CALL 4587` at `455e`, SI==0xb arm `CALL 7c62` at `4575`,
`[BP-0x56]`==0 arm `CALL 30d8` at `457e`, then `POP SI` / `POP DI` /
`MOV SP,BP` / `POP BP` / `RET` at `4581..4586` — normal void return
(noreturn paths exit via `CALL 22ad` at `4240/43c6/451a` instead).

Callee summary: 38 callees (`get_function_callees`, limit 100). Known-role
loader/script helpers: `find_substring` (`614a`, via thunk `0x1000:7d1a`),
`mem_grow_relocate` (`CALLF 0x1000:0b12` at `11bd:4445`), `30d8`, `22ad`
(noreturn, via thunk `0x1000:3e7d`). Resolved names with roles still open:
`62f8`, `6054`, `6250`. Unknown domain (~30 unread): `18f1`, `195d`, `243e`, `25ee`, `2620`, `2d40`,
`4587`, `45c3`, `4645`, `4665`, `6120`, `6395`, `6400`, `6618`, `6667`,
`6672`, `66a2`, `66b9`, `6869`, `689e`, `68b9`, `68c2`, `698b`, `69ac`,
`69e0`, `6c74`, `6d9c`, `76db`, `7c62` (+ 2 thunks of `61ee`).

Why it exceeds a bound: a config-probe fan-out (`4665`/`4645`/`6120`/
`45c3`/`195d` loops at `3f66..41a1`) feeds a central multi-arm dispatch on
the SI mode word (`DEC AX` / `JNZ` chain plus `SUB AX,0x3/0x10a/0x64/
0x171a/0xdad` at `11bd:42de..4368`, ~20 arms) publishing distinct
`[BP-0x5a]`/`[0x10ee]`/`[0x2e]` constants, with further globals published
(`0xeca`/`0xecc`/`0xece` at `40b0/40d4/40f6`, `0x11d4`, `0x46`/`0x47`,
`0x10ee`). Mode/config/video/domain unknowns mix across 30 unread callees —
bounding needs its own slice.

## 3ed8 probe cluster (verified 2026-09-29, program `/fifa96.exe`)

The `3f66..41a1` probe loops inside `FUN_11bd_3ed8` feed caller-string
arguments to four config probes: `4665` is called at `11bd:3f66`/`3f7a`,
`4645` at `11bd:3f89` (plus 2× inside `45c3`), `6120` at `11bd:3fea`/`406d`/
`4099`/`40bd`/`40e6`/`4108`/`414b`/`4160` (plus 1× inside `45c3`), and `45c3`
at `11bd:40a5`/`40c9`/`40f2`/`4114`/`4172`/`4196` (all callers per
`get_function_xrefs`). Each probe publishes back to its loop caller: the two
leaf scanners return pointers (`4645` an advanced string pointer, `6120` a
match pointer or NULL), `4665` copies a matched value into the caller buffer
sourced from the `[0x9b8]` table, and `45c3` returns a parsed numeric value
(`M`-suffixed values scaled ×1024). OPEN: the downstream consumers of the
published values (which loop iterations feed the `42de..4368` SI-mode
dispatch arms) and the role of `195d` (called by `45c3`, out of scope).

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_4645 | 11bd:4645 | body `11bd:4645..4664`, 17 insns, leaf (0 callees); `CMP byte ptr [SI],0x20` at 11bd:464e + `CMP byte ptr [SI],0x9` at 11bd:4653 (space/tab skip); `MOV AX,SI` at 11bd:465e (returns advanced pointer); called at 11bd:3f89 + 2× inside 45c3 | skip_blank_chars | none — behavioral (blank skipper) |
| FUN_11bd_6120 | 11bd:6120 | body `11bd:6120..6149`, 24 insns, leaf (0 callees); `SCASB.REPNE ES:DI` strlen at 11bd:6130 + char scan at 11bd:613a; `CMP byte ptr [DI],AL` at 11bd:613d; miss `XOR DI,DI` at 11bd:6141 (NULL); 8× callers in 3ed8 loops + 1× inside 45c3 | find_char_in_string | none — behavioral (char finder) |
| FUN_11bd_4665 | 11bd:4665 | body `11bd:4665..46dd`, 47 insns, leaf (0 callees); `CMP word ptr [0x9b8],0x0` at 11bd:466c + `MOV AX,[0x9b8]` at 11bd:4677 (table source); per-char `CMP byte ptr ES:[BX],AL` at 11bd:4695; `MOV word ptr [BP-0x6],0x80` at 11bd:46b1 (copy bound) into `[BP+0x6]` buffer; `MOV byte ptr [SI],0x0` at 11bd:46d3 (NUL); miss returns 0 (`SUB AX,AX` at 11bd:4673); called at 11bd:3f66/3f7a | lookup_copy_config_string | none — behavioral (config string lookup + value copy) |
| FUN_11bd_45c3 | 11bd:45c3 | body `11bd:45c3..4644`, 71 insns; callees: 4645, 195d, 45ab, 6120, 61ee-thunk; skip-blanks via 4645-thunk `CALL 0x1000:6215` at 11bd:45d8; hex accumulate `SHL DI,CL` (CL=4) at 11bd:4615 + `SUB AX,0xab0` at 11bd:4617 + `ADD DI,AX` at 11bd:461a; `CMP byte ptr [BX],0x4d` ('M') at 11bd:4629; `SHL DI,CL` (CL=10, ×1024) at 11bd:463b else `MOV DI,0xffff` at 11bd:4634; returns AX=DI at 11bd:463d; called 6× in 3ed8 loops (11bd:40a5/40c9/40f2/4114/4172/4196) | parse_config_number | none — behavioral (config number parser, M-suffix kilobytes) |

### SI-producer boundary: 11bd:3f9c (verdict: BOUNDARY — no rename, program untouched)

Question: where does the SI mode word dispatched at `42de..4368`
(`MOV AX,SI` at `11bd:42db`, `DEC AX`/`JNZ` chain) come from?
Derivation facts (disassembly of `FUN_11bd_3ed8`, program `/fifa96.exe`;
far-thunk delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx`, verified uniform on
12 call sites, e.g. `CALL 0x1000:6235` at `3f66` = `4665`):

- The anchor `MOV SI,word ptr ES:[BX]` at `11bd:3f1b` (ES=0xF000/BX=0xFFFE)
  + `AND SI,0xff` at `3f1e` is DEAD for the dispatch: `CALL 0x1000:4910`
  (= `2d40`) at `3f38` + `MOV SI,AX` at `3f3b` overwrites SI. The BIOS word
  only gates `CMP SI,0xfb` at `3f22` / `CMP SI,0xfd` at `3f28` →
  `INC byte ptr [0x11f0]` at `3f2e`.
- `FUN_11bd_2d40` (body `11bd:2d40..2d42`: `SUB AX,AX` + `RET`; sole caller
  is `3f38` per `get_function_xrefs`) always returns 0, so SI=0 at `3f3b`
  always and `OR SI,SI` at `3f4b` always takes `JZ` to the `MOV SI,0xa`
  default at `3f52`. The nonzero path (`JMP` at `3f4f` → `41a4`) is
  unreachable as disassembled.
- SI writers inside the `3f66..41a1` probe window: `MOV SI,0x3` at `3fa5`,
  `MOV SI,0xb` at `3fce`, `MOV SI,0x2` at `402e` (each gated by a nonzero
  `195d` result via `OR AX,AX` at `3fa1/3fca/402a` + `JZ`), and
  `MOV SI,AX` at `4063` (61ee-thunk `CALL 0x1000:7c50` at `4037` plus
  `SUB AX,0x3880` / `SBB DX,0x1` adjust at `4057/405a`).
- `195d` (leaf, 0 callees; 6 sites in 3ed8 at
  `3f9c/3fb4/3fc5/3fd9/4017/4025` per xrefs, plus 1 inside
  `parse_config_number` at `45df`) is a compare predicate returning 1 on
  first-string exhaustion and 0 on mismatch — it gates the SI constants
  but the constants live in the 3ed8 body, so it is not the producer.
- Past the window SI is re-sourced again: `PUSH SI` + `CALL 0x1000:6157`
  (= `4587`) at `41a4/41a5` takes SI as an argument, `OR SI,SI` is read at
  `41ba` with a `NEG`-merge at `41c1..41c5`, and conditional on
  `[BP-0x4]==0` (at `426c`) the `689e/6869/68b9` sequence at `4272..429d`
  reloads SI via `MOV SI,word ptr [BP+0xff16]` at `42c3` before
  `MOV AX,SI` at `42db`.
- Globals published in the window (`[0xeca]` at `40b0`, `[0xece]` at
  `40d4`, `[0xecc]` at `40f6`, `[0x14]` at `4118`, all from `45c3` outputs)
  are side channels never loaded into SI.

Why it exceeds one layer: the dispatched SI is a merge across at least
five distinct helper layers (2d40 zero-gate, 195d predicate-gated body
constants, 61ee-thunk arithmetic, 4587 arg-handoff, 689e/6869/68b9 stack
re-source) plus body defaults — no single called FUN "computes or
publishes the mode value". Per the one-layer guard: STOP, no rename.

## 3ed8 mode dispatch (verified 2026-09-29, program `/fifa96.exe`)

Entry `MOV AX,SI` at `11bd:42db` opens a 22-arm decrement chain
(`DEC AX`/`JNZ`-to-next-link with a `JMP`-to-body per link at
`11bd:42dd..4348`, then `SUB AX,<const>`/`JNZ` links with constants
`0x3`/`0x10a`/`0x64`/`0x171a`/`0xdad` at `11bd:433d..4366`; far-target
delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx` holds for every link target).
SI values are cumulative subtractions: arms 1–16 take SI `1..0x10`,
then `0x13` (16+3), `0x14` (+1), `0x11e` (+0x10a), `0x182` (+0x64),
`0x189c` (+0x171a), `0x2649` (+0xdad). Pre-chain prefix at
`42d3..42d6` (`CALL 0x1000:857c`, `MOV [0x10ee],0xff`, gated on
`[0x47]` bit `0x80`) is setup, not an arm. Normal arms rejoin toward
the `451e` tail via `JMP 0x1000:60ee` (= `11bd:451e`) sites (`43ea`,
`4408`, `4420`, `4461`, `4493`, `44b0`, `44b7`, `44e5`) or mid-body
joins (`43aa`, `4403`, `4418`, `442f`, `4469`, `44a3`); the
fall-through default (`JMP 0x1000:60b7` at `4368` → `11bd:44e7`)
publishes `[0x11d4]=0x6` and ends in the noreturn `CALL 0x1000:3e7d`
(`=22ad`) at `451a`, never merging to `451e`. Guard-hit proxy count (NOT the
briefed depth verdict): 15 of 22 arms contain ≥1 CALL in-body (incl. via
joins) and are deferred to Task 2 with no callee opened; 7 arms (6, 8,
11, 12, 13, 16, 21) are call-free. The briefed metric (call list deeper
than one layer) requires opening callees, so Task 1 records this ≥1-CALL
proxy only — Task 2 confirms actual depth. No range in `42db..4368` is uncovered (every link is a
contiguous `DEC`/`SUB`+`JNZ`/`JZ`+`JMP` triple — see NOT-COVERED note).

| # | Condition | Target | Publishes | Calls (address only) |
|---|-----------|--------|-----------|----------------------|
| 1 | SI==1: `DEC AX` at 42dd; `JNZ 42e3` at 42de (not taken); `JMP` at 42e0 | 11bd:436e | `MOV [BP-0x5a],0x381` at 436e; `MOV [BP-0x5e],AL` at 4383; `MOV [0x11fe],0x146` at 4386; `MOV [0x1200],0x40` at 438c; `MOV [0xeca],0x500` at 43a4; `OR [0x14],0x4` at 43aa; `MOV [0x11d4],0x5` at 43af; `MOV [0xece],AX` at 43da; `MOV [0xa8],AX` at 43e3; `INC [0x11f0]` at 43e6 | `CALL 0x1000:8844` at 4379; `CALL 0x1000:7fd0` at 4380; `CALL 0x1000:8844` at 43bb; `CALL 0x1000:3e7d` at 43c6 |
| 2 | SI==2: `DEC AX` at 42e3; `JNZ 42e9` at 42e4; `JMP` at 42e6 | 11bd:43ed | `MOV [BP-0x5a],0x8da` at 43ed; `MOV [0x120c],AL` at 43f5; `OR [0x120c],0x2` at 43fe; `MOV [0x10ef],0x1` at 4403 | `CALL 0x1000:8242` at 43f2 |
| 3 | SI==3: `DEC AX` at 42e9; `JNZ 42ef` at 42ea; `JMP` at 42ec | 11bd:440b | `MOV SI,0x3` at 440b; `MOV [BP-0x5a],0x2824` at 440e; `MOV [0x10ee],0x9` at 4413; `MOV [0x10ef],0x1` at 4418 | `CALL 0x1000:85b0` at 441d |
| 4 | SI==4: `DEC AX` at 42ef; `JNZ 42f5` at 42f0; `JMP` at 42f2 | 11bd:4469 | always `MOV SI,0x9` at 4469 + `MOV [BP-0x5a],0x8b2` at 446c; gated `[0x11f0]!=0` at 4471 AND `[0x2f]==2` at 4478 path: `CALL 8289` at 447f, then nonzero-return `MOV [0x10ee],0xa` at 4489 + `MOV [BP-0x5a],0x749` at 448e (overwrite; zero-return exits via 4496 `MOV AX,0x13` with no publish); gate-fail path: 4418-tail join `MOV [0x10ef],0x1` at 4418 | `CALL 0x1000:8289` at 447f; `CALL 0x1000:85b0` at 441d (via 4418 gate-fail join) |
| 5 | SI==5: `DEC AX` at 42f5; `JNZ 42fb` at 42f6; `JMP` at 42f8 | 11bd:4423 | `MOV [BP-0x5a],0x3a7` at 4423; `[0x2f]>=3` path `MOV [0x36],0x80` at 442f; `[0x3e]!=0` path `MOV [0xf82],AX` at 4453; arm-1 tail from 43aa: `OR [0x14],0x4` at 43aa + `MOV [0x11d4],0x5` at 43af + `MOV [0xece],AX` at 43da + `MOV [0xa8],AX` at 43e3 + `INC [0x11f0]` at 43e6 | `CALLF 0x1000:0b12` at 4445; `CALL 0x1000:400e` at 444f; `CALL 0x1000:8844` at 43bb; `CALL 0x1000:3e7d` at 43c6 (via 43aa tail join) |
| 6 | SI==6: `DEC AX` at 42fb; `JNZ 4301` at 42fc; `JMP` at 42fe | 11bd:445c | `MOV [BP-0x5a],0x71a` at 445c (skips `MOV SI,0x6` at 4459; SI already 6) | none |
| 7 | SI==7: `DEC AX` at 4301; `JNZ 4307` at 4302; `JMP` at 4304 | 11bd:446c | `MOV [BP-0x5a],0x8b2` at 446c; gated path as row 4: nonzero-return `MOV [0x10ee],0xa` at 4489 + `MOV [BP-0x5a],0x749` at 448e; gate-fail path: 4418-tail join `MOV [0x10ef],0x1` at 4418 | `CALL 0x1000:8289` at 447f; `CALL 0x1000:85b0` at 441d (via 4418 gate-fail join) |
| 8 | SI==8: `DEC AX` at 4307; `JNZ 430d` at 4308; `JMP` at 430a | 11bd:4486 | `MOV SI,0x8` at 4486; `MOV [0x10ee],0xa` at 4489; `MOV [BP-0x5a],0x749` at 448e | none |
| 9 | SI==9: `DEC AX` at 430d; `JNZ 4313` at 430e; `JMP` at 4310 | 11bd:446c | `MOV [BP-0x5a],0x8b2` at 446c; gated path as row 4: nonzero-return `MOV [0x10ee],0xa` at 4489 + `MOV [BP-0x5a],0x749` at 448e; gate-fail path: 4418-tail join `MOV [0x10ef],0x1` at 4418 | `CALL 0x1000:8289` at 447f; `CALL 0x1000:85b0` at 441d (via 4418 gate-fail join) |
| 10 | SI==0xa: `DEC AX` at 4313; `JNZ 4319` at 4314; `JMP` at 4316 | 11bd:44a3 | `MOV [BP-0x5a],0x905` at 44a3; joins arm-3 tail at 4418 (`MOV [0x10ef],0x1`) | `CALL 0x1000:85b0` at 441d (via join) |
| 11 | SI==0xb: `DEC AX` at 4319; `JNZ 431f` at 431a; `JMP` at 431c | 11bd:44ab | `MOV [BP-0x5a],0x29bc` at 44ab | none |
| 12 | SI==0xc: `DEC AX` at 431f; `JNZ 4325` at 4320; `JMP` at 4322 | 11bd:44b2 | `MOV [BP-0x5a],0x679` at 44b2 | none |
| 13 | SI==0xd: `DEC AX` at 4325; `JNZ 432b` at 4326; `JMP` at 4328 | 11bd:44b9 | `MOV [BP-0x5a],0x8ac` at 44b9; `MOV [0x10ee],0x9` at 44be; joins arm-2 tail at 4403 (`MOV [0x10ef],0x1`) | none |
| 14 | SI==0xe: `DEC AX` at 432b; `JNZ 4331` at 432c; `JMP` at 432e | 11bd:44c6 | `MOV [BP-0x5a],0x3d6` at 44c6; arm-1 tail from 43aa: `OR [0x14],0x4` at 43aa + `MOV [0x11d4],0x5` at 43af + `MOV [0xece],AX` at 43da + `MOV [0xa8],AX` at 43e3 + `INC [0x11f0]` at 43e6 | arm-1 tail calls via 43aa join (`CALL 0x1000:8844` at 43bb; `CALL 0x1000:3e7d` at 43c6) |
| 15 | SI==0xf: `DEC AX` at 4331; `JNZ 4337` at 4332; `JMP` at 4334 | 11bd:44ce | `MOV [BP-0x5a],0x462` at 44ce; `JGE 44dd` at 44d8 selects join: `[0x2f]<3` direct to 43aa tail at 44da, `[0x2f]>=3` via 442f `MOV [0x36],0x80` path then 43aa tail (`OR [0x14],0x4` at 43aa + `MOV [0x11d4],0x5` at 43af + `MOV [0xece],AX` at 43da + `MOV [0xa8],AX` at 43e3 + `INC [0x11f0]` at 43e6) | `CALL 0x1000:8844` at 43bb; `CALL 0x1000:3e7d` at 43c6 (both branches converge via 43aa tail) |
| 16 | SI==0x10: `DEC AX` at 4337; `JNZ 433d` at 4338; `JMP` at 433a | 11bd:44e0 | `MOV [BP-0x5a],0x4f7` at 44e0 | none |
| 17 | SI==0x13: `SUB AX,0x3` at 433d (cumul. 16+3=19); `JNZ 4345` at 4340; `JMP` at 4342 | 11bd:4464 | `MOV [0xed0],0x0` at 4464; falls into arm-4 body at 4469 (`MOV SI,0x9` at 4469 + `MOV [BP-0x5a],0x8b2` at 446c; gated nonzero-return `MOV [0x10ee],0xa` at 4489 + `MOV [BP-0x5a],0x749` at 448e; gate-fail 4418-tail `MOV [0x10ef],0x1` at 4418) | `CALL 0x1000:8289` at 447f (via fall-through); `CALL 0x1000:85b0` at 441d (via 4418 gate-fail join) |
| 18 | SI==0x14: `DEC AX` at 4345 (cumul. 20); `JNZ 434b` at 4346; `JMP` at 4348 | 11bd:449b | `MOV [0xed0],0x0` at 449b; `MOV SI,0xa` at 44a0; falls into arm-10 at 44a3 (`MOV [BP-0x5a],0x905` at 44a3 + 4418-tail `MOV [0x10ef],0x1` at 4418 via 44a8 join) | `CALL 0x1000:85b0` at 441d (via 44a8→4418 join) |
| 19 | SI==0x11e: `SUB AX,0x10a` at 434b (cumul. 20+266=286); `JNZ 4353` at 434e; `JMP` at 4350 | 11bd:4469 | `MOV SI,0x9` at 4469 + `MOV [BP-0x5a],0x8b2` at 446c (shared target with row 4); gated nonzero-return `MOV [0x10ee],0xa` at 4489 + `MOV [BP-0x5a],0x749` at 448e; gate-fail 4418-tail `MOV [0x10ef],0x1` at 4418 | `CALL 0x1000:8289` at 447f; `CALL 0x1000:85b0` at 441d (via 4418 gate-fail join) |
| 20 | SI==0x182: `SUB AX,0x64` at 4353 (cumul. 286+100=386); `JNZ 435b` at 4356; `JMP` at 4358 | 11bd:440b | `MOV SI,0x3` at 440b; `MOV [BP-0x5a],0x2824` at 440e; `MOV [0x10ee],0x9` at 4413; `MOV [0x10ef],0x1` at 4418 (shared target with row 3) | `CALL 0x1000:85b0` at 441d |
| 21 | SI==0x189c: `SUB AX,0x171a` at 435b (cumul. 386+5914=6300); `JNZ 4363` at 435e; `JMP` at 4360 | 11bd:4459 | `MOV SI,0x6` at 4459; `MOV [BP-0x5a],0x71a` at 445c | none |
| 22 | SI==0x2649: `SUB AX,0xdad` at 4363 (cumul. 6300+3501=9801); `JZ 436b` at 4366 (taken) | 11bd:436b | `MOV SI,0x1` at 436b; falls into arm-1 body at 436e (row-1 publishes: `[BP-0x5a]`=0x381, `[BP-0x5e]`=AL, `[0x11fe]`=0x146, `[0x1200]`=0x40, `[0xeca]`=0x500, `OR [0x14]`=0x4, `[0x11d4]`=0x5, `[0xece]`=AX, `[0xa8]`=AX, `INC [0x11f0]`) | `CALL 0x1000:8844` at 4379; `CALL 0x1000:7fd0` at 4380; `CALL 0x1000:8844` at 43bb; `CALL 0x1000:3e7d` at 43c6 |
| default | no arm matched: `JMP 0x1000:60b7` at 4368 | 11bd:44e7 | `MOV DI,0x1190` at 44e7; `MOV [DI],0x0` at 44ea; `MOV [0x11d4],0x6` at 4510 | `CALL 0x1000:41be` at 44fa + `CALL 0x1000:7c24` at 450a (both skipped if `[BP-0x5c]==0` via `JZ 4510` at 44f1); `CALL 0x1000:3e7d` at 451a (noreturn — default never reaches 451e) |

NOT-COVERED: none inside `11bd:42db..4368` — every byte belongs to a
listed link (`DEC`/`SUB` + `JNZ`/`JZ` + `JMP` triples are contiguous:
6-byte `DEC` links `42dd..433a`, 8-byte `SUB` links
`433d/434b/4353/435b`, 6-byte `DEC` link `4345`, 8-byte tail
`4363..436a`). Shared-target pairs (7+9→446c, 4+19→4469, 3+20→440b,
1+22→436b/436e) are separate rows above, not gaps. Merge point
`11bd:451e` (`CMP [0x10ee],0xff`) and the `451e..4586` tail are exit
facts owned by the `3ed8 verdict` section, not re-verified here.

### Callee verdicts (one-layer, verified 2026-09-29, program `/fifa96.exe`)

Scope: the 15 proxy-hit arms resolve to 8 unique callees (far-thunk
delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx`, re-confirmed per callee by
`get_function_by_address` at the computed entry; unresolvable: none).
Default-row calls (`44fa`/`450a`/`451a`, body `11bd:44e7` past `4368`)
are downstream consumers — NOT in scope, untouched.
`mem_grow_relocate` (`1000:0b12..1000:0c0c`, 0 FUN callees) is already
CONFIRMED+renamed (Codec funnel section) — no new row, no re-rename.
Leaf = the callee itself is the one allowed FUN layer (0 callees);
any callee with ≥1 callee is a second layer past the arm body and
defers its arm (record, do not pursue).

| arm # | call site | callee FUN + bounds | verdict + evidence | new_name or — |
|-------|-----------|---------------------|--------------------|---------------|
| 1, 5, 14, 15, 22 | `CALL 0x1000:8844` at 4379 / 43bb | `probe_xms_installed`, `11bd:6c74..6c83`, leaf (0 callees) | CONFIRMED — `MOV AX,0x4300` at 6c74 (INT 2Fh XMS installation check) + `INT 0x2f` at 6c77 + `CMP AL,0x80` at 6c79 + `OR byte ptr [0x36],AL` at 6c7f (sets the `0x80` bit in `[0x36]` iff XMS present; same cell arm 5 publishes at 442f) | probe_xms_installed |
| 2 | `CALL 0x1000:8242` at 43f2 | `probe_bios_model`, `11bd:6672..66a1`, leaf (0 callees) | CONFIRMED — `CMP byte ptr [0x34],0x15` gate at 6672 + `MOV AH,0xc0` at 667c + `INT 0x15` at 667e (BIOS Get System Configuration) + model-byte validation `CMP CL,0xe0/0xf8/0xfc` + `CMP CH,0x4` at 6689..669b + `MOV AL,ES:[BX+0x5]` feature-byte return at 669d (0 on gate/model/carry fail) | probe_bios_model |

NOT-CONFIRMED (no rename, no row): `FUN_11bd_6400`
(`11bd:6400..641c`, leaf, called at 4380 by arms 1+22): `XOR AX,AX` at
6400 + `XOR BX,BX` at 6402 + `MOV CL,0x82` at 6404 + `INT 0xdc` at 6406
+ `SHL BX,0x6` / `MOV word ptr [0xeca],BX` at 640e/6411 + `SHL DX,0x6`
/ `MOV word ptr [0xece],DX` at 6415/6418. Missing: the `INT 0xDCh`
`CL=0x82` input/output contract (extender-private API) — the queried
source is unidentified, so no role name.

In-scope arms with nothing to verdict: 6, 8, 11, 12, 13, 16, 21
(call-free per the arm table; arm 13's 4403 join never reaches a call).

### Guard-deferred: arm 1 (11bd:436e)

Call sites: 4379 (`6c74` CONFIRMED leaf), 4380 (`6400` NOT-CONFIRMED
leaf), 43bb (`6c74`), 43c6 (deep, see below). Known: error path exits
via the noreturn call. Excess: `FUN_11bd_22ad` (`11bd:22ad..2381`,
called at 43c6) has 1 FUN callee (`FUN_11bd_25ee`) — a second layer
past the arm body (decompile surface: error-message formatting +
`0x15e8`/`0xf22` word-table scan). No rename.

### Guard-deferred: arm 3 (11bd:440b)

Call site: 441d (deep). Excess: `FUN_11bd_69e0` (`11bd:69e0..6a2c`)
has 1 FUN callee (`FUN_11bd_69c7`, `11bd:69c7..69df`) — a second layer
past the arm body (decompile surface: port-`0x92` writes + 9-byte copy
loops). No rename.

### Guard-deferred: arm 4 (11bd:4469)

Call sites: 447f (deep), 441d via the 4418 gate-fail join (deep).
Excess: `FUN_11bd_66b9` (`11bd:66b9..66d3`, at 447f) has 2 FUN callees
(`FUN_11bd_0733`, `FUN_11bd_0c0d`) plus `FUN_11bd_69e0` (at 441d, as in
arm 3) — both second layers past the arm body. No rename.

### Guard-deferred: arm 5 (11bd:4423)

Call sites: 4445 (`mem_grow_relocate`, already-named leaf), 444f
(deep), 43bb/43c6 via the 43aa tail join (`6c74` leaf + deep).
Excess: `FUN_11bd_243e` (`11bd:243e..245b`, at 444f) has 1 FUN callee
(`FUN_11bd_65c3`) plus `FUN_11bd_22ad` (at 43c6, as in arm 1) — both
second layers past the arm body. No rename.

### Guard-deferred: arm 7 (11bd:446c)

Shared body with arm 9: 447f (`66b9`) + 441d via the 4418 gate-fail
join (`69e0`) — both deep as in arm 4. No rename.

### Guard-deferred: arm 9 (11bd:446c)

Same as arm 7 (shared target): 447f + 441d, both deep. No rename.

### Guard-deferred: arm 10 (11bd:44a3)

Call site: 441d via the 44a8→4418 join (deep: `69e0` as in arm 3).
No rename.

### Guard-deferred: arm 14 (11bd:44c6)

Call sites: 43bb (`6c74` leaf) + 43c6 (deep) via the 43aa join.
Excess: `FUN_11bd_22ad` (as in arm 1). No rename.

### Guard-deferred: arm 15 (11bd:44ce)

Same 43aa-tail convergence as arm 14: 43bb (`6c74` leaf) + 43c6
(`22ad` deep). No rename.

### Guard-deferred: arm 17 (11bd:4464)

Falls through into the arm-4 body: 447f (`66b9` deep) + 441d via the
4418 join (`69e0` deep). No rename.

### Guard-deferred: arm 18 (11bd:449b)

Call site: 441d via the 44a8→4418 join (deep: `69e0` as in arm 3).
No rename.

### Guard-deferred: arm 19 (11bd:4469)

Shared target with arm 4: 447f (`66b9`) + 441d (`69e0`), both deep.
No rename.

### Guard-deferred: arm 20 (11bd:440b)

Shared target with arm 3: 441d (`69e0` deep). No rename.

### Guard-deferred: arm 22 (11bd:436b)

Falls through into the arm-1 body: 4379/43bb (`6c74` leaf), 4380
(`6400` NOT-CONFIRMED leaf), 43c6 (`22ad` deep). No rename.

Closeout: guard-deferred arms are 14 of 22
(1, 3, 4, 5, 7, 9, 10, 14, 15, 17, 18, 19, 20, 22) = 63.6% > 1/3 —
STOP diving, skeleton-plus closeout (arm table complete, deep dives
`69e0→69c7`, `66b9→0733/0c0d`, `243e→65c3`, `22ad→25ee` deferred to a
follow-up slice). Fully in-scope arm: 2 (row above). Call-free arms:
6, 8, 11, 12, 13, 16, 21.

## 3ed8 tail (verified 2026-09-29, program `/fifa96.exe`)

From the dispatch fall-through at `11bd:4368` the arm bodies (`436b..44e5`)
run their publishes then rejoin toward the `451e` merge (`JMP 0x1000:60ee`
sites at `43ea`/`4408`/`4420`/`4461`/`4493`/`44b0`/`44b7`/`44e5`, mid-body
joins at `43aa`/`4403`/`4418`/`442f`); the fall-through default
(`JMP 0x1000:60b7` at `4368` → `11bd:44e7`) issues its two calls
(`25ee` at `44fa`, `6054` at `450a`, skipped if `[BP-0x5c]==0` via `JZ 4510`
at `44f1`) then publishes `[0x11d4]=0x6` and exits noreturn via `22ad` at
`451a`, never reaching `451e`. The merge publishes the mode byte
(`CMP [0x10ee],0xff` at `451e`, `MOV [0x10ee],AL` / `MOV [0x2e],AL` at
`4527/452c`) then runs the exit arms (`6250` at `4536`, `4587` at `455e`,
SI==0xb → `7c62` at `4575`, `[BP-0x56]`==0 → `30d8` at `457e`) before a
bare epilogue (`POP SI` / `POP DI` / `MOV SP,BP` / `POP BP` / `RET` at
`4581..4586`) with no AX staging — void return into `2d9c`. Far-thunk
delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx` holds for every tail call
(`41be→25ee`, `7c24→6054`, `3e7d→22ad`, `7e20→6250`, `6157→4587`,
`9832→7c62`, `4ca8→30d8`; all seven targets confirmed as `3ed8` callees
via `get_function_callees`, `4587` confirmed at `11bd:4587` with body
`4587..45aa`). No Stevens/char-string text surrounds the default calls
(address/immediate pushes only).

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| dispatch fall-through link | 11bd:4368 | `JMP 0x1000:60b7` at 4368 → `11bd:44e7` (default entry) | none |
| arm-1/22 body head | 11bd:436b..4383 | `MOV SI,0x1` at 436b; `MOV [BP-0x5a],0x381` at 436e; `CMP [BP-0x5e],0x0` at 4373 + `JNZ` at 4377; `OR AX,AX` at 437c + `JNZ` at 437e; `MOV [BP-0x5e],AL` at 4383 | `CALL 0x1000:8844` (=6c74) at 4379; `CALL 0x1000:7fd0` (=6400) at 4380 |
| arm-1 body mid | 11bd:4386..43aa | `MOV [0x11fe],0x146` at 4386; `MOV [0x1200],0x40` at 438c; `LES BX,[0x11fe]` at 4392; `TEST ES:[BX],0x80` at 4396 + `JZ` at 439a; `CMP [0xeca],0x500` at 439c + `JNC` at 43a2; `MOV [0xeca],0x500` at 43a4; `OR [0x14],0x4` at 43aa | none |
| arm-1 body tail + error exit | 11bd:43af..43ea | `MOV [0x11d4],0x5` at 43af; `CMP [BP-0x5e],0x0` at 43b5 + `JNZ` at 43b9; `OR AX,AX` at 43be + `JNZ` at 43c0; `MOV AX,0x12` at 43c2 + `PUSH` at 43c5; `MOV AX,[0xecc]` at 43ca + `ADD AX,[0xeca]` at 43cd; `MOV [BP-0x58],AX` at 43d1; `CMP AX,[0xece]` at 43d4 + `JGE` at 43d8; `MOV [0xece],AX` at 43da; `MOV AX,[0xece]` at 43dd + `SUB AX,0x400` at 43e0; `MOV [0xa8],AX` at 43e3; `INC [0x11f0]` at 43e6; `JMP 0x1000:60ee` (=451e) at 43ea | `CALL 0x1000:8844` (=6c74) at 43bb; `CALL 0x1000:3e7d` (=22ad) at 43c6 |
| arm-2 body | 11bd:43ed..4408 | `MOV [BP-0x5a],0x8da` at 43ed; `MOV [0x120c],AL` at 43f5; `CMP [BP-0x56],0x0` at 43f8 + `JZ` at 43fc; `OR [0x120c],0x2` at 43fe; `MOV [0x10ef],0x1` at 4403; `JMP 0x1000:60ee` (=451e) at 4408 | `CALL 0x1000:8242` (=6672) at 43f2 |
| arm-3/20 body | 11bd:440b..4420 | `MOV SI,0x3` at 440b; `MOV [BP-0x5a],0x2824` at 440e; `MOV [0x10ee],0x9` at 4413; `MOV [0x10ef],0x1` at 4418; `JMP 0x1000:60ee` (=451e) at 4420 | `CALL 0x1000:85b0` (=69e0) at 441d |
| arm-5 body | 11bd:4423..4456 | `MOV [BP-0x5a],0x3a7` at 4423; `CMP [0x2f],0x3` at 4428 + `JL` at 442d; `MOV [0x36],0x80` at 442f; `JMP 0x1000:5f7a` (=43aa tail) at 4434; `CMP [0x3e],0x0` at 4437 + `JNZ` at 443c; `JMP 0x1000:5f7a` at 443e; `MOV AX,0x5e` at 4441/444b + `PUSH` at 4444/444e; `POP BX` at 444a/4452; `MOV [0xf82],AX` at 4453; `JMP 0x1000:5f7a` at 4456 | `CALLF 0x1000:0b12` at 4445; `CALL 0x1000:400e` (=243e) at 444f |
| arm-21/6 body | 11bd:4459..4461 | `MOV SI,0x6` at 4459; `MOV [BP-0x5a],0x71a` at 445c; `JMP 0x1000:60ee` (=451e) at 4461 | none |
| arm-17 head + arm-4/7/9/19 body | 11bd:4464..4499 | `MOV [0xed0],0x0` at 4464; `MOV SI,0x9` at 4469; `MOV [BP-0x5a],0x8b2` at 446c; `CMP [0x11f0],0x0` at 4471 + `JZ` at 4476; `CMP [0x2f],0x2` at 4478 + `JNZ` at 447d; `OR AX,AX` at 4482 + `JZ 0x1000:6066` (=4496 path) at 4484; `MOV SI,0x8` at 4486; `MOV [0x10ee],0xa` at 4489; `MOV [BP-0x5a],0x749` at 448e; `JMP 0x1000:60ee` (=451e) at 4493; `MOV AX,0x13` at 4496 + `JMP 0x1000:60e9` (=4519, default noreturn push) at 4499 | `CALL 0x1000:8289` (=66b9) at 447f |
| arm-18 head + arm-10 body | 11bd:449b..44a8 | `MOV [0xed0],0x0` at 449b; `MOV SI,0xa` at 44a0; `MOV [BP-0x5a],0x905` at 44a3; `JMP 0x1000:5fe8` (=4418 tail) at 44a8 | none (reaches 441d via join; counted once in arm-3/20 row) |
| arm-11/12 bodies | 11bd:44ab..44b7 | `MOV [BP-0x5a],0x29bc` at 44ab + `JMP 0x1000:60ee` at 44b0; `MOV [BP-0x5a],0x679` at 44b2 + `JMP 0x1000:60ee` at 44b7 | none |
| arm-13 body | 11bd:44b9..44c3 | `MOV [BP-0x5a],0x8ac` at 44b9; `MOV [0x10ee],0x9` at 44be; `JMP 0x1000:5fd3` (=4403 tail) at 44c3 | none (reaches 441d via join; counted once in arm-3/20 row) |
| arm-14/15/16 bodies | 11bd:44c6..44e5 | `MOV [BP-0x5a],0x3d6` at 44c6 + `JMP 0x1000:5f7a` (=43aa tail) at 44cb; `MOV [BP-0x5a],0x462` at 44ce; `CMP [0x2f],0x3` at 44d3 + `JGE 0x1000:60ad` (=44dd → 442f path) at 44d8; `JMP 0x1000:5f7a` at 44da; `JMP 0x1000:5fff` (=442f path) at 44dd; `MOV [BP-0x5a],0x4f7` at 44e0 + `JMP 0x1000:60ee` (=451e) at 44e5 | none (converge via 43aa/442f joins; calls counted once in arm-1/arm-5 rows) |
| default row head | 11bd:44e7..44f1 | `MOV DI,0x1190` at 44e7; `MOV [DI],0x0` at 44ea; `CMP [BP-0x5c],0x0` at 44ed + `JZ 0x1000:60e0` (=4510) at 44f1 | none |
| default call 1 | 11bd:44f3..44fe | `MOV AX,0xec2` at 44f3 + `PUSH` at 44f6; `MOV AX,DI` at 44f7 + `PUSH` at 44f9; `POP BX` at 44fd/44fe | `CALL 0x1000:41be` (=25ee) at 44fa |
| default call 2 | 11bd:44ff..450d | `MOV AX,0x14` at 44ff + `PUSH` at 4502; `PUSH [BP-0x5c]` at 4503; `MOV AX,0x1197` at 4506 + `PUSH` at 4509; `ADD SP,0x6` at 450d | `CALL 0x1000:7c24` (=6054) at 450a |
| default noreturn | 11bd:4510..451a | `MOV [0x11d4],0x6` at 4510; `MOV AX,0xffec` at 4516 + `PUSH` at 4519 | `CALL 0x1000:3e7d` (=22ad) at 451a |
| mode publish | 11bd:451e..4532 | `CMP [0x10ee],0xff` at 451e + `JNZ 0x1000:60fa` (=452a, taken path skips the 4527 store but keeps the 452c store) at 4523; `MOV AX,SI` at 4525; `MOV [0x10ee],AL` at 4527; `MOV AX,SI` at 452a; `MOV [0x2e],AL` at 452c; `PUSH [BP-0x5a]` at 452f; `MOV [BP+0xff14],AX` at 4532 | none |
| exit call 6250 | 11bd:4536..455c | `POP BX` at 4539; `CMP [0x46],0x0` at 453a + `JNZ 0x1000:6118` (=4548, taken path skips the `[0x46]` publish block) at 453f; `MOV AL,[BP+0xff14]` at 4541; `MOV [0x46],AL` at 4545; `MOV [0x42],0x9fa` at 4548; `MOV [0x44],0x20` at 454e; `AND [0x47],0xfe` at 4554; `MOV AL,[0x46]` at 4559 + `CBW` at 455c + `PUSH` at 455d | `CALL 0x1000:7e20` (=6250) at 4536 |
| exit call 4587 | 11bd:455e..456d | `POP BX` at 4561; `OR AX,AX` at 4562 + `JNZ 0x1000:613b` (=456b) at 4564; `OR [0x47],0x1` at 4566; `MOV AX,SI` at 456b; `MOV [0x2e],AL` at 456d | `CALL 0x1000:6157` (=4587) at 455e |
| exit arm SI==0xb | 11bd:4570..4575 | `CMP SI,0xb` at 4570 + `JNZ 0x1000:6148` (=4578, falls into the next gate) at 4573 | `CALL 0x1000:9832` (=7c62) at 4575 |
| exit arm [BP-0x56]==0 | 11bd:4578..457e | `CMP [BP-0x56],0x0` at 4578 + `JNZ 0x1000:6151` (=4581 epilogue) at 457c | `CALL 0x1000:4ca8` (=30d8) at 457e |
| epilogue (void return into 2d9c) | 11bd:4581..4586 | `POP SI` at 4581; `POP DI` at 4582; `MOV SP,BP` at 4583; `POP BP` at 4585; `RET` at 4586 — no `MOV AX,...` staging between 457e and RET | none |

NOT-COVERED: none in `11bd:4368..4586` — rows above tile the window
contiguously (join-reached calls at `441d`/`43bb`/`43c6`/`442f`/`43aa`
are counted once in their home rows, noted but not double-counted in
join rows).

### Tail callee verdicts (one-layer, verified 2026-09-29, program `/fifa96.exe`)

Scope: the 7 tail calls in the walk table resolve via
`get_function_by_address` to 7 bodies (unresolvable: none). Leaf = the
callee itself is the one allowed FUN layer (0 callees); ≥1 callee
defers the site (record, do not pursue); `22ad` guard-deferred without
diving per the whitelist rule. Already-named `mem_grow_relocate`
(called at `4445` in the arm-5 body): grow-role fits the tail context
(`MOV AX,0x5e` size pushes at `4441/444b`) — confirmed, no re-rename,
no new row. Task-1 flag confirmed: `JNZ 0x1000:60fa` at `4523`
targets `11bd:452a` (far-thunk delta holds), so the taken path skips
`MOV AX,SI` at `4525` + `MOV [0x10ee],AL` at `4527` only and keeps
`MOV AX,SI` at `452a` + `MOV [0x2e],AL` at `452c` — the asymmetric
store stands as written.

| call site | callee FUN + bounds | verdict + evidence | new_name or — |
|-----------|---------------------|--------------------|---------------|
| `CALL 0x1000:41be` at 44fa | `copy_string_aligned`, `11bd:25ee..261f`, leaf (0 callees) | CONFIRMED — `SCASB.REPNE ES:DI` strlen at 2603 + `NOT CX` at 2605; odd-head `TEST AL,0x1` at 260c + `MOVSB` at 2610; `SHR CX,0x1` at 2612 + `MOVSW.REP` at 2614 + `ADC CX,CX` at 2616 + `MOVSB.REP` at 2618 (word-aligned copy); called with `0xec2` + `DI` (`=0x1190` buffer) pushes at 44f3..44f9 | copy_string_aligned |
| `CALL 0x1000:7c24` at 450a | `copy_string_bounded`, `11bd:6054..607b`, leaf (0 callees) | CONFIRMED — `LODSB` at 6068 + `OR AL,AL` at 6069 + `JZ` (NUL stop) + `STOSB` at 606d + `LOOP` at 606e (CX bound); `XOR AL,AL` at 6070 + `STOSB.REP` at 6072 (NUL fill); returns dest `MOV AX,BX` at 6074; called with `0x14` + `[BP-0x5c]` + `0x1197` at 44ff..4509 | copy_string_bounded |
| `CALL 0x1000:7e20` at 4536 | `publish_mode_vector`, `11bd:6250..627e`, leaf (0 callees) | CONFIRMED — `MOV [0x9ba],BX` at 6255 (arg publish); gate `CMP [0x2f],0x3` at 6259 + `CMP [0x2e],0x2` at 6266 selects `BX=0x2824` override at 626d; pair loads `CS:[BX-4]`/`CS:[BX-2]` at 6270/6277 into `[0x9bc]`/`[0x9be]` at 6274/627b | publish_mode_vector |
| `CALL 0x1000:6157` at 455e | `test_mode_member`, `11bd:4587..45aa`, leaf (0 callees) | CONFIRMED — `CMP [BP+0x4]` against `0x1` at 458a / `0x5` at 4590 / `0xe` at 4596 / `0xf` at 459c; match `MOV AX,0x1` at 45a2 else `SUB AX,AX` at 45a7; returns 1 iff arg in `{1,5,0xe,0xf}` | test_mode_member |

All four renames verb-led; no `decode_*` (no byte transform observed
in any member — two are verbatim copiers, one publishes words, one
tests membership). Plates `C: none — behavioral (<role>)` set on all
four; `save_program` on `/fifa96.exe` — success.

### Guard-deferred (tail): 11bd:22ad

Called at `451a` (default noreturn). 1 FUN callee
(`copy_string_aligned`, freshly CONFIRMED above — still a second
layer past the `3ed8` body, so the layer rule fires regardless).
Whitelist: guard-deferred without diving, no rename.

### Guard-deferred (tail): 11bd:7c62

Called at `4575` (SI==0xb arm). 6 FUN callees (`016c`, `0290`,
`092c`, `199a`, `1df7`, `79fc`) — a second layer past the tail body.
No rename, no dive.

### Guard-deferred (tail): 11bd:30d8

Called at `457e` (`[BP-0x56]`==0 arm). 4 FUN callees (`304f`,
`file_open_dos`, `file_read_dos`, `file_seek_dos`) — a second layer
past the tail body (loader file-trio infra). No rename, no dive.

### 3ed8 closeout

Delimited: entry conditions (two word args, `62f8` forward, BIOS
equipment-word probe, `[0x11d4]=2`/`68c2`/`2d40`/`[0x11d4]=3`
sequence); body bounds `11bd:3ed8..4586` (667 insns, `RET` boundary
with adjacent `4587` callee); config-probe fan-out `3f66..41a1`
(4 probes CONFIRMED+renamed: `skip_blank_chars`, `find_char_in_string`,
`lookup_copy_config_string`, `parse_config_number`); SI-producer
boundary at `3f9c` (`2d40` zero-gate, `195d`-gated body constants,
`61ee`-thunk arithmetic, `4587` arg-handoff, `689e`/`6869`/`68b9`
stack re-source); 22-arm mode dispatch `42db..4368` (full arm table,
2 probes CONFIRMED+renamed, `6400` NOT-CONFIRMED, 14 arms
guard-deferred); tail walk `4368..4586` (23 rows, void return into
`2d9c`, asymmetric `452a` store confirmed); tail callee verdicts
(4 CONFIRMED+renamed above, 3 guard-deferred, `mem_grow_relocate`
confirmed without re-rename). Open: the 4 slice-7 deep dives
(`69e0→69c7`, `66b9→0733/0c0d`, `243e→65c3`, `22ad→25ee`); the
`6400` INT `DC`h contract (`CL=0x82` extender-private API,
queried source unidentified); the `[0x9b8]` table writer (source
read by `lookup_copy_config_string` at `466c/4677`, writer unknown);
the `2d40` split-label question (`2d40..2d42` zero-return adjacent
to `raise_boot_error` at `2d43` — one function split or two?);
the 2 new tail deferred (`7c62` 6 callees, `30d8` 4 callees).

## 22ad error exit (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_22ad` (body `11bd:22ad..2381` per `get_function_by_address` —
matches the slice-8 anchor, delta none) takes one word error code at
`[BP+4]`, formats a `"[NN]  "` prefix into the `BP-0x98` stack buffer
(`'['` at `22db`, decimal `IDIV`-by-10 digits via the `[BX+0xab0]`
digit table, `']'`/spaces/NUL at `231c..232a`), scans the `0x15e8`
word table bounded by `[0xf22]` for a code match (calling
`copy_string_aligned` on match), prints via the `[0xe6c]` vector
twice (conditional `0x1190`-prefixed line, unconditional second line),
and returns via plain `RET` at `2381` — no in-body trap: `HLT`,
self-`JMP` loop, and `INT` are all absent from the 89-insn body, so the
program-wide noreturn is caller convention (push a code, never resume),
not a hardware mechanism in `22ad`.

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| arg intake | 11bd:22ad..22d0 | `PUSH BP` at 22ad; `MOV BP,SP` at 22ae; `SUB SP,0x9c` at 22b0; `MOV [BP-0x2],0x0` at 22b6; `CMP [BP+0x4],0x0` at 22bb + `JGE` at 22bf; `[BP-0x2]=1` at 22c1 + `NEG [BP+0x4]` at 22c6 (abs of code); `MOV [BP+0xff66],AX` at 22cc; `LEA SI,[BP+0xff68]` at 22d0 | — |
| `[NN]` format | 11bd:22d4..232a | `CMP [0xf21],0x1` at 22d4 + `JZ` at 22d9 (skip when set); `MOV [BP+0xff68],0x5b` (`'['`) at 22db; `IDIV CX` (CX=`0xa`) at 22ed/2301/2311; digit `MOV AL,[BX+0xab0]` at 2305; `MOV [SI-0x1],0x5d` (`']'`) at 231c; two `MOV [SI-0x1],0x20` at 2321/2326; `MOV [SI],0x0` at 232a | — |
| `0x15e8`/`0xf22` scan | 11bd:232d..2357 | `MOV DI,0x15e8` at 232d; `CMP [0xf22],DI` at 2332 + `JBE` exit at 2336; `CMP AX,[BP+0x4]` at 2338 + `JNZ` skip at 233b; skip path `INC DI` + `CMP [DI-0x1],0x0` at 2346/2347 + `JNZ` at 234b; `MOV AX,[DI]` at 234d; `INC DI` x2 at 234f/2350; `MOV [BP+0xff66],AX` at 2351; `OR AX,AX` at 2355 + `JGE` at 2357 | `CALL 0x1000:41be` at 233f (=25ee) |
| 25ee match call | 11bd:233d..2344 | `PUSH DI` at 233d + `PUSH SI` at 233e (table string + formatted buffer); `POP BX` x2 at 2342/2343 | `CALL 0x1000:41be` at 233f (=25ee; far-thunk delta `0x41be-0x1BD0=0x25ee` holds) |
| print + epilogue | 11bd:2359..2381 | `CMP [BP-0x2],0x0` at 2359 + `JZ` at 235d (skips first print); `PUSH 0x1190` + `LEA AX,[BP+0xff68]` + `PUSH` at 235f..2367; `SUB AX,AX` + `PUSH` + `LEA AX,[BP+0xff68]` + `PUSH` at 236e..2375; `POP SI` at 237c; `POP DI` at 237d; `MOV SP,BP` at 237e; `POP BP` at 2380; `RET` at 2381 | `CALL [0xe6c]` at 2368; `CALL [0xe6c]` at 2376 |

Callees (`get_function_callees`, count 1): sole FUN callee
`copy_string_aligned` (`25ee`) — linkage confirmed, nothing else
pursued. The two `CALL word ptr [0xe6c]` sites (print vector) are not
FUN-resolved; recorded address-only, no dive.

Noreturn mechanism: plain `RET` at `11bd:2381` (epilogue
`237c..2381` cited above). Ruled out: `HLT` (absent from body),
infinite self-`JMP` (no backward jump to self in body), `INT`
(absent from body). Verdict: `22ad` itself returns; noreturn is a
caller-level convention, NOT an in-body trap.

### Raisers of 22ad

38 xrefs (`get_function_xrefs` = `get_xrefs_to`, all
`CALL 0x1000:3e7d` except `6025` which is `JMP 0x1000:3e7d`).
Known-list confirm: `raise_boot_error` site `2d4d` CONFIRMED;
arm-1/22 site `43c6` CONFIRMED (also reached via the `43aa` tail join
by arms 5/14/15); tail-default site `451a` CONFIRMED. Previously
noted (now confirmed as xrefs): `2d9c` site `2ea6`, `3ed8`
pre-dispatch site `4240`. All other rows are ADDs vs the known list.
Each site pushes one word code (immediate, except `14bc` dynamic and
`6025` tail-jump with no push at site).

| Raiser | Call site | Context |
|--------|-----------|---------|
| FUN_11bd_2d9c | 11bd:2ea6 | `CMP [0x2f],0x3` at 2e9b + `JGE` at 2ea0; `MOV AX,0xf` at 2ea2 + `PUSH AX` at 2ea5 — code 0xf |
| raise_boot_error (2d43) | 11bd:2d4d | `MOV [0x11d4],0x7` at 2d43; `MOV AX,0x15` at 2d49 + `PUSH AX` at 2d4c — code 0x15 |
| FUN_11bd_3ed8 (pre-dispatch) | 11bd:4240 | `MOV [0x11d4],0x4` at 4232; `OR AX,AX` at 4238 + `JGE` at 423a; `MOV AX,0x11` at 423c + `PUSH AX` at 423f — code 0x11 |
| FUN_11bd_3ed8 (arm 1/22, via 43aa join also arms 5/14/15) | 11bd:43c6 | `OR AX,AX` at 43be + `JNZ` at 43c0 (fall-through); `MOV AX,0x12` at 43c2 + `PUSH AX` at 43c5 — code 0x12 |
| FUN_11bd_3ed8 (default) | 11bd:451a | `MOV [0x11d4],0x6` at 4510; `MOV AX,0xffec` at 4516 + `PUSH AX` at 4519 — code 0xffec (-20) |
| FUN_11bd_304f | 11bd:30ca | `CMP [BP+0x6],0x0` at 30c2 + `JZ` at 30c6; `PUSH -0x2` at 30c8 — code -2 |
| FUN_11bd_3844 | 11bd:387a | `OR AX,AX` at 3874 + `JNZ` at 3876 (fall-through); `PUSH 0x9` at 3878 — code 9 |
| FUN_11bd_0c28 | 11bd:0c39 | `JZ` at 0c35; `PUSH 0x1a` at 0c37 — code 0x1a (26) |
| FUN_11bd_53e0 | 11bd:5539 | `OR AX,AX` at 5533 + `JNZ` at 5535 (fall-through); `PUSH 0x1b` at 5537 — code 0x1b (27) |
| FUN_11bd_14ac | 11bd:14bc | `PUSH SS:[BX+0x2]` at 14b8 — dynamic code from caller stack (no immediate) |
| FUN_11bd_7290 | 11bd:728d | `PUSH 0x1f` at 728b — code 0x1f (31) |
| FUN_11bd_7290 | 11bd:731f | `PUSH 0xd` at 731d — code 0xd (13) |
| FUN_11bd_3986 | 11bd:39f4 | `OR AX,AX` at 39ee + `JNZ` at 39f0 (fall-through); `PUSH 0xd` at 39f2 — code 0xd (13) |
| FUN_11bd_4b37 | 11bd:4b5f | `CMP [0xe70],0x0` at 4b56 + `JGE` at 4b5b (fall-through); `PUSH -0x8` at 4b5d — code -8 |
| FUN_11bd_6e20 | 11bd:6e11 | `PUSH 0x20` at 6e0f — code 0x20 (32) |
| FUN_11bd_6e20 | 11bd:715d | `PUSH 0x20` at 715b — code 0x20 (32) |
| FUN_11bd_7522 | 11bd:7581 | `PUSH 0x21` at 757f — code 0x21 (33) |
| FUN_11bd_76db | 11bd:78a8 | `CMP AX,0xffff` at 78a1 + `JNZ` at 78a4 (fall-through); `PUSH 0x17` at 78a6 — code 0x17 (23) |
| FUN_11bd_76db | 11bd:78bd | `OR AL,AL` at 78b5 + `JZ` at 78b7 (fall-through on nonzero); `PUSH 0x18` at 78bb — code 0x18 (24) |
| FUN_11bd_76db | 11bd:7930 | `INT 0x67` at 7925; `OR AH,AH` at 7927 + `JZ` at 7929; `PUSH 0x19` at 792e — code 0x19 (25) |
| FUN_11bd_381d | 11bd:3834 | `CMP [0xe70],0x0` at 382b + `JGE` at 3830 (fall-through); `PUSH -0x8` at 3832 — code -8 |
| FUN_11bd_32c6 | 11bd:351a | `OR AX,AX` at 3514 + `JNZ` at 3516 (fall-through); `PUSH 0x3` at 3518 — code 3 |
| FUN_11bd_32c6 | 11bd:353f | `CMP CX,[0x97c]` at 3537 + `JC` at 353b (taken); `PUSH 0x4` at 353d — code 4 |
| FUN_11bd_32c6 | 11bd:35c4 | `OR AX,[0x1202]` at 35b9 + `JNZ` at 35bd; `PUSH 0x6` at 35c2 — code 6 |
| FUN_11bd_32c6 | 11bd:37b0 | `AND AL,0x18` at 37a8 + `CMP AL,0x10` at 37aa + `JZ` at 37ac (taken); `PUSH 0x7` at 37ae — code 7 |
| FUN_11bd_1b0a | 11bd:1c2b | `OR AX,AX` at 1c25 + `JNZ` at 1c27 (fall-through); `PUSH 0x16` at 1c29 — code 0x16 (22) |
| FUN_11bd_3a89 | 11bd:3acf | `PUSH -0xe` at 3acd — code -0xe (-14) |
| FUN_11bd_5686 | 11bd:56b2 | `CMP CX,AX` at 56ac + `JNC` at 56ae (taken); `PUSH 0x28` at 56b0 — code 0x28 (40) |
| FUN_11bd_5686 | 11bd:5706 | `MOV [0x11d4],0xb` at 56f6; `OR AX,[0x98e]` at 56fe + `JNZ` at 5702 (fall-through); `PUSH 0xa` at 5704 — code 0xa (10) |
| FUN_11bd_5686 | 11bd:581b | `OR AX,AX` at 5815 + `JNZ` at 5817 (fall-through); `PUSH 0xb` at 5819 — code 0xb (11) |
| FUN_11bd_5686 | 11bd:58be | `OR AX,AX` at 58b8 + `JNZ` at 58ba (fall-through); `PUSH 0xc` at 58bc — code 0xc (12) |
| FUN_11bd_601d | 11bd:6025 | `JMP 0x1000:3e7d` (tail transfer, no immediate push at site) |
| FUN_11bd_479a | 11bd:47ac | `INT 0x21` at 47a6 + `JNC` at 47a8 (fall-through on carry); `PUSH 0x22` at 47aa — code 0x22 (34) |
| (no enclosing FUN — `get_function_by_address` finds none) | 11bd:486b | `TEST [BP+0xff6c],0x1` at 4862 + `JNZ` at 4867 (fall-through); `PUSH 0x1e` at 4869 — code 0x1e (30) |
| (no enclosing FUN — `get_function_by_address` finds none) | 11bd:48b9 | `OR AX,AX` at 48b3 + `JNZ` at 48b5 (fall-through); `PUSH 0xa` at 48b7 — code 0xa (10) |
| (no enclosing FUN — `get_function_by_address` finds none) | 11bd:48ed | `CMP SI,AX` at 48e7 + `JBE` at 48e9 (taken); `PUSH 0xe` at 48eb — code 0xe (14) |
| (no enclosing FUN — `get_function_by_address` finds none) | 11bd:4990 | `OR AX,AX` at 498a + `JNZ` at 498c (fall-through); `PUSH 0x6` at 498e — code 6 |
| FUN_11bd_46de | 11bd:473c | `CMP BX,DX` at 4730 + `JL`/`JG` + `CMP CX,AX` at 4736 + `JBE` at 4738 (taken); `PUSH 0x6` at 473a — code 6 |

No rename this task (map-only, program untouched).

### Verdict: 22ad (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on the corrected premise (caller-convention exit, not in-body
trap): argument contract — one word error code at `[BP+4]`, abs at
`22bb..22c6`; formatting/scan behavior — `[NN]` prefix `22d4..232a`,
`0x15e8`/`0xf22` scan `232d..2357`, sole FUN callee `copy_string_aligned`
(`25ee`) at site `233f`; exit/return mechanism — plain `RET` at `2381`
(`HLT`/self-`JMP`/`INT` all absent from the 89-insn body, re-verified this
task), so program-wide noreturn is caller convention. Renamed +
plate-set + `save_program` on `/fifa96.exe` — success.

| FUN_11bd_22ad | 11bd:22ad | arg `[BP+4]` abs `22bb..22c6`; `[NN]` format `22d4..232a`; `0x15e8`/`0xf22` scan + `25ee` call at `233f`; two `[0xe6c]` prints `2359..2376`; `RET` at `2381` (HLT/self-JMP/INT absent) | print_error_message | none — behavioral (format [NN] error prefix, scan 0x15e8 table, print via [0xe6c] vector; returns via RET at 2381, caller-convention exit) |

### Deferral unlock: 22ad

`print_error_message` (`11bd:22ad..2381`) is now CONFIRMED+renamed, so the
`22ad` call in each guard site below needs no dive — the callee role
(formats the `[NN]` code prefix, resolves the message via the `0x15e8`
table + `25ee`, prints through the `[0xe6c]` vector) is settled:

- arm 1 (`### Guard-deferred: arm 1 (11bd:436e)`, site `43c6`): error path
  pushes code `0x12` then calls `22ad` — no dive, callee prints the
  code-tagged message and returns.
- arm 5 (`### Guard-deferred: arm 5 (11bd:4423)`, site `43c6` via the
  `43aa` tail join): same `43c6` error exit as arm 1 — no dive.
- arm 14 (`### Guard-deferred: arm 14 (11bd:44c6)`, site `43c6` via the
  `43aa` join): same `43c6` error exit — no dive.
- arm 15 (`### Guard-deferred: arm 15 (11bd:44ce)`, same `43aa`-tail
  convergence, site `43c6`): same `43c6` error exit — no dive.
- arm 22 (`### Guard-deferred: arm 22 (11bd:436b)`, site `43c6` via
  fall-through into the arm-1 body): same `43c6` error exit — no dive.
- tail default (`### Guard-deferred (tail): 11bd:22ad`, site `451a`):
  publishes `[0x11d4]=0x6`, pushes code `0xffec` (-20), calls `22ad` —
  no dive, callee role settled (the `25ee` second layer is now a
  CONFIRMED leaf callee, not an open question).

## 30d8 file reader (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_30d8` (body `11bd:30d8..326b` per `get_function_by_address` —
matches the slice-8 anchor, delta none; 125 insns; signature
`void FUN_11bd_30d8(void)`) is a one-shot static-file reader: the latch
byte `[0xe72]` lets the body run once (repeat calls return at once), the
prologue opens the `0x1190` static path via `file_open_dos` iff the
`[0xe70]` handle cell reads negative, a `0xb0`-byte stack buffer at
`[BP-0xb2]` (`ENTER 0xbc` frame — no heap) is vetted once through `304f`,
then a loop runs seek (`file_seek_dos`) → read (`file_read_dos`,
`0xb0` bytes) over header-driven offsets while each read returns a full
`0xb0` and the buffer keeps its `'BW'` magic; post-loop maxima/flags are
published (`[0x14]`, `[0x120e]`, `[0xa16]`, `[0x1206]`, `[0xecc]`,
`[0x11d2]`, `[0xece]`, `[0x15]`) and the function returns void
(`LEAVE`/`RET`, no AX staging) — the `457e` arm receives no value back.
Callees (`get_function_callees`, count 4): exactly the known four, no
fifth callee. Xrefs: two callers — `11bd:457e` (3ed8 tail
`[BP-0x56]`==0 arm) and `11bd:7776` (inside `FUN_11bd_76db`,
address-only, no dive). CALL targets below use the uniform far-thunk
delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx` (each decoded target confirmed
by `get_function_callees`). No rename this task (map-only, program
untouched).

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| frame + one-shot latch | 11bd:30d8..30e7 | `ENTER 0xbc,0x0` at 30d8; `MOV AL,[0xe72]` at 30dc; `INC byte ptr [0xe72]` at 30df; `OR AL,AL` at 30e3; `JZ 0x1000:4cba` (=30ea, first call enters) at 30e5; `JMP 0x1000:4e3a` (=326a epilogue, repeat calls no-op) at 30e7 | — |
| open gate | 11bd:30ea..30ff | `CMP word ptr [0xe70],0x0` at 30ea + `JGE` at 30ef (skip open when handle valid); `PUSH 0x1190` at 30f1 (static filename pointer); `CMP word ptr [0xe70],0x0` at 30f8 + `JGE` at 30fd (proceed); `JMP 0x1000:4e3a` (=326a, open-fail return) at 30ff | `CALL 0x1000:7b88` at 30f4 (=5fb8 file_open_dos) |
| zero init | 11bd:3102..310f | `SUB AX,AX` at 3102; `MOV word ptr [BP-0xba],[BP-2],[BP-0xb8],[BP-0xb6],AX` at 3104/3108/310b/310f | — |
| 304f vet | 11bd:3113..3126 | `PUSH 0xb0` at 3113; `PUSH AX` (=0) at 3116; `LEA AX,[BP-0xb2]` at 3117 + `PUSH AX` at 311b (stack buffer pointer); `ADD SP,0x6` at 311f; `OR AX,AX` at 3122 + `JNZ 0x1000:4cf9` (=3129, enter loop) at 3124; `JMP 0x1000:4d83` (=31b3, skip loop) at 3126 | `CALL 0x1000:4c1f` at 311c (=304f) |
| BW check + maxima | 11bd:3129..3183 | `CMP byte ptr [BP-0xb2],0x42` ('B') at 3129 + `JNZ` (loop exit) at 312e; `CMP byte ptr [BP-0xb1],0x57` ('W') at 3130 + `JZ` (continue) at 3135; `OR [0x14],AX` at 313d; three max-pairs (`[BP-0xb8]`/`[BP-0xba]`/`[BP-2]`/`[BP-0xbc]` vs header words) at 3141..3183 | — |
| seek | 11bd:3187..319d | `MOV AX,[BP-0x94]` + `OR AX,[BP-0x96]` at 3187/318b + `JZ` (loop exit) at 318f; `PUSH word ptr [BP-0x94]` at 3191; `PUSH word ptr [BP-0x96]` at 3195; `POP BX` x2 at 319c/319d | `CALL 0x1000:7b9a` at 3199 (=5fca file_seek_dos) |
| read + loop-back | 11bd:319e..31b0 | `PUSH 0xb0` at 319e (length 176); `LEA AX,[BP-0xb2]` at 31a1 + `PUSH AX` at 31a5 (same stack buffer); `POP BX` x2 at 31a9/31aa; `CMP AX,0xb0` at 31ab + `JNZ` (short-read exit) at 31ae; `JMP 0x1000:4cf9` (=3129, loop-back) at 31b0 | `CALL 0x1000:7bb2` at 31a6 (=5fe2 file_read_dos) |
| post-loop publishes | 11bd:31b3..3265 | `MOV [0x120e],AX` at 31c4; `MOV [BP-0xb6],AX` at 31cb; `MOV byte ptr [0xa16],0x2` at 31d3; `MOV [0x1206],AX` at 31e1/31fc; `MOV [0xecc],AX` at 3219/324a (`SUB AX,[0xeca]` at 3246); `OR [0x11d2],0x1` at 321c; `MOV [0xece],0xfffe` at 3236; guards `CMP [0xecc],0x3c00` / `CMP [0xeca],0x4000` / `CMP [0xece],0x4000` at 324d/3255/325d; `OR [0x15],0x20` at 3265 | — |
| return (void) | 11bd:326a..326b | `LEAVE` at 326a; `RET` at 326b — no `MOV AX,...` staging after 3246; decompile `void FUN_11bd_30d8(void)` ends bare `return;` | — |

Trio order: open (`30f4`, once, prologue-gated on `[0xe70]<0` with
filename word `0x1190`) → per-iteration seek (`3199`, offset words
`[BP-0x94]`/`[BP-0x96]`) → read (`31a6`, length `0xb0` into
`[BP-0xb2]`, byte count back in AX); the loop repeats while AX==`0xb0`
and exits on short read, non-`'BW'` magic, or zero offset. The `0x1190`
static's target bytes are NOT verified here (DS base unknown — same
OPEN as the dispatch opens in Script semantics); the pushed pointer
word itself is the cited fact.

`304f` surface: body `11bd:304f..30d7` (adjacent, confirms the `30d8`
boundary), returns `undefined2`; single call site `311c` with pushed
args (`0xb0`, `0`, `[BP-0xb2]` buffer pointer); nonzero AX enters the
read loop, zero skips to `31b3`. No dive (brief scope).

Return contract: VOID — no value is handed back to the `457e` arm.
Ruled out: pointer return (no pointer staged into AX before `RET`),
count/status return (AX after `3246` is compare scratch for the
`[0xecc]` guards, never staged; signature is `void`). Effects are
globals-only (publish row above).

### Verdict: 30d8 (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on all four legs (evidence rows above, not rewritten):
intake source — static filename word `0x1190` pushed at `30f1`,
prologue-gated on `[0xe70]<0` (`30ea..30ef`), opened once via
`file_open_dos` at `30f4`, one-shot latch `[0xe72]` at `30dc..30e7`;
trio order — open (`30f4`) → seek (`3199`, offsets
`[BP-0x94]`/`[BP-0x96]`) → read (`31a6`, `0xb0` into `[BP-0xb2]`,
loop-back at `31b0` while AX==`0xb0`); publish target — post-loop
maxima/flags `[0x14]`, `[0x120e]`, `[0xa16]`, `[0x1206]`, `[0xecc]`,
`[0x11d2]`, `[0xece]`, `[0x15]` (`31b3..3265`); return contract —
VOID (`LEAVE`/`RET` at `326a/326b`, no AX staging, `void` signature;
pointer/count/status ruled out). Renamed + plate-set +
`save_program` on `/fifa96.exe` — success.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_30d8 | 11bd:30d8 | intake `0x1190` push at 30f1 + `[0xe70]<0` gate at 30ea + latch `[0xe72]` at 30dc; trio open 30f4 → seek 3199 → read 31a6 (`0xb0` into `[BP-0xb2]`); publishes `[0x14]/[0x120e]/[0xa16]/[0x1206]/[0xecc]/[0x11d2]/[0xece]/[0x15]` at 31b3..3265; `LEAVE`/`RET` at 326a/326b, void signature | read_static_bw_file | none — behavioral (one-shot static-file reader) |

### Verdict: 304f (CONFIRMED 2026-09-29, program `/fifa96.exe`)

One-layer role (body `11bd:304f..30d7` per `get_function_by_address`,
53 insns, signature `undefined2`; disassembly + decompile read this
task, no deeper dive): header vetting probe over the `30d8` stack
buffer. Call-site args at `311c` are (`[BP-0xb2]` buffer at `[BP+4]`,
`0` at `[BP+6]`, `0xb0` length at `[BP+8]`). Each pass seeks to
`[0x11da]`/`[0x11dc]` via `file_seek_dos` (`CALL 0x1000:7b9a` at
`3060`) and reads `[BP+8]` bytes via `file_read_dos`
(`CALL 0x1000:7bb2` at `306b`), exiting on short read (`CMP AX,[BP+8]`
at `3070` + `JNZ` at `3073`); `'MF'` (`CMP 0x4d/0x46` at `3078/307d`)
accumulates words at `BX+2`/`BX+4` into `[0x11da]`/`[0x11dc]`
(`ADD`/`ADC` at `3089/308d`); `'BW'` (`CMP 0x42/0x57` at `30a5/30aa`)
returns 1 (`MOV AX,0x1` at `30b0`); any other magic issues disk-reset
(`MOV AH,0xd` + `INT 0x21` at `30b5/30b7`) with a retry countdown
(`[BP-2]` from 2 at `3053`, `DEC` at `30b9`, `JGE` loop at `30c0`);
exhaustion with `[BP+6]!=0` raises via `print_error_message`
(`PUSH -0x2` at `30c8`, `CALL 0x1000:3e7d` at `30ca`), then zeroes
`[0x11da]`/`[0x11dc]` (`30d0/30d3`) and returns 0. Renamed +
plate-set + `save_program` on `/fifa96.exe` — success.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_304f | 11bd:304f | seek 3060 + read 306b over `[BP+4]` buffer / `[BP+8]` length; MF accumulate at 3089/308d; BW→1 at 30b0; disk-reset retry 30b5..30c0; fail raise at 30ca then zero `[0x11da]/[0x11dc]`, return 0 | vet_file_header | none — behavioral (header vetting probe) |

### Guard-deferred: 304f second layer (11bd:300b)

`FUN_11bd_300b` (body `11bd:300b..304e` per `get_function_by_address`,
called at `309f` on the `'MZ'` path, `CALL 0x1000:4bdb`) is a second
layer past the `vet_file_header` body — recorded address-only, no
dive, no rename. The other second-layer call (`print_error_message`
at `30ca`) needs no dive: CONFIRMED+renamed (22ad verdict section).

File-trio confirm (no re-rename, call context matches slice-1 roles):
`file_open_dos` (`5fb8`, at `30f4`), `file_seek_dos` (`5fca`, at
`3199` + `3060`), `file_read_dos` (`5fe2`, at `31a6` + `306b`).

### Deferral unlock: 457e

`read_static_bw_file` (`11bd:30d8..326b`) is now CONFIRMED+renamed, so
the `30d8` call in the 3ed8 tail arm (`### Guard-deferred (tail):
11bd:30d8`, site `457e`, gated on `[BP-0x56]==0` at `4578/457c`) needs
no dive — the callee role (one-shot `0x1190` static-file read: open
once, vet through `vet_file_header`, seek/read `'BW'` loop, publish
maxima/flags to globals) is settled. What the arm receives back on
return: nothing — `30d8` returns void (`LEAVE`/`RET` at `326a/326b`,
no AX staging), effects are globals-only, and the arm falls straight
into the `4581..4586` epilogue.

## 30d8 second caller (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_76db` (body `11bd:76db..79f5` per `get_function_by_address`,
272 insns; no prior bounds recorded, delta none) is a hardware/memory
setup routine: EMS/XMS presence probes (`INT 0x21 AX=0x3567` at `76e0`,
`INT 0x67` `0xde00`/`0xde0a`/`0xde01` sites), a retry loop that must
fall through with `AH==0` before any setup runs, a mode publish
(`MOV [0x2e],0xb` at `7771`), the one-shot `30d8` static-file read at
`7776` (zero stack args — identical intake to the `457e` first call,
which likewise pushes nothing), then `3844` at `7779`, memory sizing
via `mem_grow_relocate` (`CALLF 0x1000:0b12` at `7898`), zero-fill and
error exits via `print_error_message` (codes `0x17`/`0x18`/`0x19` at
`78a8`/`78bd`/`7930`). The gate guarding `7776` is the EMS
retry-loop fall-through (`JZ`/`JNZ` loop-backs to `7740` at
`7754`/`775e`/`7767`), NOT the `[0xe72]` latch — the latch gates
inside `30d8` itself (`30dc..30e7`). Post-call the caller consumes
`30d8`'s publishes directly: `[0xa16]` at `779b`, `[0x15]` at `779f`,
`[0xecc]` at `7840`, `[0xece]` at `785a`, `[0x11d2]` at `78dd`.
Far-thunk delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx` holds for every
call/jump target below (re-confirmed per site against the 13-callee
list; `7776`: `0x4ca8−0x1BD0=0x30d8`). No rename this task (map-only,
program untouched).

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| probe head | 11bd:76db..773e | `PUSH SI`/`PUSH DI` at 76db/76dc; `INT 0x21 AX=0x3567` at 76e0/76e3; ES vs `[0x58]` at 76e7 + `JZ` at 76eb; EMM magic `CMP ES:[0xb],0x4d4d` at 76ed + `JNZ` at 76f4, `CMP ES:[0xe],0x5858` at 76f6 + `JNZ` at 76fd, `'E'`/`'X'`/`'0'` compares at 76ff/7707/770f + `JNZ` at 7705/770d/7715; `TEST [0x47],0x80` at 7717 + `JNZ` at 771c; `TEST [0x14],0x2` at 771e + `JNZ` at 7723; `INT 0x67 AX=0xde00` at 7725/7728 + `OR AH,AH` at 772a + `JZ` at 772c; `INT 0x67 AH=0x43,BX=1` at 772e..7733 + `OR AH,AH` at 7735 + `JNZ` at 7737 | `CALL 0x1000:855b` at 76dd (=698b); `CALL 0x1000:82a4` at 7740 (=66d4); `CALL 0x1000:46ab` at 7744 (=2adb) |
| retry-loop gate | 11bd:7751..7767 | `JZ 0x1000:9310` (=7740 loop-back) at 7754 over the `7751` probe result; `CALL 0x1000:927b` (=76ab) at 7759 + `OR AH,AH` at 775c + `JNZ 0x1000:9310` (=7740) at 775e; `MOV AX,0xde0a` + `INT 0x67` at 7760/7763 + `OR AH,AH` at 7765 + `JNZ 0x1000:9310` (=7740) at 7767 — every failure returns to `7740`, fall-through reaches `7769` | `CALL 0x1000:82a4` at 7751 (=66d4); `CALL 0x1000:927b` at 7759 (=76ab) |
| pre-call publishes | 11bd:7769..7771 | `MOV [0x50],BL` at 7769; `MOV [0x51],CL` at 776d; `MOV [0x2e],0xb` at 7771 — no `PUSH` between `7771` and the call | — |
| 30d8 call | 11bd:7776 | `CALL 0x1000:4ca8` (=30d8 `read_static_bw_file`); decompile emits `read_static_bw_file();` with no assigned return | `CALL 0x1000:4ca8` at 7776 (=30d8) |
| post-call sequence | 11bd:7779..77a8 | `CALL 0x1000:5414` (=3844) at 7779; `CMP [0xdec],0x602` at 777c + `JC` at 7782; `[0xdee]=4`/`[0xdf0]=0xfc00`/`[0xdf2]=0x801` at 7784/778a/7790; flag merge `MOV AL,[0x47]` + `AND 0x80` at 7796/7799, `OR AL,[0xa16]` at 779b, `MOV AH,[0x15]` + `AND 0x20` at 779f/77a3, `OR AL,AH` at 77a6, store `CS:[0x76aa]` at 77a8 | `CALL 0x1000:5414` at 7779 (=3844) |
| downstream uses of 30d8 publishes | 11bd:77dd..785a | `CMP [0xa16],0x0` at 77dd + `JNZ` at 77e3; `OR [0xa16],0x1` at 78d8; `TEST [0x11d2],0x1` at 78dd + `JNZ` at 78e2; `MOV [0xecc],0xef00` at 78e4; `CMP [0xecc],AX` at 7840 + `JC` at 7844; `MOV [0xece],DX` at 785a | `CALLF 0x1000:0b12` at 7898; `CALL 0x1000:8193` at 789d (=65c3); `CALL 0x1000:3e7d` at 78a8/78bd/7930 (=22ad) |
| epilogue | 11bd:79bb..79cc | `MOV AX,0x1` at 79bb; `MOV [0xaa4],0x3d15` at 79be; `MOV [0xaa6],0x3d68` at 79c4; `POP DI`/`POP SI` at 79ca/79cb; `RET` at 79cc (plus the `79cd..79f5` join tail re-sourcing `[0xeca]`/`[0xece]` and `JMP 0x1000:9583` at 79f4) | `CALL 0x1000:9786` at 7990 (=7bb6) |

Argument compare (`457e` vs `7776`): identical intake — both sites
push zero stack words (`457e` preceded only by `CMP [BP-0x56],0x0` at
`4578` + `JNZ` at `457c` per the 3ed8 tail table; `7776` preceded only
by `MOV [0x2e],0xb` at `7771`). No slot differences exist: `30d8`
takes no filename/buffer/length parameters (void signature); its
`0x1190`/`0xb0`/`[BP-0xb2]` intake is internal. Gates differ: `457e`
on `[BP-0x56]==0`, `7776` on the EMS retry-loop `AH==0`
fall-through — neither gate is the `[0xe72]` latch.

### Latch [0xe72] in 76db

| Latch op | Address | Evidence |
|----------|---------|----------|
| read/write inside 76db | NOT-FOUND | Searched `11bd:76db..79f5` (`disassemble_function`, 272 insns — no `e72` operand) + program-wide operand search `e72` (5 matches, none in `76db..79f5`): only `0e45`/`0e98`/`30dc`/`30df`/`1991:155e` |
| setter (address-only deferral) | 11bd:0e45 + 11bd:0e98 | `XCHG byte ptr CS:[0xe72],AL` at 0e45; `MOV CS:[0xe72],AL` at 0e98 — unenclosed bytes below `FUN_11bd_0ef4` (`get_function_by_address` finds no FUN at `0e30`/`0e45`/`0e98`; nearest FUN entry `0ef4`, body `0ef4..0ef6`), amid an `INT 0x0` vector-install sequence (`INT 0x0` at `0e71`, SS/SP save/restore at `0e58..0e8e`); not among `76db`'s 13 callees (`0ef4`, `2adb`, `3844`, `65c3`, `66d4`, `698b`, `76ab`, `7b34`, `7b50`, `7bb6`, `0b12`, `22ad`, `30d8` — none contains an `e72` operand per the program-wide search), so no one-layer resolve; deferred with boundary named |
| xref index check | 11bd:0e72 | `get_xrefs_to` on `11bd:0e72`: 0 refs — the `CS:`-relative stores at `0e45`/`0e98` are not indexed as xrefs to the data address; claim sourced to disassembly lines instead |
| false positive | 1991:155e | `JZ 0x1000:ae72` — far-jump code target (`segment:offset`), not a data ref to `[0xe72]`; excluded |

### Verdict: 76db CONFIRMED

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_76db | 11bd:76db | Role: hardware/memory setup (EMS/XMS probes at 76e0/7725/7760, mode publish `[0x2e]=0xb` at 7771, `30d8` at 7776, `3844` at 7779, sizing via `0b12` at 7898, exits 0x17/0x18/0x19); gate: EMS retry-loop fall-through (`JZ`/`JNZ` to 7740 at 7754/775e/7767); args: zero stack words, identical to `457e`; post-call: consumes `[0xa16]` at 779b, `[0x15]` at 779f, `[0xecc]` at 7840, `[0xece]` at 785a, `[0x11d2]` at 78dd | setup_memory_hardware | none — behavioral (hardware/memory setup) |

Renamed in program `/fifa96.exe` with plate `C: none —
behavioral (hardware/memory setup)`; `save_program` confirmed.
All four CONFIRM legs (role sketch, gate, argument compare,
post-call use) stand on the walk rows above — no leg missing.

### Latch note: [0xe72] shared one-shot

Polarity (cited, `disassemble_function` on `11bd:30d8`):
`MOV AL,[0xe72]` at `30dc` + `OR AL,AL` at `30e3` (TEST) +
`JZ 0x1000:4cba` at `30e5` (first arrival, AL==0, falls into the
body) vs `JMP 0x1000:4e3a` at `30e7` (repeat arrival, latch
already incremented by `INC byte ptr [0xe72]` at `30df`, skips
the body). Whoever reaches `30d8` first does the real `0x1190`
static-file read; the other call is a no-op skip.

Caller order: NOT-DETERMINED as a single static order. Both
`30d8` call sites hang off one function: `FUN_11bd_3ed8` calls
`30d8` directly at `457e` (`get_function_xrefs` on `11bd:30d8`:
`457e` + `7776`) and calls `76db` at three dispatch sites
`41cf`/`420e`/`421d` (`get_function_xrefs` on `11bd:76db`, all
from `FUN_11bd_3ed8`; `76db`'s sole caller is `3ed8` per
`get_function_callers`), each address-upstream of `457e` — but
the dispatch is conditional (`CMP SI,0xb` + `JZ` at `41c7/41ca`
with a `JMP`-over at `41cc`; `JNZ`-over at `420c`; `JMP`-past
at `421a`), so paths exist that reach the `457e` tail arm
without firing any `76db` arm. On paths where a dispatch arm
fires, `7776` runs first and does the read while `457e` skips;
on paths where all three arms are bypassed, `457e` does the
read. No single first-vs-second order holds for all paths.

Setter status: deferred-with-boundary (unchanged from the latch
table above: `0e45`/`0e98` unenclosed bytes below `FUN_11bd_0ef4`,
no FUN, not among `76db`'s callees).

## 7c62 exit arm (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_7c62` (body `11bd:7c62..7d3d` per `get_function_by_address`
— matches the slice-8 anchor, delta none; signature
`uint FUN_11bd_7c62(undefined2 param_1)`, 79 insns) takes zero stack
words (no prologue, no PUSH before either gated call; caller `4575`
likewise push-free per the 3ed8 tail rows) with `param_1`
register-passed (fastcall) and forwarded to `199a`. Gate
`[0xe00]!=0` (`CMP` at `7c62` + `JZ` at `7c67`) admits the pair
`199a` (site `7c69`) → `016c` (site `7c6c`); `[0xdff]` then splits
EMS-setup (`!=0`: CX derived from `[0xece]`, tail `JMP 79fc` at
`7c89` iff `[0x47]` bit `0x80`, else out-of-body `JZ` to the `7c2b`
EMS block) from the `[0xdfa]`/`[0xadc]`-gated main block (`CALLF
[0xaec]` loop, `0290(0)` at `7ced`, `092c` verify at `7d15`,
fail-path `092c` at `7d1e`, zero-fill, `1df7` at `7d39`).
Publishes: `[0xeca]+=CX` at `7d25` and an `ES:DI` zero-fill
(`ES=[0xaa]+0x200` segment, `STOSW.REP` at `7d37`). Return: a word
in AX on every path (`092c`-AX on success, `1df7`-AX on the fail
path, `79fc`-AX via the tail jump, entry-AX via the `7c61` RET for
the latch/guard exits) — and the `4575` arm ignores it (next insn
after the call is `CMP [BP-0x56],0x0` at `4578`, no AX read).
Overlap: `199a`/`016c` take identical zero-stack-word intake here
(sites `7c69`/`7c6c`) and in `run_postload_init` (sites `62a0`/
`62b7`) — shared-helper evidence; gates differ (`[0xe00]` here vs
`[0x2e]` there). Callees (`get_function_callees`, count 6): exactly
the known six, no seventh. No rename this task (map-only, program
untouched).

Far-thunk delta `0x1000:xxxx − 0x1BD0 = 11bd:xxxx` holds for all six
call/jump targets (`356a→199a`, `1d3c→016c`, `1e60→0290`,
`24fc→092c`, `39c7→1df7`, `95cc→79fc`) and the caller
(`9832→7c62` at `4575`).

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| arg intake | 11bd:7c62 | no prologue (first insn `CMP byte ptr [0xe00],0x0` at 7c62); no PUSH before 7c69/7c6c; decompile `uint __fastcall FUN_11bd_7c62(undefined2 param_1)` forwards `param_1` to 199a | — |
| latch gate | 11bd:7c62..7c67 | `CMP byte ptr [0xe00],0x0` at 7c62 + `JZ 0x1000:9831` (=7c61 RET, unenclosed, see out-of-body row) at 7c67 | — |
| call 199a | 11bd:7c69 | `CALL 0x1000:356a` (=199a); arg `param_1` register-passed (decompile `FUN_11bd_199a(param_1)`; zero PUSH at site) | `CALL 0x1000:356a` at 7c69 (=199a) |
| call 016c | 11bd:7c6c | `CALL 0x1000:1d3c` (=016c); no args (decompile `FUN_11bd_016c()`; zero PUSH at site) | `CALL 0x1000:1d3c` at 7c6c (=016c) |
| dff split | 11bd:7c6f..7c8c | `CMP byte ptr [0xdff],0x0` at 7c6f + `JZ 0x1000:985c` (=7c8c main block) at 7c74; `!=0` path: `MOV CX,[0xece]` at 7c76 + `SUB CX,0x1000` at 7c7a + `SHR CX,0xc` at 7c7e + `INC CX` at 7c81; `TEST [0x47],0x80` at 7c82 | — |
| tail jump 79fc | 11bd:7c87..7c89 | `JZ 0x1000:97fb` (=7c2b EMS block, unenclosed) at 7c87; `JMP 0x1000:95cc` (=79fc, tail transfer — no CALL insn; callee attribution via `get_function_callees` + decompile `FUN_11bd_79fc()` with no args, result returned) at 7c89 | `JMP 0x1000:95cc` at 7c89 (=79fc) |
| main-block guards | 11bd:7c8c..7c97 | `MOV CX,[0xdfa]` at 7c8c + `JCXZ 0x1000:9831` (=7c61 RET) at 7c90; `CMP [0xadc],0x0` at 7c92 + `JZ 0x1000:9831` at 7c97 | — |
| CALLF block | 11bd:7c99..7ceb | `PUSH SI`/`PUSH DI` at 7c99/7c9a; EAX=`[0xeca]<<0xa` at 7c9b/7ca1; EDX=`[0x98]<<4` adjust at 7ca5..7cb4; stack struct pushes at 7cb7..7ccd (`PUSH EAX`, `OR AX,0xeee`, `PUSH 0x0/DX/[0xadc]/SS/BX/0x0/0x0/0x4`); `SHR CX,0x2` at 7cd7 + `MOV AH,0xb` at 7cda; `OR AX,AX` at 7ce0 + `JZ 0x1000:98f1` (=7d21 fail path) at 7ce2; `ADD [BX]/[SI+0xc],EDI(=0x1000)` at 7ce4/7ce7 + `LOOP` at 7ceb | `CALLF [0xaec]` at 7cdc (indirect, not FUN-resolved — address-only, no dive) |
| call 0290 | 11bd:7ced | `CALL 0x1000:1e60` (=0290); arg 0 register-passed (decompile `FUN_11bd_0290(0)`; zero PUSH at site) | `CALL 0x1000:1e60` at 7ced (=0290) |
| call 092c (verify) | 11bd:7cf0..7d15 | `PUSH 0x38`/`POP ES` at 7cf0/7cf2; EBX=`[BX+0x14]` at 7cf5; EAX=EBX&#124;0xeee at 7cf9/7cfc; `CX=[0xdfa]>>2` at 7cff/7d03; `CMP EAX,ES:[EBX]` at 7d06 + `JNZ 0x1000:98ee` (=7d1e fail path) at 7d0b; `ADD EBX/EAX,EDI` at 7d0d/7d10 + `LOOP` at 7d13 | `CALL 0x1000:24fc` at 7d15 (=092c; success — AX returned at 7d1d) |
| fail path: 092c + zero-fill + 1df7 | 11bd:7d1e..7d39 | `CALL 0x1000:24fc` at 7d1e (=092c); `MOV CX,[0xdfa]` at 7d21; `ADD [0xeca],CX` at 7d25 (publish); `MOV AX,[0xaa]` at 7d29 + `ADD AH,0x2` at 7d2c + `MOV ES,AX` at 7d2f; `XOR AX,AX`/`XOR DI,DI` at 7d31/7d33 + `SHR CX,0x1` at 7d35 + `STOSW.REP ES:DI` at 7d37 (zero-fill); `CALL 0x1000:39c7` at 7d39 (=1df7); `JMP 0x1000:98e8` (=7d18 epilogue join) at 7d3c | `CALL 0x1000:24fc` at 7d1e (=092c); `CALL 0x1000:39c7` at 7d39 (=1df7; AX returned at 7d1d) |
| epilogue (in-body return) | 11bd:7d18..7d1d | `ADD SP,0x18` at 7d18; `POP DI`/`POP SI` at 7d1b/7d1c; `RET` at 7d1d — AX holds 092c-AX (via 7d15) or 1df7-AX (via 7d3c join); signature `uint` | — |
| out-of-body edges (NOT 7c62 publishes) | 11bd:7c2b..7c61 | `JZ/JCXZ` targets `0x1000:9831` = `11bd:7c61` (`RET` — bare, entry-AX); `JZ` at 7c87 targets `11bd:7c2b` (unenclosed bytes, `get_function_by_address` finds no FUN): `MOV CS:[0x79f6],CX` at 7c2b, `INT 0x67 AX=0xde04` at 7c48/7c4b, `JNZ 0x1000:9476` (=11bd:78a6, `76db` 0x17-error neighborhood) at 7c4f, `LOOP 0x1000:9818` (=7c48) at 7c5f — shared EMS block, recorded address-only, no dive | — |

Call sequence (address order): `199a` at `7c69` (`param_1` via
register) → `016c` at `7c6c` (no args) → `79fc` via tail `JMP` at
`7c89` (no args, EMS-setup path only) → `0290(0)` at `7ced` →
`092c` at `7d15` (verify; success returns its AX) → `092c` at
`7d1e` (fail path) → `1df7` at `7d39` (fail-path AX returned).
`CALLF [0xaec]` at `7cdc` is indirect (not FUN-resolved).

Overlap (`016c`/`199a` here vs `run_postload_init` at `11bd:627f`,
disassembly + decompile read this task, no dive into either callee,
`run_postload_init` row untouched): `199a` — here site `7c69`
gated on `[0xe00]!=0` (`7c62`/`7c67`), arg `param_1` register-passed
(zero PUSH); there site `62a0` (`CALL 0x1000:356a`) gated on
`[0x2e]!=0` (`CMP byte ptr [0x2e],0x0` at `6299` + `JZ` at `629e`),
arg register-passed (decompile `FUN_11bd_199a(extraout_CX)`, zero
PUSH). `016c` — here site `7c6c` fall-through after `199a`, no args
(zero PUSH); there site `62b7` (`CALL 0x1000:1d3c`) inline sequence,
no args (decompile `FUN_11bd_016c()`, zero PUSH). Verdict:
same-role (shared-helper) evidence on intake — identical
zero-stack-word call shape at all four sites; gate contexts differ
(`[0xe00]` here vs `[0x2e]` there), cited, not elided.

Return contract: `7c62` hands a word in AX back to the `4575` arm
(`092c`-AX / `1df7`-AX / `79fc`-AX / entry-AX per path above), which
the arm ignores — `CMP [BP-0x56],0x0` at `4578` reads no AX (3ed8
tail rows). Ruled out: VOID (signature is `uint`, AX staged on the
main paths), caller-gated status (no `OR AX,AX`/`JZ`/`MOV` of AX
between `4575` and `457e`).

### Verdict: 7c62 (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on all four legs (evidence rows above, not rewritten):
intake — zero stack words, `param_1` register-passed (fastcall),
forwarded to `199a` (arg-intake row + `7c69` site); call order —
`199a` at `7c69` → `016c` at `7c6c` → `79fc` via tail `JMP` at
`7c89` (EMS-setup path only) → `0290(0)` at `7ced` → `092c` at
`7d15` (verify) → `092c` at `7d1e` (fail path) → `1df7` at `7d39`
(call-sequence statement); publish — `[0xeca]+=CX` at `7d25` plus
the `ES:DI` zero-fill (`ES=[0xaa]+0x200`, `STOSW.REP` at `7d37`);
return contract — word in AX on every path (`092c`-AX / `1df7`-AX /
`79fc`-AX / entry-AX), ignored by the `4575` arm (VOID and
caller-gated status ruled out). Renamed + plate-set +
`save_program` on `/fifa96.exe` — success.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_7c62 | 11bd:7c62 | intake zero stack words + `param_1` forwarded to 199a at 7c69; six-call order 7c69/7c6c/7c89/7ced/7d15/7d1e/7d39; publishes `[0xeca]+=CX` at 7d25 + `ES:DI` zero-fill at 7d37; AX word return per path, arm-ignored at 4578 | execute_exit_arm | none — behavioral (SI==0xb exit arm) |

### Verdict: 092c (CONFIRMED 2026-09-29, program `/fifa96.exe`)

One-layer role (body `11bd:092c..0930` per `get_function_by_address`,
2 insns, signature `void`, 0 FUN callees): indirect tail-transfer
stub through the `[0x9bc]` mode-vector cell — `NOP` at `092c` +
`JMP word ptr [0x9bc]` at `092d`, no `RET` in-body — the `CALL`s at
`7d15`/`7d1e` push `7d18`/`7d21` and resume there only via the vector
target's own `RET` (the target owns the continuation). Called at `7d15`
(verify; success returns its AX via the `7d1d` epilogue) and `7d1e`
(fail path). Renamed + plate-set + `save_program` on `/fifa96.exe`
— success.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_092c | 11bd:092c | `NOP` at 092c + `JMP word ptr [0x9bc]` at 092d, no RET; 0 FUN callees; called at 7d15/7d1e | dispatch_mode_vector | none — behavioral (indirect tail transfer through [0x9bc]) |

### Verdict: 1df7 (CONFIRMED 2026-09-29, program `/fifa96.exe`)

One-layer role (body `11bd:1df7..1e1c` per `get_function_by_address`,
16 insns, signature `void`, 0 FUN callees — the two `CALLF [0xaec]`
sites are indirect, address-only, no dive): per-slot teardown loop
over `[BX+0xadc]` words — `MOV BX,0x10` at `1df7`, `DEC BX` x2 per
pass (`1dfa/1dfb`), `JS` exit at `1dfc`; nonzero slot
(`MOV DX,[BX+0xadc]` at `1dfe` + `OR DX,DX` at `1e02` + `JZ` skip
at `1e04`) issues `CALLF [0xaec]` with `AH=0xd` at `1e09` then
`AH=0xa` at `1e0f`, then zeroes the slot
(`MOV word ptr [BX+0xadc],0x0` at `1e14`); `RET` at `1e1c`.
Called at `7d39` (fail path; its AX returned at `7d1d` — per the
`7c62` path-table fail-path row and the epilogue row's "via `7d3c`
join").
Renamed + plate-set + `save_program` on `/fifa96.exe` — success.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_1df7 | 11bd:1df7 | BX 0x10→0 step -2 over `[BX+0xadc]` (1df7..1dfc); nonzero slot → `CALLF [0xaec]` AH=0xd/0xa at 1e09/1e0f + zero at 1e14; `RET` at 1e1c; 0 FUN callees; called at 7d39 | clear_slot_entries | none — behavioral (per-slot teardown loop) |

NOT-CONFIRMED (no rename, no row): `FUN_11bd_0290`
(body `11bd:0290..0292`, 1 insn, 0 FUN callees): `MOV DX,0x20`
at `0290` with no `RET`/`JMP` in the delimited body — the role of
`DX=0x20` and the exit mechanism are both missing from disassembly
(the body falls through into the adjacent `FUN_11bd_0293` at
`0293..02b4`; the `0290(0)` decompile surface reads past the
delimited body, flagged per Task 1 — disassembly wins, so no
one-layer role is stated).

### Guard-deferred (7c62 arm): 11bd:199a

Body `11bd:199a..1a85` (82 insns). 1 FUN callee (`FUN_11bd_27a4`,
`11bd:27a4..27ae`) — a second layer past the `7c62` body. No dive,
no rename.

### Guard-deferred (7c62 arm): 11bd:016c

Body `11bd:016c..0245`. 1 FUN callee (`FUN_11bd_2081`,
`11bd:2081..2088`) — a second layer past the `7c62` body. No dive,
no rename.

### Guard-deferred (7c62 arm): 11bd:79fc

Body `11bd:79fc..7a87`. 3 FUN callees (`FUN_11bd_0290` at
`11bd:0290..0292`, `dispatch_mode_vector` at `11bd:092c..0930`,
`FUN_11bd_0ef4` at `11bd:0ef4..0ef6`) — second layers past the
`7c62` body (the `092c` layer is now a CONFIRMED leaf callee, still
a second layer, so the layer rule fires regardless — same precedent
as `22ad→25ee`). No dive, no rename.

### Deferral unlock: 4575

`execute_exit_arm` (`11bd:7c62..7d3d`) is now CONFIRMED+renamed, so
the `7c62` call in the 3ed8 tail arm (`### Guard-deferred (tail):
11bd:7c62`, site `4575`, `CALL 0x1000:9832` gated on SI==0xb at
`4570/4573`) needs no dive — the callee role (gated `199a`/`016c`
pair, EMS/main split, `CALLF` block + verify, `[0xeca]` publish +
zero-fill, word return) is settled. What the arm receives back on
return: a word in AX (`092c`-AX / `1df7`-AX / `79fc`-AX /
entry-AX per path), which the arm ignores — the next insn
`CMP [BP-0x56],0x0` at `4578` reads no AX before the `457e` gate.

### Overlap note: 016c/199a

Shared-helper verdict (no change to the `run_postload_init` row):
`199a`/`016c` take identical zero-stack-word intake here (sites
`7c69`/`7c6c`, gated on `[0xe00]` at `7c62/7c67`) and in
`run_postload_init` (sites `62a0`/`62b7`, gated on `[0x2e]` at
`6299/629e`) — same-role evidence on call shape, gate contexts
differ as cited. Both callees are guard-deferred above (second
layers `27a4`/`2081`), so no role claim beyond the shared intake
shape is made here.

## 0290/0293 fall-through (verified 2026-09-29, program `/fifa96.exe`)

`FUN_11bd_0290` (body `11bd:0290..0292` per `get_function_by_address`)
is one instruction — `MOV DX,0x20` at `0290` (bytes `ba2000`,
`disassemble_bytes` on `028e..0296`) — a selector-staging prelude that
falls through into `FUN_11bd_0293` (body `11bd:0293..02b4`, 12 insns,
last insn `JMP word ptr [0x9c2]` at `02b1` ending at `02b4` — matches
the delimited end; delta none). The split is a genuine pair, not an
artifact: `0293` has its own direct caller — `CALL 0x1000:1e63`
(near `e8`, delta `0x1e63−0x1bd0=0293`) at `11bd:0dc1`
(`FUN_11bd_0db2`, sole direct entry per `get_xrefs_to` and the
program-wide `:1e63` instruction search, one match) — and that caller
stages DX itself (`MOV DX,word ptr [BP+0x8]` at `0db5`), which is
exactly the register `0293` consumes at `0298` (`MOV SS,DX`); the
other eight sites enter through the stub to get the fixed selector
`DX=0x20`. Role of `0293`: a protected-mode transition — `PUSHA` at
`0293`, pre-hook `CALL word ptr [0x9c0]` at `0294`, `SS←DX` at `0298`
then `DS/ES←0x20` via `MOV DX,0x20`/`MOV DS,DX`/`MOV ES,DX` at
`029a/029d/029f`, `LLDT 0x68` at `02a1/02a4`, `SMSW`+`OR AX,[0x40]`+
`LMSW` at `02a7/02aa/02ae` (MSW←MSW∪control-mask global, PE on bit 0),
exit via `JMP word ptr [0x9c2]` at `02b1` — no `RET` anywhere and no
in-body `POPA` to pair the `PUSHA`: the vector target owns the unwind.
Both slice-12 missing legs close at the body level: DX=0x20 identity =
the SS selector staged for `MOV SS,DX` at `0298` (stub value for
0290-entry callers, caller value for the `0dc1` entry — asymmetric
with DS/ES, which re-stage the fixed `0x20` at `029a`); exit mechanism
= hook call through cell `[0x9c0]` + tail transfer through cell
`[0x9c2]`, with caller-side continuation evidence — `0db2` resumes at
`0dc4` after the call and repairs DS/ES at `0dca`/`0dcd`
(`MOV DS,[BP+0xa]`/`MOV ES,[BP+0xc]`), and the `7ced` arm resumes at
`7cf0` with `PUSH 0x38`/`POP ES` (slice-12 rows) — so the contract is
return-to-site+3, GPRs restored through the PUSHA frame, segments
clobbered, NO value consumed: neither post-site reads AX (the
decompile `0290(0)` arg lands in the saved frame, `*(puVar2+-4)=
param_1`, and comes back on the unwind — not a return slot). NOT-
CONFIRMED residue (missing fact named, boundary unaffected):
`[0x9c0]` hook has no found writer — `get_xrefs_to` on `11bd:09c0`
returns 0 refs and the program-wide operand search `0x9c0]` shows the
`0294` site as sole mention, so the hook target is runtime-written and
unidentified; `[0x9c2]` has one static writer — `MOV word ptr
[0x9c2],0x296d` at `41ee` (`FUN_11bd_3ed8`, gated: `CMP byte ptr
[0x2f],0x3` at `41e7` + `JL 0x1000:5e27` (=4257) at `41ec` skips it
while the mode word is below 3) — but `get_function_by_address` finds
no FUN at `11bd:296d` and the 16-byte dump there is unresolved
code-ish (`8E` MOV-Sreg forms recur), so the per-path target value and
the unwind shape there (POPA+RET vs terminal transfer) stay deferred.
Decompiler divergences cited above: `0290`'s decompile inlines the
whole `0293` body past its one-insn delimitation (slice-12 flag
re-confirmed — the `*(puVar2+-6)=0x20` frame store is the stub's
`MOV DX,0x20`), and `0293`'s decompile drops the `SMSW`/`LMSW` pair;
disassembly wins both times. Zero Ghidra writes this task (no rename,
no plate, no boundary change, no save).

| Question | Evidence | Verdict-so-far |
|----------|----------|----------------|
| entries into 0290? | 8 × `CALL 0x1000:1e60` (`e8` rel16 near; delta `0x1e60−0x1bd0=0290`), `get_xrefs_to` count 8 + program-wide `:1e60` search 8/8 exact: `0269` `FUN_11bd_0251`, `11f2` `FUN_11bd_11ed`, `1227` `FUN_11bd_1222`, `2081` `FUN_11bd_2081` (body-entry site), `670a` `FUN_11bd_6701`, `7a50` `FUN_11bd_79fc`, `7a8d` `FUN_11bd_7a88`, `7ced` `execute_exit_arm` | enumerated (all via stub) |
| entries into 0293 direct? | exactly one: `CALL 0x1000:1e63` at `11bd:0dc1` (`FUN_11bd_0db2`) — `get_xrefs_to` count 1, `:1e63` search 1 match; reachable by straight-line from the `0db2` entry (`JNZ 0x1000:29a3`(=0dd3) at `0dba` never taken — `XOR AX,AX` at `0db8` sets ZF) | REAL-PAIR |
| stub identity at 0290 | `MOV DX,0x20` only (bytes `ba2000`, `disassemble_bytes` `028e..0296`) — not NOP, not JMP, not RET: deliberate selector staging consumed at `0298` `MOV SS,DX`; direct-entry caller supplies its own DX instead (`0db5`), so the stub is a distinct entry contract, not padding | REAL-PAIR (deliberate prelude) |
| artifact from above? | `FUN_11bd_0251` body ends `11bd:028f` (`get_function_by_address`) with `IRETD` at `028e` (bytes `66cf`) — handler terminates; `0290` is not swallowed fall-through | no |
| boundary verdict | `0293` direct entry exists with its own DX contract + stub is deliberate staging + clean end-of-function above | REAL-PAIR |
| [0x9c0]/[0x9c2] cell values | `[0x9c0]`: 0 data xrefs, sole mention = consumer `0294` — no writer found (missing fact: hook writer). `[0x9c2]`: writer `MOV ,0x296d` at `41ee` gated `41e7`/`41ec` (`[0x2f]>=3`); `11bd:296d` no FUN, unresolved bytes — target semantics deferred (missing fact: per-path cell value + target unwind) | OPEN (residue, boundary not at stake) |

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| entry state | 11bd:0293 | via stub: DX=`0x20` (`MOV DX,0x20` at `0290`); via `0dc1`: DX=`[BP+0x8]` (`0db5`); other GPRs = caller's; CALL return word on stack (`7cf0` from `7ced`, `0dc4` from `0dc1`); flags = caller's, never read in-body (no conditional insn in the 12-insn `disassemble_function` list) | — |
| save frame | 11bd:0293 | `PUSHA` (byte `60` — 16-bit push, no `0x66` prefix; decompile mirrors it as the 8 frame stores `puVar2+-2..-0x10`, incl. DX save) | — |
| pre-hook | 11bd:0294 | `CALL word ptr [0x9c0]` (`ff16c009`), pushes return `0298` (decompile `*(puVar2+-0x12)=0x1e68`); DX must survive the hook — `0298` consumes it | cell `[0x9c0]`: no writer found (0 xrefs on `11bd:09c0`, `0x9c0]` search = sole consumer at `0294`) — runtime, no dive |
| SS load (DX=0x20 role) | 11bd:0298 | `MOV SS,DX` — consumes the entry DX: `0x20` from the `0290` stub, caller value from `0dc1` | — |
| DS/ES load | 11bd:029a..029f | `MOV DX,0x20` at `029a` + `MOV DS,DX` at `029d` + `MOV ES,DX` at `029f` — re-staged fixed, unlike SS; last two DX consumers in-body | — |
| LDT load | 11bd:02a1..02a4 | `MOV AX,0x68` + `LLDT AX` (decompile `LocalDescriptorTableRegister(0x68)`) | — |
| PE merge | 11bd:02a7..02ae | `SMSW AX` + `OR AX,word ptr [0x40]` + `LMSW AX` — MSW ∪ global mask (PE=bit0); decompiler drops the SMSW/LMSW pair — disassembly wins | — |
| exit/tail | 11bd:02b1 | `JMP word ptr [0x9c2]` (`ff26c209`, 4 bytes → ends `02b4` = delimited body end) — indirect near tail; no RET, no in-body POPA; vector target owns unwind; continuation evidence: `0db2` post-call segment repair at `0dca`/`0dcd` | cell `[0x9c2]`: static writer `41ee` (`MOV ,0x296d`, gated `41e7`/`41ec`); `11bd:296d` no FUN — address-only, no dive |
| publishes | — | zero memory stores in the 12 insns (PUSHA/CALL stack pushes only); publishes = register state: SS←DX, DS/ES←0x20, LDTR←0x68, MSW←MSW∪`[0x40]` | — |
| 7ced contract | 11bd:7ced → 7cf0 | slice-12 rows (cited, no re-walk): resumes `PUSH 0x38`/`POP ES` at `7cf0`/`7cf2` (repairs ES after the pair's `0x20`), no AX read (verify loop `CMP EAX,ES:[EBX]` at `7d06` consumes the stack struct); twin shape at `0db2` (`0dc4`+ segment repair) | — |

### Verdict: 0290 (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on the identity leg (REAL-PAIR disposition above): the
one-insn body `MOV DX,0x20` at `0290` (bytes `ba2000`,
`disassemble_bytes` `028e..0296`) is deliberate selector staging, not
pad or artifact — all eight `CALL 0x1000:1e60` sites enter specifically
for the fixed `DX=0x20` (entries row; `7ced` `execute_exit_arm` among
them); the sole in-image consumer of that DX is `MOV SS,DX` at `0298`
(SS-load row — DS/ES re-stage their own fixed `0x20` at `029a`, so the
staged value feeds only SS); the direct `0293` entry at `0dc1` proves
the contract is DX-supply, not fall-through convenience (that caller
stages its own `DX=[BP+0x8]` at `0db5`); and the `11bd:0251` body above
terminates with `IRETD` at `028e` — `0290` is not swallowed. Role
stated with citations, no missing leg material to it: renamed +
plate-set (`C: none — behavioral (selector-staging prelude …)`).

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_0290 | 11bd:0290 | `MOV DX,0x20` only (bytes `ba2000`); sole DX consumer `MOV SS,DX` at 0298 (DS/ES re-stage at 029a); 8 `CALL 0x1000:1e60` sites enter for the fixed selector; direct-`0293` caller `0db2` stages own DX at `0db5`; `0251` ends `IRETD` at `028e` — deliberate staging, not pad | stage_ss_selector | none — behavioral (selector-staging prelude) |

### Verdict: 0293 (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on all three legs (walk rows above, not rewritten): role —
consumes entry `DX` as the SS selector (`MOV SS,DX` at `0298`, the
stub's `0x20` or the `0dc1` caller's `[BP+0x8]`), re-stages `DS/ES←0x20`
at `029a/029d/029f`, loads `LDTR←0x68` at `02a1/02a4`, merges the
global control mask into the MSW (`SMSW`/`OR [0x40]`/`LMSW` at
`02a7/02aa/02ae`), saves all GPRs (`PUSHA` at `0293`) and publishes
register state only (zero memory stores — publishes row); exit
mechanism — pre-hook `CALL word ptr [0x9c0]` at `0294` + tail transfer
`JMP word ptr [0x9c2]` at `02b1`, no `RET`, no in-body `POPA` — the
vector target owns the unwind, with caller-side continuation evidence
(`0db2` repairs DS/ES at `0dca/0dcd`; `7ced` arm repairs ES at
`7cf0/7cf2`, no AX read — both slice-12 rows); callees — leaf at FUN
level (`get_function_callees` count 0), the two cell-mediated transfers
indirect and deferred with the missing pieces named, no dive. Naming
judgment recorded: `[0x40]` has exactly one read program-wide (the
`02aa` `OR`; program-wide operand search) and no direct store found, so
the MSW bit0 (PE) value — hence the transition direction — is unsettled
in-binary; the name therefore states the mechanism without asserting
enter-vs-exit, and neither cell value (`[0x9c0]` hook identity,
`[0x9c2]` per-path target) is material to that role claim. Renamed +
plate-set (`C: none — behavioral (mode-switch core …)`).

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_0293 | 11bd:0293 | `PUSHA` at 0293; `CALL [0x9c0]` at 0294; `SS←DX` at 0298 (`0x20` via stub / caller value via `0dc1`); `DS/ES←0x20` at 029a..029f; `LLDT 0x68` at 02a1/02a4; `SMSW`+`OR [0x40]`+`LMSW` at 02a7..02ae; tail `JMP [0x9c2]` at 02b1, no RET/POPA — vector target unwinds; 0 FUN callees | execute_mode_switch | none — behavioral (mode-switch core) |

### Boundary disposition

REAL-PAIR — boundary KEPT as-is, no Ghidra boundary action taken:
neither `delete_function` nor `create_function` was run, and no
re-bounding is warranted. Evidence (boundary rows above): `0293` has
its own direct entry (`CALL 0x1000:1e63` at `0dc1`, sole per
`get_xrefs_to` + program-wide `:1e63` search) with its own DX contract
(`0db5`), the `0290` stub is deliberate selector staging consumed at
`0298`, and `11bd:0251` above terminates at `IRETD` `028e`. Ghidra
before-state = after-state: `stage_ss_selector` body `11bd:0290..0292`,
`execute_mode_switch` body `11bd:0293..02b4` (`get_function_by_address`
re-read post-write — bounds unchanged; xrefs unchanged: 8 entries into
`0290`, 1 direct into `0293`).

### Resolution: slice-12 `0290` NOT-CONFIRMED leg

The `## 7c62 exit arm` NOT-CONFIRMED row (`FUN_11bd_0290` — "the role
of `DX=0x20` and the exit mechanism are both missing from disassembly")
is superseded at the pair level, row left untouched here: NOW CLOSED —
DX role (`SS` selector staged for `MOV SS,DX` at `0298`; stub value for
the 8 entering sites, caller value for the direct `0dc1` entry) and
exit mechanism (hook `CALL [0x9c0]` + tail `JMP [0x9c2]`, vector target
owns unwind, caller-side repair evidence `0dca/0dcd` and `7cf0/7cf2`).
STAYS OPEN (named, none material to the two assigned roles): `[0x9c0]`
hook writer/target identity (0 data xrefs, sole mention the `0294`
consumer — runtime); `[0x9c2]` per-path cell value and the unwind shape
at its `0x296d` target (static gated writer `41ee`, no FUN, unresolved
bytes — `3ed8` mode-dispatch territory, Task 2+ material); `[0x40]` MSW
mask runtime value (single read `02aa`, no direct store found — mode
direction unsettled, reflected in the direction-neutral name). Ghidra
writes this task: two renames + two plates + `save_program` on
`/fifa96.exe` — success.

## 296d hook target (verified 2026-09-29, program `/fifa96.exe`)

The `[0x9c2]` value `0x296d` written at `41ee` lands inside a 714-byte
gap (`find_code_gaps` row `1000:43e1..1000:46aa` = `11bd:2811..2ada`,
between `FUN_11bd_27e4` body `27e4..2810` and `FUN_11bd_2adb` body
`2adb..2ae8`); `11bd:296d` is an instruction boundary both ways (dry-run
walk from the gap start decodes the preceding block to `RET` `c3` at
`296c`; a bounded decode AT `296d` is clean 386 code). The hidden target
is a 3-instruction restore-and-resume epilogue: `MOV FS,[0xd60]` at
`296d` + `MOV GS,[0xd62]` at `2971` (reloading the segments saved at
`2880/2884` — `MOV [0xd62],GS`/`MOV [0xd60],FS`, `8c2e620d`/`8c26600d`)
+ tail `JMP 0x1000:1e85` (= `11bd:02b5`) at `2975` (`e93dd9`) — an exact
exit cited to the byte, boundary proposal `[296d..2977]`, classification
FUNC (no call sites, no memory writes beyond segment state). The landing
pad closes the slice-13 deferred leg: `02b5` is `POPA` (`61`) and `02b6`
`RET` (`c3`) (dry-run) — the `PUSHA` frame from `execute_mode_switch`
`0293` is unwound there and the `RET` returns to the CALL site +3,
matching the `0dc4`/`7cf0` continuation evidence; the unwind shape is
POPA+RET on the landing pad, the vector target itself transfer-tails.
Cell forensics: `[0x9c0]` has NO in-exe writer (searches exhausted —
`get_xrefs_to` 0 refs, operand `0x9c0` = sole consumer `0294`, operand
`9c0` adds only the `1991:3093` `JZ 0x1000:c9c0` branch-target false
hit); `[0x9c2]` writer set confirmed and NOT extended (`02b1` read +
`41ee` write `c706c2096d29`, same 2 hits for both operand forms), and
the value `0x296d` appears exactly once program-wide (at `41ee`). The
block starting at `2978` (paging: `CMP [0xdfe],1`, `OR EAX,0x80000000`
CR0, LIDT-adjacent hole code) is NOT part of this boundary — the `2975`
JMP is unconditional, so `2978` is unreachable from the `296d` entry and
has no static entry found (`0x2978` operand search 0 matches, xrefs 0).
Zero Ghidra writes this task: `disassemble_bytes` used exclusively with
`dry_run=true`; no function created/renamed/re-bounded, no save.

| Question | Evidence | Verdict-so-far |
|----------|----------|----------------|
| gap covering 11bd:296d? | `find_code_gaps` (total 130, page offset 0/limit 100 — sorted-disjoint, unique coverer found on page 0), verbatim row: `{"start":"1000:43e1","end":"1000:46aa","size":714,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_27e4","before_function_address":"11bd:27e4","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` — `1000:453d` = `11bd:296d` sits +`15c` inside it | GAP-CONFIRMED, not aligned to `296d` (hole = whole unanchored PM-restore subsystem `2811..2ada`) |
| anything defined at 296d? | `get_function_by_address` 11bd:296d → `{"error":"No function found for 11bd:296d"}` | nothing defined |
| covered neighbors | prev `FUN_11bd_27e4` body `27e4..2810` = gap start −1 (re-read); next `FUN_11bd_2adb` body `2adb..2ae8` = gap end +1 (re-read) | hole bounded by real function bodies both sides |
| first bytes at 296d (entry plausibility) | dry-run `disassemble_bytes` `296d..`: `8e26600d` = `MOV FS, word ptr [0xd60]` — a legal 386 segment load (reg field /4), recurs verbatim at `282c` inside the hole's decoded stream; not pad, not mid-instruction | CODE, plausible entry |
| stream alignment at 296d | dry-run decode from gap start `2811` (369-byte window to `2981`): contiguous through `…2965 MOV CR0,EAX` `0f22c0`, `2968 POP EAX` `6658`, `296a POP BX` `5b`, `296b POP AX` `58`, `296c RET` `c3` → next insn at exactly `296d` (two local decode-skips at `2811..281f`/`28e7..28eb` elsewhere in the window, none near the target) | instruction boundary |

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| entry | 11bd:296d | sole static feeder = cell `[0x9c2]` value `0x296d` (`MOV [0x9c2],0x296d` at `41ee`, `c706c2096d29`, gated `41e7`/`41ec` per slice 13; consumer `JMP [0x9c2]` at `02b1`); `get_xrefs_to(11bd:296d)` = 0 refs; operand search `0x296d` = 1 hit program-wide (`41ee`) — entry reached by the dynamic target alone | — |
| insn 1 (FS restore) | 11bd:296d | `MOV FS, word ptr [0xd60]` (`8e26600d`) — reload of the copy saved by the hole's forward path: `MOV word ptr [0xd60], FS` at `2884` (`8c26600d`), dry-run-cited | — |
| insn 2 (GS restore) | 11bd:2971 | `MOV GS, word ptr [0xd62]` (`8e2e620d`) — twin save `MOV word ptr [0xd62], GS` at `2880` (`8c2e620d`) | — |
| exit (tail JMP, cited) | 11bd:2975 | `JMP 0x1000:1e85` (`e93dd9`, 3 bytes → ends `2977`) — unconditional near tail = exact function exit; target = `11bd:02b5` = exactly one byte past the `JMP [0x9c2]` (`02b1..02b4`), i.e. the resume point of `execute_mode_switch`'s caller stream | — |
| landing pad (unwind) | 11bd:02b5..02b6 | dry-run `disassemble_bytes` `02b5..02c1`: `POPA` (`61`) + `RET` (`c3`) — balances `PUSHA` at `0293` and returns to CALL site +3 (`0dc4`/`7cf0` per slice-13 continuation rows); at `02b7` a second mode-switch clone starts (`PUSHA` `60`, `CALL word ptr [0x9c0]` `ff16c009` at `02b8`, `MOV AX,0x20`/`MOV DS`/`MOV ES` at `02bc..`) — gap row `1000:1e85..1000:2302` (neighbors `execute_mode_switch`/`FUN_11bd_0733`); no dive | 02b8 cell-mediated (same `[0x9c0]`), address-only |
| block after exit (NOT in boundary) | 11bd:2978 | `CMP byte ptr [0xdfe],0x1` (`803efe0d01`) + paging-enable block (`PUSH EAX` `6650`, `MOV EAX,CR0` `0f20c0`, `OR EAX,0x80000000` `660d00000080`, `MOV CR0,EAX` `0f22c0` …) — unreachable from the `296d` entry (the `2975` JMP is unconditional); zero static entries found (`get_xrefs_to(2978)` 0, operand `0x2978` 0 matches) — deferred, ownership open | — |
| boundary + classification | [11bd:296d..2977] | entry = the dynamic cell target `0x296d` alone; aligned both from `2811`-stream (boundary above) and AT the address; exact exit cited (`JMP` `e93dd9` ending `2977`); 3 insns, zero calls, zero memory stores (segment loads only); shape = restore-and-resume epilogue stub, not mid-function flow | FUNC (11-byte epilogue stub) |

| Cell | Searches run (enumerated) | Hits (address + mnemonic + bytes) | Classification | Verdict |
|------|---------------------------|-----------------------------------|----------------|---------|
| [0x9c0] (pre-hook) | `get_xrefs_to(11bd:09c0)`; `search_instructions` operand `0x9c0`; operand `9c0` (13976 defined insns scanned each) | xrefs: 0. `0x9c0`: 1 — `0294` `CALL word ptr [0x9c0]` `ff16c009`. `9c0`: 2 — same `0294` + `1991:3093` `JZ 0x1000:c9c0` `741b` (branch-target substring in another program segment, NOT a data operand). No `MOV [0x9c0],…`/`POP`-into/store form at all | 1 read (`0294`), 0 writes | NOT-IN-EXE — writer searches exhausted; hook target is runtime-written (loader/relocation-time), identity unidentified |
| [0x9c2] (tail vector) | `get_xrefs_to(11bd:09c2)`; operand `0x9c2`; operand `9c2` | xrefs: 0 (despite defined refs — xref capture unreliable for these DS-relative cells; instruction search is the authority). `0x9c2`: 2 — `02b1` `JMP word ptr [0x9c2]` `ff26c209` + `41ee` `MOV word ptr [0x9c2],0x296d` `c706c2096d29`. `9c2`: same 2. Supplementary operand `0x296d`: 1 — `41ee` only | 1 read (`02b1`), 1 write (`41ee`) | writer set CONFIRMED (`{41ee}`), NOT extended; sole value `0x296d` → epilogue above; gate `41e7`/`41ec` (`[0x2f]>=3`) per slice 13 |

Search-method caveat (recorded for reuse): `search_instructions`
enumerates defined instructions only (13976 scanned), so relative jumps
inside the two big holes (`02b5..0733`, `2811..2ada`) are invisible to
operand searches until decoded; the dry-run windows cited above
(`2811..2981`, `02b5..02c1`) show no jump targeting `296d`/`2978` within
their coverage, and `296d`'s only static feeder is the `41ee` cell write.

### Verdict: 296d (CONFIRMED 2026-09-29, program `/fifa96.exe`)

CONFIRMED on all legs (walk/cell rows above, not rewritten): role — the
11-byte body reloads FS/GS from the selector cells `[0xd60]`/`[0xd62]`
(`MOV FS,[0xd60]` `8e26600d` at `296d`, `MOV GS,[0xd62]` `8e2e620d` at
`2971` — exact counterparts of the hole's save pair `MOV [0xd60],FS`
`2884`/`MOV [0xd62],GS` `2880`) and tail-transfers to the landing pad
(one restore-and-resume epilogue; "mode" enters only through the landing
pad's frame unwind, so the name states mechanism, not caller lore); exit
mechanism cited — `JMP 0x1000:1e85` (`e93dd9`) at `2975`, ends `2977`,
unconditional near tail, first plausible exit in address order (no
earlier RET/JMPF/IRET); callee — single transfer target is the landing
pad `02b5..02b6` (`POPA` `61` + `RET` `c3`), body-level cited by Task-1's
dry-run `02b5..02c1` window and NOT walked further (out-of-slice,
cite-only); boundary defended by the gap rows above (`296d` inside
`1000:43e1..1000:46aa`, `get_function_by_address` error pre-write,
`can_rename_at_address` type `undefined` = no symbol, xrefs 0) + entry
evidence (sole static feeder the `41ee` cell write; `get_xrefs_to` 0;
operand `0x296d` 1 hit program-wide). No leg missing → renamed +
plate-set.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_296d → restore_fs_gs_and_resume | 11bd:296d | `MOV FS,[0xd60]` at 296d + `MOV GS,[0xd62]` at 2971 (reloads of the `2884`/`2880` save pair) + tail `JMP 0x1000:1e85` `e93dd9` at 2975 ending 2977; sole feeder = `41ee` cell value (`get_xrefs_to` 0, operand `0x296d` 1 hit); post-write: body exactly `296d..2977`, callees = {`FUN_11bd_02b5` (landing-pad tail)}, xrefs-to still 0 — entry stays purely dynamic | restore_fs_gs_and_resume | none — behavioral (FS/GS selector-cell restore + tail-resume epilogue) |

No leaf verdict row: the only callee (`FUN_11bd_02b5`) is the landing pad,
cited at body level only, renamed nothing, per the one-layer guard.

### Writes (before-state → post-write state)

Before-state: gap `11bd:2811..2ada` (hole row above), nothing defined at
`11bd:296d` — `get_function_by_address` → `"No function found"`,
`can_rename_at_address` → type `undefined`, `get_xrefs_to` → 0 refs;
bytes UNDEFINED in the listing (Task 1 was `dry_run=true` throughout).
Write sequence, each with result: (1) real `disassemble_bytes`
`11bd:296d..2977` → 3 insns defined (`8e26600d`/`8e2e620d`/`e93dd9`),
identical to the Task-1 dry-run head. (2) `create_function(11bd:296d)` →
`FUN_11bd_296d`, reported `body_size:13`; re-read shows the requested
boundary KEPT exact — body `296d..2977` — and the +2 flow effect landed
in a SEPARATE auto-created landing-pad function `FUN_11bd_02b5`
(`02b5..02b6` = the dry-run `POPA`+`RET`), not a merged body; the
`disassemble_first=false` retry was therefore NOT needed and was not
attempted. `11bd:02b7` re-checked after save: still no function — the
second mode-switch clone (`PUSHA`/`CALL [0x9c0]` at `02b7..02b8`) is
untouched; `FUN_11bd_02b5` itself was left exactly as created (no rename,
no plate, no re-bound). (3) `rename_function` → `restore_fs_gs_and_resume`
(verb-led snake_case from the walk's mechanism: FS/GS restore, tail
resume; no `decode_*`, no caller lore). (4) `set_comment` plate →
`C: none — behavioral (FS/GS selector-cell restore + tail-resume
epilogue)`. (5) `save_program(/fifa96.exe)` → success; post-write
re-confirm (the state the row above cites): name + body `296d..2977` +
plate via `get_function_by_address`/`get_comment`, `get_function_callees`
= {`FUN_11bd_02b5`}, `get_function_xrefs` = 0.

### Hook cells

The two cells are one pair around the transition core
(`execute_mode_switch` `0293..02b4`, per the `## 0290/0293 fall-through`
section — cited, not rewritten). `[0x9c0]` = PRE-SWAP HOOK: called at
`0294` (`CALL word ptr [0x9c0]` `ff16c009`, pushes return `0298`, so DX
must survive it) and again by the `02b7` twin at `02b8`; writer set:
NOT-IN-EXE — searches exhausted per the cell table above
(`get_xrefs_to(11bd:09c0)` 0 refs; operand `0x9c0` → sole mention the
`0294` consumer; operand `9c0` → same plus the `1991:3093`
branch-target false hit; no store form among the 13976 scanned defined
insns), so the hook target is runtime-installed (loader/self-relocation)
with identity unidentified — a negative over the in-exe search space, not
a claim that no writer exists at runtime. `[0x9c2]` = CONTINUATION
VECTOR: sole read `02b1` (`JMP word ptr [0x9c2]` `ff26c209` — the core's
tail exit), sole writer `41ee` (`MOV word ptr [0x9c2],0x296d`
`c706c2096d29`, set `{41ee}` confirmed and NOT extended per the cell
table); the vector is ARMED exactly when `CMP byte ptr [0x2f],0x3` at
`41e7` passes the `JL 0x1000:5e27` (= `4257`) skip at `41ec`, i.e. mode
word `[0x2f] >= 3`. The armed-path target is now the named
`restore_fs_gs_and_resume` (`11bd:296d..2977`). Pair summary: pre-swap
notification (cell-mediated, runtime target, unresolved identity) +
post-transition continuation (statically armed at `41ee` when
`[0x2f]>=3`, restores FS/GS from the `2880/2884` selector cells, tails
into `POPA`+`RET` at `02b5..02b6` and returns to the CALL site +3 — the
`0dc4`/`7cf0` contract of the `## 0290/0293 fall-through` section).
Xref caveat applies to both cells (0 data xrefs despite defined
referencing insns; instruction enumeration is the authority — row above).

### Deferral resolution

Closes, from the `## 0290/0293 fall-through` section's NOT-CONFIRMED
residue list (`0293` verdict + `### Resolution: slice-12` STAYS-OPEN
paragraph — rows untouched, resolved here by reference): (a) `[0x9c2]`
per-path cell value — on the armed path (`[0x2f]>=3`) the vector points
at `0x296d` and `restore_fs_gs_and_resume` IS what the armed path does:
reloads FS/GS from `[0xd60]`/`[0xd62]` (the `2880/2884` saves) and
transfer-tails to the landing pad — value `0x296d` appears exactly once
program-wide (`41ee`), so there is no second static target; (b) the
unwind shape at that target — `POPA` (`61`) + `RET` (`c3`) at
`02b5..02b6` (dry-run-cited, and now flow-anchored as
`FUN_11bd_02b5`): the `PUSHA` frame from `0293` unwinds there and the
`RET` returns to CALL site +3, honoring the `0dc4`/`7cf0` continuation
evidence — the vector target itself never returns, it transfer-tails;
(c) `[0x9c0]` writer question — resolved NEGATIVE: not-in-EXE (search
enumeration in the cell table above), the hook target is runtime-written.
Stays open (named, none material to the epilogue's verdict): `[0x9c0]`
installer identity (whoever writes it at runtime is unidentified);
`0x2978` paging-block ownership (unreachable from the `296d` entry, zero
static entries found — the `296d`-callee second-layer question in
miniature, deferred per the one-layer guard); `[0x40]` MSW mask writer
(single read `02aa`, no direct store found — slice-13 list, mode
direction still unsettled); the `02b5`-region twin (`PUSHA` `60` at
`02b7` + `CALL word ptr [0x9c0]` at `02b8` — still no function at
`02b7` post-write, untouched; future slice material together with the
as-yet-unnamed `FUN_11bd_02b5` landing pad).

## 02b7 twin (verified 2026-09-29, program `/fifa96.exe`)

The second mode-switch clone flagged at `11bd:02b7` in the `## 296d hook
target` deferral list is proven UNOWNED at current state and walked to a
cited exit with `disassemble_bytes` in `dry_run=true` only: the proposed
body `[02b7..02f8]` (66 bytes, 24 insns, whole-region `read_memory` dump
`60ff16c009b82000…000f00d0061f61c3`) opens exactly like
`execute_mode_switch` (`PUSHA` `60` at `02b7`; `CALL word ptr [0x9c0]`
`ff16c009` at `02b8` — return slot `02bc`, hook consumes it) and carries
the identical PE-merge op (`SMSW AX` `0f01e0` @`02c3`,
`OR AX,word ptr [0x40]` `0b064000` @`02c6`, `LMSW AX` `0f01f0` @`02ca`
— cite the sibling map row for `02a7/02aa/02ae`, not re-walked), then
DEVIATES where the sibling tails out: no `SS←DX` head-load (staging is
`MOV AX,0x20`/`MOV DS`/`MOV ES` at `02bc/02bf/02c1` — note the task
context guessed `MOV DX,0x20`@`02ba`; bytes say `02ba..02bb` = `c009`,
the little-endian cell operand of the `02b8` CALL, and the loader is
`MOV AX,0x20` `b82000`@`02bc`), a slot-cursor sequence (`MOV SI,[0xf52]`
@`02cd`, `AND SI,0x38` @`02d1`, then the 6-byte skip pair at
`02d4..02d9` — see walk row), `MOV DS,0x8` @`02dd/02e0`, three stores
into the cursor slot (`MOV [SI+0x2],DI` `897c02`,
`MOV [SI+0x4],DL` `885404`, `MOV [SI+0x7],DH` `887407`), the computed
stack switch `MOV SS,SI` `8ed6` @`02ed`, `LLDT 0x68` (`MOV AX,0x68` +
`0f00d0`) @`02ef/02f2`, `DS←ES` via `PUSH ES`/`POP DS` @`02f5/02f6`, and
a SELF-closing exit `POPA` `61` @`02f7` + `RET` `c3` @`02f8` — the twin
unwinds its own `PUSHA` frame and returns to its caller's return slot,
whereas the sibling delegates the unwind to the `[0x9c2]` vector target
and the landing pad (per its map rows; cite-only). The twin never reads
`[0x9c2]` — the continuation vector is absent from its body. Entry has
NO static feeder: `get_xrefs_to(11bd:02b7)` 0 refs, defined-insn operand
searches `1e87`/`0x1e87`/`0x2b7` 0/0/0 matches (13981 insns scanned), and
the program-wide raw-byte search `871ebd11` (far-pointer form of
`0x1e87`:`0x11bd`, covers undefined holes too) 0 hits — entry is
dynamic/installed or lives inside an undecoded hole; the boundary is
defended by structure, not caller count. Direction pairing: BOTH bodies
apply the same set-only `OR [0x40]` mask merge; neither contains an
`AND`/`ANDNOT` MSW op — no PE-clearing leg exists in this pair, so the
enter-vs-exit question stays UNDECIDED and the `[0x40]` writer is
re-cited as the missing leg for the runtime slice (slice-13 rows
1180-1194 untouched). Zero Ghidra writes this task: `disassemble_bytes`
exclusively `dry_run=true`, plus `get_function_by_address`,
`find_code_gaps`, `get_xrefs_to`, `search_instructions`,
`search_byte_patterns`, `read_memory` (reads only).

| Question | Evidence | Verdict-so-far |
|----------|----------|----------------|
| function at 02b7? | `get_function_by_address(11bd:02b7)` → `{"error":"No function found for 11bd:02b7"}` — actual response, current state | NO-FUNCTION (as expected from slice 14's post-write spot-check; no state change to resolve) |
| covering gap row | `find_code_gaps` (total 131, offset 0/limit 100, twin row on page 0), verbatim: `{"start":"1000:1e87","end":"1000:2302","size":1148,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"FUN_11bd_02b5","before_function_address":"11bd:02b5","after_function":"FUN_11bd_0733","after_function_address":"11bd:0733"}` — `1000:1e87` = `11bd:02b7` (0x11bd0+0x2b7 = 0x11e87), twin start = gap start exactly; end = `11bd:0732`, next owner `FUN_11bd_0733` | GAP-CONFIRMED, gap BEGINS at `02b7` (no discrepancy vs slice-14's row `1000:1e85..1000:2302`/`execute_mode_switch` neighbor — it reflowed to start `1000:1e87` with before_function `FUN_11bd_02b5` when the landing pad was created in slice-14 Task 2) |
| gap total drift | 131 now vs 130 at slice-14 Task 1: the `296d` row split into `1000:43e1..1000:453c` + `1000:4548..1000:46aa` around the new `restore_fs_gs_and_resume` body (`296d..2977` = `1000:453d..4547`); the `02b7` row same count, shifted start | consistent post-write bookkeeping, no unexplained coverage |
| FUN_11bd_02b5 bounds | `get_function_by_address(11bd:02b5)` → `{"name":"FUN_11bd_02b5","body_start":"11bd:02b5","body_end":"11bd:02b6"}` | EXACTLY SIZED `02b5..02b6` (POPA/RET landing pad unchanged; the twin starts one byte past its end) |
| static entries into 02b7 | `get_xrefs_to(11bd:02b7)` → 0 refs; `search_instructions` operand `1e87` → 0 matches, `0x1e87` → 0, `0x2b7` → 0 (13981 defined insns scanned); `search_byte_patterns` `871ebd11` → no matches (raw scan incl. undefined holes) | NO-FEEDER-FOUND — entry dynamic or inside an undecoded hole (weaker than `296d`, which at least had the `41ee` cell-write feeder); does not defeat the boundary (structure-leg rows below) |

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| entry / save frame | 11bd:02b7 | `PUSHA` (byte `60`, dry-run `02b7..02fe` window 1; raw dump byte 1) — same opener as sibling `0293`; fall-through INTO `02b7` impossible: preceding owner byte `02b6` is the landing pad's `RET` (cited slice-14 row), flow never reaches `02b7` linearly | — |
| pre-hook | 11bd:02b8 | `CALL word ptr [0x9c0]` (`ff16c009`, 4 bytes → return slot next insn at `02bc`) — SECOND read of `[0x9c0]` (first: sibling `0294`); cell operand little-endian `c009` = `0x9c0` sits at `02ba..02bb`, correcting the task-context guess of `MOV DX,0x20`@`02ba` (bytes from `02ba`: `c0 09 b8 20 00` = CALL tail + start of `b82000`) | cell `[0x9c0]`: writer set unchanged (NOT-IN-EXE per slice-14 cell table; searches defined-only — the `02b8` consumer is in undefined bytes, invisible until decoded, exactly the recorded method caveat) |
| DS/ES staging | 11bd:02bc..02c1 | `MOV AX,0x20` (`b82000`) + `MOV DS,AX` (`8ed8`) + `MOV ES,AX` (`8ec0`) — selector `0x20` fixed via AX; NO `SS←DX` head-load (sibling has `MOV SS,DX`@`0298` — its map row, cite-only) | — |
| PE merge | 11bd:02c3..02ca | `SMSW AX` (`0f01e0`) + `OR AX,word ptr [0x40]` (`0b064000`) + `LMSW AX` (`0f01f0`) — set-only merge, same op AND same operand-cell as sibling `02a7/02aa/02ae` (cited rows; `0x40]`-operand text search is defined-only so it never saw `02c6` — this walk is its second sighting) | — |
| cursor compute | 11bd:02cd..02d1 | `MOV SI,word ptr [0xf52]` (`8b36520f`) + `AND SI,0x38` (`83e638`, ends `02d3`) | new cells touched: `[0xf52]` (read here) |
| 6-byte decode skip | 11bd:02d4..02d9 | tool returned NO instructions for these bytes from every dry-run window attempted (`02b7`-start, `02d1..02da`, `02d2..02e2`, `02d4..02de`, `02d4..02d9` alone → `instructions_total:0`); `read_memory` raw = `0336540f 8306520f` + `08`@`02dc`; aligned manual decode from `02d1`'s end: `02d4 ADD SI,word ptr [0xf54]` (`0336540f`) + `02d8 ADD word ptr [0xf52],0x8` (`8306520f08`) lands EXACTLY on the tool's own resume point `02dd` and consumes every byte (66-byte region dump concatenates cleanly end-to-end); the tool's emitted alternative (`PUSH DX`@`02da` `52`, `INVD`@`02db` `0f08`) leaves `02d4..02d9` undecoded AND breaks the frame — an unmatched `+2` push would make `POPA`/`RET`@`02f7/02f8` pop DX-pushed data as return address | AMBIGUOUS-BY-TOOL, RESOLVED-BY-BYTES (both parses converge at `02dd`; only the `ADD`/`ADD` parse balances PUSHA↔POPA and explains all bytes; recorded as a finding, byte evidence wins per disassembly-rule) |
| DS re-stage | 11bd:02dd..02e0 | `MOV AX,0x8` (`b80800`) + `MOV DS,AX` (`8ed8`) — second selector immediate (`0x8`) | — |
| slot stores | 11bd:02e2..02ea | `MOV word ptr [SI+0x2],DI` (`897c02`) + `MOV byte ptr [SI+0x4],DL` (`885404`) + `XOR DH,DH` (`32f6`) + `MOV byte ptr [SI+0x7],DH` (`887407`) — the sibling publishes ZERO memory stores (its map row); these writes (SS-default addressing) + the `+0x8` cursor bump (slot stride 8) are the twin's distinguishing side effect | — |
| stack switch | 11bd:02ed | `MOV SS,SI` (`8ed6`) — selector COMPUTED from the `[0xf52]/[0xf54]` cursor arithmetic, not an immediate (sibling: `SS←DX` staged selector, cited row) | — |
| LDT load | 11bd:02ef..02f2 | `MOV AX,0x68` (`b86800`) + `LLDT AX` (`0f00d0`) — same LDTR selector as sibling `02a1/02a4` (cited row) | — |
| DS←ES copy | 11bd:02f5..02f6 | `PUSH ES` (`06`) + `POP DS` (`1f`) — ESP net 0; no sibling counterpart | — |
| EXIT (cited) | 11bd:02f7..02f8 | `POPA` (`61`) + `RET` (`c3`) — self-unwind: `PUSHA`@`02b7` ↔ `POPA` (16+16 bytes, CALL/hook RET pair already consumed at `02bc`), `RET` pops the CALLER's return → classic near-call contract; NOT the sibling's tail `JMP [0x9c2]` (cited `02b1` row); first exit in address order, nothing earlier in the walk (no unconditional JMP/RET between `02b7` and `02f7`; the `02b8` CALL is the only transfer before it) | — |
| bytes after exit | 11bd:02f9.. | `NOP` (`90`), `0000` `ADD byte ptr [BX+SI],AL`, desynced `2e8f06fa02` `POP word ptr CS:[0x2fa]` — pad-like, NOT in proposed boundary | — |
| proposed boundary | [11bd:02b7..02f8] | 66 bytes / 24 insns, entry gap row starts at `02b7` exactly, prologue↔epilogue balanced, whole-region byte dump = instruction concatenation, `02b6` RET kills fall-through into `02b7`; why a function start despite 0 feeders: structure (full PUSHA…POPA/RET frame contract, zero leftover bytes) + sole-occupancy of `[02b7..02f8]` inside the `1000:1e87..1000:2302` gap; exit lands INSIDE the same gap at `02f8` — no `2978..2ada` contact (out-of-scope block never entered by this flow) | boundary FUNC-proposed (Task 2 verdict material) |

| Contact | Twin `02b7..02f8` (walked) | Sibling `0293..02b4` (CITED from `## 0290/0293 fall-through`, not re-walked) | Implication |
|---------|---------------------------|------------------------------------------------------------------|-------------|
| `[0x40]` MSW mask op | `SMSW`@`02c3` + `OR AX,[0x40]`@`02c6` (`0b064000`) + `LMSW`@`02ca` — SET-only | `SMSW`@`02a7` + `OR AX,[0x40]`@`02aa` + `LMSW`@`02ae` — SET-only (map rows 1204/1255) | SAME op, SAME cell: neither body can CLEAR PE (no AND/ANDNOT form in either) — the pairing adds NO exit-to-PM leg ⇒ direction rule ("one sets, one clears") NOT met → UNDECIDED |
| `[0x9c0]` pre-hook | `CALL [0x9c0]`@`02b8` (`ff16c009`) — read-site #2 | `CALL [0x9c0]`@`0294` — read-site #1 (map row 1200) | slice-13/14 cell story UNCHANGED (writer still none in 13981 defined insns; `02b8` was invisible to defined-only searches per the recorded caveat): read fan-out 1→2, hook is shared by both bodies — contact-point evidence only, NOT-IN-EXE verdict not overturned |
| `[0x9c2]` tail vector | ABSENT — twin exits `POPA`/`RET`@`02f7/02f8` | sole read `JMP [0x9c2]`@`02b1` (map row 1205) | twin does not participate in the continuation-vector story (`{41ee}` writer set, value `0x296d` — cited slice-14 rows); it RETURNS instead of transferring |
| segment stores | `MOV DS,AX`@`02bf`(`0x20`), `MOV ES,AX`@`02c1`(`0x20`), `MOV DS,AX`@`02e0`(`0x8`), `MOV SS,SI`@`02ed`(computed), `POP DS`@`02f6` | `MOV SS,DX`@`0298`(caller/stub DX), `MOV DS`/`MOV ES`@`029d/029f`(`0x20`) | twin clobbers/re-purposes segments AND switches SS to a table-computed selector; sibling only stages — different job at the same transition machinery |
| selector immediates | `0x20`@`02bc`, `0x8`@`02dd`, `0x68`@`02ef`(LLDT@`02f2`) | `0x20`@`029a`(+`0290` stub), `0x68`@`02a1`(LLDT@`02a4`) | shared constants (`0x20` data selector, `0x68` LDT selector); `0x8` and the `[0xf52]/[0xf54]` cursor cells are twin-only |
| memory stores | `[SI+0x2]`@`02e2`, `[SI+0x4]`@`02e5`, `[SI+0x7]`@`02ea`, `word [0xf52]` bump@`02d8`(byte-evidence parse) | ZERO stores (map row 1206) | twin mutates a slot table + cursor cell — a stateful walk; sibling stateless |
| unwind shape | self: `POPA`+`RET` inside body | delegated: tail `JMP [0x9c2]` → `296d` epilogue → landing pad `02b5..02b6` (cited slice-14) | the "twin" is not a clone: same OPENER and same mask op, different exit contract |
| gap-coverage caveat | operand searches see defined insns only; EVERY byte of the twin body was undefined at search time, so `1e87`/`0x2b7` text searches could not have seen twin-internal immediates either — raw-byte search `871ebd11` covers the holes and found nothing; no proposed-region byte is also immediate-encoded in a way that defeats the walk's boundaries (`02dd` resume point identical under both parses; the `02d4` cell operand `540f`/`520f` shapes are cited per parse) | same caveat recorded slice 14 (M1) | searches over/into the gap are NOT a coverage proof — walk evidence is |

Direction verdict (this task, evidence table above): **UNDECIDED** —
the instructions show both bodies performing the SAME set-only
`OR AX,word ptr [0x40]` merge (`02c6` cited by bytes `0b064000`; sibling
`02aa` cited by map row); nothing in the pair clears PE, so no
enter-vs-exit call is decidable from instruction evidence alone and the
pairing's only new information is the uniformity itself (a hypothetical
PE-CLEARING clone would have carried `AND`/`ANDNOT`-`0f20/0f22` CR-style
or `AND AX,[mask]`+`LMSW` — none present). The `[0x40]` writer/runtime
value remains the missing leg (slice-13 rows 1194/1245-1248 re-cited:
reads now `{02aa, 02c6}`, direct stores still 0 among defined insns),
handed to the runtime slice.

### Writes (before-state → post-write state)

Before-state, re-confirmed live immediately before any write (same values
as the tables above; quoted verbatim):
`get_function_by_address(11bd:02b7)` →
`{"error":"No function found for 11bd:02b7"}`;
`get_xrefs_to(11bd:02b7)` →
`{"references":[],"count":0,"offset":0,"limit":100,"total":0}`; covering
gap row (Task 1, pre-write state):
`{"start":"1000:1e87","end":"1000:2302","size":1148,...,"before_function":"FUN_11bd_02b5","after_function":"FUN_11bd_0733"}`;
`audit_global(11bd:0f52)` →
`{"name":"","type":"","xref_count":0,...}` — the cursor cells are undefined
globals with no symbol and no xrefs (the twin body was still undecoded).
Write sequence, each with actual result: (1) real `disassemble_bytes`
`11bd:02b7` (end-exclusive → decoded `02b7..02f7`, 65 bytes):
`{"success":true,...,"instructions_total":24,"truncated":false}` — the tool
REPRODUCED the Task-1 skip exactly: it emitted nothing for `02d4..02d9`
(raw `03 36 54 0f 83 06`) and defined its own alternative `PUSH DX` (`52`)
@`02da` + `INVD` (`0f08`)@`02db..02dc`, resuming at `MOV AX,0x8`@`02dd`;
all other 24 emitted insns byte-identical to the dry-run walk. Follow-up
`disassemble_bytes` `11bd:02f8` →
`{"instructions":[{"address":"11bd:02f8","mnemonic":"RET","bytes":"c3"}],"instructions_total":1}`
— the cited-exit `RET` defined. (2) `create_function(11bd:02b7)` →
`{"success":true,"function_name":"FUN_11bd_02b7","body_size":29,...}`;
re-read `get_function_by_address(11bd:02b7)` →
`{"body_start":"11bd:02b7","body_end":"11bd:02d3"}` — the analyzer's
NEAREST CONSISTENT BODY: flow breaks at the tool-undecoded pocket
`02d4..02d9` (29 bytes / 10 insns = `PUSHA`..`AND SI,0x38`). NO
over-extension past `02f8` occurred (the body UNDER-shoots), so the
`disassemble_first=false` retry is inapplicable — a dry-run of it against
the live function returns
`{"dry_run":true,"error":"Function already exists at 11bd:02b7: FUN_11bd_02b7"}`,
and delete-then-recreate would be fighting the analyzer: NOT done. The
skip region was NOT hand-forced: the byte-evidence `ADD SI,[0xf54]`
(`0336540f`)@`02d4` + `ADD word [0xf52],0x8`
(`8306520f08`)@`02d8` parse remains recorded-but-unapplied; the listing
carries the tool's `PUSH DX`/`INVD` parse at `02da..02dc`. Residual state
inside the proposed boundary: pocket `02d4..02d9` UNDEFINED (6 bytes) and
tail `02da..02f8` DEFINED-BUT-OUTSIDE the function body (13 insns — the
`PUSH DX`/`INVD`/`MOV AX,0x8`/…/`POPA`/`RET` chain incl. the cited exit);
`get_function_by_address` at `02da` and `02d4` both →
`{"error":"No function found..."}`. (3) Rename: NOT performed —
NOT-CONFIRMED-at-name (verdict below; missing leg = `[0xf52]`/`[0xf54]`
cell roles). (4) `set_comment` plate at `11bd:02b7` →
`{"status":"success","message":"Set plate comment at 11bd:02b7"}`
(boilerplate Algorithm/Parameters/Returns warnings only — same as prior
slices' behavioral plates). (5) `save_program(/fifa96.exe)` →
`{"success":true,...}`. Post-write re-confirm (the state the verdict row
cites; actual responses): `get_function_by_address(11bd:02b7)` →
`{"name":"FUN_11bd_02b7","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}`;
`get_comment(11bd:02b7)` → plate read back verbatim (text quoted below in
the verdict row's plate cell); `get_function_xrefs(11bd:02b7)` →
`{"references":[],"count":0,"total":0}` (still zero static entries —
creation defined bytes, it did not manufacture feeders); `find_code_gaps`
row reflowed to
`{"start":"1000:1ea4","end":"1000:2302","size":1119,"has_undefined_bytes":true,"has_orphaned_instructions":true,"before_function":"FUN_11bd_02b7","after_function":"FUN_11bd_0733"}`
(`1000:1ea4` = `11bd:02d4` — the pocket start; −29 bytes = the created
body; `has_orphaned_instructions:true` now covers the `02da..02f8` tail
inside the same row; total still 131 — one row shrank, none added).

### Verdict: 02b7 twin (CONFIRMED-FUNC / NOT-CONFIRMED-at-name, created 2026-09-29, program `/fifa96.exe`)

FUNC verdict — created (flow-clean within the analyzer's own decode;
boundary defended by sole gap occupancy from `02b7`, no fall-through from
`02b6`'s `RET`, and the `PUSHA`↔`POPA`/`RET` structural contract; exit
CITED with bytes: `POPA` `61`@`02f7` + `RET` `c3`@`02f8`; callees: no FUN
callees, sole transfer the dynamic `CALL word ptr [0x9c0]`@`02b8` — runtime
identity deferred, so leaf-or-deferred holds). NAME withheld — the missing
leg is the cell roles of `[0xf52]`/`[0xf54]`: the twin's distinguishing
half is the cursor compute `SI = [0xf52]&0x38 + [0xf54]` (+ the `[0xf52]`
`+8` bump under the byte parse) feeding BOTH the slot stores
(`[SI+2]`/`[SI+4]`/`[SI+7]`, DS=0x8) AND the computed `SS←SI` selector —
8-byte stride + masked index + table-base add is compatible with several
mechanisms (selector/stack table walk, descriptor-table patch, slot
allocator cursor), and a live `audit_global(11bd:0f52)` shows no name, no
type, 0 xrefs — nothing in-program pins the cell identity, so any
mechanism-level name would be lore. Per the create-without-rename middle:
the FUN record stands, the shape is documented in the plate, and
`FUN_11bd_02b7` is RETAINED until a slice names the cells (or the runtime
slice resolves `[0x9c0]`/`[0x40]` context). Plate set
(`C: none — behavioral (mode-switch twin shape: PUSHA + hook CALL [0x9c0]
+ DS/ES 0x20 staging + [0x40] MSW OR-merge + slot-cursor compute
[0xf52]&0x38+[0xf54] with [0xf52]+8 bump + slot stores
[SI+2]/[SI+4]/[SI+7] + computed SS switch + LLDT 0x68 + self POPA/RET;
unnamed pending [0xf52]/[0xf54] cell roles; body truncated at
tool-undecoded 02d4..02d9, tail 02da..02f8 defined but outside body)`).
No direction word is asserted anywhere (UNDECIDED — `### Direction
question` below); no `decode_*`; no caller lore.

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_02b7 | 11bd:02b7 | created: real `disassemble_bytes` 24+1 insns (skip reproduced), `create_function` → body `02b7..02d3` (analyzer nearest-consistent; pocket `02d4..02d9` undefined, tail `02da..02f8` orphaned-in-listing); entry `PUSHA` `60`@`02b7`; hook `CALL [0x9c0]`@`02b8`; `SMSW`/`OR [0x40]`/`LMSW`@`02c3/02c6/02ca`; cursor `MOV SI,[0xf52]`/`AND SI,0x38`@`02cd/02d1`; cited exit `POPA` `61`@`02f7`+`RET` `c3`@`02f8`; 0 xrefs before and after | NOT-CONFIRMED-at-name — `FUN_11bd_02b7` retained; missing leg: cell roles of `[0xf52]`/`[0xf54]` | none — behavioral (plate above) |

### Direction question

Answer to the question slice 13 deferred: **NOT an enter/exit pair as far
as static code shows — both set-only OR; the question moves to the runtime
`[0x40]` writer.** Both bodies merge the same cell with the same op —
sibling `execute_mode_switch`: `SMSW`@`02a7` + `OR AX,word ptr [0x40]`
(`0b064000`)@`02aa` + `LMSW`@`02ae` (cited rows of
`## 0290/0293 fall-through`); twin: `SMSW`@`02c3` +
`OR AX,word ptr [0x40]` (`0b064000`)@`02c6` + `LMSW`@`02ca` (rows above) —
neither carries any `AND`/`ANDNOT` MSW form, so no PE-clearing leg exists
in the pair and the decide-rule ("one sets, the other clears") is NOT met.
`[0x40]`'s runtime value (bit0 = PE target) and its installer identity are
handed to the RUNTIME slice (reads now `{02aa, 02c6}`, direct stores still
0 among defined insns — slice-13 rows re-cited by Task 1). What the twin
DOES show, per its walk rows + this create: selector staging
`MOV AX,0x20`@`02bc` feeding segment stores `MOV DS`@`02bf`/`MOV ES`@`02c1`
(and `MOV DS,0x8`@`02dd/02e0`, `DS←ES`@`02f5/02f6`); the selector-mask
arithmetic `AND SI,0x38`@`02d1` over `[0xf52]`; the cursor `ADD`s at
`02d4/02d8` (`0336540f` / `8306520f08` — byte-evidence parse; the defined
listing instead carries the tool's `PUSH DX`@`02da`+`INVD`@`02db`
alternative, recorded as such); slot stores `[SI+2]/[SI+4]/[SI+7]` with
the computed `SS←SI`@`02ed` and `LLDT 0x68`@`02ef/02f2`. That evidence
characterizes the twin as the STATEFUL half of the pair (table/cursor
mutation + stack switch around the same MSW merge, self-unwinding) — which
is a role observation, not a direction observation: none of it changes the
`MSW ← MSW ∪ [0x40]` semantics, so the enter-vs-exit question remains with
the `[0x40]` writer at runtime.

### Deferrals

Carried/extended from this section's Task-1 rows and the
`## 296d hook target` deferral list: (a) `[0x40]` MSW mask WRITER and
installer identity → runtime slice (reads `{02aa, 02c6}`, stores 0 among
defined insns — the direction question's sole missing leg, as stated
above); (b) `[0x9c0]` hook target identity → runtime (writer set
NOT-IN-EXE per slice-14; the twin's `02b8` is read-site #2, contact-point
evidence only; the twin has NO call targets beyond this dynamic hook — no
second-layer callee question); (c) `2978..2ada` paging-block ownership →
out of slice scope (twin exits at `02f8` inside its own gap; never
entered); (d) NEW: `[0xf52]`/`[0xf54]` cell roles — the naming leg for
`FUN_11bd_02b7` (needs their writer/value evidence; both currently
undefined globals, 0 xrefs; the byte-parse cursor bump at `02d8` is itself
a writer candidate pending the pocket's disposition); (e) NEW: the
`02d4..02d9` decode pocket + orphaned `02da..02f8` tail — a future
listing-disposition item (bytes-authority `ADD`/`ADD` re-decode vs keeping
the tool's `PUSH DX`/`INVD` parse; this slice deliberately did not
hand-force per the no-fight rule — see `### Writes`). Leaf verdicts
elsewhere: none new surfaced — the twin's only transfers and cell
contacts were already tabled by Task 1.

## 02b7 twin completion (verified 2026-09-29, program `/fifa96.exe`)

Summary. Cell roles: **PINNED, both cells** — neither `[0xf52]` nor
`[0xf54]` is twin-only, so the THIN condition in the `## 02b7 twin`
verdict row ("twin-only contacts") is NOT met and the naming leg's
missing fact is now supplied: `[0xf54]` is the BASE of an 8-byte-stride
slot table and `[0xf52]` is a CURSOR into it. The base is written by a
non-twin site — `MOV word ptr [0xf54],AX` at `11bd:5812` (`FUN_11bd_5686`),
AX from `CALL 0x1000:579c` (= `11bd:3bcc`, called with count `0x1c` at
`5807..580e`, whose body walks slots at stride 8 — `MOV BX,0x8`@`3bea`,
`MOV SI,word ptr ES:[BX]`@`3bef`, `SHR SI,0x3`@`3bf2`, and `SHL BX,0x3`
at `3c0d`/`3c2c`/`3c57`/`3c6e` as the slot-address step) — and is then
scaled IN PLACE
by `SHL word ptr [0xf54],0x3` at `584b`, i.e. stored as a slot COUNT and
converted to a byte offset by the same ×8 the twin's arithmetic uses; the
only other `[0xf54]` contacts are reads at `581f`/`583b`. The cursor
`[0xf52]` has TWO non-twin writers plus a save/restore pair, all with the
same ±8 stride the twin's `AND SI,0x38` mask bounds: `SUB word ptr
[0xf52],0x8` at `1991:057a` (bytes `832e520f08`, DS←`0x20` staged at
`1991:056c/056e`) and `POP word ptr [0xf52]` at `1991:0d23` (store),
paired with `PUSH word ptr ES:[0xf52]` at `1991:0cc9` (read) inside
`FUN_1991_0c9e`, a stack-switch handler that saves the cursor across a
`REP MOVSW` frame copy and restores it before `IRETD` — and that handler
touches the same family of cells (`ES:[0x996]`/`ES:[0x99e]`, the stack
slot pointers at `1991:0cb5`/`0cb0`). So the role reading is: allocate a
block of 8-byte slots (`3bcc`), scale the handle to a byte offset
(`[0xf54]`), then walk it with a cursor (`[0xf52]`) that one consumer
advances +8 and another decrements −8, saving/restoring it around a stack
switch. That is the "base+cursor pair written by X" shape, and it makes a
mechanism-level name for `FUN_11bd_02b7` decidable from evidence rather
than lore (the rename itself is NOT performed here — this pass is
read-only). Two hardening residues are named below; neither re-opens the
THIN test, because both concern VALUE knowledge and segment-alias
bookkeeping, not contact sets. Pocket: **tool parse UNCHANGED** — every
dry-run window re-emitted slice-15's behavior verbatim (nothing for
`02d4..02d9`, `PUSH DX`@`02da`, `INVD`@`02db`, resume `02dd`), the raw
bytes still read `0336540f8306` with zero diff against the map quote, and
the byte-authority `ADD`/`ADD` parse is now INDEPENDENTLY CORROBORATED:
both pocket encodings recur verbatim against these same two cells
elsewhere in the image (`832e520f08` at `1991:057a` is the same `83 /ib +
disp16 [0xf52]` shape with only the `/digit` changed; `0306540f` at
`11bd:583b` is the same `03 /r + disp16 [0xf54]` shape), so the adopted
parse is a live instruction form, not a hand-fitted reading.
Method caveat, stated once and applying to every negative below:
`search_instructions` scans DEFINED instructions only (14006 per run,
reported in every response), and the whole twin region `11bd:02d4..02f8`
is partly undecoded, so operand searches cannot see pocket/`1991:057a`
bytes — the two raw-byte scans (`520f`, `540f`) are the coverage
authority here and are the reason each negative row lists the searches
that ran. Second caveat: `get_xrefs_to` is dead for these near-data
operands — all five cells return `count:0`, and control probes on
known-referenced cells confirm the artifact (`get_xrefs_to(11bd:0040)` →
0 refs despite the `02aa`/`02c6`/`2a6c` readers;
`get_xrefs_to(11bd:098e)` → 0 refs despite `56ef` writing it), so
xref counts are reported but are NOT evidence of contact absence;
instruction enumeration is. `audit_global` on all five cells: identical
blank state — `name:""`, `type:""`, `length:0`, `plate_comment:""`,
`xref_count:0`, issues `["generic_name","untyped","missing_plate_comment"]`
(3 hard).

| cell | search run | hits (addr, mnemonic) | role reading |
|------|-----------|----------------------|--------------|
| `[0xf54]` (BASE) | `search_instructions` operand `0xf54` → 4; operand `f54` → 5; `0xf5` → 12 (family superset); `get_xrefs_to(11bd:0f54)` → `{"references":[],"count":0,"total":0}`; `audit_global(11bd:0f54)` → blank; raw `search_byte_patterns 540f` → 5 (`02d6`,`5813`,`5820`,`583d`,`584d`); far forms `540fbd11` → none | `11bd:5812` `MOV [0xf54],AX` (a3540f) — WRITE, sole non-twin writer, AX = `CALL 0x1000:579c`(`11bd:3bcc`, arg `0x1c`@`580d`) result; `11bd:581f` `MOV AX,[0xf54]` (a1540f) — READ; `11bd:583b` `ADD AX,word ptr [0xf54]` (0306540f) — READ; `11bd:584b` `SHL word ptr [0xf54],0x3` (c126540f03) — READ+WRITE (in-place ×8 = slot-count → byte offset, same stride as the twin's `AND 0x38`/`+8`); `11bd:02d4` `ADD SI,word ptr [0xf54]` (`0336540f`, byte-evidence parse) — READ by the twin, inside the undecoded pocket (invisible to the operand search; raw hit `540f`@`02d6`). FALSE HIT rejected: `11bd:47d6` `MOV AX,word ptr [BP + 0xff54]` — frame-relative disp8-negated lookalike, no absolute operand | slot-table BASE, allocated+counted by `FUN_11bd_5686` through the stride-8 walker `11bd:3bcc`, consumed by the twin as the block base |
| `[0xf52]` (CURSOR) | operand `0xf52` → 3; operand `f52` → 4; operand `0xf52]` → 3; `get_xrefs_to(11bd:0f52)` → `count:0`; `audit_global(11bd:0f52)` → blank; raw `search_byte_patterns 520f` → 5 (`02cf`,`02da`,`1991:057c`,`1991:0ccc`,`1991:0d25`); far form `520fbd11` → none | `11bd:02cd` `MOV SI,word ptr [0xf52]` (8b36520f) — READ, twin (defined); `11bd:02d8` `ADD word ptr [0xf52],0x8` (`8306520f08`) — WRITE (+8 advance), twin, INSIDE the undecoded pocket (raw hit `520f`@`02da`); `1991:057a` `SUB word ptr [0xf52],0x8` (`832e520f08`) — WRITE (−8 retreat), UNOWNED bytes (`get_function_by_address(1991:057a)` → no function), reached with DS←`0x20` (`1991:056c/056e`), so NOT visible to any operand search; `1991:0d23` `POP word ptr [0xf52]` (8f06520f) — WRITE (restore), `FUN_1991_0c9e`; `1991:0cc9` `PUSH word ptr ES:[0xf52]` (26ff36520f) — READ (save), `FUN_1991_0c9e`. FALSE HIT rejected: `11bd:337d` `JMP 0x1000:4f52` (bytes `eb03`) — branch-target text match, not a memory operand | 8-byte-stride CURSOR into the `[0xf54]` block: `+8` consumer (twin, to publish a slot), `−8` consumer (`1991:057a`, to release/roll back), `PUSH`/`POP` save-restore around a stack switch — the `AND SI,0x38` mask bounds it to 8 slots |
| `[0xf50]` (neighbor, SP save) | operand `0xf50` → 5; `get_xrefs_to(11bd:0f50)` → `count:0`; `audit_global(11bd:0f50)` → blank; raw `500f` → 21 = the 5 defined contacts' disp16 bytes (`6021`,`62e0`,`6e78`,`7297`,`7316`) + 9 contacts in
undecoded bytes (`001d`,`0bbc`,`0cf2`,`1367`,`1472`,`1491`,`1592`,`16f6`,`1991:09bc`) + 7 straddle artifacts
(`295e`,`2982`,`2a94`,`2a97`,`2cdf`,`6b8f`,`720b`) | DEFINED (the 5 the operand search found): `11bd:601f` `MOV SP,word ptr [0xf50]` — READ (`FUN_11bd_601d`, 6-insn stack-pop tail); `11bd:62de` `MOV word ptr [0xf50],SP` — WRITE (`run_postload_init`); `11bd:6e77` `MOV AX,[0xf50]` — READ (`FUN_11bd_6e20`); `11bd:7295` `MOV word ptr [0xf50],SP` — WRITE and `11bd:7314` `MOV word ptr [0xf50],BP` — WRITE (`FUN_11bd_7290`, DPMI-ish IRET frame builder at `730a..731c`: `MOV SS,AX`/`LEA SP,[DI+-0x6]`/`MOV [0xf50],BP`/`IRET`). UNDECODABLE-BY-SEARCH contacts (raw `500f` + dry-run, no function per `get_function_by_address`): READ `MOV SP,[0xf50]` at `11bd:001a` (bytes `368b26500f`, SS-override form, in the `11bd:0000` stub), `0bba`, `0cf0`, `1365`, `148f`, `1590`, `16f4` and `1991:09ba`; WRITE `MOV [0xf50],SP` at `11bd:1470`. FALSE/STRADDLE rejected: `295d`/`2982`/`2cdf` (`6650 PUSH EAX` + `0f20` CR-read), `2a94`/`2a97` (`PUSH AX` + `PUSH GS`/`PUSH FS`), `6b8f` (`PUSH ES:AX`), `720b` (inside `ROR byte ptr [BP+DI+0x5026],0x1`); ADDRESS-only: `11bd:033d` `MOV DI,0xf56` / `0369` `MOV SI,0xf56` use `0xf56` as an address immediate | saved-`SP` cell of the stack-switch state block — the cluster `0xf50/0xf52/0xf54` is stack/slot machinery, which is exactly the context the twin's `MOV SS,SI`@`02ed` plugs into; the raw scan also shows this cell has 8 contacts the defined-only search could NOT see, so "5 hits" understates the family and the caveat is demonstrated on a cell whose whole accessor set is visible |
| `[0xf56]` (neighbor) | operand `0xf56` → 0 matches (14006 defined insns); `get_xrefs_to(11bd:0f56)` → `count:0`; `audit_global(11bd:0f56)` → blank; raw `560f` → 4 (`033e`,`036a`,`28e5`,`2a68`) | NEGATIVE on defined-instruction evidence: the searches that ran are operand `0xf56` (0), operand `0xf5` (12, none at `[0xf56]`), and `get_xrefs_to` (0 refs, xref channel dead per caveat). Raw scan adds what the operand search structurally cannot see: `11bd:28e4` `MOV AX,[0xf56]` (`a1560f`) — READ, in the `2811..2ada` deferred block (unowned: `get_function_by_address(11bd:28e4)` → no function); `11bd:033d`/`0369` treat `0xf56` as an ADDRESS (4-byte save/restore pair with `0x467`); FALSE HIT rejected: `11bd:2a68` — the bytes straddle `6656` `PUSH ESI`@`2a67` + `0f01e1` `SMSW CX`@`2a69` | contact exists, but in unowned/deferred bytes and as an address constant — no role claim; per scope note, cited as context, not widened |
| `[0xf58]` (neighbor) | operand `0xf58` → 0 matches (14006 defined insns); `get_xrefs_to(11bd:0f58)` → `count:0`; `audit_global(11bd:0f58)` → blank; raw `580f` → 1 (`11bd:28ec`) | NEGATIVE on defined-instruction evidence (searches that ran: operand `0xf58` → 0, operand `0xf5` → 12 with no `[0xf58]` mention, `get_xrefs_to(11bd:0f58)` → 0 refs — xref counts meaningless per the control probes). One raw-scan hit: disp16 bytes `58 0f` at `11bd:28ec`, i.e. `MOV AX,[0xf58]` (`a1580f`) at `28eb..28ed` — READ, in unowned bytes (`get_function_by_address(11bd:28e4)` → no function), immediately preceded in the same dry-run window by the `[0xf56]` read at `28e4` and followed by `MOV ES:[0x469],AX` at `28ee`; the value is copied out to `ES:[0x467]`/`ES:[0x469]` (`28e7`/`28ee`, `26a3 6704`/`26a3 6904`). CAVEAT on this reading: the `28d8..28f2` dry-run emit itself skips `28e7..28eb` and resyncs at `28ec` (a SECOND instance of the `02d4` skip-and-resync pathology), so the `28eb` instruction is my aligned decode of the raw dump (`28eb=a1 28ec=58 28ed=0f`), not a tool emit — out-of-scope deferred-block material, flagged not adopted | `[0xf56]`+`[0xf58]` is the 4-BYTE unit (copied as a pair to `0x467`, read back as two words) — contrast the assignment's `32-bit descriptor-ish pair` question: `[0xf52]`/`[0xf54]` are NEVER accessed as a dword (every enumerated operand is a word op), so the pair structure is base+cursor, not a single 32-bit field |
| `[0x40]` (mask, re-cite) | operand `0x40]` → 8; `11bd:2a6c` NOT among them (undefined bytes — it surfaced only in the `disassemble_bytes` dry-run window `2a60..2a74`, which I ran to classify the `560f` raw hit at `2a68`) | Reads `11bd:02aa` (`execute_mode_switch`) and `11bd:02c6` (twin) — both `OR AX,word ptr [0x40]` `0b064000`; NEW read sighting `11bd:2a6c` `MOV AX,[0x40]` (`a14000`, `NOT AX`/`AND AX,CX`/`LMSW` at `2a6f/2a71/2a73`, unowned bytes in the deferred `2811..2ada` block — that sequence is an MSW CLEAR path, the form slice 15 said is absent from the twin/sibling pair). FALSE HITS rejected (frame-relative `0xffc0`-style lookalikes): `11bd:3286`/`3292`/`32a2` `LEA AX,[BP+-0x40]`, `3433` `MOV word ptr [BP+-0x40],0x50`, `35e9`/`3600` `MOV ES,word ptr [BP+-0x40]` | re-cited only: reader set grows to {`02aa`,`02c6`,`2a6c`} (the last from a dry-run window, not from the operand search), direct stores still 0 among defined insns — the `[0x40]` writer stays the runtime slice's missing leg (`## 02b7 twin` deferral (a) unchanged); the `2a6c..2a73` AND-NOT-`[0x40]`+`LMSW` shape belongs to the deferred paging block, flagged for that slice, not adopted here |

| Pocket item | Evidence (this pass, verbatim) | Result |
|-------------|-------------------------------|--------|
| raw bytes `11bd:02d4..02d9` | `read_memory(11bd:02d4,6)` → `{"address":"11bd:02d4","length":6,"data":[3,54,84,15,131,6],"hex":"0336540f8306"}` | IDENTICAL to the map's quoted `0336540f8306` — zero diff |
| pocket + seam context | `read_memory(11bd:02d4,12)` → `0336540f8306520f08b80800` (`0x08`@`02dc`, `b8 08 00`@`02dd..02df` = `MOV AX,0x8`) | slice-15's "+`08`@`02dc`" quote re-confirmed; the 6-byte pocket boundary `02d9`/`02da` falls mid-instruction under the adopted parse |
| tool behavior, pocket alone | `disassemble_bytes` `dry_run=true` `11bd:02d4..02d9` → `{"dry_run":true,"success":true,"start_address":"11bd:02d4","end_address":"11bd:02d8","bytes_disassembled":5,"message":"Successfully disassembled 5 byte(s)","instructions":[],"instructions_total":0,"truncated":false}` | EMITS NOTHING (and clamps the echoed end to `02d8`) — unchanged from slice 15's `instructions_total:0` finding |
| tool behavior, left seam | `disassemble_bytes` `dry_run=true` `11bd:02cf..02e3` → `AND SI,0x38`@`02d1` (`83e638`), `PUSH DX`@`02da` (`52`), `INVD`@`02db` (`0f08`), `MOV AX,0x8`@`02dd` (`b80800`), `MOV DS,AX`@`02e0` (`8ed8`), `MOV word ptr [SI + 0x2],DI`@`02e2` (`897c02`) — `instructions_total:6` | The pocket is SKIPPED, not mis-parsed in place: the tool jumps `02d3 → 02da`, leaving `02d4..02d9` undefined, then resumes at `02dd`. `disassemble_bytes` `dry_run=true` `11bd:02d2..02d8` → `instructions:[],"instructions_total":0` (second emptiness probe) |
| adopted parse, field-level | `02d4` `03 /r` `ADD r16,r/m16` + ModRM `36` = mod`00` reg`110`(SI) rm`110` → disp16 `540f` = `[0xf54]` → `ADD SI,[0xf54]` (4 bytes `02d4..02d7`); `02d8` `83 /0 ib` `ADD r/m16,imm8` + ModRM `06` = mod`00` reg`000`(/0 = ADD) rm`110` → disp16 `520f` = `[0xf52]`, imm8 `08` → `ADD word ptr [0xf52],0x8` (5 bytes `02d8..02dc`) | Consumes all 9 bytes `02d4..02dc` and lands on `02dd` = the tool's own resume point (slice-15 convergence re-confirmed) |
| tool parse, field-level | `02da` `52` = `PUSH DX` (opcode-only, no ModRM, reg field `010` = DX); `02db` `0f 08` = `INVD` (2-byte opcode, no ModRM) | bytes `52 0f 08`@`02da..02dc` are read as two instructions; under the adopted parse the SAME bytes are disp16-lo/`0f`-hi/imm8 of the `ADD [0xf52],0x8` |
| agree/diverge | Both parses: `02d1` `AND SI,0x38` and everything from `02dd` onward. Divergence is exactly `02d4..02d9` (tool: undefined; adopted: `ADD SI,[0xf54]`) plus the byte-level ownership of `02da..02dc` | Agrees with slice 15's "converge at `02dd`" statement word for word; nothing about the tool's emit changed |
| NEW corroboration | `832e520f08` (`SUB word ptr [0xf52],0x8`) at `1991:057a` and `0306540f` (`ADD AX,word ptr [0xf54]`) at `11bd:583b` | Both pocket encodings are live, repeated forms aimed at these same two cells — the adopted parse needs no hand-forcing argument beyond that, on top of the frame-balance finding (`PUSH DX` unbalanced against `POPA`/`RET`@`02f7/02f8`) recorded in `## 02b7 twin` |

Hardening residue (named; does not re-open the PINNED test): (1) cell
VALUES — the static bytes under `11bd:0f4e..0f6d` are a run of
`CALL 11bd:0f6e` stubs (`e81d0000`,`e8190000`,`e8150000`,…; the target
`0f6e` is an unowned stack-save stub, `CLI`/`PUSH DS`/`MOV DI,0x1000`/
`ES:[0x996]` walk at `0f6e..0f89`), so no static initial value is
readable for `[0xf52]`/`[0xf54]`/`[0xf56]`/`[0xf58]`, and `audit_global`
finds no data definition at any of them — the runtime slice's value
snapshot (the `[0x40]`/`[0x9c0]`-style deferral) is what would turn the
role claim into a value-confirmed one; (2) segment-alias bookkeeping —
the `1991:` sites are offset-nominal relative to their own block, and
same-runtime-cell identity with the `11bd:`-rendered cells rests on the
observed DS/ES staging (`1991:056c/056e` `PUSH 0x20`/`POP DS`;
`ES←CS:[0x5680]` at `1991:0ca9`, and `CS:[0x5680]` itself written only by
`MOV word ptr [0x5680],SS` at `11bd:7048`) plus the shared
`[0x996]`/`[0x99e]`/`[0xf50]` cluster, not on a static symbol; and (3)
disposition — adopting `ADD`/`ADD` over the listing's `PUSH DX`/`INVD` is
still the un-run `### Writes` item (deferral (e)), and `[0xf52]`'s `+8`
writer stays inside that pocket, so the cursor's `+8` leg is byte-evidence
until the pocket is re-decoded while its `−8` and save/restore legs are
already defined/listing-visible. Contact-set facts (both cells non-twin,
writer identities as cited) are independent of all three.

Zero Ghidra writes this task: `disassemble_bytes` exclusively
`dry_run=true` (23 windows, no write-mode call made at all), plus
`read_memory`, `inspect_memory_content`,
`search_instructions` (operand patterns run, with hit counts: `0xf50` 5,
`0xf52` 3, `0xf54` 4, `0xf56` 0, `0xf58` 0, `f52` 4, `f54` 5, `0xf5` 12,
`0xf52]` 3, `0xa87` 3, `0x996` 30, `0x99e` 5, `0x5680` 3, `0x40]` 8),
`search_byte_patterns` (`520f` 5, `540f` 5, `560f` 4, `580f` 1, `500f`
21, `520fbd11`/`540fbd11`/`bd11520f` none), `get_xrefs_to` (five cells +
two controls), `audit_global` (five cells), `get_function_by_address`,
`disassemble_function`. No rename, no plate, no data definition, no
flow/listing change, no `save_program`; `FUN_11bd_02b7` still
body `11bd:02b7..02d3`, pocket still undefined, tail still
defined-but-outside-body; `/media/felipe/FIFAPCCD/` untouched.

### Pocket repair (executed 2026-09-29, Task 2, program `/fifa96.exe`)

Pre-state re-confirmed live immediately before any write:
`get_function_by_address(11bd:02b7)` →
`{"name":"FUN_11bd_02b7","address":"11bd:02b7","signature":"undefined2 FUN_11bd_02b7(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}`;
plate `get_comment(11bd:02b7)` → slice-15 plate read back verbatim
(quoted in the `### Name disposition` before-row); gap row
`{"start":"1000:1ea4","end":"1000:2302","size":1119,"has_undefined_bytes":true,"has_orphaned_instructions":true,"before_function":"FUN_11bd_02b7","before_function_address":"11bd:02b7","after_function":"FUN_11bd_0733","after_function_address":"11bd:0733"}`
(total 131) — all identical to the `## 02b7 twin` `### Writes`
post-state. Two sanctioned paths only, per brief Step 1; no third path
invented; `02f9+` and the `2978..2ada` block never touched.

| Step | Command as run | Tool response (verbatim) | Reading |
|------|----------------|--------------------------|---------|
| real pocket disassembly | `disassemble_bytes` start `11bd:02d4` length `6` (no dry_run — the sanctioned real write) | `{"success":true,"start_address":"11bd:02d4","end_address":"11bd:02d9","bytes_disassembled":6,"message":"Successfully disassembled 6 byte(s)","instructions":[],"instructions_total":0,"truncated":false}` | The skip is reproduced in WRITE mode: 6 bytes "disassembled", ZERO instructions emitted, exact range echoed `02d4..02d9` (no clamp this time). Post-run `read_memory(11bd:02d4,6)` → `{"address":"11bd:02d4","length":6,"data":[3,54,84,15,131,6],"hex":"0336540f8306"}` — bytes intact, pocket still undefined in effect |
| bounds re-read #1 | `get_function_by_address(11bd:02b7)` | `{"name":"FUN_11bd_02b7","address":"11bd:02b7","signature":"undefined2 FUN_11bd_02b7(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}` | NOT joined (refusal #1: the analyzer did not re-flow the body over the pocket) |
| sanctioned nudge | `create_function` at `11bd:02b7`, `disassemble_first=false` | `{"error":"Function already exists at 11bd:02b7: FUN_11bd_02b7"}` | Refused against the existing function object; no new object created ⇒ nothing of mine to delete (delete-nothing rule satisfied trivially) |
| bounds re-read #2 | `get_function_by_address(11bd:02b7)` | `{"name":"FUN_11bd_02b7","address":"11bd:02b7","signature":"undefined2 FUN_11bd_02b7(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}` | Refusal #2 ⇒ disposition final |
| post-state tool re-probe | `disassemble_bytes` `11bd:02d4..02d9` `dry_run=true` | `{"dry_run":true,"success":true,"start_address":"11bd:02d4","end_address":"11bd:02d8","bytes_disassembled":5,"message":"Successfully disassembled 5 byte(s)","instructions":[],"instructions_total":0,"truncated":false}` | Tool state unchanged from before (same skip + `02d8` clamp as the Task-1 quote) |
| tail state | `get_function_by_address(11bd:02da)` | `{"error":"No function found for 11bd:02da"}` | Tail `02da..02f8` still defined-but-outside any body — the orphan stands |

Disposition: **RATIFIED-TRUNCATION** (not JOINED). Reason the map keeps
the tail orphaned: both sanctioned paths were exhausted and each was
answered by the analyzer's own output — the real pocket disassembly
emitted zero instructions (the skip is a write-mode tool behavior, not a
dry-run artifact), and the single `create_function` nudge refused against
the existing object. Per the no-fight rule the byte-evidence `ADD`/`ADD`
parse (`0336540f`/`8306520f08`) stays recorded-but-unapplied, exactly the
slice-15 posture; the full confirmed boundary `02b7..02f8` with the
`PUSHA`↔`POPA`/`RET` contract remains defended in the `## 02b7 twin`
verdict row, which this section supersedes on disposition only (deferral
(e) of that section is hereby dispositioned: ratified-truncated, pocket
left undefined by tool refusal, tail left orphaned by analyzer refusal —
no hand-forcing). Residue (3) of this section's summary therefore stands
unchanged: `[0xf52]`'s `+8` writer (and `[0xf54]`'s base-add) stay
byte-evidence-in-pocket.

### Name disposition (PINNED — executed 2026-09-29, Task 2, program `/fifa96.exe`)

Rename (verb-led snake_case, mechanism-level):
`FUN_11bd_02b7` → **`write_slot_from_cursor`** — `rename_function` →
`{"status":"success","message":"Success: Renamed function at FUN_11bd_02b7 from 'FUN_11bd_02b7' to 'write_slot_from_cursor'","warnings":["Function name 'write_slot_from_cursor' — main part 'write_slot_from_cursor' is not PascalCase. Expected: WriteSlotFromCursor","Function name 'write_slot_from_cursor' — main part 'write_slot_from_cursor' contains underscores. Use PascalCase after the module prefix."]}`
(warnings are the tool's PascalCase style default; the repo convention
is snake_case — cf. `clear_slot_entries`, `publish_mode_vector`,
`execute_mode_switch` — so the name stands). Collision check before
renaming: `search_functions` `slot` → only `clear_slot_entries @
11bd:1df7`; `cursor` → 0 functions.

| Item | Before (slice-15 state) | After (post-write read-back) |
|------|--------------------------|------------------------------|
| name | `FUN_11bd_02b7` (`get_function_by_address` quote in `### Pocket repair` pre-state) | `write_slot_from_cursor` — `{"name":"write_slot_from_cursor","address":"11bd:02b7","signature":"undefined2 write_slot_from_cursor(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}` |
| plate | `C: none — behavioral (mode-switch twin shape: PUSHA + hook CALL [0x9c0] + DS/ES 0x20 staging + [0x40] MSW OR-merge + slot-cursor compute [0xf52]&0x38+[0xf54] with [0xf52]+8 bump + slot stores [SI+2]/[SI+4]/[SI+7] + computed SS switch + LLDT 0x68 + self POPA/RET; unnamed pending [0xf52]/[0xf54] cell roles; body truncated at tool-undecoded 02d4..02d9, tail 02da..02f8 defined but outside body)` | `C: none — behavioral (slot-table cursor consumer: reads [0xf52] cursor, bounds to 8 slots via AND SI,0x38, then [0xf54]-base add and [0xf52]+8 bump byte-evidenced in pocket 02d4..02d9, writes slot fields [SI+2]/[SI+4]/[SI+7], computed SS←SI, LLDT 0x68, self POPA/RET; cells PINNED non-twin-only ([0xf54] base written at 11bd:5812 and scaled x8 at 584b, [0xf52] cursor with -8 consumer at 1991:057a and save/restore at 1991:0cc9/0d23 — the 1991 contacts are offset-nominal to their own segment, same-runtime-cell identity rests on the DS/ES←0x20 staging not a static symbol); name safe under either pocket parse (no advance/base-add asserted as listing fact); body truncated at tool-refused 02d4..02d9, tail 02da..02f8 defined but outside body)` — `set_comment` → `{"status":"success","message":"Set plate comment at 11bd:02b7","warnings":["Plate comment missing Algorithm section","Plate comment missing Parameters section","Plate comment missing Returns section"]}` (boilerplate warnings only, same as prior slices' behavioral plates); `get_comment(11bd:02b7)` read-back verbatim = the text in this cell |

Name rationale (PINNED branch; mechanism from the body + pinned roles,
no direction words, no caller lore): the body reads the CURSOR
(`MOV SI,word ptr [0xf52]`@`02cd`, defined) and bounds it to the
8-slot window the BASE table uses (`AND SI,0x38`@`02d1`, defined); the
pocket byte-parse adds the base (`ADD SI,[0xf54]`@`02d4`) and advances
the cursor (`ADD word ptr [0xf52],0x8`@`02d8`); the stores publish slot
fields (`[SI+2]`/`[SI+4]`/`[SI+7]`, defined in the tail). Because the
adopted parse never joined the body (RATIFIED above), the `+8` and
base-add legs remain byte-evidence-in-pocket — so the name asserts only
what is listing-visible under EITHER pocket parse: consult the cursor,
write the slot (`write_slot_from_cursor`). "write"/"slot"/"cursor" are
mechanism vocabulary; no enter/exit/switch-mode word appears (the
direction question stays UNDECIDED per `## 02b7 twin` `### Direction
question`); no hook/`[0x9c0]` caller identity enters the name (its writer
set is still NOT-IN-EXE, deferral (b)).

Carried condition (residue (2) of this section's summary, recorded in the
plate too): the `[0xf52]` −8/save/restore contacts live in the `1991:`
segment and are **offset-nominal to their own segment** — same-runtime-cell
identity with the `11bd:`-rendered cells rests on the observed DS/ES
staging (`PUSH 0x20`/`POP DS`@`1991:056c/056e`;
`ES←CS:[0x5680]`@`1991:0ca9`) plus the shared `[0x996]`/`[0x99e]`/`[0xf50]`
cluster, NOT on a static symbol. If the runtime slice shows selector
`0x20` does not base to the `11bd` block, the cursor's non-twin-writer
leg weakens back toward THIN and this name must be re-examined.

Verdict-leg update (by reference, not rewritten): the slice-15
`### Verdict` row's `NOT-CONFIRMED-at-name` leg — "missing leg: cell
roles of `[0xf52]`/`[0xf54]`" — **is closed on the Task-1 contact
enumeration**: `[0xf54]` = slot-table BASE (sole writer `11bd:5812`
`MOV [0xf54],AX` from the stride-8 walker `11bd:3bcc`, scaled in place
`SHL [0xf54],0x3`@`584b`), `[0xf52]` = 8-stride CURSOR (non-twin −8
writer `1991:057a`, save/restore `1991:0cc9`/`0d23`) — neither cell is
twin-only, so the THIN condition fails and the rename above executes.
Deferral (d) of `## 02b7 twin` is thereby consumed; deferrals (a)/(b)/(c)
stand untouched.

### Post-state (Task 2)

`save_program(/fifa96.exe)` →
`{"success":true,"program":"fifa96.exe","message":"Program saved successfully"}`.
Final quotes: function `{"name":"write_slot_from_cursor","entry_point":"11bd:02b7","body_start":"11bd:02b7","body_end":"11bd:02d3"}`
(bounds unchanged — RATIFIED-TRUNCATION; rename does not re-flow);
plate = the after-cell quoted above, read back verbatim; pocket still
undefined (`read_memory` = `0336540f8306`); tail `02da..02f8` still
orphaned (`get_function_by_address(11bd:02da)` → `{"error":"No function
found for 11bd:02da"}`); `find_code_gaps` twin row reflowed to name only:
`{"start":"1000:1ea4","end":"1000:2302","size":1119,"has_undefined_bytes":true,"has_orphaned_instructions":true,"before_function":"write_slot_from_cursor","before_function_address":"11bd:02b7","after_function":"FUN_11bd_0733","after_function_address":"11bd:0733"}`,
total still 131. Writes this task: the real `disassemble_bytes(11bd:02d4,
length 6)`, one `create_function` nudge (refused, no effect),
`rename_function`, `set_comment`, `save_program` — nothing else;
`/media/felipe/FIFAPCCD/` untouched.

## paging block 2978..2ada (verified 2026-09-29, program `/fifa96.exe`)

Read-only attribution pass over the paging block — the right half of the
original hole `11bd:2811..2ada`, split by slice-14's
`restore_fs_gs_and_resume` create (`296d..2977`) into the skip-pocket
`2811..296c` (out of scope) and this block `2978..2ada` (355 bytes).
Result: **ZERO attributed entries.** Every `CALL`/`JMP` operand pattern
run against the 14006 defined instructions resolves either to the wrong
region (code-space `29xx`/`2axx` renderings = `11bd:0dxx`/`0ef4`, since
far/near targets print in the `0x1000:` code space and the block renders
as `0x1000:4548..46aa`) or to addresses ≥ `11bd:2adb` in/after the next
function; the in-range code-space windows (`0x1000:45`, `0x1000:46`)
produce 0/8 hits with every hit recomputed out-of-range. The `[0x9c2]`
writer set is unchanged (`41ee` sole writer, immediate `0x296d` — the
epilogue, NOT in this block); no absolute-cell store with an immediate in
`2978..2ada` exists among defined instructions; raw far-pointer pairs
for the three candidate entries (`2978`, `2a30`, `2a6c`) are 0-hit
program-wide, including inside undefined holes. The block head byte at
`2978` (`803efe0d01` = `CMP byte ptr [0xdfe],0x1`, slice-14 citation) and
the MSW-clear locator at `2a6c` (`a14000 f7d0 23c1` = `MOV AX,[0x40]` /
`NOT AX` / `AND AX,CX`, `0f` opening `LMSW` at `2a73`, slice-16 flag)
were re-confirmed by raw `read_memory` and remain UNDEFINED. With zero
attributed entries there are no walks: `disassemble_bytes` was not
invoked at all this pass. Caveats standing: `search_instructions` is
defined-instruction-only (14006 scanned) so intra-gap relative flow
cannot surface, and `get_xrefs_to` is dead for these operand forms
(control probes on `2978`/`2a6c` return 0 like every other cell) —
instruction enumeration + raw-byte scans are the authority, and the
entry mechanism therefore remains dynamic or gap-internal (e.g. fall-in
from the `2811..296c` pocket's undecoded flow), unresolved. Quote protocol:
read_memory `hex` fields must be reconciled against the response's `data`
array or disassembly before being quoted verbatim; an 8-byte render glitch
(this section, found at review) produced `23c8` where bytes are `23c1`.

| Gap-state item | Evidence (verbatim) | Boundary reading |
|----------------|---------------------|------------------|
| covering row | `find_code_gaps` (total 131, page offset 0/limit 100): `{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` | `1000:4548 − 0x1bd0 = 11bd:2978` ✓ start, `1000:46aa − 0x1bd0 = 11bd:2ada` ✓ end, size 355 — matches the slice-14 post-split expectation (`1000:4548..1000:46aa`) |
| left pocket row | `{"start":"1000:43e1","end":"1000:453c","size":348,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_27e4","before_function_address":"11bd:27e4","after_function":"restore_fs_gs_and_resume","after_function_address":"11bd:296d"}` | `1000:453c − 0x1bd0 = 11bd:296c` — pocket ends exactly one byte before the block; split confirmed |
| left neighbor | `get_function_by_address(11bd:2977)` → `{"name":"restore_fs_gs_and_resume","entry_point":"11bd:296d","body_start":"11bd:296d","body_end":"11bd:2977"}` | block starts at end+1 ✓ |
| pocket edge byte | `get_function_by_address(11bd:296c)` → `{"error":"No function found for 11bd:296c"}` | pocket `2811..296c` still unowned ✓ |
| right neighbor | `get_function_by_address(11bd:2adb)` → `{"name":"FUN_11bd_2adb","body_start":"11bd:2adb","body_end":"11bd:2ae8"}` | block ends at start−1 ✓; `0x1000:46ab − 0x1bd0 = 2adb` corroborated live by the `7744`/`792b` CALLs below |
| head byte | `read_memory(11bd:2978,5)` → `803efe0d01` | `CMP byte ptr [0xdfe],0x1` — slice-14 quote byte-exact, still undefined listing |
| MSW-clear locator | `read_memory(11bd:2a6c,8)` → `a14000f7d023c10f` | `MOV AX,[0x40]`@`2a6c` + `NOT AX`@`2a6f` + `AND AX,CX`@`2a71` + `0f`(@`2a73`, `LMSW` opener) — slice-16 flag byte-exact, still undefined, still not adopted |

| Search run | Hits (addr, mnemonic → target) | Verdict |
|------------|--------------------------------|---------|
| `search_instructions` operand `0x1000:29` | 12: `0d38` JNZ→`292c`, `0d60` JMP→`2926`, `0d7a` JZ→`294f`, `0d8d`/`0d95`/`0d9b` CALL→`2932` ×3, `0d90`/`0d9e` JNZ→`297c` ×2, `0dba` JNZ→`29a3`, `0e3c` CALL→`2950`, `0eaf` CALL→`2982`, `11c2` CALL→`2950` | all targets −`0x1bd0` = `11bd:0d5c/0d56/0d7f/0d62/0dac/0dd3/0d80/0db2` — every one lands in `11bd:0dxx`, OUT of `2978..2ada` (enumerated pattern hits the wrong region: code-space `29xx` ≠ `11bd:29xx`) |
| `search_instructions` operand `0x1000:2a` | 7: `699c`/`72c7`/`77bc`/`798a`/`7a02`/`7a26` CALL + `69b2` JMP, all →`0x1000:2ac4` | `2ac4 − 1bd0 = 11bd:0ef4` (`FUN_11bd_0ef4`) — OUT of range ×7 |
| `search_instructions` operand `11bd:29` | 0 | negative (no `11bd:29xx` segment-form renderings) |
| `search_instructions` operand `11bd:2a` | 0 | negative |
| `search_instructions` operand `:296` | 0 | negative |
| `:297` | 2: `0d90`/`0d9e` JNZ→`0x1000:297c` | `→11bd:0dac` OUT (same insns as `0x1000:29` run) |
| `:298` | 1: `0eaf` CALL→`0x1000:2982` | `→11bd:0db2` OUT |
| `:299` `:29b` `:29c` `:29d` `:29e` `:29f` `:2a0` `:2a1` `:2a2` `:2a3` `:2a4` `:2a5` `:2a6` `:2a7` `:2a8` `:2a9` `:2aa` `:2ab` `:2ad` | 0 each (19 patterns) | negative ×19 |
| `:29a` | 1: `0dba` JNZ→`0x1000:29a3` | `→11bd:0dd3` OUT |
| `:2ac` | 7: the `0x1000:2ac4` set | `→11bd:0ef4` OUT ×7 (same insns) |
| `search_instructions` operand `0x1000:45` (added: code-space window `4500..45ff` = `11bd:2930..2a2f`) | 0 | negative — NO defined `CALL`/`JMP` targets anywhere in the block's first half |
| `search_instructions` operand `0x1000:46` (added: `4600..46ff` = `11bd:2a30..2b2f`) | 8: `2ae2` JZ→`46b8`→`2ae8`; `2b02` JZ→`46da`→`2b0a`; `2b3e` JNS→`46f8`→`2b28`; `2b45` JMP→`46f0`→`2b20`; `2b68` CALL→`46bb`→`2aeb`; `2bc9` CALL→`46e1`→`2b11`; `7744`/`792b` CALL→`46ab`→`2adb` ×2 | every recomputed target ≥ `2aeb`/`2adb` — at/after `FUN_11bd_2adb`, OUT of `2978..2ada`; `46ab−1bd0=2adb` = block end `46aa`+1, boundary arithmetic confirmed |
| `search_instructions` operand `0x9c2` | 2: `02b1` JMP `[0x9c2]` (read), `41ee` MOV `[0x9c2],0x296d` (write) | writer set UNCHANGED (`{41ee}`); sole immediate `0x296d` is the epilogue, NOT in-range (`< 2978`) — no new into-range vector store |
| `search_instructions` operand `9c2` | 2 (same) | superset check — identical, negative for extensions |
| `search_instructions` operand `0x2978` | 0 | candidate entry head: no defined-instruction reference |
| `search_instructions` operand `0x2a6c` | 0 | candidate MSW-clear entry: no defined-instruction reference |
| `search_instructions` operand `0x29` (immediate family, subsumes every `0x297x..0x29fx` text) | 4: `1000:006e` SUB `[BP+DI+0x29]` (disp8); `2f18` MOV `[BP+-0x18],0x29bc`; `41ee` MOV `[0x9c2],0x296d`; `44ab` MOV `[BP+-0x5a],0x29bc` | `0x29bc` IS numerically in `2978..2ada`, but both stores target BP-RELATIVE frame slots, not absolute vector cells — and the owners differ: `2f18` lives in `FUN_11bd_2ec9` (body `2ec9..2f4b`), `44ab` in `FUN_11bd_3ed8` (body `3ed8..4586`), the `[0x9c2]` publisher itself (same function as the `41ee` vector write) → REJECTED as code-entry writers; the runtime-slice value-coincidence lead points at BOTH functions (`2ec9` and `3ed8`); `41ee` out-of-range as above; `006e` disp-only noise |
| `search_instructions` operand `0x2a` (immediate family) | 17: `2adb` MOV `DX,CS:[0x2ad9]` + `7739` MOV `CS:[0x2ad9],DX`; 7× `[BP+-0x2a]` disp8 (`3730`,`374e`,`3751`,`375f`,`376f`,`3776`,`37d8`); 4× imm `0x2a` (`3850`,`3894`,`5b6c`,`6e7a`); `[SI+0x2a]`@`6b3f`; `1991:3807`/`3829`/`5457` (other segment) | ZERO transfers/stores with an in-range value; NEW DATA CONTACT noted: the word at `CS:[0x2ad9]` (bytes `2ad9..2ada`, the block's LAST two bytes) is read by `FUN_11bd_2adb`'s first insn and written by `setup_memory_hardware`@`7739` — in-block bytes serving as a live data cell, cited not adopted (ownership question for a later slice) |
| `search_byte_patterns` `7829bd11` (far-ptr LE of `0x2978:0x11bd`) | no matches (covers undefined holes) | no raw far-pointer data entry for block head |
| `search_byte_patterns` `6c2abd11` (`0x2a6c:0x11bd`) | no matches | none for MSW-clear candidate |
| `search_byte_patterns` `302abd11` (`0x2a30:0x11bd`) | no matches | none for mid-block candidate |
| `search_byte_patterns` `7829` / `6c2a` / `302a` (raw LE word immediates) | no matches / no matches / no matches | no `MOV [cell],imm`-style entry value even in undecoded bytes |
| `get_xrefs_to(11bd:2978)` / `get_xrefs_to(11bd:2a6c)` | 0 refs each | dead channel per slice-16 controls — reported, NOT evidence |

| Walk disposition | Result |
|------------------|--------|
| Attributed entries | **None** (0 of 3 candidate entries `2978`/`2a30`/`2a6c` has a resolved static feeder; see attribution table) |
| Walks executed | 0 — per the zero-entry rule no `disassemble_bytes` window was run this pass; `No attributed entries` is the deliverable: the block's entry mechanism stays dynamic/gap-internal (defined-only search caveat + dead-xref caveat recorded above) |

Deferral update for `## 02b7 twin` deferral (c) (`2978..2ada` ownership):
the entry question is now swept with the enumerated negative above —
`[0x9c2]` extension ruled out, far-pointer pairs ruled out, in-range
immediate stores ruled out among defined insns — leaving only (i)
fall-in/relative flow from the undecoded `2811..296c` pocket, (ii)
runtime-installed pointers, and (iii) the `CS:[0x2ad9]` tail data cell
as open legs. `2a6c..2a73` stays flagged-not-adopted. Zero Ghidra writes
this task: `find_code_gaps`, `get_function_by_address` ×3,
`search_instructions` (36 operand runs: `0x1000:29` 12, `0x1000:2a` 7,
`11bd:29` 0, `11bd:2a` 0, `:296`–`:2ad` 24 patterns 11 total hits all OUT,
`0x1000:45` 0, `0x1000:46` 8, `0x9c2` 2, `9c2` 2, `0x2978` 0, `0x2a6c` 0,
`0x29` 4, `0x2a` 17), `search_byte_patterns` (6 runs, all none),
`read_memory` (2), `get_xrefs_to` (2 controls); no `disassemble_bytes`
(dry-run or otherwise), no rename, no plate, no save;
`/media/felipe/FIFAPCCD/` untouched.

### Verdicts

None apply — zero walked functions, so there is no role, no mechanism name,
no cited exit, and no CONFIRMED/NOT-CONFIRMED row to write. This is the
plan's pre-declared legal zero-entry branch, not an omission: the walk
disposition table above is the basis, quoted by row — "Attributed entries |
**None** (0 of 3 candidate entries `2978`/`2a30`/`2a6c` has a resolved
static feeder; see attribution table)" and "Walks executed | 0 — per the
zero-entry rule no `disassemble_bytes` window was run this pass". The
per-walked-function verdict rule (CONFIRMED → verb-led mechanism name +
plate `C: none — behavioral (<role>)`; NOT-CONFIRMED-at-name → create-only,
missing leg named) is recorded here as applying VACUOUSLY: no leg is
"missing" per function because no function was walked; the missing leg is
the ENTRY LEG ITSELF, block-wide, and it is dispositioned below and in the
deferral lines rather than per function.

### Entry attribution (conclusion)

**Zero static entries into `2978..2ada` from defined instructions** — the
honest negative stands as the section's deliverable. Evidence is the
enumerated sweep in the attribution table above, cited by row (not
rewritten): 36 `search_instructions` operand runs over the 14006 defined
instructions (`0x1000:29` 12 hits, `0x1000:2a` 7, `:296`–`:2ad` 24 patterns
11 hits, `0x1000:45` 0, `0x1000:46` 8, `0x9c2`/`9c2` 2, `0x2978`/`0x2a6c`
0, `0x29` 4, `0x2a` 17 — every raw hit recomputed, none in range), 6
`search_byte_patterns` data probes (far-ptr pairs + raw LE words for
`2978`/`2a30`/`2a6c`, all no-match across defined AND undefined bytes),
and 2 `get_xrefs_to` controls (0 refs each — dead channel, reported not
relied). The `[0x9c2]` writer set is unchanged (`41ee` sole writer,
immediate `0x296d`, out of range). Caveat carried from the sweep rows:
`search_instructions` is DEFINED-INSTRUCTION-ONLY, so intra-gap relative
flow cannot surface — fall-in from the undecoded `2811..296c` pocket
remains OPEN; the block is therefore fallthrough-only statically, with the
entry mechanism unresolved among the three legs named in the deferral
update above.

Strongest dynamic lead, now correctly attributed (post-fix-round ownership
of the `0x29` immediate-family row): the immediate `0x29bc` — numerically
inside the block — is stored into BP-relative FRAME SLOTS by TWO different
functions: `11bd:2f18` (`MOV [BP+-0x18],0x29bc`) inside `FUN_11bd_2ec9`
(body `2ec9..2f4b`), and `11bd:44ab` (`MOV [BP+-0x5a],0x29bc`) inside
`FUN_11bd_3ed8` (body `3ed8..4586`, the `[0x9c2]`-vector publisher family —
`41ee` is the same function). Slot stores are not absolute vector cells →
REJECTED as static entry writers; recorded as a RUNTIME-SLICE LEAD pointing
at BOTH functions, lead-not-evidence. Second flagged-not-adopted contact:
`CS:[0x2ad9]` (bytes `2ad9..2ada`, the block's last two cells) is a LIVE
DATA TAIL CELL — read at `2adb` (`FUN_11bd_2adb` first insn,
`MOV DX,CS:[0x2ad9]`) and written at `7739` (`setup_memory_hardware`,
`MOV CS:[0x2ad9],DX`) — flagged-not-adopted; ownership question deferred.
Task-2 report Concern 3 (copied): if a future ownership decision classes
`2ad9..2ada` as data, the block's code range shrinks to `2978..2ad8` —
noted so a later create doesn't blindly claim the tail.

### Writes (before-state → after-state — ZERO-WRITE branch)

NONE. Zero writes this task: no `disassemble_bytes` (real or dry), no
`create_function`, no `rename_function`, no `set_comment`, no
`save_program` — authorized by the zero-entry branch (nothing was walked,
so nothing has a cited boundary to create at; the inherited cap + ratify
rule was never engaged and stays intact for a future slice). The block's
before-state and after-state are UNCHANGED, proven by two live read-backs
run THIS task (project `fifa96`, program `/fifa96.exe`), each verbatim
identical to the corresponding Task-1 row above:

| Live re-read (this task) | Verbatim response | Matches Task-1 row |
|--------------------------|-------------------|--------------------|
| `get_function_by_address(11bd:2a6c)` | `{"error":"No function found for 11bd:2a6c"}` | no-function at MSW-clear locator ✓ (`2a6c..2a73` still UNDEFINED, still flagged-not-adopted) |
| `find_code_gaps` (total 131, page offset 0/limit 100), covering row | `{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` | covering-row quote byte-exact ✓ — boundaries `2978`/`2ada` (−`0x1bd0`), size 355, neighbors unchanged |

Scope guards honored by non-action: `11bd:2811..296c` and the twin orphan
`02da..02f8` were not entered, not repaired, not mutated; no pre-existing
function body touched (`FUN_11bd_2adb`, `restore_fs_gs_and_resume` states
re-read only); `/media/felipe/FIFAPCCD/` untouched.

### Deferrals

- Unwalked attributed entries by name/address: **N/A** — zero attributed
  entries (nothing to walk, nothing deferred-by-name at this step).
- Unattributed remainder: **the whole block `2978..2ada` — all 355 bytes —
  handed back NAMED-OPEN**, every byte still undefined listing
  (`has_undefined_bytes:true` in the covering row above); the block is
  larger than this slice may claim per the plan's scope check.
- Runtime legs unchanged: `[0x40]` contents (slice-15 direction UNDECIDED)
  and `[0x9c0]` installer identity (NOT-IN-EXE, runtime-written) are
  UNCHANGED by this slice — no walk occurred that could name their
  installer, and no direction claim was made anywhere in this section.
- Out-of-scope skip pocket `2811..296c`: UNTOUCHED; tracked state stands in
  this section's own gap table (left-pocket row `1000:43e1..1000:453c`
  size 348 + pocket-edge `get_function_by_address(11bd:296c)` error — both
  re-confirmed by the `find_code_gaps` page just re-read).
- Twin orphan `02da..02f8`: UNTOUCHED; tracked state stands in
  `## 02b7 twin completion` `### Pocket repair` tail row
  (`get_function_by_address(11bd:02da)` → `{"error":"No function found for
  11bd:02da"}` — orphan still stands).
- Next-slice candidates: the FRAME-SLOT QUESTION — the `0x29bc` stores at
  `FUN_11bd_2ec9`/`2f18` (`[BP+-0x18]`) and `FUN_11bd_3ed8`/`44ab`
  (`[BP+-0x5a]`), and who consumes those slots (whether the frames reach
  the block via any runtime path); plus the `CS:[0x2ad9]` tail-cell
  ownership question (`2ad9..2ada` = the block's last two bytes as live
  data across `FUN_11bd_2adb`/`setup_memory_hardware`@`7739`).

## 0x29bc slot consumers (verified 2026-09-29, program `/fifa96.exe`)

Read-only trace pass on the slice-17 runtime-slice lead: the immediate
`0x29bc` (numerically INSIDE block `11bd:2978..2ada`) is stored into BP-relative
frame slots at `2f18` in `FUN_11bd_2ec9` and `44ab` in `FUN_11bd_3ed8`. Question:
do those slots have readers whose values feed an indirect transfer into the
block (a mechanism), or are the slots data-only? Result:
**SLOT-READERS-DATA-ONLY** — readers exist in BOTH functions, every consumption
form is non-branch (CMP/flag-Jcc/global-store/argument-PUSH into DIRECT calls),
and zero indirect `CALL`/`JMP` forms exist in either function; `11bd:29bc`
remains UNDEFINED with no static entry chain surfaced. The `0x29bc` stores now
carry full consumer evidence and are demoted from "untested lead" to
"settled-data-only" for the entry question. Disassembly rendering note: both
displacements render as `word ptr [BP + -0x18]` / `word ptr [BP + -0x5a]`
(Ghidra `disp8 − 0x100` form; store encoding `c7 46 <disp8> imm16`, load
`8b 46 <disp8>`, push `ff 76 <disp8>`). Quote protocol honored: every
`read_memory` `hex` field below was reconciled against the response's own
`data` array before quoting (all 11 reads matched byte-for-byte; the one
initial mismatch was a transcription error in the reconciliation script, not a
tool render glitch — the 384-byte window re-reconciled MATCH at 384/384 bytes).

### Site confirmations (Step 1)

| Probe | Verbatim response | Reconciliation / reading |
|-------|-------------------|--------------------------|
| `get_function_by_address(11bd:2ec9)` | `{"name":"FUN_11bd_2ec9","address":"11bd:2ec9","signature":"undefined FUN_11bd_2ec9(void)","entry_point":"11bd:2ec9","body_start":"11bd:2ec9","body_end":"11bd:2f4b"}` | function, body `2ec9..2f4b` ✓ matches slice-17 |
| `get_function_by_address(11bd:3ed8)` | `{"name":"FUN_11bd_3ed8","address":"11bd:3ed8","signature":"undefined FUN_11bd_3ed8(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:3ed8","body_start":"11bd:3ed8","body_end":"11bd:4586"}` | function, body `3ed8..4586` ✓ matches slice-17 |
| `get_function_by_address(11bd:29bc)` | `{"error":"No function found for 11bd:29bc"}` | no-function ✓ — block interior still fully undefined |
| `read_memory(11bd:2f18,8)` | `{"address":"11bd:2f18","length":8,"data":[199,70,232,188,41,246,6,20],"hex":"c746e8bc29f60614"}` | data→hex: `c7 46 e8 bc 29 f6 06 14` == hex field ✓; store = `MOV word ptr [BP + -0x18],0x29bc` (`e8`→−0x18, `bc29`→0x29bc); slice-17 byte quote `c746e8bc29` VERBATIM-CONFIRMED |
| `read_memory(11bd:44ab,8)` | `{"address":"11bd:44ab","length":8,"data":[199,70,166,188,41,235,108,199],"hex":"c746a6bc29eb6cc7"}` | data→hex: `c7 46 a6 bc 29 eb 6c c7` == hex field ✓; ACTUAL operand derived live: `c7 46 a6` = `MOV word ptr [BP + -0x5a],0x29bc` (disp8 `a6`→−0x5a) — displacement CONFIRMED `[BP+-0x5a]` (matches slice-17 rendering; was "NOT yet derived" per plan); following `eb6c` = `JMP rel8` → `451e` (`60ee−1bd0` ✓) |

### Slot trace (Step 2)

`FUN_11bd_2ec9` / slot `[BP + -0x18]` (body bytes cited from reconciled reads
`11bd:2ec9,12` `558bec83ec18c646f31ac746`, `11bd:2f00,52`
`7403e8e129c746e8402d803e2e000b740cf6064700807505c746e8bc29f6061400087405c746e87f62833e6e0e0074098b46e839`,
`11bd:2f36,12` `0e760f8b46e8a36e0e50ff76`):

| Slot | Function | Store sites (bytes → value) | Reader sites (bytes) | Consumption form |
|------|----------|------------------------------|----------------------|------------------|
| `[BP + -0x18]` | `FUN_11bd_2ec9` | `2f05 c746e8402d`→`0x2d40`; `2f18 c746e8bc29`→`0x29bc` (lead); `2f24 c746e87f62`→`0x627f` | `2f30 8b46e8 MOV AX,word ptr [BP + -0x18]`; `2f39 8b46e8 MOV AX,word ptr [BP + -0x18]` | data-only (see AX walk) |
| context | — | `2ecc 83ec18 SUB SP,0x18` (frame allocation — immediate, not a slot access; same numeral as displacement, classified non-reference) | — | — |

AX forward walk (disassembly order, until clobber): `2f30` load→AX → `2f33`
`39066e0e CMP word ptr [0xe6e],AX` (flags only) → `2f37` `760f JBE 0x1000:4b18`
(direct flag-conditional, `4b18−1bd0=2f48`) → CLOBBER `2f39` re-load → `2f3c`
`a36e0e MOV [0xe6e],AX` (global store — high-water clamp) → `2f3f` `50 PUSH AX`
(argument) → `2f43` `e859ef CALL 0x1000:3a6f` (DIRECT near call, `3a6f−1bd0=1e9f`)
→ `2f46` `5b POP BX`; AX live-ends at the call return. NO register-as-target
transfer anywhere on the walk; the value exits only into flags, the global cell
`[0xe6e]`, and a stack argument.

`FUN_11bd_3ed8` / slot `[BP + -0x5a]` (body bytes cited from reconciled reads
`11bd:436e,384` (hex beginning `c746a68103…`, ending `…f704eb37bf9011c6050083`)
and `11bd:452f,6` `ff76a6898614`):

| Slot | Function | Store sites (bytes → value) | Reader sites (bytes) | Consumption form |
|------|----------|------------------------------|----------------------|------------------|
| `[BP + -0x5a]` | `FUN_11bd_3ed8` | `436e c746a68103`→`0x381`; `43ed c746a6da08`→`0x8da`; `440e c746a62428`→`0x2824`; `4423 c746a6a703`→`0x3a7`; `445c c746a61a07`→`0x71a`; `446c c746a6b208`→`0x8b2`; `448e c746a64907`→`0x749`; `44a3 c746a60509`→`0x905`; `44ab c746a6bc29`→`0x29bc` (lead); `44b2 c746a67906`→`0x679`; `44b9 c746a6ac08`→`0x8ac`; `44c6 c746a6d603`→`0x3d6`; `44ce c746a66204`→`0x462`; `44e0 c746a6f704`→`0x4f7` — ALL FOURTEEN are `MOV word ptr [BP + -0x5a],imm16` immediate stores from a dispatch cascade | `452f ff76a6 PUSH word ptr [BP + -0x5a]` (the ONLY reader in 667 instructions) | data-only: stack argument → `4536 e8171d CALL 0x1000:7e20` (DIRECT near call, `7e20−1bd0=6250`; callee cited-address-only, not dived) |

`3ed8` has NO `MOV reg,[BP + -0x5a]` reader → no register propagation walk
required. Every store block terminates in a direct `JMP` and the reader sits in
the convergence region `44e7..4536` (cited example: `44b0 eb6c`→`451e`
(`60ee−1bd0`)); exhaustive per-path CFG analysis of `3ed8` is out of scope (its
full verdict is a non-goal) — what is settled is that the ONE reader site of
the slot, wherever reached, consumes only via the `452f`→`4536` argument push. Value-family semantics
(cited, not asserted): the stored set `0x381..0x905`/`0x2824`/`0x29bc` behaves
like selector/ID codes; the `2ec9` slot behaves like a size/request value
compared and clamped against running maximum `[0xe6e]`. The `2ec9` global sink
`[0xe6e]`: per the complete program-wide `[`-operand sweeps below, `0xe6e` is
NOT an operand cell of any defined indirect transfer — the side effect cannot
become a jump table without a defined reader.

Optional `analyze_dataflow` runs (reconciled, disassembly stays authority):
backward from `2f30` terminated "chain exhausted" at step 2 (SP/`PUSH BP`
frame arithmetic; `PTRADD SP,const:0xffe6` at asm line `MOV AX,word ptr [BP +
-0x18]`, code `1000:4b00` = `2f30` ✓ delta); backward from `452f` likewise
(PTRADD asm `PUSH word ptr [BP + -0x5a]`, `1000:60ff` = `452f` ✓). Neither
reached store cells (store→load memory aliasing is not followed by the PCode
walk) — no contradiction, no new info; the disassembly-order walk above stands.

### Indirect-transfer enumeration + catch-all sweep (Step 3)

Indirect transfers IN the two functions (from the complete disassembly dumps —
`2ec9` 45 instructions, `3ed8` 667 instructions — cross-checked by scoped
`search_instructions`):

| Function | `CALL reg` | `CALL [mem]` | `JMP reg` | `JMP [mem]` | far indirect | far direct | direct CALL/JMP | `RET` |
|----------|-----------|--------------|-----------|-------------|--------------|-----------|-----------------|-------|
| `FUN_11bd_2ec9` | 0 | 0 | 0 | 0 | 0 | 0 | all (`e82ef8`→`2718`, `e8e129`→`58e6`, `e859ef`→`1e9f`; `JMP`/`Jcc` all rel8/rel16 literal) | `2f4b c3` (stack return-address — unrelated to slot) |
| `FUN_11bd_3ed8` | 0 | 0 | 0 | 0 | 0 | 1: `4445 9a120b0010 CALLF 0x1000:0b12` — far POINTER IMMEDIATE (`9A`), operand value source = the instruction's own immediate, cited; NOT slot-fed | all `e8`/literal targets | `4586 c3` (stack return-address) |

Zero indirect forms fed from either slot; enumeration operand-value-source
column is therefore vacuous by evidence (no indirect transfer exists to trace).

Catch-all sweep (40 `search_instructions` runs, program scope over the 14006
defined instructions unless noted; all responses `truncated:false`):

| Run (mnemonic + operand pattern) | Hits | Disposition |
|----------------------------------|------|-------------|
| `CALL` + `29bc` | 0 | no transfer references the lead value |
| `CALL` + `0x29bc` | 0 | ditto (both accepted render forms) |
| `JMP` + `29bc` | 0 | ditto |
| `JMP` + `0x29bc` | 0 | ditto |
| `CALLF` + `29bc` | 0 | ditto (far forms) |
| `JMPF` + `29bc` | 0 | ditto (far forms) |
| (no mnemonic) + `29bc` | 2 | EXACTLY the two known slot stores `2f18`/`44ab` (`MOV`, bytes `c746e8bc29`/`c746a6bc29`) — program-wide confirmation of slice-17's `0x29` row; no new consumer, none is a transfer |
| (no mnemonic) + `0x1000:458c` (block cell `29bc` in near-target rendering, `29bc+1bd0=458c`) | 0 | no defined direct transfer targets the block cell (extends slice-17's `0x1000:45`=0 half-block negative to this exact cell) |
| `CALL` + `-0x18` | 0 | slot displacement absent from every `CALL` operand program-wide |
| `CALL` + `-0x5a` | 0 | ditto for `3ed8` slot |
| `JMP` + `-0x18` | 0 | ditto |
| `JMP` + `-0x5a` | 0 | ditto |
| `CALL` + `+ 0x18` / `CALL` + `+ 0x5a` | 0 / 0 | positive-displacement side of the family — negative |
| `JMP` + `+ 0x18` / `JMP` + `+ 0x5a` | 0 / 0 | ditto — negative |
| `CALL` + `[` (program) | 9 | all OUTSIDE both functions: `0249 [0x97a]`, `0294/02b8 [0x9c0]`×2, `183e [0xac2]`, `2368/2376 [0xe6c]`×2, `4fd3 [BP+0x4]` (`FUN_11bd_4f83` — displacement ≠ our slots), `1991:3bbf [0xaa4]`, `1991:3bf5 [0xaa6]`; no `0xe6e`, no `-0x18`/`-0x5a`, no `29bc` |
| `JMP` + `[` (program) | 9 | all OUTSIDE: `02b1 [0x9c2]` (known slice-17 vector reader), `092d [0x9bc]`, `0934 [0x9be]`, `25ea [0xd10]`, `1991:` ×5 CS-relative table forms; ditto — no contact |
| `CALLF` + `[` (program) | 20 | all OUTSIDE (`[0x1e]`, `[0xaec]`×7, `[BP-0x10]`, `[0xd5a]`×2, `[BP-0x4]`, `[BP-0xe]`, `[BP+0x0]`×4, `[0x12c4]`, `[0x22]`×2 …); none in our bodies, none reads our displacements |
| `JMPF` + `[` (program) | 5 | all OUTSIDE (`DS:[0xaf2]`, `CS:[0x17be]`×2, `CS:[SI-0x6]`, `CS:[BX+0x4d78]`) |
| `CALL` + `AX/BX/CX/DX/SI/DI/SP/BP` (8 runs) | 1/0/0/0/0/0/0/1 | `56e7 ffd0 CALL AX` (`FUN_11bd_5686`) and `4fd3 ff5604 CALL [BP+0x4]` (dup of `[` run) — both OUTSIDE our functions; the two dumps show no register-operand transfers |
| `JMP` + `AX/BX/CX/DX/SI/DI/SP/BP` (8 runs) | 2/5/4/0/1/2/0/0 | `1991:3fb1/412e JMP AX`, `11bd:675a ffe3 JMP BX`, `5fb1/5fb6/6d3b ffe1 JMP CX` (+`1991:4168`), `1991:` `[BX…/DI…]` forms, `1991:4227 ffe7 JMP DI` — ALL OUTSIDE both functions; no slot contact |
| scoped `CALL`+`[` / `JMP`+`[` in `FUN_11bd_2ec9` | 0 / 0 (45 insns scanned) | function-scoped cross-check agrees with the dump |
| scoped `CALL`+`[` / `JMP`+`[` in `FUN_11bd_3ed8` | 0 / 0 (667 insns scanned) | ditto |
| `get_xrefs_to(11bd:29bc)` | 0 refs | CONTROL ONLY — dead channel for absolute-operand forms (slice-16/17 rulings); reported, NOT relied upon |

### Verdict-so-far (Step 4)

**SLOT-READERS-DATA-ONLY** — the three MECHANISM-FOUND conditions, individually:

1. *Both `0x29bc` store sites live and byte-exact*: **YES** — `2ec9`/`3ed8`
   functions confirmed live (`2ec9..2f4b`, `3ed8..4586`); stores `2f18`
   `c746e8bc29` (`[BP+-0x18]`) and `44ab` `c746a6bc29` (`[BP+-0x5a]`,
   displacement derived live this pass) reconciled hex-vs-data verbatim.
2. *Slot readers exist*: **YES** — `2ec9`: `2f30`/`2f39` `MOV AX,[BP+-0x18]`;
   `3ed8`: `452f` `PUSH [BP+-0x5a]` (sole reader of 667 instructions).
3. *A reader's value feeds an indirect transfer (the mechanism leg)*: **NO** —
   full consumption inventory: `2ec9` AX walk ends in flags, the `[0xe6e]`
   high-water store, and a PUSH argument into the DIRECT call at `2f43`
   (`11bd:1e9f`); `3ed8`'s sole reader is a PUSH argument into the DIRECT call
   at `4536` (`11bd:6250`); both functions contain ZERO register- or
   memory-operand `CALL`/`JMP` forms (the only far form, `4445` `CALLF`, is a
   `9A` pointer-immediate); program-wide sweep puts no transfer on either slot
   displacement, no transfer carrying `0x29bc`, and no `CALL/JMP` whose
   operand cell is `[0xe6e]`; `0x1000:458c` (= `11bd:29bc` as near target) = 0
   defined hits.

Conditions 1+2 hold but 3 fails on complete enumeration → SLOT-READERS-DATA-ONLY
(not NOT-FOUND: readers exist and are cited; not FOUND: no branch consumption).
SLOT-READER-NOT-FOUND branch not applicable. **No entry walk executed** — the
conditional dry-run walk to a cited exit applies only under MECHANISM-FOUND;
`11bd:29bc` therefore stays UNDEFINED, and the slice-17 "runtime path from the
frame slots to the block" lead is CLOSED for the static slot-consumer angle:
the frames' `0x29bc` values are message/size-like data consumed as arguments.
The block-entry question stays with the remaining open legs named in slice-17
(fall-in from `2811..296c`, runtime-installed pointers, `CS:[0x2ad9]` tail
cell) — none of them involves these slots.

### Writes (before-state → after-state — ZERO-WRITE branch)

NONE. Read-only pass as tasked: no `disassemble_bytes` (dry-run or real —
verdict branch does not authorize the walk), no `create_function`, no rename,
no comment, no `save_program`. Tool inventory: `get_function_by_address` ×3,
`read_memory` ×11 (all reconciled: `2f18`8, `44ab`8, `2f00`52, `436e`384,
`452f`6, `2f36`12, `2ec9`12, `2ee7`4, `2f43`9, `4581`6, `4536`3),
`disassemble_function` ×2 (45 + 667 instructions), `search_instructions` ×40
runs, `get_xrefs_to` ×1 (control), `analyze_dataflow` ×2 (optional, reconciled,
no contradiction). Scope guards honored: `3ed8` walked for the slot question
only (its own verdict is a non-goal); callees `11bd:1e9f`/`11bd:6250` cited
address-only, not dived; `2811..296c` pocket not entered; no pre-existing
function state mutated; `/media/felipe/FIFAPCCD/` untouched.

### Disposition (Task 2 — settled 2026-09-29)

**SLOT-READERS-DATA-ONLY** — final disposition for this slice, restated with
the three MECHANISM-FOUND condition answers and the function-scope wording:

1. *Both `0x29bc` store sites live and byte-exact*: **YES** — `2f18`
   `c746e8bc29` → `MOV word ptr [BP + -0x18],0x29bc` (`FUN_11bd_2ec9`) and
   `44ab` `c746a6bc29` → `MOV word ptr [BP + -0x5a],0x29bc`
   (`FUN_11bd_3ed8`), displacement derived live; hex-vs-data reconciled
   before quoting (Site confirmations table).
2. *Slot readers exist*: **YES** — `2ec9`: `2f30`/`2f39` `8b46e8`
   `MOV AX,word ptr [BP + -0x18]` (consumption per the AX walk); `3ed8`:
   `452f ff76a6 PUSH word ptr [BP + -0x5a]`, sole reader in 667
   instructions, feeding the DIRECT `4536 e8171d CALL 0x1000:7e20` =
   `CALL 11bd:6250`.
3. *A reader's value feeds an indirect transfer*: **NO** — zero register- or
   memory-operand `CALL`/`JMP` in either function (complete dumps + 4
   scoped searches); program-wide sweeps put no transfer on either slot
   displacement, none carrying `0x29bc`, `[0xe6e]` is no indirect-transfer
   operand cell, and the near rendering `0x1000:458c` has 0 defined hits.

The verdict is **function-scoped**: it settles the consumption of
`[BP+-0x18]`/`[BP+-0x5a]` inside the two consumer bodies (`2ec9..2f4b`,
`3ed8..4586`) only. The callees `11bd:1e9f` (arg = slot value + `[BP+0x4]`)
and `11bd:6250` (arg = slot value) are cited address-only and could
themselves indirect-branch on the pushed value — that open condition belongs
one layer OUTSIDE these functions and is carried here as a deferral (line 1
below), not as part of the verdict.

No `### Entry function` row is emitted (the FOUND branch was not triggered);
`11bd:29bc` remains UNDEFINED block interior. The two `0x29bc` stores are
settled data-only for the entry question: the values behave as
message/size-family data consumed as DIRECT-call arguments (value-family
note in Slot trace above).

### Writes (Task 2 — DATA-ONLY branch: ZERO writes; live unmoved-proof pair)

NONE. Per the brief's Step 2 for this branch the program was not touched;
the only two calls executed this task are read-only, quoted verbatim:

| Probe | Verbatim response (live, this task) | Unmoved check |
|-------|--------------------------------------|---------------|
| `get_function_by_address(11bd:29bc)` | `{"error":"No function found for 11bd:29bc"}` | byte-identical to the Task-1 Step-1 quote (Site confirmations row 3) — no function was created at the lead cell |
| `find_code_gaps(min_size=1)` | envelope `{"total":131,"offset":0,"limit":100,…}`; covering row verbatim: `{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` | block `11bd:2978..2ada` (`4548−1bd0=2978`, `46aa−1bd0=2ada` ✓) still **size 355**, `has_undefined_bytes:true`, same neighbors — unchanged vs slice-17's row and Task-1's re-read; no shrink, no reflow |

Write-tool inventory for this task: zero — no `disassemble_bytes` (dry-run or
real), no `create_function`, no `rename_function`/`rename_symbol`, no
`set_comment`/`batch_set_comments`, no `set_global`, no `save_program`; no
Ghidra transaction was opened.

### Deferrals (Task 2)

- **What would promote `0x29bc` from data to entry:** a callee of the pushed
  slot value performing an indirect transfer on that argument. Named
  follow-up candidates at one-layer distance: `11bd:1e9f` (called from
  `2ec9`/`2f43`, arg = `[BP+-0x18]` value pushed at `2f3f`) and `11bd:6250`
  (called from `3ed8`/`4536`, arg = `[BP+-0x5a]` value pushed at `452f`) —
  requires walking those bodies for `CALL`/`JMP` reading the incoming stack
  argument or a register loaded from it; out of slice-18 scope.
- **`29bc` data-meaning question** (what the bytes at `11bd:29bc` themselves
  are): not exercised this slice — the block stays **fully named-open
  `2978..2ada`** (covering gap row quoted above); under DATA-ONLY no shrink
  arithmetic applies (the FOUND-branch split to `2978..29bb` + `X+1..2ada`
  was never triggered).
- **`3ed8`/`2ec9` full verdicts remain non-goals:** `3ed8` was walked for the
  slot question only (its ~66 direct calls and jump-table regions untouched
  beyond classification); `2ec9`'s whole-function semantics likewise.
- **Other-slot leads surfaced, cited not dived:** `3ed8` sibling slot
  `[BP+-0x58]` (feeds `[BX]` memory reads at `3ffc`/`4002` — NON-transfer,
  Slot trace note), and program-wide `4fd3 CALL [BP+0x4]` in
  `FUN_11bd_4f83` (different frame, no contact with our slots); these join
  the data-surface list alongside `[0xe6e]`.
- **Slice-17 leads untouched unless closed here:** fall-in pocket
  `2811..296c`, runtime-installed pointers, `CS:[0x2ad9]` tail cell, twin
  orphan `02da..02f8` — all stand as previously recorded; only the static
  slot-consumer angle is closed by this section.
- **Runtime legs unchanged:** `[0x40]` contents (slice-15 direction
  UNDECIDED) and `[0x9c0]` installer identity (NOT-IN-EXE, runtime-written)
  — a zero-write pass occurred; nothing in this slice could have moved them.

## callee arg question (verified 2026-09-29, program `/fifa96.exe`)

Read-only one-layer-out trace of the slice-18 deferral (Task-2 Deferrals line
1): do the two callees of the pushed `0x29bc`-bearing slot values —
`11bd:1e9f` (args pushed at `2f3f`/`2f40` in `FUN_11bd_2ec9`) and `11bd:6250`
(arg pushed at `452f` in `FUN_11bd_3ed8`) — indirect-branch on the received
argument? Result: **NEITHER-CALLEE-INDIRECTS-ON-ARG**. `FUN_11bd_1e9f` =
NONE-INDIRECT (61 instructions, zero register- or memory-operand `CALL`/`JMP`
forms; the arg arrives into `DI` at `1ea3` and is consumed as string-write
destination pointer arithmetic plus transformed stores — never as a branch
operand). `publish_mode_vector` = CELL-STORAGE (the arg is stored VERBATIM one
instruction after entry: `6255` `891eba09` `MOV word ptr [0x9ba],BX` — store
site + cell + bytes cited; per the named-and-deferred guard its consumers were
NOT followed). Slice-18's promotion condition — "a callee of the pushed slot
value performing an indirect transfer on that argument" — resolves NOT-MET at
the callee-body layer; any remaining entry leg sits further out (at the
`[0x9ba]` cell consumers, deferred, and the untouched slice-17 runtime legs).
No INDIRECT-ON-ARG form surfaced, so the conditional one-hop dry-run walk was
not triggered and `disassemble_bytes` was not invoked (dry-run or otherwise);
`disassemble_function` was the sole dump tool. Frame note: NEITHER callee uses
a BP frame — both index args off `BX` set by the entry `MOV BX,SP` (`8bdc`),
so the actual arrival displacements are `SS:[BX + 0x4]`/`[BX + 0x2]`, recorded
at actual addresses below. Quote protocol honored: all twelve `read_memory`
responses' `hex` fields reconciled against their own `data` arrays before
quoting (12/12 MATCH).

### Site confirmations (Step 1)

| Probe | Verbatim response | Reconciliation / reading |
|-------|-------------------|--------------------------|
| `get_function_by_address(11bd:1e9f)` | `{"name":"FUN_11bd_1e9f","address":"11bd:1e9f","signature":"undefined2 FUN_11bd_1e9f(void)","entry_point":"11bd:1e9f","body_start":"11bd:1e9f","body_end":"11bd:1f23"}` | ACTUAL name recorded — default `FUN_11bd_1e9f` (not renamed, not assumed); live function, body `1e9f..1f23` (61-instruction dump below) |
| `get_function_by_address(11bd:6250)` | `{"name":"publish_mode_vector","address":"11bd:6250","signature":"undefined publish_mode_vector(void)","entry_point":"11bd:6250","body_start":"11bd:6250","body_end":"11bd:627e"}` | slice-8 name LIVE at 6250 ✓; body `6250..627e` (14-instruction dump below) |
| caller re-derive `read_memory(11bd:2f3f,12)` | `{"address":"11bd:2f3f","length":12,"data":[80,255,118,4,232,89,239,91,91,139,229,93],"hex":"50ff7604e859ef5b5b8be55d"}` | data→hex reconciled ✓. Decode: `2f3f 50 PUSH AX` (slot `[BP+-0x18]` value, loaded `2f39`) → `2f40 ff7604 PUSH word ptr [BP + 0x4]` → `2f43 e859ef CALL 0x1000:3a6f` (`3a6f−1bd0=1e9f` ✓; rel16 `ef59`=−0x10A7, `2f46−0x10A7=1e9f` ✓) → `2f46 5b`/`2f47 5b POP BX` ×2 = two-word cleanup → `2f48 8be5`/`2f4a 5d` epilogue. Push order fixes arrival: slot value (pushed FIRST) lands furthest → callee `[BX+0x4]`; `2ec9`'s own arg (pushed LAST) → `[BX+0x2]` |
| caller re-derive `read_memory(11bd:452f,12)` | `{"address":"11bd:452f","length":12,"data":[255,118,166,137,134,20,255,232,23,29,91,128],"hex":"ff76a6898614ffe8171d5b80"}` | data→hex reconciled ✓. Decode: `452f ff76a6 PUSH word ptr [BP + -0x5a]` (slice-18's sole slot reader) → `4532 898614ff MOV word ptr [BP + -0xec],AX` (caller-side, non-transfer) → `4536 e8171d CALL 0x1000:7e20` (`7e20−1bd0=6250` ✓) → `4539 5b POP BX` one-word cleanup → callee single arg at `[BX+0x2]` |

### Indirect-transfer enumeration (Step 2 — complete dumps, slice-18 reviewer's method)

Scope anchors: `disassemble_function(11bd:1e9f)` returned **61 instructions**
(`count:61`, `1e9f..1f23`); `disassemble_function(11bd:6250)` returned **14
instructions** (`count:14`, `6250..627e`; first invocation's response was
truncated mid-listing at `6274` without a `count` field and was re-invoked —
the complete response is the one quoted). Every `CALL`/`JMP`/`Jcc`/far form
in each dump gets a row; far deltas recomputed `− 0x1bd0` per hit.

`FUN_11bd_1e9f` — 61 instructions, ALL transfers (6):

| Site | Rendered operand | Class | Recompute / value source |
|------|------------------|-------|--------------------------|
| `1eac` | `JNZ 0x1000:3ada` | literal near-direct Jcc | `3ada−1bd0=1ecd` (in body) |
| `1eae` | `CALL 0x1000:3af4` | literal near-direct CALL | `3af4−1bd0=1f24` (past `body_end 1f23` — cited, not followed) |
| `1edb` | `CALL 0x1000:3afe` | literal near-direct CALL | `3afe−1bd0=1f2e` |
| `1f02` | `INT 0x21` | software interrupt — LITERAL number operand; target resolved at runtime via IVT entry `0x21`, not an instruction operand and not arg-fed | — |
| `1f09` | `RET` | near return — stack return-address | — |
| `1f22` | `JMP 0x1000:3ab2` | literal near-direct JMP | `3ab2−1bd0=1ee2` (in body) |

Counted split: 6 transfers = 4 literal near (`CALL`×2, `JNZ`, `JMP`) + 1
`RET` (stack) + 1 `INT 0x21` (runtime IVT, literal number); indirect
`CALL`/`JMP`/`Jcc`/far: **0**. No far form (`9A`/`EA`/`FF` renderings) exists
in the dump.

`publish_mode_vector` — 14 instructions, ALL transfers (3):

| Site | Rendered operand | Class | Recompute / value source |
|------|------------------|-------|--------------------------|
| `625e` | `JC 0x1000:7e40` | literal near-direct Jcc | `7e40−1bd0=6270` (in body); bytes corroborate: `7210` at `625e` → `6260+0x10=6270` ✓ |
| `626b` | `JNZ 0x1000:7e40` | literal near-direct Jcc | `→6270` (flags from the `[0x2e]` CMP, not arg-fed) |
| `627e` | `RET` | near return — stack return-address | — |

Counted split: 3 transfers = 2 literal near Jcc + 1 `RET`; indirect: **0**;
no `CALL` at all; no far form. Flag sources of both Jccs are global `CMP`s
(`[0x2f]`, `[0x2e]`), not the arg.

Catch-all corroboration (`search_instructions`, function-scoped, mnemonic +
`[` operand):

| Run | Response | Reading |
|-----|----------|---------|
| `CALL` + `[` in `FUN_11bd_1e9f` | `{"matches":[],"match_count":0,"instructions_scanned":61,"truncated":false,"scope":"function:FUN_11bd_1e9f"}` | 0 indirect CALLs; scanned 61 = dump total ✓ |
| `JMP` + `[` in `FUN_11bd_1e9f` | `{"matches":[],"match_count":0,"instructions_scanned":61,"truncated":false, …}` | ditto ✓ |
| `CALL` + `[` in `publish_mode_vector` | `{"matches":[],"match_count":0,"instructions_scanned":14,"truncated":false,"scope":"function:publish_mode_vector"}` | 0; scanned 14 = dump total ✓ |
| `JMP` + `[` in `publish_mode_vector` | `{"matches":[],"match_count":0,"instructions_scanned":14,"truncated":false, …}` | ditto ✓ |

(`JMP`+`[` in Ghidra renderings covers `Jcc` too only if disassembled as
`JMP` — irrelevant here: every Jcc in both dumps is listed above and printed
with a literal `0x1000:` target.)

### Arg propagation (Step 3)

`FUN_11bd_1e9f` — arg = slot `[BP+-0x18]` value pushed `2f3f`:

| Arg arrival form | Frame slot | Every reference (store/load, addr+bytes+operand) | Clobbers | Terminal form |
|------------------|-----------|--------------------------------------------------|----------|---------------|
| stack push (PUSH AX @`2f3f`), 2-word frame | `[BX + 0x4]` (`BX=SP` set by `1e9f 8bdc MOV BX,SP`; **no BP frame**; read `1ea3` pre-pushes since `57`/`56` come after) | SLOT READS: `1ea3 368b7f04 MOV DI,word ptr SS:[BX + 0x4]` — the ONLY reference in 61 insns (sibling arg `[BX + 0x2]` = `2ec9`'s own `[BP+0x4]` value: ZERO references in the dump — recorded, no claim). DI HOPS: `1eb4 83c70f ADD DI,0xf` → `1eb7 83e7f0 AND DI,0xfff0` → `1eba 8bc7 MOV AX,DI` → AX offshoot `1ebc c1e8.. SHR AX,0x4` → `1ec9 ADD AX,DX` → `1ecc XCHG word ptr [0xa10],AX` (**transformed** paragraph form stored — not the value verbatim) → `1ed2 MOV DS,AX` (AX clobber). DI continues: `1ed6 f3a5 MOVSW.REP ES:DI,SI` (consumed as write-DESTINATION offset; `ES=DX=CS` per `1ec3 8cca MOV DX,CS` → `1ed4 8ec2 MOV ES,DX`; DI incremented by the string op = value consumed) → `1ee7 8bdf MOV BX,DI` → `1ef1 031eb609 ADD BX,[0x9b6]` → `1ef8 2bd8 SUB BX,AX` → `1efa 891e5a00 MOV word ptr [0x5a],BX` (derived form). Final DI read: `1f1e 2bcf SUB CX,DI` in tail block `1f0a..1f22` | DI never re-loads arg; BX-arg-pointer dead after `1ebf 8b1e100a MOV BX,[0xa10]` (arg window never re-walked) | NO transfer ever reads DI/AX/BX-derived chain as target (enumeration above: zero indirect forms); value life ends in pointer arithmetic + transformed stores + `1f09 RET` function exit |

`publish_mode_vector` — arg = slot `[BP+-0x5a]` value pushed `452f`:

| Arg arrival form | Frame slot | Every reference (store/load, addr+bytes+operand) | Clobbers | Terminal form |
|------------------|-----------|--------------------------------------------------|----------|---------------|
| stack push (PUSH `[BP+-0x5a]` @`452f`), 1-word frame | `[BX + 0x2]` (`BX=SP` set by `6250 8bdc MOV BX,SP`; **no BP frame**) | SLOT READS: `6252 8b5f02 MOV BX,word ptr [BX + 0x2]` — the ONLY reference in 14 insns. BX HOPS: `6255 891eba09 MOV word ptr [0x9ba],BX` — VERBATIM cell store, one insn after entry. **CELL-STORAGE — STOP HERE; `[0x9ba]` consumers DEFERRED (named-and-deferred).** BX later REDEFINED `626d bb2428 MOV BX,0x2824` (literal — numeral shared with the `3ed8` dispatch family value `440e`→`0x2824`; recorded as numeral fact, no role claim); the subsequent writes `6274 a3bc09 MOV [0x9bc],AX` / `627b a3be09 MOV [0x9be],AX` take AX from `2e8b47fc MOV AX,word ptr CS:[BX + -0x4]` / `2e8b47fe MOV AX,word ptr CS:[BX + -0x2]` — fed by the LITERAL `0x2824`, NOT by the arg (slice-8's `[0x9bc]` write re-confirmed at cited bytes) | arg value fully consumed by the `6255` store; BX arg-holder clobbered at `626d` | CELL-STORAGE at `6255` → `[0x9ba]` (bytes `891eba09`); no branch form reads the arg (both Jccs flag-fed from globals `[0x2f]`/`[0x2e]`) |

Optional `analyze_dataflow` runs (accelerator only; disassembly stays
authority; any use reconciled): backward from `1e9f`@`1eb4` on `DI`
terminated "chain exhausted" at step 7 — `LOAD`/`SEGMENTOP`/`PTRADD SP,const`
under asm `MOV DI,word ptr SS:[BX + 0x4]`, code `1000:3a73` (`3a73−1bd0=1ea3`
✓ disassembly cite) — reaches the arg load and stops at stack arithmetic,
exactly as the table above. Backward from `6250`@`6252` terminated at step 0
with `INT_ADD SP,const:0x2` under asm `MOV BX,word ptr [BX + 0x2]`, code
`1000:7e22` (`7e22−1bd0=6252` ✓) — the arg slot address expression; a first
anchor attempt at `6255` with variable `BX` returned the candidate error
(`No varnode … Candidates: [in_DS, sVar1, DAT_1000_031f]`) and was superseded
by the `6252` run; the store leg stands on the cited disassembly bytes
`891eba09`. No contradiction with the walks above; no new info beyond them.

### Disposition-so-far (Step 4)

| Function | Disposition | Basis (cited) |
|----------|-------------|---------------|
| `FUN_11bd_1e9f` | **NONE-INDIRECT** | 61-instruction complete dump, 6/6 transfers classified, 0 indirect (sweeps `instructions_scanned:61` 0/0); arg arrives `1ea3`→`DI`, never re-entered any transfer operand; terminal forms = string-write offset (`f3a5`), transformed stores (`XCHG [0xa10]`, `MOV [0x5a]`), function exit (`1f09 RET`) |
| `publish_mode_vector` | **CELL-STORAGE** | arg read `6252 8b5f02` → verbatim store `6255 891eba09 MOV word ptr [0x9ba],BX` (site + cell + bytes cited); 14-instruction dump, 3/3 transfers classified, 0 indirect (sweeps `instructions_scanned:14` 0/0); `[0x9ba]` consumers NOT followed per guard |

**Combined answer to the slice-18 condition:** NO — neither callee
indirect-branches on the received `0x29bc`-bearing argument. The deferral's
promotion path ("a callee … performing an indirect transfer on that argument")
is NOT-MET at this layer. The arg question relocates ONE layer further out at
`6250`: the verbatim cell `[0x9ba]` — its consumers are the next open leg and
are DEFERRED (named-and-deferred; not exercised this pass). At `1e9f` the arg
instead feeds runtime write-address arithmetic (`ES=CS` string destination) —
flagged-not-adopted numeral-adjacency only (aligned `DI` of a `0x29bc`-family
value can select low-window offsets at runtime); no role claim, consumers
n/a, entry relevance NONE. Slice-17's other runtime legs (`2811..296c`
fall-in, runtime-installed pointers, `CS:[0x2ad9]` tail cell) remain
untouched and stand. Scope guard honored: neither callee's name/role was
promoted or altered — `FUN_11bd_1e9f` stays default-named,
`publish_mode_vector` keeps its slice-8 name; the dumps informed the ONE
carried question only.

### Writes (ZERO-WRITE branch)

NONE. Read-only pass as tasked: no `disassemble_bytes` (INDIRECT-ON-ARG
branch not triggered, so the one-hop dry-run was neither needed nor
authorized), no `create_function`, no rename, no comment, no `save_program`;
no Ghidra transaction was opened. Tool inventory: `get_function_by_address`
×2, `disassemble_function` ×3 (61-insn dump; `6250` dump ×2 — first response
truncated, complete one with `count:14`), `read_memory` ×12 (caller re-derives
`2f3f`12/`452f`12 + propagation bytes `1e9f`8, `1eb4`10, `1ebf`12, `1ed0`6,
`1ed6`4, `1ee7`4, `1ef1`12, `1f1b`9, `6250`16, `626d`18; hex-vs-data 12/12
MATCH), `search_instructions`
×4 (function-scoped `CALL`+`[`/`JMP`+`[` sweeps, all `truncated:false`),
`analyze_dataflow` ×3 (2 reconciled uses, 1 superseded candidate-error call),
`get_xrefs_to` ×0 (dead-channel ruling — not needed for this question; no
new cell was under entry-scrutiny this pass, the `[0x9ba]` consumer sweep is
the deferred leg). `/media/felipe/FIFAPCCD/` untouched; pre-existing function
state re-read only.

### Disposition (Task 2 — settled 2026-09-29)

**NEITHER-CALLEE-INDIRECTS-ON-ARG** — final disposition for this slice,
the three-way outcome per callee restated with the citations that
establish it (enumeration scope = complete-dump instruction totals;
sweeps and cell-store site as recorded in the Step 2/Step 3 tables
above):

1. *`FUN_11bd_1e9f` — **NONE-INDIRECT***: complete dump **61
   instructions** (`count:61`, body `1e9f..1f23`) as enumeration scope
   — 6/6 transfers classified (4 literal near-direct + `RET` +
   literal-number `INT 0x21` with runtime-IVT target); indirect
   `CALL`/`JMP`/`Jcc`/far forms: 0, corroborated by function-scoped
   sweeps `CALL`+`[` = 0 and `JMP`+`[` = 0 at `instructions_scanned:61`
   = dump total. The arg arrives at the single slot read `1ea3`
   `368b7f04 MOV DI,word ptr SS:[BX + 0x4]` and never appears as a
   transfer operand (terminal forms: string-write destination offset,
   transformed cell stores `1ecc`/`1efa`, `1f09 RET` exit).
2. *`publish_mode_vector` — **CELL-STORAGE***: complete dump **14
   instructions** (`count:14`, body `6250..627e`) as enumeration scope
   — 3/3 transfers classified (2 literal near-direct Jccs, flags from
   global `CMP`s + `RET`); indirect forms: 0, sweeps `CALL`+`[` = 0 and
   `JMP`+`[` = 0 at `instructions_scanned:14` = dump total. The arg
   lands **VERBATIM into `[0x9ba]`** at `6255 891eba09`
   `MOV word ptr [0x9ba],BX` — store site + cell + bytes cited; no
   indirect-branch form on the arg exists in the body. `[0x9ba]` is
   distinct from slice-8's `[0x9bc]` publish target: the `[0x9bc]`/
   `[0x9be]` writes at `6274`/`627b` are fed by the LITERAL
   `626d bb2428 MOV BX,0x2824` (the 0x2824 literal trace), not by the
   arg. The `[0x9ba]` consumers were NOT dived (named-and-deferred
   guard).
3. *Slice-18 carried condition — **DISCHARGED (CLOSED at the
   callee-body layer)***: the carry sentence — "a callee of the pushed
   slot value performing an indirect transfer on that argument"
   (slice-18 `### Deferrals` line 1) — resolves **NOT-MET**: callees do
   NOT indirect on the arg — entry lead stays as slice-17/18 recorded,
   mechanism not found in either callee (`CALL`+`[` = 0 and `JMP`+`[` =
   0 across both complete dumps, scopes 61 insns and 14 insns). The
   indirect-transfer question of the arg is DONE within static scope at
   this layer; what remains of the entry question sits only in the
   runtime legs (`[0x40]`/`[0x9c0]`), the still-named-open block
   `2978..2ada`, and the relocated cell lead (point 4).
4. *Question relocation (CELL-STORAGE carry):* at `6250` the arg
   question relocates ONE layer out to the consumers of the stored cell
   **`[0x9ba]`** — cell named, store site `6255` (`891eba09`) cited,
   consumers DEFERRED per the named-and-deferred guard
   (`### Deferrals` line 1 below). At `1e9f` nothing relocates: no
   verbatim cell store exists (Arg propagation table — the `1ecc`/`1efa`
   stores are transformed forms).

No `### Entry function` row is emitted and the capped write path was
NOT executed — both were authorized only under INDIRECT-ON-ARG, which
did not occur (points 1–2 above); no conditional one-hop dry-run was
triggered.

### Writes (Task 2 — NONE-INDIRECT / CELL-STORAGE branch: ZERO writes; live unmoved-proof pair)

NONE. The program was not touched: the only Ghidra calls executed this
task are the two read-only read-backs below, quoted verbatim (live,
this task, program `/fifa96.exe`), in slice-18's unmoved-proof form:

| Probe | Verbatim response (live, this task) | Unmoved check |
|-------|--------------------------------------|---------------|
| `get_function_by_address(11bd:29bc)` | `{"error":"No function found for 11bd:29bc"}` | byte-identical to slice-18's Task-2 quote — no function was created at the lead cell by this slice |
| `find_code_gaps(min_size=1)` | envelope `{"total":131,"offset":0,"limit":100,…}`; covering row verbatim: `{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` | block `11bd:2978..2ada` (`4548−1bd0=2978`, `46aa−1bd0=2ada` ✓) still **size 355**, `has_undefined_bytes:true`, same neighbors — unchanged vs slice-17/18's rows and Task-1's re-read; no shrink, no reflow |

Write-tool inventory for this task: zero — no `disassemble_bytes`
(INDIRECT-ON-ARG branch not triggered, dry-run included), no
`create_function`, no `rename_function`/`rename_symbol`, no
`set_comment`/`batch_set_comments`, no `set_global`, no `save_program`;
no Ghidra transaction was opened. `/media/felipe/FIFAPCCD/` untouched;
pre-existing function state re-read only.

### Deferrals (Task 2)

- **`[0x9ba]` consumers = the next lead:** data cell `[0x9ba]` —
  address-only; store site `6255` (`891eba09`) cited above; consumers
  not enumerated this slice (named-and-deferred guard).
- **`FUN_11bd_1e9f` full verdict stays a non-goal:** current status as
  recorded — default-named live function, body `1e9f..1f23`,
  NONE-INDIRECT for this question only; no name/role/semantics
  asserted.
- **`publish_mode_vector` full verdict stays a non-goal:** current
  status as recorded — slice-8 name live, body `6250..627e`,
  CELL-STORAGE for this question only; its slice-8 `[0x9bc]` publish
  records stand unchanged.
- **Block `2978..2ada` stays fully named-open:** the size-355 covering
  gap row is quoted under `### Writes` above; nothing landed in-range
  this slice, so the recorded open range is untouched.
- **Runtime legs unchanged:** `[0x40]` contents (slice-15 direction
  UNDECIDED) and `[0x9c0]` installer identity (NOT-IN-EXE,
  runtime-written) — a zero-write pass occurred; nothing in this slice
  could have moved them.
- **Prior-slice statuses:** slice-18 **SLOT-READERS-DATA-ONLY** stands
  — this slice closes ONLY its deferral line 1 (the callee-indirect
  carry condition, answered NOT-MET in Disposition point 3), whose
  follow-up is superseded by the `[0x9ba]`-consumers line here;
  slice-18's remaining deferrals stand. Slice-17's open legs (fall-in
  `2811..296c`, runtime-installed pointers, `CS:[0x2ad9]` tail cell)
  stand as recorded.

## [0x9ba] consumers (verified 2026-09-29, program `/fifa96.exe`)

Read-only consumer-attribution pass on the cell slice-19 named-and-deferred:
`publish_mode_vector` stores its arg VERBATIM at `6255` `891eba09`
`MOV word ptr [0x9ba],BX` (value = `0x29bc` conditional-armed via the
`3ed8`→`452f`→`4536`→`6250` chain, slices 17–19). Question: does ANY defined
instruction read `[0x9ba]` (literal form or `[base+disp]` window form —
the `626d MOV BX,0x2824`/`6270 2e8b47fc` mirror pattern, reproduced and
extended), and if so does the loaded value reach a CALL/JMP within one hop?
Method: literal family sweeps (every pattern run + count), constant base-load
inventory (`MOV BX/SI/DI/BP,imm`), displacement-window sweeps over every
16-bit base form (`[BX +`/`[SI +`/`[DI +`/`[BP +`, the four ModRM bases plus
bare-register and base+index encodings — 16-bit real mode has no other
addressable forms; no SP-relative addressing exists), per-candidate
`base + disp → resolved` arithmetic against the `0x9b8..0x9bf` window,
dynamic-base sites recorded as OPEN-WINDOW rows, and the control probes
(`get_xrefs_to`/`list_data_items_by_xrefs` — dead channel per slices 16/18/19).
Result: **NONE-FROM-DISCIPLINE** — zero confirmed readers of `[0x9ba]` in the
enumerated static surface: the ONLY direct operand contact is the store
itself; the whole absolute-cell family `[0x9b0..0x9bf]` was swept complete
(17 hits) and shows the adjacency cells `[0x9bc]`/`[0x9be]` DO carry
transfer consumers (`092d`/`0934` `JMP word ptr` vectors — `dispatch_mode_vector`
/ `FUN_11bd_0931`) while `[0x9ba]` carries none; every constant-base
`[base+disp]` site WITH A PROVEN PRECEDING same-owner constant resolves
arithmetically OUTSIDE the window `0x9b8..0x9bf` (the mirror `6270/6277`
pair resolves `0x2824−0x4→0x2820` / `0x2824−0x2→0x2822` — window-MISS,
cited below); candidate constants in functionless ranges are
OPEN-WINDOW-with-candidate-constant per the fix-round rows, never silently
rejected).
This is a negative UNDER THE ENUMERATION, not an absence claim: undefined
gap code (block `2978..2ada` interior and the `6aae..6c3e`-style orphan
regions whose DEFINED instructions are outside function bodies), the
`1991:` overlay bank, implicit string-operand accesses (`MOVS`/`STOS`
read/write `[SI]`/`[DI]` with no rendered operand), and runtime segment
reloads (e.g. `66f6` `MOV DS,word ptr SS:[BX + 0x2]` re-DSes the default
segment that operandless absolute `[0x9ba]` references read through —
flagged-not-adopted, context only) are outside sweep visibility; the
OPEN-WINDOW rows below are honest holes, not resolved negatives.
Armed-conditional: the store fires only when the `3ed8` dispatch arms
value `0x29bc` into slot `[BP+-0x5a]` and pushes it (`452f`→`4536`);
even when armed, nothing in the enumerated static surface reads the cell
back, so the value reaches NO transfer — it dies in data at the cell
(terminal form = the `6255` store itself). Quote protocol honored: all
five `read_memory` responses below reconciled `hex` vs the response's own
`data` array (5/5 MATCH).

### Site confirmations and byte reconciliations (anchors)

| Probe | Verbatim response (data→hex reconciled) | Reading |
|-------|------------------------------------------|---------|
| `read_memory(11bd:6255,4)` | `{"address":"11bd:6255","length":4,"data":[137,30,186,9],"hex":"891eba09"}` | `137=0x89, 30=0x1E, 186=0xBA, 9=0x09` → `891eba09` ✓ — the slice-19 store cite is live and byte-exact: `MOV word ptr [0x9ba],BX` |
| `read_memory(11bd:092d,4)` | `{"address":"11bd:092d","length":4,"data":[255,38,188,9],"hex":"ff26bc09"}` | `FF 26 BC 09` ✓ `JMP word ptr [0x9bc]` — adjacency-cell TRANSFER consumer (control) |
| `read_memory(11bd:0934,4)` | `{"address":"11bd:0934","length":4,"data":[255,38,190,9],"hex":"ff26be09"}` | `FF 26 BE 09` ✓ `JMP word ptr [0x9be]` — same family shape, other cell |
| `read_memory(11bd:626d,18)` | `{"address":"11bd:626d","length":18,"data":[187,36,40,46,139,71,252,163,188,9,46,139,71,254,163,190,9,195],"hex":"bb24282e8b47fca3bc092e8b47fea3be09c3"}` | ✓ the full mirror chain: `626d bb2428 MOV BX,0x2824` / `6270 2e8b47fc MOV AX,CS:[BX+-0x4]` / `6274 a3bc09 MOV [0x9bc],AX` / `6277 2e8b47fe MOV AX,CS:[BX+-0x2]` / `627b a3be09 MOV [0x9be],AX` / `627e c3 RET` — the known window-pattern instance reproduced verbatim |
| `read_memory(11bd:2df5,8)` | `{"address":"11bd:2df5","length":8,"data":[139,94,6,255,119,2,232,31],"hex":"8b5e06ff7702e81f"}` | ✓ `2df5 8b5e06 MOV BX,[BP+0x6]` (caller-pointer base — dynamic) → `2df8 ff7702 PUSH [BX+0x2]` — the `[0x9b8]`-literal-as-ARG owner's nearest window-adjacent site, base provenance byte-exact |

### Sweeps inventory (Step 1 + Step 2 — every run, pattern + `match_count`, scope 14006 defined instructions, all responses `truncated:false` unless stated)

Literal family (11 runs):

| Sweep run (operand_pattern) | Hits | Classification (every hit quoted/instruction render) |
|-----------------------------|------|------------------------------------------------------|
| `0x9ba` | 1 | `6255` `MOV word ptr [0x9ba],BX` bytes `891eba09` — WRITE (the store). ZERO reads |
| `9ba` | 1 | identical (superset check — no extra renderings) |
| `[0x9ba]` | 1 | identical (bracket form) |
| `CS:[0x9ba]` | 0 | negative — tool accepts CS-override renders; none exists |
| override variants (`SS:/ES:/DS:[0x9ba]`) — subsumed, not separate runs | — | every override rendering contains the substring `9ba`; the `9ba` superset returned only `6255`, so all override-form renders are covered by that one-hit negative-remainder |
| `0x9b8` | 5 | ALL other-cell: `2dac MOV AX,0x9b8` (immediate ARG — see window trace), `39b4 MOV AX,[0x9b8]` READ, `466c CMP word ptr [0x9b8],0x0` READ, `4677 MOV AX,[0x9b8]` READ, `57af PUSH word ptr [0x9b8]` READ — `[0x9b8]` is live READ-ONLY data (config-string cell per owners `FUN_11bd_3986`/`lookup_copy_config_string`/`FUN_11bd_5686`) |
| `9b8` | 5 | identical (superset check) |
| `0x9bc` | 2 | `092d JMP word ptr [0x9bc]` **TRANSFER consumer** (`dispatch_mode_vector`) + `6274 MOV [0x9bc],AX` WRITE (`publish_mode_vector`) — adjacency control: this family cell DOES branch-read |
| `9bc` | 6 | above 2 + 4 FALSE-STRING rejections: `2dd6`/`2de7` `JNZ 0x1000:49bc` (near-target render; `49bc−1bd0=2dec` in-body code, not the data cell), `2f18`/`44ab` `MOV [BP+…],0x29bc` (immediate numeral `9bc` inside `0x29bc` — frame-slot stores, slices 17–18) |
| `0x9be` | 2 | `0934 JMP word ptr [0x9be]` **TRANSFER consumer** (`FUN_11bd_0931`) + `627b MOV [0x9be],AX` WRITE — same shape as `[0x9bc]` |
| `9be` | 4 | above 2 + 2 FALSE-STRING: `4dcd JMP 0x1000:69be` (`eb1f` → `4dcd+2+0x1f=4dee`; `69be−1bd0=4dee` — code target), `1991:20a6 JNZ 0x1000:b9be` (other-bank code target) |
| `[0x9b` (whole absolute-cell family window) | 17 | COMPLETE picture: `6ea2 MOV word ptr SS:[0x9b2],ES` W + `199f MOV AX,[0x9b4]` R + `5f7f PUSH [0x9b4]` R + `1991:0ff3` R + `1991:2a13` PUSH R (`[0x9b4]`), `1a1d MOV AX,[0x9b6]` R + `1ef1 ADD BX,[0x9b6]` R + `1000:0b9d MOV [0x9b6],AX` W (`[0x9b6]`), the 5 `[0x9b8]` rows, `6255` W (`[0x9ba]`), the 2 `[0x9bc]` rows, the 2 `[0x9be]` rows — **zero reads of `[0x9ba]` across the whole 9b0–9bf absolute neighborhood; the only branch-shaped consumers in the family sit on `[0x9bc]`/`[0x9be]`** |

Constant base-load inventory (imm-source constants, 9 runs):

| Sweep run | Hits | Notes |
|-----------|------|-------|
| mnemonic `MOV` + `0x2824` | 2 | `440e` slot store (`3ed8` dispatch family) + **`626d MOV BX,0x2824` — the known instance REPRODUCED verbatim** (bytes `bb2428` in both this run and the reconciled `626d` read above) |
| `MOV` + `0x9b8` / `0x9ba` / `0x9bc` / `0x9be` | 3 / 1 / 1 / 1 | `2dac MOV AX,0x9b8` + the `[0x9b8]` loads; `0x9ba`/`0x9bc`/`0x9be` rows are the CELL OPERANDS themselves (stores/reads), NOT base loads → **no register ever holds a window-cell address as an immediate** |
| `MOV` + `BX, 0x` | 33 | complete BX immediate-load census; window-relevant values: `6a97 BX=0x98e` (needs `[BX+0x2a..0x31]` sites — FUN_11bd_6a68 body `6a68..6aab` contains ZERO `[BX+…]` sites per the full partition → vacuous), `626d BX=0x2824`, `3465 BX=0x11e4`, `7684 BX=0xf7d`, `7697 BX=0x2d0a`, `0d7c BX=0xe000`, `3f13/3f18 BX=0xf000/0xfffe`, `1e6b BX=0xe822`, `2804 BX=0xd12`, smalls `{0x2,3,4,5,8,0xa,0xb,0x10,0x40,0x200,0xffff,0,0x1000}` — **none in `0x9b8..0x9bf`** |
| `MOV` + `SI, 0x` | 59 | closest: `57da SI=0x938` (owner FUN_11bd_5686 — its `[SI+` sites: NONE; reaches would need `[SI+0x80..0x87]`), `791f SI=0x940` (owner setup_memory_hardware — bare `[SI]` at `77e8` PRECEDES the load (0x940 dword would cover `0x940..0x943` ∉ window anyway); none in window |
| `MOV` + `DI, 0x` | 34 | none in window (`0xf8c,0x98,0x8c0,0x15e8,0xc20,0xa2c,0x6341,0x7330…`); reaches checked per-site below |
| `MOV` + `BP, 0x` | 1 | **only constant BP load in the program: `1000:0000 MOV BP,0x1`** (MS-DOS-stub decode region); frame-chain BP (`55`/`8b e5`/`8b ec`) elsewhere — basis for the stack-relative class rejection |

Displacement-window sweeps (form-complete for 16-bit ModRM; render-form
discovery: operand text renders with spaces — `[BX+` = 0 vs `[BX +` =
matches — both runs recorded):

| Sweep run | Hits | Coverage/notes |
|-----------|------|----------------|
| `[BX+` (no space) | 0 | render-form probe (negative) |
| `[BX +` (combined) | 500 **cap-truncated** (`instructions_scanned:7779` of 14006) | discovery run; the `offset=500` retry (an accepted tool parameter — `search_instructions` takes `offset`) returned the identical first-500 window: pagination is a NO-OP, demonstrated live this fix round with `search_instructions(operand_pattern="[BX +", offset=500, limit=1)` → first match verbatim `{"address":{"address":"1000:0009"},"function":"FUN_1991_0c9e","mnemonic":"ADD","operands":"byte ptr [BX + SI], AL","bytes":"0000"}` (`"match_count":1,"truncated":true`) — match index 500 == index 0; the stub-zone `ADD byte ptr [BX + SI],AL` flood hits the match cap → superseded by the sign+digit partition below (COMPLETE) |
| `[BX + -` (all negative disps) | 10 | full: `4cc3`/`6198`/`6270`/`6277`/`7687`/`768e`/`769a`/`76a4`/`1991:2f65`/`1991:3872` — arithmetics in window table |
| `[BX + 0` (all positive disps, single combined run) | **320** (`truncated:false`, `instructions_scanned:14006` — envelope re-quoted live this fix round; Task-1's first response tail was lost to display truncation) | reconciles exactly with the 16-run digit partition below (0+57+92+3+37+32+48+7+10+0+12+0+14+1+7+0 = 320) |
| `[BX + 0x0` … `[BX + 0xf` (digit partition = complete enumeration of positive disps) | 0+57+92+3+37+32+48+7+10+0+12+0+14+1+7+0 = **320** | every run `truncated:false` over 14006; totals sum and reconcile `[BX + 0` |
| `[SI +` | 81 | complete — full disp list includes `+0x2..+0x68`, `-0x30..-0x1`, `CS:[SI+-0x2]/-0x6` (1991) |
| `[DI +` | 76 | complete — includes `ES:[DI+0x6]`, `CS:[DI+0x66a]` (1991 transfer), `[DI+0x2a]` family |
| `[BP + -` | 500 **cap-truncated** (scanned 5140; frame displacement family, observed range −0x1..−0x98 in prefix) | class-rejected (stack frames; basis = BP census 1-hit above); window question closed instead by the TARGETED runs: `[BP + 0x9b` = **0** (no disp renders `0x9b0..0x9bf` → stub BP=0x1 can never reach the window; wrap-reach would need runtime BP≈0xFFF0±, unattributable), `[BP + 0x9` = 2 (`1991:40f6`/`41a0` `BP+0x9` sites, owners load BP from `SS:[BX+0x4]` frames `410d`/`417d` → dynamic, OPEN) |
| bare-register: `[BX]` | 137 | complete — no BX constant equals a window cell (nearest 0x98e) → every statically-resolved `[BX]` lands outside `0x9b8..0x9bf`; rest dynamic/frame → OPEN |
| `[SI]` | 40 | complete — no SI constant in window; `77e8`/`79f0` resolve to `0x940`∉ (ordering: sites precede `791f` load) or ES-override → reject/OPEN |
| `[DI]` | 34 | complete — `44ea [DI]` resolves `DI=0x1190` (`44e7` precedes) ∉ reject; `639c/63a2 CS:[DI]` resolve `0x6341`∉; `234d [DI]` → `0x15e8`∉; `6345` → `DI=0x0`→`0x0`∉; `1000:001a/008d` stub-OPEN; rest dynamic-frame |
| `[BP]` | 0 | negative — executed twice (first response lost to display truncation; both `match_count:0`) |
| `[BX + SI]` | 347 | complete (no cap!) — 335 stub-zone junk + `1000:0bd4/0bd8` + `11bd:1e73 ADC [BX+SI],AX` (owner `FUN_11bd_1e68`: BX/SI both runtime pointers, no constant pair) + 6 `1991:` sites incl. `1991:453b JMP word ptr CS:[BX + SI]` (dynamic transfer → OPEN) |
| `[BX + DI]` | 12 | complete — stubs + `mem_grow_relocate` `0bce/0bd2` + `11bd:1b9b/2295` + 4 `1991:` ES sites; no constant pair reaches window (`mem_grow_relocate`: BX uncensed-dynamic × DI=0xf8c → OPEN) |
| `[BX + SI +` / `[BX + DI +` | 3 / 3 | complete — `1000:004b`/`11bd:5b03`/`1991:3880`, stubs `0083`/`0eb8`/`0ee2`; all dynamic base+index → OPEN |
| `[BP + SI +` / `[BP + DI +` / `[BP + SI]` / `[BP + DI]` | 2 / 2 / 0 / 0 | complete — all `1000:` stub-zone junk → OPEN(stub) |
| `[BX + S +` / `[BX + D +` (mis-guess render probe) | 0 / 0 | negative — superseded by exact forms above; recorded for run-honesty |

Total `search_instructions` calls in Task-1: **61** = **59 distinct
pattern runs** + 2 duplicate executions (the `[BX +` `offset=500` retry and
one `[BP]` re-run; both recorded on their rows). The 59 distinct = 11
literal + 5 MOV-constant + 4 register-census + 39 form/window patterns
(the 3 render-format probes `[BX+`-no-space, `[BX + D +`, `[BX + S +` are
among the 39, all negative, exact forms kept). Fix-round verification runs
(`[BX + 0` envelope re-quote, the `offset=500` pagination probe,
`0x1ea6` probes) are recorded in the fix subsection, not in the 61 — and
the `[BX +` offset-retry counted among the 61 is EVIDENCED by that probe
(match index 500 == index 0), so the 61 = 59 distinct + 2 duplicates
reconciliation stands with proof, not recollection.

### Base→window arithmetic (every constant-base candidate: hit resolves to `[0x9ba]` or explicit rejection)

| Site | Rendered | Base (cited constant load) | base + disp → resolved | Verdict |
|------|----------|---------------------------|------------------------|---------|
| `6270` | `MOV AX,word ptr CS:[BX + -0x4]` | `626d bb2428 MOV BX,0x2824` | `0x2824 − 0x4 = 0x2820` ∉ `0x9b8..0x9bf` | REJECT (known mirror instance — pair feeds `[0x9bc]` WRITE from a CS-relative paging read, not the cell) |
| `6277` | `MOV AX,word ptr CS:[BX + -0x2]` | same | `0x2824 − 0x2 = 0x2822` ∉ | REJECT |
| `7687` | `MOV byte ptr CS:[BX + -0x3],0xe9` | `7684 bb7d0f MOV BX,0xf7d` | `0xf7a` ∉ | REJECT (hook-patch write) |
| `768e` | `MOV word ptr CS:[BX + -0x2],DX` | same | `0xf7b` ∉ | REJECT |
| `769a` | `MOV byte ptr CS:[BX + -0x3],0xe9` | `7697 bb0a2d MOV BX,0x2d0a` | `0x2d07` ∉ | REJECT |
| `76a4` | `MOV word ptr CS:[BX + -0x2],DX` | same | `0x2d08` ∉ | REJECT |
| `12a9` | `CMP byte ptr [BX + 0x10be],0x0` | `12a1 bb0800 MOV BX,0x8` | `0x10c6` ∉ | REJECT |
| `1dfe`/`1e14` | `[BX + 0xadc]` R/W | `1df7 bb1000 MOV BX,0x10` | `0xaec` ∉ | REJECT |
| `23de` | `OR byte ptr [BX + 0xcfa],0x40` | `23cf bb0400 MOV BX,0x4` | `0xcfe` ∉ | REJECT |
| `32c6`-family 11 sites (`36ca`…`37f1`, `[BX+0x2..0x26]`) | R/W | `3465 bbe411 MOV BX,0x11e4` | `0x11e6..0x120a` ∉ | REJECT ×11 |
| `3ed8`: `4004` `[BX+0x1]`, `4002`/`400f`/`4396` `[BX]` | R/W | `3f13 BX=0xf000` / `3f18 BX=0xfffe` | `0xffff`/`0xf001`/`0xf000`/`0xfffe` ∉ | REJECT ×4 |
| `3bcc`: `3c16`…`3c77` `[BX+0x5/0x8]`, `3bef` `[BX]` | R | `3bea BX=0x8` | `0xd`/`0x10`/`0x8` ∉ | REJECT ×6 |
| `62ee` `ES:[BX+0x2]`; `62d1`/`62d7` | R/W | `62ce BX=0xa` (`62d7` site PRECEDES load→dynamic) | `0xc`/`0xa` ∉ | REJECT (1 OPEN: `62d7` pre-load) |
| `5ef6` `ES:[BX+0x2]` | R | `5e9e bb0300 MOV BX,0x3` — same owner `load_mf_object`, load precedes site | `0x5` ∉ | REJECT |
| `6009`/`600c`/`600f` `CX,[BX + 0x6]` / `DX,[BX + 0x2]` / `DS,[BX + 0x4]` | R (NO segment override — live dump confirms; the `ES:` and `0x6`/`0x2` mix in Task-1's row were transcription artifacts, corrected) | owner `file_read_far_dos` (`6003..601c`, **13-instruction dump this fix round**, `count:13`): base = `6003 8bdc MOV BX,SP` — arg-frame pointer (a DIFFERENT body from `load_mf_object`'s `5e9e`) | `SP+0x2/0x4/0x6` — runtime stack value | REJECT as arg-frame form (base-load cited AT the body per ruling — Task-1's cross-body attribution to `5e9e` was wrong) |
| `20d0` `1991: CS:[BX+0x1ea6]` | `MOV DI,word ptr CS:[BX + 0x1ea6]` — **not a transfer** (fix: it was mis-listed among the OPEN transfer-form sites; live `0x1ea6` run: single `MOV` match) | candidate `1991:2034 MOV BX,0x4` — site and load both in a FUNCTIONLESS overlay range (`get_function_by_address(1991:2034)`/`(1991:20d0)` → error, quoted in the fix subsection); no body, so flow/ordering cannot be checked | `0x4+0x1ea6=0x1eaa` ∉ only IF the constant carries — unprovable | **OPEN-WINDOW-with-candidate-constant** (ordering-precedence treatment, mirrors `62d7`/`77e8` — Task-1's REJECT was rejected-past-staticals) |
| `1991:3920..3964` ×10 (`ES:[BX+0x5/0x6/0x16/0x2/0x10/0x14/0xa]` family) | R/W | candidate `1991:3897 MOV BX,0x8` — same functionless band (`get_function_by_address(1991:3920)` → error) | `0x8+disp→0xd..0x1e` ∉ only IF carried — unprovable | **OPEN-WINDOW-with-candidate-constant** (was REJECT ×9 — undercounted AND over-claimed) |
| `7063/7066` `[SI+0x2/0x6]` | W | `7060 SI=0x7a0` | `0x7a2`/`0x7a6` ∉ | REJECT |
| `76b6/76cc/76d1` `[SI+0x2]`, `76b3`/`76c8` `[SI]` | R/W | `76b0 SI=0x19c` | `0x19e`/`0x19c` ∉ | REJECT ×5 |
| `2347` `[DI+-0x1]`, `234d` `[DI]` | R/W | `232d DI=0x15e8` | `0x15e7`/`0x15e8` ∉ | REJECT |
| `63de/63e1/6345` | R | `6396 DI=0x6341` / `63ac DI=0x4a` / `6335 DI=0` | `0x633f`/`0x633d`→`0x48`/`0x46`→`0x2` ∉ | REJECT ×3 (both orderings miss) |
| `6eca/6edf` `ES:[DI+0x6]` | W | `6e3b DI=0xc20` | `0xc26` ∉ | REJECT |
| `2c9e/2cc1` `[DI+0x6]`, `414d` `ES:[DI+0x5]` | W/R | `1991:2c94/2cb7 DI=0x60` / `1991:4135 DI=0x8c0` | `0x66`/`0x8c5` ∉ | REJECT ×3 |
| `44ea` `[DI]`, `639c/63a2` `CS:[DI]` | W/CMP | `44e7 DI=0x1190` / `6396 DI=0x6341` | `0x1190`/`0x6341` ∉ | REJECT ×3 |
| `1868` `LEA CX,[SI+-0x1]` | (LEA — no memory access) | `1863 SI=0x5` | — | NON-LOAD |
| `6a97 BX=0x98e` | **nearest-miss constant** | owner `FUN_11bd_6a68` (`6a68..6aab`) | window reach needs a `[BX+0x2a..0x31]` site: the full `[BX +` partition lists ZERO sites in that body | NO-REACH (vacuous) |
| `2df8` `PUSH word ptr [BX + 0x2]` | owner `FUN_11bd_2d9c` (127-insn dump) | `2df5 8b5e06 MOV BX,[BP+0x6]` — caller pointer; and the `2dac MOV AX,0x9b8` constant flows ONLY `2daf PUSH AX` → arg stack into `2db8 CALL` (never into any base register; `2d9c` BX-set = `2dbe/2df5/2e87/2eb8 [BP+0x6]`, `2e16 0x80`, `LES`-loads) | base runtime | **OPEN-WINDOW** (dump-cited; `analyze_dataflow` backward at `2df8` resolved the PTRADD base to `SP` and terminated "chain exhausted" at step 0/phi `1000:49bc` (= `2dec` ✓ delta) — reconciled, disassembly stands) |
| `4cc3` | `MOV AX,word ptr ES:[BX + -0x2]` | owner `FUN_11bd_4ca1` — **128-instruction dump this fix round** (`count:128`, body `4ca1..4df6`) as enumeration scope: BX built at `4cab MOV BX,[BP+0x8]`→`4cae SHL BX,0x3`→`4cb1 ADD BX,[BP+0x4]` (frame-arg cursor; ES ← `4cb4 [BP+0x6]`) — NO constant load in body, BX census confirms | base runtime (caller-passed geometry) | **OPEN-WINDOW** (window-reach would need caller values landing `0x9ba..0x9c1`; dump-scoped) |
| `6198` | `LEA AX,[BX + -0x1]` | `find_substring` | — | **NON-LOAD** (LEA computes an address, never reads memory) |
| `1991:3872` | `LEA AX,[BX + -0x1]` | overlay | — | **NON-LOAD** |
| `1991:2f65` | `AND byte ptr ES:[BX + -0x1],0xfd` | overlay owner `FUN_1991_2d3e` — no BX constant in body | base runtime | **OPEN-WINDOW** |

OPEN-WINDOW rows (dynamic/stack/wrap-unattributable base sites cited, NOT
counted as negatives. Exhaustiveness, stated exactly: every
`[base+disp]`/`[base+index]` site whose base has a same-owner constant load
PROVEN to precede the site is individually resolved or rejected in the table
above — and NO site with such a carrying constant lands in
`0x9b8..0x9bf`. Sites whose constant load does NOT precede them in the owner
body, or whose candidate constant sits in a functionless range (flow/ordering
unprovable), are OPEN-WINDOW-with-candidate-constant — never silently
rejected; sites with no same-owner constant base at all are class-OPEN and
enumerated below. The census×site cross-check leaves NO site with a
statically RESOLVABLE base unresolved):
all `[BX+SI]`/`[BX+DI]` (+disp) pair-form sites
outside the resolved table (`11bd:1b9b`, `1e73`, `2295`, `5b03`, `1000:`
stub flood 335+6+6, `1991:321b/3253/35d0/362f/3689/3879/38b2/3977/453b/3a75/3af1`);
transfer-form OPENs — `1991:0cf7 JMP CS:[DI+0x66a]`,
`1991:1f7d JMP CS:[BX+0x1ece]`, `1991:33a5`+`1991:33e1` CS-table jumps,
`1991:453b JMP CS:[BX+SI]`, `1991:4f91 JMPF CS:[BX+0x4d78]` (all overlay-bank dynamic
tables, no `[0x9ba]` feeder exists to arm them; fix-round correction: the
Task-1 list wrongly counted `1991:20d0` here — it is `MOV DI,CS:[BX+0x1ea6]`,
a load, now an OPEN-WINDOW-with-candidate-constant row in the arithmetic
table); ES:[BX+…] struct-cursor
owners with no constant BX in their body per the 33-site census (`1ab8`,
`1d8c`, `3986`, `4bdd`, `52ef`, `5686`, `6701`, `4ca1`/`4f83`/`6084`/
`exec_loaded_image`/`probe_bios_model` families — sites `1ae7..1de4`,
`39c5/39cb`, `4a5d..4dc7`, `5327..53d5`, `5698/569c`, `6716..6726`,
`6941..694e`, `62d7`-class cursors) — base = caller pointer → OPEN;
orphan-region sites
(functionless defined code, `get_function_by_address(11bd:6ae2)` → error —
no owner dump exists to cite): `6ab2`, `6ac6`, `6acb`, `6ae2`, `6ae5`,
`6af2..6b3f` (`[SI+0x2/0x18/0x28/0x2a/0x2c/0x34/0x36/0x38]` family — the
CS:6b51-return IRET-frame builder cluster, SI = runtime stack pointer,
stack-class), `6bfd`, `6c14`, `6c23`, `6c3e`, `0e22`, `0e28`, `0ef8`,
`0efe`, `1877`, `2282`, `4901`, `49b0`, `4a5d..4ae5`
(`ES:[BX+…]` in the functionless `48e5..` band — fix-round correction: the
Task-1 listing of `4bfc..4dc7` here was wrong, those sites sit in LIVE
owners `4b91`/`4bdd`/`4ca1` and belong to the struct-cursor class above),
and every `SS:[BX+…]` arg-frame site (`11dd`, `1266`, `14ac`, `1e9f`,
`66e1`, `71d1`, `1991:3f74..416e` stub family) — SS-override stack
addressing, class-rejected with the frame-chain reason; ordering-precedence
and no-constant-base sites named by review, enumerated here as class-OPEN:
`0d65/0d68/0d6b` (owner `0d62`: its only BX constant `0d7c BX=0xe000` loads
AFTER all three sites — unprovable carry; even carried,
`0xe007/0xe004/0xe002` ∉), `129a` (`12a1 BX=0x8` loads after), `1499`
(`14a4 BX=0x2` after; `14b8` is SS: frame), `3012/302a/303a` (owner `300b`:
no BX constant), `307d/3083/3086/3098/30aa` (owner `vet_file_header`:
none), `5b12/5b2a/5b3a` (owner `5b09`: none), `5c4e` (`parse_script_text`:
none), `5fba/5fcc/5fe4` + `5fe7` (`file_open_dos`/`file_seek_dos`/
`file_read_dos`: none — same arg-wrapper shape whose sibling
`file_read_far_dos` was dumped this round with `6003 8bdc MOV BX,SP`),
`61b3/61be/61cf/61da` (`61aa`/`61c6`: none), `63ef` (candidate `63c4
BX=0x0` → `0x2` ∉ if carried — intervening `63d0..63ea` clobber-scan not
walked → precedence-cautious OPEN), `65cb` (`65c3`: none), `665e` (`6655`:
none), `6685/669d` (`probe_bios_model`: none), `686b/686e` (`6869`: none),
`7162` (`7160`: none), `73bb` (functionless: none), `7cf5`
(`execute_exit_arm`: `7bf4 MOV EBX,0x1000` — 32-bit load, carry would give
`0x1014` ∉, clobber-unwalked → OPEN); overlay-bank `[BX+disp]` forms
named per review (class-OPEN — dynamic bases, no owner constant in the
33-site BX census): `1991:4523` `SI←CS:[BX+0x44b8]` (owner
`FUN_1991_44d6` — the same body owning the `452c`/`453e` CS-table
load/JMPF forms), `1991:4f05` `word ptr [BX + 0x4d7c],0x0` store (owner
`FUN_1991_4efe`, pairing with the `1991:4f91 JMPF CS:[BX+0x4d78]`
transfer above), `1991:0ce0`/`1991:0ce3` frame-store `[BX+0x22],SS` /
`[BX+0x26],BP` (owner `FUN_1991_0c9e` — IRET-frame builder shape, the
overlay analogue of the `11bd:0e22/0e28` orphan sites); plus implicit
`MOVS`/`STOS` `[SI]`/`[DI]` forms (no rendered operand — out of sweep
visibility, e.g. the `1ed6 f3a5 MOVSW.REP` seen slice-19 in `1e9f`).

### Reader table (confirmed reads of `[0x9ba]` / window-resolved hits)

| Reader site | Bytes | Render | Cell resolved | Value flow |
|-------------|-------|--------|---------------|------------|
| — none — | — | — | — | zero confirmed reads: the `0x9ba` literal runs return only the `6255` WRITE; the `[0x9b` absolute-family run returns 17 hits, of which `[0x9ba]` contributes exactly 1 (the store); the full `[base+disp]` partition resolves NO site into `0x9b8..0x9bf` |

Classification rows (all candidates, by disposition): `6255` = WRITE(the
store); `[0x9b8]`/`[0x9b4]`/`[0x9b6]`/`[0x9b2]` accesses = other-cell
(adjacency control); `092d`/`0934` = other-cell TRANSFERS (control — they
read `[0x9bc]`/`[0x9be]`, NOT `[0x9ba]`); `9bc`/`9be` superset extras
(`2dd6/2de7/2f18/44ab/4dcd/1991:20a6`) = false-string (near-target and
immediate-numeral renders, arithmetic shown above); constant-base window
candidates = REJECTED-with-arithmetic (table); dynamic sites = OPEN-WINDOW
(rows above).

### One-hop transfer question (Step 3)

Enumeration scope: owner dump `FUN_11bd_2d9c` = **127 instructions**
(`count:127`, body `2d9c..2ec8`) — the only body pulled because it owns
both the `0x9b8` immediate (`2dac`) and the nearest-miss window candidate
(`2df8`). No confirmed reader exists, so the per-reader chain walk is
VACUOUS at hop zero: there is no loaded value, therefore no
`CALL reg`/`JMP reg`/`[mem]`-target form can be fed from `[0x9ba]` within
one hop (nor any hop) in the static surface. What the surface does show,
cited as controls: (a) the family transfer pattern EXISTS on adjacent
cells — `092d ff26bc09 JMP word ptr [0x9bc]` (`dispatch_mode_vector`) and
`0934 ff26be09 JMP word ptr [0x9be]` (`FUN_11bd_0931`) — so a future
writer+reader pair on `[0x9ba]` would be recognizable by exactly these
renderings, and today `[0x9ba]` has none; (b) the store site itself is
transfer-free (`publish_mode_vector` 14 insns, 3/3 transfers literal,
slice-19); (c) dynamic-base transfers (`1991:0cf7/1f7d/33a5/33e1/453b/4f91`
CS-relative jumps) are OPEN-WINDOW holes with no `[0x9ba]` feeder.
`get_xrefs_to(0x9ba)` verbatim `{"references":[],"count":0,…,"total":0}`;
same for `11bd:09ba` form; `list_data_items_by_xrefs(filter=all,
type_filter=all,min_xrefs=1,limit=30)` verbatim
`{"data_items":[],"count":0,"offset":0,"limit":30,"total":0}` — control
only, dead channel for absolute-operand forms per slices 16/18/19, reported
not relied upon.

### Disposition-so-far (Step 4)

**NONE-FROM-DISCIPLINE** — of the four allowed verdicts: READERS-FOUND-*
cannot apply (the reader table is empty: no literal read survived any of
the 5 direct-pattern runs, no window hit survived per-candidate
arithmetic); PARTIAL cannot apply (no reader class resolved even
provisionally); READERS-FOUND-REACH-TRANSFER is refuted at its first
condition. Armed-conditional phrasing (exact, once): the store at `6255`
fires only when the `3ed8` dispatch cascade arms `0x29bc` through the
`452f`/`4536`→`6250` path; even in that armed state, within the enumerated
static surface (61 recorded `search_instructions` calls — 59 distinct
patterns — × 14,006 defined instructions, 4 constant
base-load censuses, complete `[base+disp]` form partition, 127-instruction
owner dump, controls quoted) the cell value is NEVER read and therefore
reaches NO transfer — it dies in data at the cell. The negative binds the
enumeration, not existence: OPEN-WINDOW rows, gap/orphan-region code, the
`1991:` overlay, implicit string forms, and the runtime DS-reload chain
(`[0xcec]` → `66f6`) remain visible holes. Slice-19's named-and-deferred
leg is now discharged at this layer: the deferred consumer question has an
answer WITHIN static scope, and the `0x29bc` entry-lead chain (slices
17→19) acquires no new transfer path through `[0x9ba]` — what survives of
the entry question stays exactly in the slice-17 runtime legs plus the
holes above.

### Writes (ZERO-WRITE branch)

NONE. Read-only pass: no `disassemble_bytes` (dry-run or real — no reader
materialized, so the one-hop walk had nothing to dry-run INTO), no
`create_function`, no rename, no comment, no `save_program`; no Ghidra
transaction opened. Task-1 tool inventory: `search_instructions` ×61 calls
(59 distinct patterns — all `truncated:false` responses quoted with counts;
two cap-truncated discovery runs superseded by complete partitions/targeted
negatives, one display-truncated response re-quoted live in the fix round),
`get_function_by_address` ×3 (`2d9c` body `2d9c..2ec8`; `6a68` body
`6a68..6aab`; `6ae2` → `{"error":"No function found for 11bd:6ae2"}` —
the orphan-region no-owner proof behind the OPEN rows),
`disassemble_function` ×1 (`FUN_11bd_2d9c`, 127 instructions),
`read_memory` ×5 (`6255`4, `092d`4, `0934`4, `626d`18, `2df5`8 —
hex-vs-data 5/5 MATCH, quoted verbatim in the anchors table),
`get_xrefs_to` ×2 (controls, both ×0), `list_data_items_by_xrefs` ×1
(control, empty envelope quoted), `analyze_dataflow` ×2 (1 candidate-varnode
error `No varnode at 11bd:2df8 matches 'BX'. Candidates: [in_DS, in_SS,
puVar10]` superseded by the `puVar10` retry — resolved anchor `SP`,
terminated "chain exhausted", reconciled to the dump, disassembly remains
authority). `/media/felipe/FIFAPCCD/` untouched; pre-existing function
state re-read only; `fifa96.rep` churn left unstaged.

### Fix round (review 2026-09-29) — row corrections, each verified live

Read-only verification calls this round (added to the Task-1 inventory):
`search_instructions` ×3 (`[BX + 0` envelope re-run — verbatim tail
`"match_count":320,"instructions_scanned":14006,"truncated":false`;
operand `0x1ea6` — verbatim `{"matches":[{"address":{"address":"1991:20d0"},"mnemonic":"MOV","operands":"DI, word ptr CS:[BX + 0x1ea6]","length":5,"bytes":"2e8bbfa61e"}],"match_count":1,…,"truncated":false}` — the sole hit is a LOAD, not a transfer; the first call's envelope was lost to display truncation and re-confirmed on a second identical call),
`get_function_by_address` ×5 — verbatim
`{"error":"No function found for 1991:2034"}`,
`{"error":"No function found for 1991:20d0"}`,
`{"error":"No function found for 1991:3920"}` (functionless overlay band —
Important 1), `{"name":"file_read_far_dos","address":"11bd:6003",
"body_start":"11bd:6003","body_end":"11bd:601c"}`,
`{"name":"FUN_11bd_4ca1",…,"body_start":"11bd:4ca1","body_end":"11bd:4df6"}`,
`disassemble_function` ×2 — `file_read_far_dos` `count:13`, first insn
`6003 MOV BX,SP` (so `6009/600c/600f` are arg-frame reads of the CALLER'S
frame with NO segment override — Important 2; the `5ef6` REJECT stands on
its own-body base `5e9e BX=0x3`); `FUN_11bd_4ca1` `count:128` — BX cursor
`4cab→4cae→4cb1` from `[BP+0x8]/[BP+0x4]`, no constant (Important 1 minor
scope for the `4cc3` OPEN row).
Corrections applied: (1) `1991:20d0` re-marked OPEN-WINDOW-with-candidate-
constant and moved out of the transfer list (it is a `MOV`); `1991:3897`/
`1991:2034` rows re-marked OPEN-with-candidate-constant (functionless band
— no provable carry), with the 39xx site count corrected to ×10; (2) the
`5ef6`/`6009` row split — `6009/600c/600f` now cited to their own body's
`6003 8bdc MOV BX,SP` frame base, the phantom `ES:` on `6009` removed;
(3) the exhaustiveness claim restated (preceding-constant-only resolution;
ordering-precedence sites OPEN) and ALL review-named sites enumerated as
class-OPEN rows (`0d65-0d6b`, `129a`, `1499`, `3012/302a/303a`,
`307d-30aa`, `5b12-5b3a`, `5c4e`, `5fba/5fcc/5fe4/5fe7`, `61b3-61da`,
`63ef`, `65cb`, `665e`, `6685/669d`, `686b/686e`, `7162`, `73bb`, `7cf5`);
the orphan-region mislisting of `4bfc..4dc7` corrected (live owners);
(4) run-total label restated as 61 calls / 59 distinct patterns reconciling
the table; (5) `[BX + 0` envelope quoted (320, `truncated:false`) —
reconciles the digit partition sum; (6) `4cc3` row gains its 128-instruction
dump scope. The NONE-FROM-DISCIPLINE verdict is unchanged: every re-marked
row is a NO site for `[0x9ba]` consumption (none is a `[0x9ba]` read; the
candidate constants miss the window even if carried); the re-marks widen
the honest-hole list, not the reader set.

**Round 2 (scoped re-review, 2026-09-29):** the pagination claim challenged
as unevidenced is LIVE-EVIDENCED — `search_instructions(program=
/fifa96.exe, operand_pattern="[BX +", offset=500, limit=1)` returned the
index-500 slot as match index 0: first match verbatim
`{"address":{"address":"1000:0009"},"function":"FUN_1991_0c9e",
"mnemonic":"ADD","operands":"byte ptr [BX + SI], AL","bytes":"0000"}`,
`"match_count":1,"instructions_scanned":5,"truncated":true` — the tool
accepts `offset` but ignores it; the `[BX +` row now carries this
reproduction, so the 61 calls = 59 distinct patterns + 2 duplicates
(`[BX +` offset-retry, `[BP]` re-run) reconciliation stands (the fix-round
probe itself is excluded from the 61). Headline unit aligned at every
mention (sweeps total row, disposition paragraph — "61 recorded
`search_instructions` calls — 59 distinct patterns — × 14,006 defined
instructions", Writes row `×61 calls (59 distinct)`). Overlay disp-forms
`1991:4523` (`FUN_1991_44d6`), `1991:4f05` (`FUN_1991_4efe`) and
`1991:0ce0`/`0ce3` (`FUN_1991_0c9e`) given an explicit named row in the
OPEN enumeration. Verdict and all window dispositions unchanged; zero
Ghidra writes (one read-only search call this round).

### Disposition (Task 2 — settled 2026-09-29)

**NONE-FROM-DISCIPLINE** — final disposition for this slice, restating the
Task-1 verdict with the sweeps' totals as enumeration scope and the
three-way consumer outcome cited (every Task-1 table above is consumed by
reference, not rewritten):

1. *Enumeration scope*: **61 recorded `search_instructions` calls — 59
   distinct pattern runs + 2 duplicate executions — × 14,006 defined
   instructions**: 11 literal-family runs (the `0x9ba`/`9ba`/`[0x9ba]`
   patterns return only the `6255` WRITE), the 9-run constant base-load
   inventory (censuses **BX 33 / SI 59 / DI 34 / BP 1**), the
   form-complete `[base+disp]` partition over the four ModRM bases plus
   bare-register and base+index encodings (the `[BX + 0` envelope 320
   reconciled with its 16-run digit partition; the two cap-truncated
   discovery runs superseded by complete partitions or targeted negatives,
   pagination no-op evidenced live), and the absolute-cell family sweep
   `[0x9b` = **17 hits, of which `[0x9ba]` contributes exactly 1 — the
   store; reads: ZERO**. Every constant-base candidate with proven carry
   resolves arithmetically OUT of `0x9b8..0x9bf` (Base→window arithmetic
   table); the reader table is empty-with-reason; controls quoted
   (`get_xrefs_to(0x9ba)` ×0, `list_data_items_by_xrefs` empty envelope —
   dead channel, reported not relied upon).
2. *Three-way consumer outcome*: **NONE-FROM-DISCIPLINE**. READERS-FOUND-*
   and PARTIAL are refuted jointly by the reader table and the per-candidate
   arithmetic — no confirmed read of `[0x9ba]` survives any enumerated run;
   the one-hop transfer question is **vacuous at hop zero** (no reader ⇒ no
   loaded value ⇒ no `CALL reg`/`JMP reg`/`[mem]`-target form fed from
   `[0x9ba]` at any hop in the static surface). Sensitivity proven by
   adjacency, not assumed: the same sweeps DO find `JMP word ptr`
   transfer-consumers on the neighbor cells `[0x9bc]` (`092d`) and `[0x9be]`
   (`0934`) — the discipline detects this shape when present; `[0x9ba]`
   carries none.
3. *Armed-conditional (stated once, here, not per row)*: the store at `6255`
   fires only when the `3ed8` dispatch arms value `0x29bc` into slot
   `[BP+-0x5a]` and pushes it (`452f`→`4536`→`6250`) — readers could exist
   only under armed-state reasoning; even in that armed state **the `0x29bc`
   value reaches NO indirect transfer — NO-CONSUMERS-STATICALLY** (not
   NO-DATA-ONLY: that answer presupposes readers consuming the value as
   data, and there are none), and the value dies in data at the cell — to
   the extent static sweeps can say so; terminal form = the `6255` store
   itself.
4. *Entry-lead chain (slices 17→18→19→20)*: **CLOSED-with-answer-as-far-as
   static-sweeps-go.** The chain: slice-17 named the `0x29bc` slot stores;
   slice-18 settled the slot readers as DATA-ONLY into DIRECT calls;
   slice-19 settled callee `6250` as verbatim CELL-STORAGE at `[0x9ba]`
   with zero indirect in body; this slice settles the cell's consumers as
   NONE-FROM-DISCIPLINE. At no layer does the lead reach a transfer.
   **Nothing re-extends**: with zero readers there is no reader's-own
   consumers to name-and-defer (the one-layer guard has nothing to guard —
   the chain terminates AT the cell within static scope). What remains
   dynamic is NOT a dived lead: it lives in the OPEN-WINDOW rows (the
   enumeration's honest holes — dynamic bases `2df8`/`4cc3`,
   functionless-band OPEN-WINDOW-with-candidate-constant sites
   `1991:20d0`/`1991:3920..3964`,
   ordering-precedence sites `62d7`/`63ef`/`0d65-0d6b`, transfer-form
   OPENs `1991:0cf7/1f7d/33a5/33e1/453b/4f91`) and in the runtime legs
   (`[0x40]`/`[0x9c0]`, the `66f6` DS-reload chain, gap/orphan-region code,
   the `1991:` overlay bank, implicit `MOVS`/`STOS` forms).

The negative binds the enumeration, not existence (honesty rule): this
disposition records "no consumers attributable from the enumerated static
sweeps", never "no consumers exist".

No `### Entry function` row is emitted and the capped write path was NOT
executed — both were authorized only under materialization (READERS-FOUND
**and** a statically-resolved indirect-transfer target **and** a citable
dry-run boundary), whose first condition is already false.

### Writes (Task 2 — NONE-FROM-DISCIPLINE branch: ZERO writes; live unmoved-proof pair)

NONE. The program was not touched: the only Ghidra calls executed this
task are the two read-only read-backs below, quoted verbatim (live,
this task, program `/fifa96.exe`), in slice-18's unmoved-proof form:

| Probe | Verbatim response (live, this task) | Unmoved check |
|-------|--------------------------------------|---------------|
| `get_function_by_address(11bd:29bc)` | `{"error":"No function found for 11bd:29bc"}` | byte-identical to slice-18's and slice-19's Task-2 quotes — no function was created at the lead cell by this slice |
| `find_code_gaps(min_size=1)` | envelope `{"total":131,"offset":0,"limit":100,…}`; covering row verbatim: `{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"FUN_11bd_2adb","after_function_address":"11bd:2adb"}` | block `11bd:2978..2ada` (`4548−1bd0=2978`, `46aa−1bd0=2ada` ✓) still **size 355**, `has_undefined_bytes:true`, same neighbors — unchanged vs slice-17/18/19's rows and Task-1's re-read; no shrink, no reflow |

Write-tool inventory for this task: zero — no `disassemble_bytes`
(materialization branch not triggered; not even dry-run — there was no
cited reader or target to walk), no `create_function`, no
`rename_function`/`rename_symbol`, no `set_comment`/`batch_set_comments`,
no `set_global`, no `save_program`; no Ghidra transaction was opened; the
listing was not modified in any way. `/media/felipe/FIFAPCCD/` untouched;
pre-existing function state re-read only; `fifa96.rep` churn left
unstaged.

### Deferrals (Task 2)

- **OPEN-WINDOW holes stay:** classes with example rows cited by address —
  dynamic-base sites (`2df8`, `4cc3`), functionless-band
  candidate-constant sites (`1991:20d0`, `1991:3920..3964`),
  ordering-precedence sites (`62d7`, `63ef`, `0d65/0d68/0d6b`),
  transfer-form OPENs (`1991:0cf7/1f7d/33a5/33e1/453b/4f91`), stub/orphan/
  overlay-bank and implicit `MOVS`/`STOS` forms — enumerated above; they
  are holes in the enumeration, not leads dived; closing them needs
  runtime-side evidence, out of slice scope.
- **Reader consumers: N/A** — zero readers were confirmed, so there is no
  reader-of-a-reader to name-and-defer; the one-layer guard terminated at
  the cell itself.
- **Block `2978..2ada` stays fully named-open:** the size-355 covering gap
  row is quoted under `### Writes` above; zero writes occurred, so no
  shrink arithmetic applies (the materialization split was never
  triggered).
- **`[0x9b8]`/`[0x9bc]`/`[0x9be]` family roles untouched (cite-only):**
  `[0x9b8]` READ-only config-string cell, `[0x9bc]`/`[0x9be]` carry the
  ONLY transfer-consumers seen in the family (`092d`/`0934` `JMP word
  ptr`) — those two cells' consumers belong to their own story (slice-8
  publish writes, `dispatch_mode_vector`/`FUN_11bd_0931` reads) and were
  not re-opened here.
- **`1e9f`/`6250` full verdicts stay non-goals:** current status as
  recorded — `FUN_11bd_1e9f` default-named, NONE-INDIRECT per slice-19;
  `publish_mode_vector` slice-8 name live, CELL-STORAGE per slice-19;
  this slice cites both only as chain facts (the `6255` store inside the
  latter's body; the `4536` direct-call arrival into it).
- **Runtime legs unchanged:** `[0x40]` contents (slice-15 direction
  UNDECIDED) and `[0x9c0]` installer identity (NOT-IN-EXE,
  runtime-written) — a zero-write pass occurred; nothing in this slice
  could have moved them.
- **Prior-slice statuses, exact:** this section **closes slice-19's
  deferral line 1 ("`[0x9ba]` consumers = the next lead") — now
  dispositioned**: the consumers were enumerated and answered
  NONE-FROM-DISCIPLINE, terminating the `0x29bc` entry-lead chain at the
  cell within static scope; slice-19's remaining deferrals stand,
  slice-18's remaining deferrals stand (its line 1 was already discharged
  in slice-19), and slice-17's open legs (fall-in `2811..296c`,
  runtime-installed pointers, `CS:[0x2ad9]` tail cell) stand as
  recorded; slice-8 `publish_mode_vector` records unchanged.

### Fix wave (final whole-branch review 2026-09-29 — DI-negative ledger completion + two word-level corrections; every claim verified live this wave; zero Ghidra writes)

Read-only calls this wave: `search_instructions` ×4 (`[DI + -` full run —
verbatim `match_count:6`, `instructions_scanned:14006`, `truncated:false`;
`[0x9b` re-run for the Minor-2 count — verbatim `match_count:17`,
`truncated:false`; two function-scoped `MOV`+`DI, 0x` censuses — owner
`FUN_11bd_79fc` `match_count:1`, sole hit `{"address":"11bd:7a0e",
"operands":"DI, 0x4","bytes":"bf0400"}` (`instructions_scanned:49`), owner
`FUN_11bd_7290` `match_count:0` (`instructions_scanned:59`)),
`get_function_by_address` ×5 (`11bd:7c37` → verbatim
`{"error":"No function found for 11bd:7c37"}`; `11bd:7c2b` → same-shape
error; `11bd:7a11` → `FUN_11bd_79fc`, body `79fc..7a87`; `11bd:7311` →
`FUN_11bd_7290`, body `728b..7321`; `1991:20ae` →
`{"error":"No function found for 1991:20ae"}`), `disassemble_function` ×1
(`FUN_11bd_79fc`, `count:49` — the `7a11` precedence proof), `read_memory`
×4 (hex-vs-data 4/4 MATCH: `11bd:7311`3 `[141,101,250]`→`8d65fa` ✓;
`11bd:7a11`5 = `11bd:7c37`5 `[38,102,139,69,252]`→`26668b45fc` ✓ — a
byte-identical load-twin pair; `1991:20a6`16
`[117,6,199,70,0,56,0,64,57,78,2,117,6,199,70,2]`→
`7506c74600380040394e027506c74602` ✓), `get_xrefs_to(11bd:7a11)` ×1
(dead-channel control, ×0 — cited-precedence from the body dump remains
authority), `find_code_gaps(min_size=1)` ×1, `get_address_spaces` ×1
(single default `ram` space — the `1000:`/`11bd:`/`1991:` renders are the
same linear bytes under different paragraph notations; the gap envelope
itself crosses notations (`end":"1991:597f"`), evidencing the
`+0x1bd0`/`−0x9910` deltas used below). Wave runs recorded HERE, per the
standing convention (fix verification runs are excluded from the 61-call /
59-pattern reconciliation, cf. the Fix round section).

**(a) `[DI + -` full ledger — all six sites of the run, complete
dispositions.** Important 1: three negative-DI sites (`7311`, `7a11`,
`7c37`) appeared in NO ledger row, while the `[DI +` census row promised
"reaches checked per-site below" and the exhaustiveness paragraph promised
class-OPEN sites are "enumerated below". The run's other three sites
(`2347`, `63de`, `63e1`) were already resolved in the Base→window table.
Restating all six closes both promises; 3 committed + 3 appended = 6 = the
run's `match_count:6` — no unaccounted sixth site; the reviewer's 3-vs-6
arithmetic reconciles exactly.

| Site | Render (verbatim, bytes) | Owner | Disposition | Per-site arithmetic / precedent |
|------|--------------------------|-------|-------------|--------------------------------|
| `11bd:2347` | `CMP byte ptr [DI + -0x1],0x0` (`807dff00`) | `print_error_message` | REJECT (committed row — restated for ledger completeness) | `232d DI=0x15e8` → `0x15e7` ∉ `0x9b8..0x9bf` |
| `11bd:63de` | `SUB AX,word ptr [DI + -0x2]` (`2b45fe`) | `FUN_11bd_6395` | REJECT (committed row) | `6396 DI=0x6341` → `0x633f` ∉ |
| `11bd:63e1` | `SUB DI,word ptr [DI + -0x4]` (`2b7dfc`) | `FUN_11bd_6395` | REJECT (committed row) | `63ac DI=0x4a` → `0x46` ∉ (alt ordering `6396` → `0x633d` ∉) |
| `11bd:7311` | `LEA SP,[DI + -0x6]` (`8d65fa`) | `FUN_11bd_7290` (body `728b..7321`) | **NON-LOAD — appended row** | LEA computes an address, never reads memory — the doc's own `6198`/`1991:3872` (and `1868`) LEA treatment; owner additionally has NO constant DI load (function-scoped census `match_count:0`) — moot either way |
| `11bd:7a11` | `MOV EAX,dword ptr ES:[DI + -0x4]` (`26668b45fc`) | `FUN_11bd_79fc` (body `79fc..7a87`; 49-insn dump this wave) | **REJECT-with-arithmetic — appended row** (live evidence CORRECTS the review's "no constant DI base" parenthetical: the owner DOES have a constant base and it ADJACENTLY precedes) | `7a0e bf0400 MOV DI,0x4` is the immediately-preceding instruction — zero insns between it and `7a11`, no body branch renders a target at `0x1000:95e1`(`=7a11`); exactly the `626d`→`6270` cited-precedence form. Carried: `0x4 − 0x4 = 0x0` ∉ `0x9b8..0x9bf` (dword reach `0x0..0x3` ∉; additionally an ES-override read — the DS-default cell is not even in the addressed segment). Unprovable-base treatment would file it class-OPEN; under EITHER label it is NOT a read of `[0x9ba]` — verdict-neutral, negative survives |
| `11bd:7c37` | `MOV EAX,dword ptr ES:[DI + -0x4]` (`26668b45fc` — byte-identical twin of `7a11`) | **none** — `{"error":"No function found for 11bd:7c37"}` | **class-OPEN, functionless-in-orphan-gap — appended row**, exactly as reviewed; same no-owner-dump class as the `73bb` row | sits inside the NAMED orphan gap — `find_code_gaps` row verbatim `{"start":"1000:97fb","end":"1000:9831","size":55,"has_undefined_bytes":false,"has_orphaned_instructions":true,"before_function":"FUN_11bd_7be9","after_function":"execute_exit_arm","after_function_address":"11bd:7c62"}`; deltas `0x97fb−0x1bd0=7c2b` ✓, `0x9831−0x1bd0=7c61` ✓, site `0x7c37+0x1bd0=0x9807` ∈ gap ✓; the earlier `7c62` section already records this `11bd:7c2b..7c61` EMS block unenclosed (`get_function_by_address(11bd:7c2b)` same error live); no owner dump exists to cite → never silently rejected |

Six-site outcome: ZERO new readers of `[0x9ba]` (two appended
resolutions, one appended class-OPEN hole, one NON-LOAD); the
NONE-FROM-DISCIPLINE verdict and every committed disposition stand
unchanged; the OPEN enumeration gains `7c37` as a named hole.

**(b) Two Minor corrections of committed wording (the committed rows are
NOT rewritten here; these are the authoritative readings):**

- **Minor 2 — the `[0x9b` sweeps row:** as-written "the 5 `[0x9b8]` rows"
  is a miscount. The live re-run (17 hits, `truncated:false`, scanned
  14006) contains exactly **4** bracketed `[0x9b8]` sites — `39b4`,
  `466c`, `4677`, `57af`. `2dac MOV AX,0x9b8` is an UNBRACKETED immediate
  — correctly absent from the `[0x9b` run (it lives on the `0x9b8`
  literal row, where it belongs). Read the row as "the 4 `[0x9b8]` rows":
  the enumeration then sums `6ea2`1 + `[0x9b4]`4 (`199f`/`5f7f`/
  `1991:0ff3`/`1991:2a13`) + `[0x9b6]`3 (`1a1d`/`1ef1`/`1000:0b9d`) +
  `[0x9b8]`4 + `6255`1 + `[0x9bc]`2 + `[0x9be]`2 = **17**, matching the
  stated (correct) total; as-written it summed 18.
- **Minor 3 — the `9be` sweeps row, `1991:20a6` parenthetical:** the
  cited-only rejection "other-bank code target" lacked the bank-render
  delta arithmetic its siblings (`4dcd`: `eb1f` → `4dcd+2+0x1f=4dee`;
  `69be−1bd0=4dee`; `2dd6/2de7`: `49bc−1bd0=2dec`) carry. Quoted form
  now: live `read_memory(1991:20a6,16)` hex
  `7506c74600380040394e027506c74602` — `7506` = `JNZ rel8` →
  `20a6+2+6 = 20ae`, same bank; the rendered `0x1000:b9be` is that
  landing's other paragraph notation:
  `0x19910+0x20ae = 0x1b9be = 0x10000+0xb9be`, i.e. `b9be − 0x9910 = 20ae`
  (the 1991↔1000 paragraph delta). The landing holds DEFINED CODE
  (`39 4e 02` = `CMP word ptr [SI+0x2],CX`, read bytes; `1991:20ae` is
  functionless orphan code inside the `has_orphaned_instructions:true`
  gap `1000:b860..bab2`) — a branch-flow target, NOT a data-cell operand:
  the FALSE-STRING rejection stands, now with its arithmetic cited.

Write-tool inventory for this wave: **zero** — no `disassemble_bytes`, no
`create_function`, no rename, no comment, no `set_global`, no
`save_program`; no Ghidra transaction opened; the listing was not modified
(`/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left unstaged).
The negative survives: all three findings were enumeration hygiene; the
completed six-site ledger adds no reader of `[0x9ba]`.

## 2811..296c pocket + 9bc vector (verified 2026-09-29, program `/fifa96.exe`)

Zero-Ghidra-write classification pass on the left half of the original hole
`11bd:2811..2ada` — the pocket `2811..296c` (row `1000:43e1..1000:453c`,
size 348), the `[0x9bc]`/`[0x9be]` dispatch-vector consumer sweep, and the
`[BP+-0x5a]` 14-arg determinability table. Headline: the pocket is one
386-mode PM-transition stream — `2811..281f` PADDING (15 zero bytes),
`2820..2823` TABLE (the `[0x9bc]`/`[0x9be]` vector pair for base `0x2824`:
words `0x2864`/`0x284c`, both landing on stream instruction boundaries),
then CODE `2824..296c` with two tool-skip windows (`28e7..28eb`, `28ed`)
and tail `RET` `c3` at `296c` exactly at the gap edge. The `282c` recursion
of the `restore_fs_gs_and_resume` body re-derives byte-exact for the two
segment restores (`8e26600d 8e2e620d` @ `282c..2833` ↔ `296d..2974`),
diverging at `2834` vs `2975` (the body's tail `JMP` `e93dd9` is NOT
replicated). The consumer sweep reproduces slice-20's numbers exactly
(`0x9bc`→2, `9bc`→6, `0x9be`→2, `9be`→4; sole reads `092d`/`0934`, sole
writes `6274`/`627b`). All 14 dispatch args determine both vector words
byte-for-byte, and every resolved target insn is dry-run-cited: 0 targets
land in defined function bodies, 2 in the pocket (the `0x2824` override
pair), 2 in block `2978..2ada` (the `0x29bc` pair), 24 in three functionless
gap bands (`02d4..0732`, `073c..0928`, `0938..0bd0`) that decode a uniform
`PUSH AX; PUSH BX; MOV BX,<base>` preamble + `CLI` body entry — the mode
dispatch landing layer is itself unowned code. Quote protocol: 22
`read_memory` responses reconciled hex-vs-data — 21/22 internally MATCH;
one wide-window `data`-array glitch (`2937`: data `40` vs hex `40` vs
disassembler `ba4001`) resolved by a fresh narrow read + the dry-run bytes
(three-way agreement → byte `0x40`, `MOV DX,0x140`), the same render-path
class as slice-17's `23c1/23c8` note; all pocket quotes below are the
reconciled values. `search_instructions` is the consumer authority (defined
instructions only — `instructions_scanned:14006` on every run AT TASK-1 TIME;
post-Task-2 sweeps report 14170 as this slice's own defines grew the defined
count — historical value correct for the zero-write pass); xrefs
and data-items channels reported-not-relied (both cell probes returned 0
despite the defined `092d/6274/0934/627b` references — dead channel per
slices 16/18/19/20).

### Edge confirmation (Step 1)

| Probe | Verbatim response | Reconciliation / math |
|-------|-------------------|------------------------|
| `get_function_by_address(11bd:2811)` | `{"error":"No function found for 11bd:2811"}` | pocket start unowned ✓ |
| `get_function_by_address(11bd:296c)` | `{"error":"No function found for 11bd:296c"}` | pocket end unowned ✓ |
| `find_code_gaps` (total 131, offset 0, limit 100) pocket row | `{"start":"1000:43e1","end":"1000:453c","size":348,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_27e4","before_function_address":"11bd:27e4","after_function":"restore_fs_gs_and_resume","after_function_address":"11bd:296d"}` | `0x43e1 − 0x1bd0 = 0x2811` ✓, `0x453c − 0x1bd0 = 0x296c` ✓, size `0x453c − 0x43e1 + 1 = 0x15C = 348` ✓ = `296c − 2811 + 1` ✓; neighbors `FUN_11bd_27e4` (body ends `2810`) before / `restore_fs_gs_and_resume` (body starts `296d`) after — byte-identical to slice-17's left-pocket row (flags included: `has_undefined_bytes:false` is that row's recorded state, quoted not explained) |

### Byte-run classification (Step 2)

Walk scope: single dry-run `disassemble_bytes` window `11bd:2811`, length
`0x15c` (348), `max_instructions:400` → `{"dry_run":true,"success":true,
"start_address":"11bd:2811","end_address":"11bd:296c","bytes_disassembled":
348}` — 133 emitted instructions, 327 bytes emitted coverage; non-emitted:
`2811..281f` (15 B), `28e7..28eb` (5 B), `28ed` (1 B). Last emitted insn:
`296c RET` (`c3`, length 1) — the stream tails exactly one byte before the
`296d` function start (slice-14 boundary claim reproduced end-to-end).
Byte truth: four contiguous windows (96+96+96+60 = 348 B) reconciled
data↔hex before use.

| byte-run | range | evidence (hex+data reconciled) | class | entry/exit or table-role or pad-form |
|----------|-------|--------------------------------|-------|--------------------------------------|
| R1 | `2811..281f` (15 B) | W1 window bytes `000000000000000000000000000000` (W1 data[0..14] all 0 ↔ hex ✓) | PADDING | pad-form: 15×`0x00` zero-fill; dry-run non-emit (first emitted insn at `2820`) — slice-14's first skip window |
| R2 | `2820..2823` (4 B) | W1 `64284c28` (data `[100,40,76,40]` ✓) → LE words `[0x2820]=0x2864`, `[0x2822]=0x284c` | TABLE | role: `[0x9bc]`/`[0x9be]` vector pair for base `0x2824` — read at `6270 2e8b47fc MOV AX,CS:[BX+-0x4]`→`6274 a3bc09` and `6277 2e8b47fe MOV AX,CS:[BX+-0x2]`→`627b a3be09` (BX=`0x2824` via `626d bb2428` override; fresh 18-B read `bb24282e8b47fca3bc092e8b47fea3be09c3` data↔hex ✓ = slice-20 mirror chain verbatim); both values land on emitted boundaries (`2864 PUSH AX 50`, `284c PUSH BX 53`); tool's form-collision at `2820` (`SUB byte ptr FS:[SI + 0x28], CL`) quoted as emission artifact |
| R3 | `2824..28e6` (195 B) | clean contiguous emission `2824 MOV [0xd66],GS` (`8c2e660d`) … `28e4 MOV AX,[0xf56]` (`a1560f`) | CODE | entry-side segment saves + CR0/CMOS/port block; last-insn end cited: `28e6` |
| R4 | `28e7..28eb` (5 B) | W3 raw `26a36704a1`; emission jumps `28e4..28e6` → `28ec` | UNKNOWN-flagged (tool non-emit; slice-14's named skip) | hand shape: `28e7 26a36704` = `MOV word ptr ES:[0x467],AX` (form twin of the emitted `28ee 26a36904 MOV ES:[0x469],AX` — skip is emission-local, not capability) + `28eb a1` = head of `MOV AX,[0xf58]` (`a1580f`); resync emitted insn: `28ec POP AX` (`58`) |
| R5 | `28ec` (1 B) | emitted `58 POP AX` | CODE (stray resync alignment — byte is mid-`a1580f`; classification carries the flag) | forward-pointer: no define adopted — final state left undefined in the `28e7..28ed` interior (no-function probe `11bd:28ec` + row re-page quoted in `### Writes`) |
| R6 | `28ed` (1 B) | W3 byte `0f`; non-emit | UNKNOWN-flagged | stream re-joins at `28ee` |
| R7 | `28ee..296c` (127 B) | clean contiguous emission `28ee MOV ES:[0x469],AX` (`26a36904`) … `296c RET` (`c3`) | CODE | exit: `296c RET` = stream tail, pocket edge −0 ✓; contains the `296d` mirror region (see recursion below), the `ea` far-flush at `28b6`, PIT/CMOS port legs, `CMP [0x2e],…` switch `28f4..2926` |

Tiling check (mandated — printed sizes sum): `15 + 4 + 195 + 5 + 1 + 1 +
127 = 348` = `0x296c − 0x2811 + 1` ✓; maximal runs tile the pocket, every
byte classified exactly once. Emitted-coverage cross-check:
`4 (R2 collision) + 195 (R3) + 1 (R5) + 127 (R7) = 327` = the walk's
`348 − (15+5+1)` skipped bytes ✓.

Stream landmarks (cited from the same emission): `2880 MOV [0xd62],GS`
(`8c2e620d`) + `2884 MOV [0xd60],FS` (`8c26600d`) — the save pair the
`296d/2971` restores reload (slice-16 claim re-derived byte-exact; role
wording fixed: `2880` saves GS, `2884` saves FS); `2824/2828` = second save
pair (GS→`[0xd66]`, FS→`[0xd64]`) restored at `28c2/28c6` (`8e2e660d`,
`8e26640d`); `288f MOV EAX,CR0` `0f20c0` / `28b3 MOV CR0,EAX` `0f22c0` /
`28b6 JMPF 0x1000:448b` (`eabb28bd11` — offset `0x28bb`, segment `0x11bd`;
render `0x1000:448b − 0x1bd0 = 28bb` ✓ self-far-flush to the next insn —
the byte-level proof that the transition executes at CS paragraph `0x11bd`,
which is the assumption under which all `JMP word ptr` targets below are
read as `11bd:word`); `28a5 STR word ptr [0xdfc]` (`0f000efc0d`); LIDT
`28ca` (`0f011edc0d`); `28aa MOV CX,[0x40]` (runtime cell — cite-only per
scope guard); `2866 MOV BX,[0x9b4]` + `28d7`/`2908..2922` mode byte
`[0x2e]` switch; `296c RET`.

Recursion re-derivation (`282c` vs `296d` body, byte-by-byte): pocket
`282c..2836` = `8e26600d 8e2e620d 33` vs body `296d..2977` =
`8e26600d 8e2e620d e93dd9` (W5 read `[142,38,96,13,142,46,98,13,233,61,217]`
↔ hex `8e26600d8e2e620de93dd9` ✓ MATCH): identical at all 8 paired offsets
(`282c↔296d`, `282d↔296e`, `282e↔296f`, `282f↔2970`, `2830↔2971`,
`2831↔2972`, `2832↔2973`, `2833↔2974`); DIVERGENT from `2834↔2975`
(`33` `XOR AX,AX` vs `e9` `JMP` rel16) and `2835↔2976`. The recursion is
the two segment-restore instructions verbatim, NOT the 11-byte body — the
pocket's `282c..2833` run is inside stream run R3 and decodes clean both
ways.

### Vector consumer sweep (Step 3a)

Sweep runs (all `search_instructions`, program scope, at Task-1 time every
`instructions_scanned:14006`, `truncated:false`):

| pattern run | match_count | hits + classification |
|-------------|-------------|------------------------|
| operand `0x9bc` | 2 | `092d` `JMP word ptr [0x9bc]` `ff26bc09` (`dispatch_mode_vector`) = TRANSFER-READER; `6274` `MOV [0x9bc],AX` `a3bc09` (`publish_mode_vector`) = WRITE — slice-20's 2 REPRODUCED exactly |
| operand `9bc` | 6 | above 2 + 4 FALSE-STRINGS: `2dd6` `JNZ 0x1000:49bc` `7514` (`2dd6+2+0x14=2dec`; `49bc−1bd0=2dec` in-body `FUN_11bd_2d9c` `2d9c..2ec8`), `2de7` `JNZ 0x1000:49bc` `7503` (`2de7+2+3=2dec` ✓), `2f18` `MOV [BP+-0x18],0x29bc` `c746e8bc29`, `44ab` `MOV [BP+-0x5a],0x29bc` `c746a6bc29` (immediate-numeral renders) — slice-20's 6 REPRODUCED exactly |
| operand `0x9be` | 2 | `0934` `JMP word ptr [0x9be]` `ff26be09` (`FUN_11bd_0931`) = TRANSFER-READER; `627b` `MOV [0x9be],AX` `a3be09` = WRITE — slice-20's 2 REPRODUCED |
| operand `9be` | 4 | above 2 + `4dcd` `JMP 0x1000:69be` `eb1f` (`4dcd+2+0x1f=4dee`; `69be−1bd0=4dee` in `FUN_11bd_4ca1` `4ca1..4df6`) + `1991:20a6` `JNZ 0x1000:b9be` `7506` (`20a6+2+6=20ae`; `b9be−9910=20ae` same-bank) — slice-20's 4 REPRODUCED. Divergence note: the FIRST execution (issued batched with three other runs) returned `match_count:2` (cell hits only, both false-strings absent); the immediate solo re-run returned 4 (full set above); targeted re-probes of the two false-strings each hit 1 (`69be` → `4dcd`; `b9be` → `1991:20a6`) — the count of 4 is the reconciled truth; the 2 is recorded as a batch-execution artifact, not a state difference |
| operand `[0x9bc]` | 2 | same two — bracket-render form adds no site |
| operand `CS:[0x9bc]` | 0 | negative — no CS-override render of the cell exists |
| operand `[0x9be]` | 2 | same two (`0934`/`627b`) |
| operand `CS:[0x9be]` | 0 | negative |
| mnemonic `MOV` + operand `0x2824` | 2 | `440e` `MOV [BP+-0x5a],0x2824` `c746a62428` (slot store, one of the 14) + `626d` `MOV BX,0x2824` `bb2428` — the override census = 2 sites, slice-20's row REPRODUCED |
| operand `[BX + -` | 10 | `4cc3` (ES-base runtime → OPEN per slice-20), `6198` LEA non-load, `6270`/`6277` = THE pair-source reads (`0x2824−4→0x2820` ✓ pocket, `0x2824−2→0x2822` ✓ pocket), `7687`/`768e` (base `7684 BX=0xf7d` → `0xf7a/0xf7b` — hook-patch stores, cell-IRRELEVANT to 9bc/9be, cite-only), `769a`/`76a4` (BX=0x2d0a → `0x2d07/0x2d08` — same patch cluster, cite-only), `1991:2f65` (dynamic), `1991:3872` LEA non-load — slice-20's 10-hit set REPRODUCED; NO negative-disp site resolves into cell `0x9bc/0x9be` |
| operand `[BX + 0` (envelope) | 320 | `truncated:false`, 14006 scanned — slice-20's 320 envelope REPRODUCED; per-site window arithmetic against `0x9bc/0x9be` is slice-20's complete Base→window ledger (no constant-BX pair anywhere yields the cells; nearest constant `6a97 BX=0x98e` — vacuous body per that round's partition) — cited by reference, not re-derived per hit |

Controls (reported, NOT relied — dead channel per slices 16/18/19/20):
`get_xrefs_to(11bd:09bc)` → `{"references":[],"count":0,"total":0}`;
`get_xrefs_to(11bd:09be)` → same empty envelope — despite four defined
referencing instructions.

Reader/writer closure: `[0x9bc]` and `[0x9be]` each have exactly ONE write
(`6274`/`627b`, both in `publish_mode_vector`, fed by the `6270/6277` CS
window read of the pair) and exactly ONE defined transfer reader (`092d`
/`0934`). The cells' armed content is therefore the pocket table word pair
whenever the override path runs, and `arg−4`/`arg−2` whenever it does not
(the gate logic is `publish_mode_vector`'s: `6259 CMP [0x2f],0x3` + `JC
→6270` (carry ⇒ `[0x2f]<3` ⇒ BX stays the ARG), `6266 CMP [0x2e],0x2` +
`JNZ →6270` (≠2 ⇒ BX = ARG); the `626d MOV BX,0x2824` override runs ONLY
when `[0x2f]>=3 AND [0x2e]==2`). The 0x29bc lead (slices 17–20) is now
resolved at this layer: with arg=`0x29bc` on the non-override path, the
pair loads `CS:[0x29b8]/CS:[0x29ba]` = `0x2a5a`/`0x2a60` — INSIDE block
`2978..2ada` (the fall-in leg slice-17 named, now evidenced at the word
level; block bytes still UNDEFINED listing, `has_undefined_bytes:true`).

### Stub owners and selection (Step 3b)

| Element | Address | Evidence | Calls (address only) |
|---------|---------|----------|----------------------|
| `092d` owner | `dispatch_mode_vector`, body `092c..0930` | `get_function_by_address(11bd:092c)` → `{"name":"dispatch_mode_vector","entry_point":"11bd:092c","body_start":"11bd:092c","body_end":"11bd:0930"}`; `disassemble_function` count:2 — `NOP` @`092c` + `JMP word ptr [0x9bc]` @`092d`, no RET (slice-12 state confirmed live) | callers (`get_function_callers`): `FUN_11bd_79fc`, `FUN_11bd_7a88`, `execute_exit_arm` — 3 |
| `0934` owner | `FUN_11bd_0931`, body `0931..0937` | `get_function_by_address(11bd:0934)` resolves through `{"name":"FUN_11bd_0931","body_start":"11bd:0931","body_end":"11bd:0937"}` — CONTEXT ANSWER: `0934` lives in the SECOND STUB FUN, not undefined bytes; `disassemble_function` count:4 — `NOP` @`0931`, `PUSH AX` @`0932`, `PUSH BX` @`0933`, `JMP word ptr [0x9be]` @`0934` | callers: `FUN_11bd_0d80` — 1 |
| stub-cluster bytes | `11bd:092c..0937` | `read_memory(11bd:092c,12)` → `[144,255,38,188,9,144,80,83,255,38,190,9]` ↔ `90ff26bc09905053ff26be09` ✓ — NOP; JMP`[0x9bc]`; NOP; PUSH AX; PUSH BX; JMP`[0x9be]` | — |
| what sets the cells | `6274`/`627b` in `publish_mode_vector` | sole writers (sweep above); value = `CS:[BX−4]`/`CS:[BX−2]` at `6270/6277`, BX = override `0x2824` (gates above) or the incoming arg | — |
| what selects `bc` vs `be` | stub choice | no program-wide selector inspects the cells; selection is WHICH STUB THE CALLER ENTERS — `092c` (bare transfer) vs `0931` (which pre-pushes AX/BX, the same preamble the vector targets carry at `T..T+4`, so `JMP [0x9be]` at `+5` = the preamble-skipping entry; see 14-arg table) | caller bodies one hop out — NAMED-AND-DEFERRED |

### 14-arg dispatch determinability (Step 3c)

Domain (map `## callee arg question`/`## 0x29bc slot consumers` row, all
fourteen `MOV word ptr [BP + -0x5a],imm16` stores in `FUN_11bd_3ed8`; two
re-confirmed live in sweep responses above: `44ab`, `440e`). For each arg
X the pair sources are `CS:[X−4]`/`CS:[X−2]` (the `6270/6277` form); 4-byte
reads at X−4 cover both cells, each reconciled data↔hex (14/14 MATCH).
Jump semantics: `JMP word ptr [cell]` = NEAR transfer, IP ← cell word,
executed at CS paragraph `0x11bd` (byte-cited by the `28b6 JMPF …11bd`
flush above) → target = `11bd:word`. `w0` arms `092d`, `w1` arms `0934`.

| arg (store) | pair source cells (4-B hex, reconciled) | source home (get_function_by_address per source) | w0→[0x9bc]→092d target | w0 insn cited (dry-run) | w1→[0x9be]→0934 target | w1 insn cited (dry-run) |
|-------------|------------------------------------------|--------------------------------------------------|------------------------|--------------------------|------------------------|--------------------------|
| `0x381` (`436e`) | `037d/037f` `38093d09` | band `02d4..0732` (`No function found for 11bd:37d`) | `0x0938` | `PUSH AX` `50` (gap `0938..0bd0`, no-function error cited) | `0x093d` | `CLI` `fa` @`093d` (`MOV BX,0x1000` `bb0010` at `093a..093c` = preamble tail) |
| `0x8da` (`43ed`) | `08d6/08d8` `9b09a009` | band `073c..0928` | `0x099b` | `PUSH AX` `50` | `0x09a0` | `CLI` `fa` |
| `0x2824` (`440e`) | `2820/2822` `64284c28` | POCKET (R2 TABLE) | `0x2864` | `PUSH AX` `50` (pocket stream) | `0x284c` | `PUSH BX` `53` (pocket stream — this pair is the override path's live vector content) |
| `0x3a7` (`4423`) | `03a3/03a5` `d709dc09` | band `02d4..0732` | `0x09d7` | `PUSH AX` `50` | `0x09dc` | `CLI` `fa` |
| `0x71a` (`445c`) | `0716/0718` `5e0a630a` | band `02d4..0732` | `0x0a5e` | `PUSH AX` `50` | `0x0a63` | `CLI` `fa` |
| `0x8b2` (`446c`) | `08ae/08b0` `9f0aa40a` | band `073c..0928` | `0x0a9f` | `PUSH AX` `50` | `0x0aa4` | `CLI` `fa` |
| `0x749` (`448e`) | `0745/0747` `6f077407` | band `073c..0928` | `0x076f` | `PUSH AX` `50` | `0x0774` | `CLI` `fa` |
| `0x905` (`44a3`) | `0901/0903` `e20ae70a` | band `073c..0928` | `0x0ae2` | `PUSH AX` `50` | `0x0ae7` | `CLI` `fa` |
| `0x29bc` (`44ab`) | `29b8/29ba` `5a2a602a` | BLOCK `2978..2ada` (no-function error @`29b8` cited) | `0x2a5a` | `PUSH AX` `50` (block, still undefined listing) | `0x2a60` | `CLI` `fa` (preamble here is 6 B: `MOV BX,[0x9b4]` `8b1eb409` @`2a5c` — hence the +6 spacing for this arg only) |
| `0x679` (`44b2`) | `0675/0677` `97069c06` | band `02d4..0732` | `0x0697` | `PUSH AX` `50` | `0x069c` | `CLI` `fa` |
| `0x8ac` (`44b9`) | `08a8/08aa` `e707ec07` | band `073c..0928` | `0x07e7` | `PUSH AX` `50` | `0x07ec` | `CLI` `fa` |
| `0x3d6` (`44c6`) | `03d2/03d4` `0e041304` | band `02d4..0732` | `0x040e` | `PUSH AX` `50` | `0x0413` | `CLI` `fa` |
| `0x462` (`44ce`) | `045e/0460` `91049604` | band `02d4..0732` | `0x0491` | `PUSH AX` `50` | `0x0496` | `CLI` `fa` |
| `0x4f7` (`44e0`) | `04f3/04f5` `af05b405` | band `02d4..0732` | `0x05af` | `PUSH AX` `50` | `0x05b4` | `CLI` `fa` |

Determinability outcome: 14/14 args fully determined at the word level —
every pair cell holds static bytes, both vector words land on dry-run
emitted instructions, and the uniform landing pattern is `50 PUSH AX; 53
PUSH BX; <5-byte preamble: bb0010 MOV BX,0x1000 — or the 6-byte `2a5c`/
`2866` form MOV BX,[0x9b4]>; then w1 = the post-preamble `CLI` entry`.
`word1−word0 = +5` for twelve args, `+6` for `0x29bc` (4-byte BX-load
preamble), and the `0x2824` override pair is the odd shape (w0 at `2864`,
w1 at `284c` — 24 bytes BELOW w0, both inside the pocket: `284c` enters
the `PUSH BX; MOV BX,SS; AND BL,0xf8; PUSH 0x8; POP ES` SS-rebase preamble).
Zero targets land inside a defined function body (the expected-shape
scatter resolved: pocket 2, block 2, functionless gap bands 24 — bands
`02d4..0732` = gap row `1000:1ea4..1000:2302` `1119 B`, `073c..0928` =
`1000:230c..1000:24f8` `493 B`, `0938..0bd0` = `1000:2508..1000:27a0`
`665 B`, all neighbors function-confirmed in the live `find_code_gaps`
read above). Source-side divergence from the brief's expectation
"interiors of defined functions": NONE of the 14 sources sits in a defined
body — the interior class is empty; all twelve non-pocket/block sources sit
in the three gap bands (defined-orphan/undefined bytes — per-row
`get_function_by_address` responses quoted in the appendix below; rows
`0x381`/`0x29bc` carry inline error-cites in the table, `0x2824` does not —
the appendix's keyed set governs).

### Source-home citations — `get_function_by_address` per claimed source address (appendix)

All fourteen pair-source cells probed; responses verbatim, keyed by address
(the eleven under-cited rows of the table above are cited here live; of the
three previously-claimed inline rows, `0x381` carries the error text verbatim
and `0x29bc` points at "no-function error @`29b8` cited", while `0x2824`'s
table row carries NO inline response — the appendix is the authoritative
keyed cite set for all fourteen):

| source addr (arg) | verbatim response |
|-------------------|-------------------|
| `11bd:037d` (`0x381`) | `{"error":"No function found for 11bd:37d"}` |
| `11bd:08d6` (`0x8da`) | `{"error":"No function found for 11bd:8d6"}` |
| `11bd:2820` (`0x2824`) | `{"error":"No function found for 11bd:2820"}` |
| `11bd:03a3` (`0x3a7`) | `{"error":"No function found for 11bd:3a3"}` |
| `11bd:0716` (`0x71a`) | `{"error":"No function found for 11bd:716"}` |
| `11bd:08ae` (`0x8b2`) | `{"error":"No function found for 11bd:8ae"}` |
| `11bd:0745` (`0x749`) | `{"error":"No function found for 11bd:745"}` |
| `11bd:0901` (`0x905`) | `{"error":"No function found for 11bd:901"}` |
| `11bd:29b8` (`0x29bc`) | `{"error":"No function found for 11bd:29b8"}` |
| `11bd:0675` (`0x679`) | `{"error":"No function found for 11bd:675"}` |
| `11bd:08a8` (`0x8ac`) | `{"error":"No function found for 11bd:8a8"}` |
| `11bd:03d2` (`0x3d6`) | `{"error":"No function found for 11bd:3d2"}` |
| `11bd:045e` (`0x462`) | `{"error":"No function found for 11bd:45e"}` |
| `11bd:04f3` (`0x4f7`) | `{"error":"No function found for 11bd:4f3"}` |

Every response is a no-function error — consistent, row-by-row, with the
gap-row band memberships claimed in the table (a source inside a defined
body would return that body). Band-level target-home claims additionally
carry the two live target-side probes quoted in the run: `get_function_by_address(11bd:0938)` →
`{"error":"No function found for 11bd:0938"}` (band `0938..0bd0` entry) and
`get_function_by_address(11bd:2a5a)` → `{"error":"No function found for 11bd:2a5a"}`
(block `2978..2ada` interior); the remaining gap-band/block targets are the
same-functionless-class claim anchored by the three gap rows + these probes.

Scope guard honored: consumers of the jump targets = ONE hop — the preamble
instructions at the resolved targets are cited; the bodies behind them
(CMOS `0x70/0x71` legs, `[0xf7a]/[0xf7c]` save cluster stores at `09a2/09a6`-
style, PIC `OUT 0x20`, PIT `0x43/0x61/0x140` shapes) are named-and-deferred,
not walked. Cite-only runtime/data contacts observed in the pocket stream:
`28aa MOV CX,[0x40]` (slice-15 cell, direction untouched), `28bd MOV DS,
CS:[0x0]`, `2866`/`2a5c MOV BX,[0x9b4]` (slice-20 cell role not reopened),
`7687/768e`+`769a/76a4` patch stores → `0xf7a/0x2d07` cluster (their owners'
roles not asserted). `[0x9b8]/[0x9ba]` roles, twin orphan `02da..02f8`,
`[0x9c0]`/`[0x40]` runtime: CITED-PRIOR, NOT REOPENED.

### Writes (ZERO-WRITE branch) + inventory

NONE. `disassemble_bytes` ran EXCLUSIVELY `dry_run=true` (16 calls: the
348-byte pocket window + 14 target/anchor windows + 1 superseded misprobe
disclosed in the report); no create_function, no rename, no comment, no
set_global, no save_program; no transaction opened; pre-existing bodies
(`FUN_11bd_27e4`, `restore_fs_gs_and_resume`, `FUN_11bd_2adb`,
`dispatch_mode_vector`, `FUN_11bd_0931`, `publish_mode_vector`) re-read
only. Tool counts: `get_function_by_address` ×20 (2 edges, 14 sources, 2
stubs, 2 targets), `find_code_gaps` ×1 (total 131), `read_memory` ×22
(21/22 internal match; the 1 glitch three-way resolved above),
`search_instructions` ×14 (10 sweep patterns + 4 probes; every count and
scope on the sweep table), `disassemble_function` ×2 (owner dumps
count:2/count:4), `get_function_callers` ×2 (3 + 1, quoted),
`get_xrefs_to` ×2 (dead-channel controls, both 0). `/media/felipe/FIFAPCCD/`
untouched; `fifa96.rep` churn left unstaged. Unmoved proof: the two edge
no-function errors above are byte-identical in form to slice-17's quotes
and the pocket gap row is byte-identical to slice-17/20's — nothing moved.
Fix round 1 (review): +14 `get_function_by_address` executions — the full
source-appendix set, every response quoted in the appendix above, keyed by
address — plus the R7 size-label correction (`79` → `127` B, the earlier
value an unconverted `0x7F` misread) and the mandated tiling-sum line; zero
writes; excluded from the Task-1 counts above per the standing
fix-round-recording convention.

### Deferrals

- Pocket ownership: the stream is classified but NOT dispositioned to a
  function — create/rename requires the entry question (who reaches
  `2811..2823`/`2824`; `0x296d`-style dynamic feeders not searched for the
  pocket beyond the vector pair already cited); named-open.
- Landing-layer ownership: the 28 target sites (three gap bands + the two
  preamble forms) are decodable but unowned; their bodies (post-`CLI`
  legs, `[0xf7a]/[0xf7c]` save cluster, patchers `7670`) named-and-deferred.
- Block `2978..2ada`: still fully named-open — the `0x29bc` pair words at
  `29b8..29bb` are now evidence for the fall-in/entry lead (slice-17 leg
  (i)), still no attributed static entry; `CS:[0x2ad9]` tail-cell question
  unchanged.
- Caller bodies behind `092c`/`0931` stub entry (`79fc`, `7a88`,
  `execute_exit_arm`, `0d80`) — one-hop rule, cite-only.
- `[0x9ba]` disposition unchanged (slice-20 NONE-FROM-DISCIPLINE stands —
  this slice adds no reader; the arg-path vector writes never touch
  `[0x9ba]`).

### Writes (Task 2 — capped writes at Task-1-cited boundaries, program `/fifa96.exe`)

Before-state (verbatim, all captured before the first mutation): pocket gap
row `{"start":"1000:43e1","end":"1000:453c","size":348,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_27e4","before_function_address":"11bd:27e4","after_function":"restore_fs_gs_and_resume","after_function_address":"11bd:296d"}` (total 131) — byte-identical to
Task-1's quote: no listing movement between the passes; no-function errors
at every cited create/define address —
`get_function_by_address(11bd:2820)` → `{"error":"No function found for 11bd:2820"}`,
`(11bd:2823)` → `{"error":"No function found for 11bd:2823"}` (both TABLE
edges undefined — apply-cleared check), `(11bd:2824)` / `(11bd:284c)` /
`(11bd:2864)` → same-shape errors (no thunk/defined-byte collisions at the
entries; STOP-BLOCKED condition never fired);
`audit_global(11bd:2820)` pre → `{"address":"11bd:2820","name":"","type":"","length":0,"plate_comment":"","xref_count":0,"issues":["generic_name","untyped","missing_plate_comment"],"severity_summary":{"hard":3,"medium":0,"soft":0},"fully_documented":false}`;
`inspect_memory_content(11bd:2820,8)` → `hex_dump "64 28 4C 28 8C 2E 66 0D"`
(TABLE words + `2824` head byte, reconciled vs Task-1 W1).

TABLE class (R2 `2820..2823`) — controller path `create_array_type` +
apply + label + plate, verified `audit_global`:
(1) `create_array_type(base_type=uint16, length=2)` →
`{"status":"success","message":"Successfully created array type: ushort[2] (uint16[2])","name":"ushort[2]","base_type":"uint16","length":2}`.
(2) `apply_data_type(11bd:2820, ushort[2], dry_run=true)` →
`{"dry_run":true,"status":"success","message":"Successfully applied data type 'ushort[2]' at 11bd:2820 (size: 4 bytes)","size":4}` — conflict check
clean at both edges (no prior-slice code claim over `2820..2823`).
(3) real apply → `{"status":"success","message":"Successfully applied data
type 'ushort[2]' at 11bd:2820 (size: 4 bytes)"}`. (4) `create_label(11bd:2820,
mode_vector_source_pair)` → `{"status":"success","message":"Created label 'mode_vector_source_pair' at address 11bd:2820"}`. (5) `set_comment`
plate → `{"status":"success","message":"Set plate comment at 11bd:2820","warnings":["Plate comment missing Algorithm section","Plate comment
missing Parameters section","Plate comment missing Returns section"]}` —
plate text `C: none — behavioral (mode vector near-offset table — [0x9bc]/[0x9be] source pair words for base 0x2824; cited chain 6270/6274 + 6277/627b)`
(brief's `mode vector near-offset table` wording kept; role from the cited
chain only, no direction claims). Verification: `audit_global` post →
`{"name":"mode_vector_source_pair","type":"ushort[2]","length":4,"plate_comment":"C: none — behavioral (…)","xref_count":0,"issues":["name_missing_g_prefix","plate_line_too_long"],"severity_summary":{"hard":1,"medium":0,"soft":1}}`
— blank pre-state's three hard issues cleared to the naming-convention pair
below; `analyze_global_completeness` → `{"score":77.0,"effective_score":80.0,"band":"COMPLETE_80","missing":["name"],"deductions":[{"axis":"name","code":"name_missing_g_prefix","points":20.0,"severity":"hard"},{"axis":"comment","code":"plate_line_too_long","points":3.0,"severity":"soft","forgiven":true}]}`.
Naming-gate disclosure: the tool chain then requested a `g_` prefix
(`audit_global` hard issue) — the rename attempt
`rename_symbol(11bd:2820 → g_mode_vector_source_pair)` was rejected by the
Hungarian gate verbatim: `{"status":"rejected","issue":"name_quality","issue_msg":"Global 'g_mode_vector_source_pair' has no recognized Hungarian prefix after 'g_' (got 'mode_vector_source_pair')"}`; the two machine gates
(`g_`+Hungarian) conflict with the controller-mandated wording (snake_case,
behavioral role, no direction claims — and repo precedent: zero
named/defined globals exist, `list_globals(filter=named,type_filter=defined)`
→ `{"count":0,"total":0}`, so there is no established prefix to match).
Decision: keep `mode_vector_source_pair` exactly as mandated, gates recorded
above. `xref_count:0` is the dead-xref channel (slices 16/18/19/20 controls)
— the two `6270/6277` window reads address `0x2820/0x2822` dynamically via
`BX` and are invisible to static xrefs by construction.

CODE class — path per controller: real `disassemble_bytes` at each run,
then `create_function` at the cited entries (`2824` fall-through-side,
`284c`/`2864` vector targets), post-check bounds, cap {one nudge → ratify}.
Real disassembly: (a) run R3 `disassemble_bytes(11bd:2824..28e6)` →
`{"success":true,"start_address":"11bd:2824","end_address":"11bd:28e5","bytes_disassembled":194,…,"instructions_total":69,"truncated":false}` —
69 insns, first `2824 MOV word ptr [0xd66], GS 8c2e660d`, `282c MOV FS,
[0xd60]`/`2830 MOV GS,[0xd62]` (the recursion pair, matching Task-1's
dry-run listing byte-for-byte), `283c CALL 0x1000:27b9`, `2848 JMP
0x1000:2492`, `284b RET`, `284c PUSH BX 53`, SS-rebase preamble verbatim
(`8cd3 MOV BX,SS` / `80e3f8 AND BL,0xf8` / `6a08 PUSH 0x8` / `07 POP ES` /
`fa CLI` / `268707 XCHG ES:[BX],AX` ×2 / `8ed3 MOV SS,BX`), `2862 JMP
0x1000:443b`, `2864 PUSH AX 50`, `2866 MOV BX,[0x9b4]`, `288f MOV EAX,CR0`,
`2897 BTR EAX,0x1f`, `28b3 MOV CR0,EAX`, `28b6 JMPF 0x1000:448b`, `28ca
LIDT word ptr [0xddc]`, last `28e4 MOV AX,[0xf56] a1560f` (len 3; the
envelope end/count reported `28e5`/194 — envelope arithmetic lag, quoted as
returned). (b) run R7 `disassemble_bytes(11bd:28ee..296c)` →
`{"success":true,…,"end_address":"11bd:296b","bytes_disassembled":126,…,"instructions_total":61,"truncated":false}` — 61 insns through
`296b POP AX`; the cited tail `296c RET` was NOT emitted by the window
request, defined by a follow-up 1-byte real disassembly `disassemble_bytes(11bd:296c, length 1)` →
`{"success":true,…,"instructions":[{"address":"11bd:296c","mnemonic":"RET","length":1,"bytes":"c3"}],"instructions_total":1}` —
`296c` is a Task-1-cited CODE byte (R7 exit), NOT a skip byte; the skip
bytes `28e7..28eb`/`28ed` and `??? 28ec` were never disassembled and never
used as entries.
Creates: `create_function(11bd:2824)` → `{"success":true,"function_name":"FUN_11bd_2824","body_size":60}`; bounds read-back
`get_function_by_address(11bd:2824)` → body `2824..284b` (RET-terminated;
analyzer split R3 at the `284b RET` — actual resulting body recorded).
`create_function(11bd:284c)` → `{"success":true,"function_name":"FUN_11bd_284c","body_size":236}`; first read-back envelope `284c..295c`.
`create_function(11bd:2864)` → `{"success":true,"function_name":"FUN_11bd_2864","body_size":219}`; post-split read-backs: `FUN_11bd_2864`
body `2864..295c`, `FUN_11bd_284c` re-split to body `284c..2863` (exact
preamble block; the stream blocks moved to the last-created entry — Ghidra
block-ownership, recorded not fought). Zero nudges: no create was retried,
`disassemble_first=false` never used — cap compliance: all three entries
succeeded first-call. Ownership probes verbatim: `(11bd:28e7)`/`(11bd:28ec)`/
`(11bd:28ee)`/`(11bd:295d)`/`(11bd:296c)` → all `{"error":"No function
found for 11bd:…"}` (skip/??? bytes untouched; orphan blocks outside every
body); `(11bd:286b)`/`(11bd:28fe)` → resolve to `FUN_11bd_2864` (jump-fed
blocks).
Flow side effects (analyzer-decided, ratified per the slice-17
`FUN_11bd_02b5` precedent — left as created: no rename, no plate):
`create_function(2824)`'s `2848 JMP` target became `FUN_11bd_08c2`
(`get_function_by_address` → body `08c2..08d5`; dump `{"instructions":[{"08c2 MOV AX,0x9db"},{"08c5 MOV CX,DS"},{"08c7 CALL 0x1000:1f0c"},…,"count":9]}`),
whose `08c7 CALL 0x1000:1f0c` (= `033c`, delta ✓) in turn produced
`FUN_11bd_033c` (`body_start":"11bd:033c","body_end":"11bd:035f`);
`FUN_11bd_0bc3` (`0bc3..0bd0`, 7-insn dump; its only pocket-side call edge
is the `2903 CALL 0x1000:2793` (`e8bde2`, `0x2793−0x1bd0=0x0bc3` ✓) inside
the now-orphan block `2901..2907` — exact creation trigger not reconstructed,
recorded as flow effect). `283c CALL 0x1000:27b9` (= `0beb`) resolved into
the PRE-EXISTING `FUN_11bd_0be9` (`body_start 0be3` — unchanged
`0be3..0bef`); callee read-backs: `FUN_11bd_2824` → `{"callees":[{"name":"FUN_11bd_08c2"},{"name":"FUN_11bd_0be9"}],"total":2}`,
`FUN_11bd_284c`/`FUN_11bd_2864` → `{"callees":[],"total":0}`.
Neighbors re-read: `FUN_11bd_27e4` `27e4..2810` and `restore_fs_gs_and_resume`
`296d..2977` unchanged ✓; `get_function_count` 294 → 300 (+6 = 3 cited
creates + 3 flow-effect auto-creates, exact).
`save_program` → `{"success":true,"program":"fifa96.exe","message":"Program saved successfully"}`.

PADDING/SKIP/UNKNOWN — no-action citations: R1 `2811..281f` PADDING:
untouched (zero bytes, no define, no disassembly); R4 `28e7..28eb` SKIP:
untouched — post probes `{"error":"No function found for 11bd:28e7"}` and
`has_undefined_bytes:true` still covers them; R6 `28ed` (1 B): untouched;
R5 `28ec` (`???`): untouched — the analyzer's own first-pass stray `POP AX`
emit was never adopted, no define at `28ec`. Slice-14's contiguous-stream
cite (`## 296d hook target` row "stream alignment at 296d") is now embodied
as program state: the decode chain `2824→…→296c RET` is defined, the pocket
tail `296c RET` sits exactly one byte before the `296d` function start.

Post-state gap re-page (`find_code_gaps` total 131 → 136, offset 0, limit
100): the pocket row is GONE, replaced by four rows —
`{"start":"1000:43e1","end":"1000:43f3","size":19,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_27e4","after_function":"FUN_11bd_2824"}`
(= `2811..2823`: PADDING 15 B + TABLE 4 B — defined data stays row-listed),
`{"start":"1000:44b7","end":"1000:44cd","size":23,"has_undefined_bytes":true,"has_orphaned_instructions":true,…,"before_function":"FUN_11bd_2864","after_function":"restore_fs_gs_and_resume"}`
(= `28e7..28fd`: skip 7 B + orphan block `28ee..28fd` 16 B),
`{"start":"1000:44d1","end":"1000:44d7","size":7,…,"has_orphaned_instructions":true}` (= `2901..2907`),
`{"start":"1000:452d","end":"1000:453c","size":16,…,"has_orphaned_instructions":true}` (= `295d..296c`, contains `2965 MOV CR0,EAX`
and the `296c RET`). Math: `19+23+7+16 = 65` uncovered vs old 348 → `283 B`
now function-covered inside the pocket = `FUN_11bd_2824` 40 (`2824..284b`)
+ `FUN_11bd_284c` 24 (`284c..2863`) + `FUN_11bd_2864` 219 (envelope
`2864..295c` = 249 B minus its uncovered interior `28e7..28ed` 7 +
`28ee..28fd` 16 + `2901..2907` 7 = 30); the TABLE's 4 B sit inside row 1 as
defined data (row-listed, `has_undefined_bytes:false`), not counted as
function coverage: `40+24+219 = 283` ✓. Band rows also
split at the flow-effect functions: `1000:1ea4..2302` (1119) → `1000:1ea4..1f0b`
(104) + `1000:1f30..2302` (979) around `FUN_11bd_033c`; `1000:230c..24f8`
(493) → `1000:230c..2491` (390) + `1000:24a6..24f8` (83) around
`FUN_11bd_08c2`; `1000:2508..27a0` (665) → `1000:2508..2792` (651) +
`FUN_11bd_0bc3` tail. Page-0 observed row delta = +6; the program-wide
total moved +5, so one row outside the fetched page-0 window (never quoted
in either pass) merged — recorded as observation, not attributed.

### Disposition (Task 2)

Pocket verdict: the left pocket `2811..296c` is the unanchored PM-restore
subsystem's mode-vector landing layer and its entry stream — the `0x2824`
override's `[0x9bc]`/`[0x9be]` pair now lives as typed data
(`g`-gate-pending `mode_vector_source_pair`, `ushort[2]` = `0x2864`/`0x284c`),
and the two vector words land on real function entries (`FUN_11bd_2864`
`PUSH AX`-headed, `FUN_11bd_284c` SS-rebase-preamble-headed) with the
stream head `FUN_11bd_2824` — mechanism ops cited from the created bodies:
`288f MOV EAX,CR0`/`2897 BTR EAX,0x1f`/`28b3 MOV CR0,EAX` + `28b6 JMPF`
flush + `28ca LIDT word ptr [0xddc]` + `28a5 STR word ptr [0xdfc]` +
segment saves/restores (`2880/2884` ↔ `28c2/28c6`, the `282c/2830` twin of
the `296d/2971` restores) + `[0x2e]` mode-byte switch with CMOS `0x70/0x71`,
PIC `0x20`, `0xF2/0xF6` and `DX=0x140`/`0x404` port arms (the Task-1
one-hop PIT-`0x43`-shape mention lives band-side, not in these bodies). Names: all three created FUNs stay default-named — role naming at
mechanism level is defensible ONLY partially (the bodies' ops support a
teardown/restore vocabulary but the stream's entry attribution and the
port-leg consumer trees are the deferred layers), and NOT-CONFIRMED-at-role
gets no rename per the write rule; the missing leg for each name is the
entry question (who reaches `2811..2823`/`2824` — slice-17's
"unanchored…-subsystem" leg, open) plus the `[0x2e]`-arm semantics (one-hop
rule). Determinability verdict: the `[0x9bc]`/`[0x9be]` dispatch IS
statically determinable PER ARG (14/14, Task-1 table): the armed pair is
`arg−4`/`arg−2` under the gate-free path and the pocket pair `0x2864`/`0x284c`
under the `626d` override; per-arg targets land 2 in pocket (now `FUN_11bd_284c`
entry `284c` and `FUN_11bd_2864` entry `2864` — targets named with their
homes, the YES-branch discharge), 2 in block `2978..2ada` (`0x2a5a`/`0x2a60`,
still undefined listing), and 24 in the functionless gap bands (`02d4..0732`,
`073c..0928`, `0938..0bd0` — each band functionless except the three
flow-island FUNs `08c2`/`033c`/`0bc3` above) — ZERO of the 24 band + 2 block
targets fall in pre-existing defined bodies (the 2 pocket targets' landing
insns are entries of bodies THIS slice created); the
selector between the two cells is WHICH STUB the caller enters (`092c` vs
`0931`, Task-1 owner table) and the cell content is fully static per arg.
Slice-prior status: slice-17's OPEN note (`## 296d hook target`, "hole =
whole unanchored PM-restore subsystem `2811..2ada`") is PARTLY CLOSED by
this disposition — the left pocket `2811..296c` is now classified, typed
(TABLE), and function-owned (3 FUNs) with the vector pair wired; the note's
right half (block `2978..2ada` ownership) and the "unanchored" entry leg for
`2824` itself STAND (extended by reference, not rewritten); slice-16's
concern items: `28e7..28eb` skip — CLOSED as classified UNKNOWN-flagged and
LEFT UNTOUCHED (probes quoted above: still no-function, `has_undefined_bytes:true`
row member — disposition: emission-local artifact, hand shapes cited in
Task-1 R4, no define forced); save-pair `2880/2884` — CLOSED byte-exact
(Task-1 re-derivation: `2880` saves GS→`[0xd62]`, `2884` saves FS→`[0xd60]`,
restores at `28c2/28c6` and the `296d/2971` epilogue reload — now inside
`FUN_11bd_2864`'s body).

### Deferrals (Task 2 additions)

- Orphan R7 blocks: `28ee..28fd`, `2901..2907`, `295d..296c` (the `2965
  MOV CR0,EAX` reload block and the `28fb/2903` call edges) — defined
  instructions, NO cited entry, no function created over them; ownership
  open.
- Flow-effect band FUNs (analyzer-created, ratified as-is, boot-table
  territory NOT followed per the scope guard): `FUN_11bd_08c2` (`08c2..08d5`,
  CMOS `0x70/0x71` legs + `08c7 CALL 033c`), `FUN_11bd_033c` (`033c..035f`,
  new owner-island in band `02d4..0732`, role unattributed), `FUN_11bd_0bc3`
  (`0bc3..0bd0`, `IN/OUT 0x92` NMI-shaped arm); their consumer trees are
  target-side one hop, closed by citation.
- `[0x9c0]` installer identity and `[0x40]` MSW runtime legs: cite-only
  prior status (slices 15/17), untouched this write batch.
- `[0x9b8]/[0x9ba]` roles: slice-19/20 dispositions stand, no new reader
  from the created bodies.
- Twin orphan `02da..02f8`: untouched; the twin-band gap row merely split at
  `FUN_11bd_033c` (row math above).
- Block `2978..2ada` and `CS:[0x2ad9]`: unchanged (`1000:4548..46aa` row
  re-quoted identical, `has_undefined_bytes:true`).
- Remaining gap pages: total 131 → 136 (page-0 delta +6, one unquoted
  page->100 row merged, disclosed); the 24 gap-band dispatch targets stay
  functionless except the three flow-effect islands above.
- Post-`CLI` leg bodies behind every vector target (the landing layer):
  named-and-deferred per Task-1's one-hop ruling; the `79fc/7a88/
  execute_exit_arm/0d80` stub-caller bodies likewise.

## vector dispatch handlers (verified 2026-09-29, program `/fifa96.exe`)

Zero-Ghidra-write pass over the landing layer of the `[0x9bc]`/`[0x9be]`
dispatch: the 26 band+block targets of the `## 2811..296c pocket + 9bc
vector` determinability table (the two pocket targets `0x2864`/`0x284c` are
out of scope — they already own created FUNs) are deduped, every unique
offset owner-probed live, and each handler dry-run-walked from its cited
landing to a CITED exit (`RET`/`HLT`/tail-`JMP`/boundary stop-short at a
defined byte). Headline results: (1) the 26 edges land on 26 pairwise-distinct
offsets, collapsing to **13 handler bodies** — each arg's `w0` lands on the
body's head (prelude entry via the bare `092d` stub) and its `w1` lands on
the post-prelude `CLI` byte (preamble-skipping entry via `FUN_11bd_0931`'s
`0934`, which pre-pushes AX/BX at `0932/0933`); (2) twelve of thirteen
preludes are byte-identical `50 53 bb0010 fa` (`PUSH AX;PUSH BX;MOV
BX,0x1000;CLI` = the `0x0938` cited form) and the block handler `0x2a5a`
is the `50 53 8b1e b409 fa` form (`MOV BX,[0x9b4]` base, +6 spacing = the
`0x2a5a` cited form); (3) every handler ends at a cited terminator — a
`RET` or an `HLT` followed by a `JMP`-back halt-retry (`ebfd` idiom) or a
self-spin (`ebfe` idiom), and ten of thirteen also carry a **far-return
block** whose entry address the handler itself stores as an IP:CS pair
into the `[0x467]/[0x469]` (or `[0x3fc]/[0x3fe]`, `[0x160]/[0x162]`,
`[0x4a2]/[0x4a4]`) cluster with the CS half taken from cell `[0x9b6]`;
(4) the walk of the block handler `0x2a5a` REACHES `0x2a6c` — the
slice-16-flagged MSW-clear shape is inline static code in the handler body
(SMSW `0f01e1`@`2a69`, `MOV AX,[0x40]`@`2a6c`, NOT `f7d0`@`2a6f`,
AND `23c1`@`2a71`, LMSW `0f01f0`@`2a73`), and the same walk yields the
FIRST attributed entry into block `2978..2ada`: body `[2a5a..2ad8]`,
tail-cited `RET (c3)` @`2ad8`, the `CS:[0x2ad9]` tail cell excluded;
(5) one listing collision: `04be..04bf` is a defined data unit
`DAT_11bd_04be` (`undefined2`) carved into the middle of handler `0x0491`'s
stream — stop-short cited on both sides below. No walk touches the flow
islands `08c2`/`033c`/`0bc3` (the three `0x1000:26a5`-rendered branch
targets recompute to `11bd:0ad5`, NOT `08d5` — delta arithmetic cited per
edge). `disassemble_bytes` ran exclusively `dry_run=true`; names, creates,
plates: NONE (Task 2 territory). Reader context per slice-21: `092d` @
`dispatch_mode_vector` (`092c..0930`) arms the `w0` path, `0934` @
`FUN_11bd_0931` (`0931..0937`) arms the `w1` path; far/near target render
delta `0x1bd0` applied to every branch cite.

### Dedupe of the 26 (Step 1)

Edges: 13 band+block args × 2 vector words = 26. Unique offsets: **26 —
every target appears exactly once (multiplicity 1; zero address collisions)**.
Structural collapse: 13 handler bodies — for twelve args `word1−word0=+5`
(3-byte `bb0010` preamble tail), for `0x29bc` `+6` (4-byte `8b1eb409`); the
`w1` offset is an instruction boundary of the `w0`-aligned decode in all 13
walks (the emitted `CLI (fa)` row). Owner state live at every unique offset
(`get_function_by_address`, 26/26 no-function errors — the same-form
response as slice-21's two probes):

| unique offset | arrived from (arg: source-cell words, cited) | owner state at offset |
|---------------|---------------------------------------------|------------------------|
| `11bd:040e` | `0x3d6` (`44c6`): `03d2/03d4` = `0e04/1304` | `{"error":"No function found for 11bd:040e"}` |
| `11bd:0413` | `0x3d6` (same pair, `w1`) | `{"error":"No function found for 11bd:0413"}` |
| `11bd:0491` | `0x462` (`44ce`): `045e/0460` = `9104/9604` | `{"error":"No function found for 11bd:0491"}` |
| `11bd:0496` | `0x462` (same pair, `w1`) | `{"error":"No function found for 11bd:0496"}` |
| `11bd:05af` | `0x4f7` (`44e0`): `04f3/04f5` = `af05/b405` | `{"error":"No function found for 11bd:05af"}` |
| `11bd:05b4` | `0x4f7` (same pair, `w1`) | `{"error":"No function found for 11bd:05b4"}` |
| `11bd:0697` | `0x679` (`44b2`): `0675/0677` = `9706/9c06` | `{"error":"No function found for 11bd:0697"}` |
| `11bd:069c` | `0x679` (same pair, `w1`) | `{"error":"No function found for 11bd:069c"}` |
| `11bd:076f` | `0x749` (`448e`): `0745/0747` = `6f07/7407` | `{"error":"No function found for 11bd:076f"}` |
| `11bd:0774` | `0x749` (same pair, `w1`) | `{"error":"No function found for 11bd:0774"}` |
| `11bd:07e7` | `0x8ac` (`44b9`): `08a8/08aa` = `e707/ec07` | `{"error":"No function found for 11bd:07e7"}` |
| `11bd:07ec` | `0x8ac` (same pair, `w1`) | `{"error":"No function found for 11bd:07ec"}` |
| `11bd:0938` | `0x381` (`436e`): `037d/037f` = `3809/3d09` | `{"error":"No function found for 11bd:0938"}` |
| `11bd:093d` | `0x381` (same pair, `w1`) | `{"error":"No function found for 11bd:093d"}` |
| `11bd:099b` | `0x8da` (`43ed`): `08d6/08d8` = `9b09/a009` | `{"error":"No function found for 11bd:099b"}` |
| `11bd:09a0` | `0x8da` (same pair, `w1`) | `{"error":"No function found for 11bd:09a0"}` |
| `11bd:09d7` | `0x3a7` (`4423`): `03a3/03a5` = `d709/dc09` | `{"error":"No function found for 11bd:09d7"}` |
| `11bd:09dc` | `0x3a7` (same pair, `w1`) | `{"error":"No function found for 11bd:09dc"}` |
| `11bd:0a5e` | `0x71a` (`445c`): `0716/0718` = `5e0a/630a` | `{"error":"No function found for 11bd:0a5e"}` |
| `11bd:0a63` | `0x71a` (same pair, `w1`) | `{"error":"No function found for 11bd:0a63"}` |
| `11bd:0a9f` | `0x8b2` (`446c`): `08ae/08b0` = `9f0a/a40a` | `{"error":"No function found for 11bd:0a9f"}` |
| `11bd:0aa4` | `0x8b2` (same pair, `w1`) | `{"error":"No function found for 11bd:0aa4"}` |
| `11bd:0ae2` | `0x905` (`44a3`): `0901/0903` = `e20a/e70a` | `{"error":"No function found for 11bd:0ae2"}` |
| `11bd:0ae7` | `0x905` (same pair, `w1`) | `{"error":"No function found for 11bd:0ae7"}` |
| `11bd:2a5a` | `0x29bc` (`44ab`): `29b8/29ba` = `5a2a/602a` (BLOCK) | `{"error":"No function found for 11bd:2a5a"}` |
| `11bd:2a60` | `0x29bc` (same pair, `w1`) | `{"error":"No function found for 11bd:2a60"}` |

Live gap-row context for the boundary claims (`find_code_gaps` this pass,
total 136 — slice-21's post-state count): band `0360..0732` row
`{"start":"1000:1f30","end":"1000:2302",…,"before_function":"FUN_11bd_033c","after_function":"FUN_11bd_0733"}`;
band `073c..08c1` row `{"start":"1000:230c","end":"1000:2491",…,"after_function":"FUN_11bd_08c2"}`;
band `0938..0bc2` row `{"start":"1000:2508","end":"1000:2792",…,"before_function":"FUN_11bd_0931","after_function":"FUN_11bd_0bc3"}`;
block row `{"start":"1000:4548","end":"1000:46aa",…,"before_function":"restore_fs_gs_and_resume","after_function":"FUN_11bd_2adb"}`
(`1000:1f30/2302/230c/2491/2508/2792/4548/46aa` − `0x1bd0` =
`0360/0732/073c/08c1/0938/0bc2/2978/2ada` ✓). All 13 proposed ranges above
sit inside these rows — no proposed interior crosses a function body.
Inter-handler unclaimed regions (source cells live in them; not walked):
`045e..0490` (holds arg `0x462` pair `045e/0460` — emitted in the `040e`
window as misaligned fragments `91 04 96 04` ↔ LE words ✓ reconciled),
`04f3..05ae` (arg `0x4f7` pair `04f3/04f5` = bytes `af 05 b4 05` emitted
`SCASW`@`04f3`+`ADD AX,0x5b4`@`04f4` in the `04d3` window ✓),
`0675..0696` (arg `0x679` pair `0675/0677` = bytes `97 06 9c 06` emitted
`XCHG AX,DI`/`PUSH ES`/`PUSHF`/`PUSH ES` in the `0665` window ✓),
`06fc..0732` (H4's callee region), `073c..076e` (H5's `230c`-callee +
arg `0x749` pair `0745/0747`), `0852..08c1` (H6 beyond-exit region + args
`0x8ac`/`0x8b2` pairs `08a8..08ab`/`08ae..08b1`), `09cf..09d3` (H8
JZ-leg block — taken leg of `JZ`@`09c2`) and `09d4..09d6` (gate stub —
reached statically only from H9's gate),
`0b94..0bc2` (unowned non-vector block ending in `CALL print_error_message`
+ fallthrough to island `0bc3` — left unclaimed).

### Per-handler walks (Step 2)

All windows `disassemble_bytes` `dry_run=true` (23 calls incl. one refused
probe + three disclosed off-alignment probes). Calls are address-only (no
dives; scope guard honored — callee bodies, `[0x9b4]`/`[0x9b6]` consumer
roles, port-leg semantics named-and-deferred). `H` numbers ordered by
landing offset. `far/near render − 0x1bd0` applied to every branch cite.

**H1 `0x040e`/`0x0413` (arg `0x3d6`)** — window `dry_run` `11bd:040e`
(100 B emitted `040e..0471`; walk trimmed at cited exit `045d`).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `040e`–`0413` | `50 PUSH AX`; `53 PUSH BX`; `bb0010 MOV BX,0x1000`; `fa CLI` @`0413` (= `w1`) | — |
| gate | `0414`–`041d` | `803e2f0003 CMP byte [0x2f],0x3`; `7203 JC 0x1000:1fee` (→`041e`); `e92e24 JMP 0x1000:441c` (→ `11bd:284c` = pocket FUN entry — flow edge to DEFINED byte, boundary keeps own last byte `041d`) | → `284c` |
| body | `041e`–`042b` | `60 PUSHA`; `891e7c0f MOV [0xf7c],BX`; `89267a0f MOV [0xf7a],SP`; `b83800 MOV AX,0x38`; `8ec0 MOV ES,AX` | — |
| far-ret store | `042c`–`0439` | `26c70660014304 MOV word ES:[0x160],0x443`; `a1b609 MOV AX,[0x9b6]`; `26a36201 MOV ES:[0x162],AX` — return pair `0x443:[0x9b6]` | — |
| ports+halts | `043a`–`0442` | `e4f0 IN AL,0xf0`; `0c01 OR AL,1`; `eb00 JMP+0` (→`0440`); `e6f0 OUT 0xf0,AL`; `f4 HLT` @`0442` — **primary cited exit** | — |
| return block | `0443`–`045d` | entry cite = stored IP `0x443`; `e4f2 IN AL,0xf2`; `0c01`; `eb00` (→`0449`); `e6f2 OUT 0xf2,AL`; `bb0010 MOV BX,0x1000`; `8edb MOV DS,BX`; `8e167c0f MOV SS,[0xf7c]`; `8b267a0f MOV SP,[0xf7a]`; `8ec3 MOV ES,BX`; `61 POPA`; `5b POP BX`; `58 POP AX`; `c3 RET` @`045d` — **static exit cite** | — |
| contacts | — | cells `[0x2f]`, `[0x9b6]`, `[0xf7c]`, `[0xf7a]`, `ES:[0x160]`, `ES:[0x162]`; segment stores `ES←0x38`, `DS/ES←0x1000`; ports `0xf0/0xf2` | — |
| boundary | `[040e..045d]` (80 B) | why-function-start: dynamic vector landing `w0` from dedupe row (entry cite); `w1` `0413` second entry; range inside row `1000:1f30..2302`, no defined bytes interior; ends one byte before arg-`0x462` source cell `045e` | — |

**H2 `0x0491`/`0x0496` (arg `0x462`)** — windows `dry_run` `11bd:0491`
(45 B — emission ENDED at `04bd` because the linear decoder does not carry across the `04be` defined unit (skip-and-report, same disclosed behavior as the `04c0`-probe row below); envelope `0491..04bd` reflects the window length),
`11bd:04c0` (32 B, off-alignment disclosed below), `11bd:04d3` (48 B).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0491`–`0496` | `50 53 bb0010 fa` (`0496 CLI` = `w1`) | — |
| gate | `0497`–`04a0` | `803e2f0003 CMP byte [0x2f],0x3`; `7203 JC` →`04a1`; `e9ab23 JMP 0x1000:441c` → `284c` (edge to defined; own bytes `049e..04a0` owned) | → `284c` |
| body | `04a1`–`04bd` | `60 PUSHA`; `[0xf7c]/[0xf7a]` saves (`891e7c0f`,`89267a0f`); `fa CLI` @`04aa` (second CLI); `b83800`/`8ec0 ES←0x38`; `26c706fc03d304 MOV ES:[0x3fc],0x4d3`; `a1b609 MOV AX,[0x9b6]`; `26a3fe03 MOV ES:[0x3fe],AX` — last owned byte `04bd` | — |
| **collision** | `04be`–`04bf` | first FOREIGN bytes: `DAT_11bd_04be` defined `undefined2` (`analyze_data_region`; `current_name/current_type` cited); raw `read_memory(11bd:04be,16)` `data[186,132,4,237,235,0,37,254,254,239,235,0,186,4,4,176]` ↔ `hex ba8404edeb0025fefeefeb00ba0404b0` ✓ MATCH (also ↔ `inspect_memory_content` 32 B `BA 84 04 ED…` ✓) — unit carves `ba8404 = MOV DX,0x484` (2 of 3 bytes); post-unit stream re-aligns at `04c1`/`04c2` | — |
| continued primary | `04c0`–`04d2` | `04d3`-probe and `04c0`-probe emissions agree from `04c2`: `eb00` (→`04c4`), `25fefe AND AX,0xfefe`, `ef OUT DX,AX`, `eb00`, `ba0404 MOV DX,0x404`, `b004 MOV AL,4`, `eb00`, `ee OUT DX,AL`, `f4 HLT` @`04d2` — **primary cited exit** (true-alignment head of this run = the carved `MOV DX,0x484` @`04be..04c0`, reconstructed from the reconciled raw bytes — flagged not adopted) | — |
| return block | `04d3`–`04f2` | entry cite = stored IP `0x4d3` (`ES:[0x3fc]` @`04b0`); `ba0404 MOV DX,0x404`; `ec IN AL,DX`; `24f9 AND AL,0xf9`; `0c01 OR AL,1`; `eb00`; `ee OUT DX,AL`; `eb00`; `bb0010/b8?? MOV BX,0x1000; 8edb DS; 8e167c0f SS←[0xf7c]; 8b267a0f SP←[0xf7a]; 8ec3 ES←BX; 61 POPA; 5b; 58; c3 RET` @`04f2` — **static exit cite** | — |
| contacts | — | `[0x2f]`, `[0x9b6]`, `[0xf7c]`, `[0xf7a]`, `ES:[0x3fc]/[0x3fe]`; ports `0x484/0x404`; second CLI `04aa` | — |
| boundary | `[0491..04d2]` primary + `[04d3..04f2]` return | collision-conditioned: a create over the contiguous `[0491..04f2]` would have to resolve `DAT_11bd_04be` first — Task-2 decision; Task-1 proposal records stop-short `04bd|04be` cite on both sides | — |

**H3 `0x05af`/`0x05b4` (arg `0x4f7`)** — windows `dry_run` `11bd:05af`
(100 B `05af..0612`… emitted `05af..0603` coverage), `11bd:0604` (100 B),
`11bd:0665` (48 B).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `05af`–`05b4` | `50 53 bb0010 fa` (`05b4 CLI` = `w1`) | — |
| body head | `05b5`–`05c6` | `60 PUSHA`; `52 PUSH DX`; `[0xf7c]/[0xf7a]` saves; `a1b609 MOV AX,[0x9b6]`; `50`; `b86506 MOV AX,0x665`; `50` — stack far frame with return IP `0x665` | — |
| CMOS nibble legs | `05c7`–`065f` | `8bcb MOV CX,BX`; `ba6803 MOV DX,0x368`; alternating `b018/b26a 240f ee` / `b268 b017 ee` / `SHR AL,0x4` nibble OUTs to `DX=0x36a/0x368` with commands `0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10` + `05fb/0638/0641` `eb00` stubs; `8bcc MOV CX,SP` @`0610`; `b008`+`b26a ee` + `b05f e6a1` @`065b..0662` | — |
| exit | `0663`–`0664` | `b05f MOV AL,0x5f`; `e6a1 OUT 0xa1,AL`; `f4 HLT` @`0664` — **cited exit** | — |
| return block | `0665`–`0674` | entry cite = pushed IP `0x665` @`05c6`; `b0f0 MOV AL,0xf0`; `e6a0 OUT 0xa0,AL`; `bb0010/b8?? MOV BX,0x1000`; `8edb DS`; `8ec3 ES`; `5a POP DX`; `61 POPA`; `5b`; `58`; `c3 RET` @`0674` — **static exit cite**; `0675` = arg-`0x679` source byte (adjacency ✓) | — |
| contacts | — | `[0x9b6]`, `[0xf7c]`, `[0xf7a]`; ports `0x368/0x36a` (OUTs), `0xa1`, `0xa0`; NO ES/segment-0x38 stores | — |
| boundary | `[05af..0664]` primary + `[0665..0674]` return (contiguous `[05af..0674]`, 198 B) | inside row `1000:1f30..2302`; no defined-byte interior | — |

**H4 `0x0697`/`0x069c` (arg `0x679`)** — windows `dry_run` `11bd:0697`
(100 B), re-run (102 B, exact exit).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0697`–`069c` | `50 53 bb0010 fa` (`069c CLI` = `w1`) | — |
| body | `069d`–`06bf` | `60 PUSHA`; `[0xf7c]/[0xf7a]` saves; `b83800/8ec0 ES←0x38`; `a1b609 MOV AX,[0x9b6]`; `26a36904 MOV ES:[0x469],AX`; `26c7066704ca06 MOV ES:[0x467],0x6ca` — return pair `0x6ca:[0x9b6]`; `26c60612040a MOV byte ES:[0x412],0xa`; `b4c0 MOV AH,0xc0` | — |
| retry loop | `06c1`–`06c9` | `e83800 CALL 0x1000:22cc` (→`11bd:06fc`); `33c9 XOR CX,CX`; `e2fe LOOP 0x1000:2296` (→`06c6` self); `ebf5 JMP 0x1000:228f` @`06c8..06c9` (→`06bf` = back to `MOV AH,0xc0`) — **cited exit: tail-JMP retry loop** | → `06fc` |
| reload | `06ca`–`06e1` | entry cite = stored IP `0x6ca`; `2e8b1e0000 MOV BX,CS:[0x0]`; `8edb MOV DS,BX`; `8e167c0f/8b267a0f SS/SP←[0xf7c]/[0xf7a]`; `33c0/8ec0 ES←0`; `26a21204 MOV ES:[0x412],AL`; `8ec3 ES←BX` | — |
| second call + PIC | `06e3`–`06f6` | `e81600 CALL 0x1000:22cc` (→`06fc`); `e469 IN AL,0x69`; `eb00` (→`06ea`); `0c04 OR AL,4`; `e669 OUT 0x69,AL`; `eb00` (→`06f0`); `e4a0 IN AL,0xa0`; `eb00`; `0c80`; `e6a0 OUT 0xa0,AL` | → `06fc` |
| exit | `06f8`–`06fb` | `61 POPA`; `5b POP BX`; `58 POP AX`; `c3 RET` @`06fb` — **static exit cite** | — |
| contacts | — | `[0x9b6]`, `[0xf7c]`, `[0xf7a]`, `CS:[0x0]`, `ES:[0x467]/[0x469]`, `ES:[0x412]`; ports `0x69`, `0xa0`; `06fc` = first byte after own RET (callee region, unclaimed, address-only) | — |
| boundary | `[0697..06fb]` (165 B) | inside row `1000:1f30..2302` ✓ | — |

**H5 `0x076f`/`0x0774` (arg `0x749`)** — windows `dry_run` `11bd:076f`
(100 B), `11bd:07d2` (48 B — **off-alignment disclosed**: starts 2 bytes
early, its `07d2 POP DI`/`07d3 JMP [SI]` read the `e8 5f ff` rel-bytes of
the aligned `07d1 CALL`; aligned ops below from `07d4` = `24 3f`, corroborated by both windows' bytes).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `076f`–`0774` | `50 53 bb0010 fa` (`0774 CLI` = `w1`) | — |
| body head | `0775`–`0795` | `e466 IN AL,0x66` (CMOS before PUSHA); `60 PUSHA`; `a8a0 TEST AL,0xa0`; `7504 JNZ` →`0780`; `0ca0/ e666 OUT 0x66,AL`; `[0xf7c]/[0xf7a]` saves; `6a38 PUSH 0x38/07 POP ES`; `26c7066704b907 MOV ES:[0x467],0x7b9`; `a1b609`; `26a36904 MOV ES:[0x469],AX` — return pair `0x7b9:[0x9b6]` | — |
| RTC legs | `0799`–`07b4` | `b84401 MOV AX,0x144`; `e89dff CALL 0x1000:230c` (→`073c`); three `eb00` stubs (`2371/2373/2375` → `07a1/07a3/07a5`); `b84400 MOV AX,0x44`; CALL `073c`; `b045 MOV AL,0x45`; `e883ff CALL 0x1000:2303` (→`11bd:0733` = FUN_11bd_0733 — call edge into DEFINED function, one hop); `0c80`; `86c4 XCHG AH,AL`; CALL `073c` | → `073c` ×4, `0733` |
| exit | `07b7` | `ebfe JMP 0x1000:2387` (→ `07b7` itself) — **cited exit: self-spin tail-JMP** | — |
| return block | `07b9`–`07e6` | entry cite = stored IP `0x7b9`; `b80010/8ed8 DS←0x1000`; `8ec0 ES`; `8e167c0f/8b267a0f SS/SP`; `61 POPA`; `a8a0 TEST AL,0xa0`; `7502 JNZ` →`07cf`; `e666 OUT 0x66,AL`; `b045`; `e85fff CALL 0x1000:2303` (→`0733`); [aligned 07d4+] `243f AND AL,0x3f`; `803e350000 CMP byte [0x35],0`; `7502 JNZ` →`07df`; `0c40 OR AL,0x40`; `86c4 XCHG AH,AL`; `e858ff CALL 0x1000:230c` (→`073c`); `5b`; `58`; `c3 RET` @`07e6` — **static exit cite** | → `0733`, `073c` |
| contacts | — | `[0x9b6]`, `[0xf7c]`, `[0xf7a]`, `ES:[0x467]/[0x469]`, `[0x35]`; port `0x66`; ES←0x38 | — |
| boundary | `[076f..07e6]` (120 B) | ends exactly one byte before next landing `07e7` (tiling, pocket-style); inside row `1000:230c..2491` ✓ | — |

**H6 `0x07e7`/`0x07ec` (arg `0x8ac`)** — windows `dry_run` `11bd:07e7`
(100 B), `11bd:084d` (48 B), `11bd:087c` (40 B — **off-alignment
disclosed**: starts 1 byte early; `087c OR BH,DH` reads the `74 08` JZ
rel-byte; aligned coverage ends at the cited exit `0851` anyway).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `07e7`–`07ec` | `50 53 bb0010 fa` (`07ec CLI` = `w1`) | — |
| timer legs | `07ed`–`0814` | `e421 IN AL,0x21`; `60 PUSHA`; `33c0/ e643 OUT 0x43,AL(0)`; `8b16b609 MOV DX,[0x9b6]`; `33c9 XOR CX,CX`; `b00b/ e620 OUT 0x20,AL(0xb)`; `6a38 PUSH 0x38`; `e440 IN AL,0x40`; `2ac8 SUB CL,AL`; `1f POP DS`; `e440`; `1ae8 SBB CH,AL`; `9c PUSHF`; `e420 IN AL,0x20`; `a801 TEST AL,1`; `7404 JZ` →`0810`; `b020/ e620 OUT 0x20,AL(0x20 EOI)`; `9d POPF` | — |
| vector-save + reprogram | `0815`–`084c` | `ff362200 PUSH [0x22]`; `ff362000 PUSH [0x20]`; `b0fe/ e621 OUT 0x21,AL`; `b010/ e643 OUT 0x43,AL(0x10)`; `89162200 MOV [0x22],DX`; `c70620005208 MOV [0x20],0x852`; `e640 OUT 0x40,AL`; `891e6904 MOV [0x469],BX`; `89266704 MOV [0x467],SP` (PLAIN stores — register save, NOT a far-ret pair); `6a20 PUSH 0x20/1f POP DS`; `b034/ e643 OUT 0x43,AL(0x34)`; `33c0/ e640`; `290ec609 SUB [0x9c6],CX`; `831ec80900 SBB [0x9c8],0` | — |
| exit | `084d`–`0851` | `e640 OUT 0x40,AL`; `e98302 JMP 0x1000:26a5` — target = `0x26a5−0x1bd0` = **`11bd:0ad5`** (NOT `08d5` — no island touch) = the shared shutdown tail below; **cited exit: tail-JMP**, own last byte `0851` | → `0ad5` |
| beyond-exit region | `0852`–`087b` | emitted, NOT walked (after unconditional tail-JMP): `fa CLI`; `83c406 ADD SP,6`; `8ed8 DS←0`; `8f062000 POP [0x20]`; `8f062200 POP [0x22]`; `e8fbfa CALL 0x1000:1f30` (→`0360`); `8606cc09 XCHG [0x9cc],AL`; `3c08/7424/0ac0/741c/3906c809/7d14/3c21/7408` gate chain to `0ad5`/`088d`/`0885`; `ff06c809 INC [0x9c8]`; `cd08 INT 0x8`; `eb04` →`0891`; `b020/ e620`; `61 POPA`… — installed-vector body candidate (the `[0x20]/[0x22]` cells H6 saved are its far vector) — recorded, unclaimed | → `0360` |
| contacts | — | `[0x9b6]`, `[0x20]`, `[0x22]`, `[0x467]/[0x469]`, `[0x9c6]/[0x9c8]`, `[0x9cc]`; ports `0x20/0x21/0x40/0x43`; DS←0x38/0x20 staging | — |
| boundary | `[07e7..0851]` (107 B) | inside row `1000:230c..2491` ✓; tail-JMP target owned elsewhere (shared tail) | — |

**H7 `0x0938`/`0x093d` (arg `0x381`)** — window `dry_run` `11bd:0938`
(100 B emitted through next landing `099b` — adjacency cite).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0938`–`093d` | `50 53 bb0010 fa` (`093d CLI` = `w1`) — THE cited `0x0938` form | — |
| gate | `093e`–`0947` | `803e2f0003 CMP byte [0x2f],0x3`; `7203 JC` →`0948`; `e9041f JMP 0x1000:441c` →`284c` (edge to defined) | → `284c` |
| body | `0948`–`0977` | `b00e/ e637 OUT 0x37,AL`; `b80010/50/50` (0x1000 frame); `60 PUSHA`; `ff36b609 PUSH [0x9b6]`; `687b09 PUSH 0x97b` — far frame `0x97b` (inside own inline-stream region); `b00a/ e637`; `b83800/8ed8 DS←0x38`; `891e0604 MOV [0x406],BX`; `89260404 MOV [0x404],SP` (DS-based save pair at seg 0x38); `be7809 MOV SI,0x978`; `b90300 MOV CX,3`; `fc CLD`; `baf000 MOV DX,0xf0`; `2e6e OUTSB DX,CS:SI` | — |
| inline stream | `0978`–`0980` | `0000`,`00b00fe6`,`37`,`eb00` emitted as code fragments — the OUTSB operand bytes SI walks (recorded; decoded-by-linear-tool artifact, not claimed as flow) | — |
| A20/ports | `0981`–`0994` | `e652 OUT 0x52,AL`; `813e35000080 CMP word [0x35],0x8000`; `7406 JZ` →`0991`; `b000/ e6f2 OUT 0xf2,AL`; `eb04` →`0995`; `b003/ e6f6 OUT 0xf6,AL` | — |
| exit | `0995`–`099a` | `61 POPA`; `07 POP ES`; `1f POP DS`; `5b`; `58`; `c3 RET` @`099a` — **cited exit**; `099b` = H8 landing (adjacency cite) | — |
| contacts | — | `[0x2f]`, `[0x9b6]`, `[0x404]/[0x406]`, `[0x35]`; DS←0x38; ports `0x37/0x52/0xf2/0xf6` | — |
| boundary | `[0938..099a]` (99 B) | inside row `1000:2508..2792`; starts 1 after stub-cluster end `0937` (`FUN_11bd_0931` body edge adjacency ✓) | — |

**H8 `0x099b`/`0x09a0` (arg `0x8da`)** — windows `dry_run` `11bd:099b`
(100 B `099b..09fe`), `11bd:0b60` (99 B, return block + tail region).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `099b`–`09a0` | `50 53 bb0010 fa` (`09a0 CLI` = `w1`) | — |
| body | `09a1`–`09c1` | `60 PUSHA`; `891e7c0f/89267a0f` saves (the `09a2/09a6` cluster cite of slice-21 ✓); `b83800/8ec0 ES←0x38`; `a1b609 MOV AX,[0x9b6]`; `26a36904 MOV ES:[0x469],AX`; `26c70667046e0b MOV ES:[0x467],0xb6e` — return pair `0xb6e:[0x9b6]`; `f6060c1202 TEST byte [0x120c],2`; `740b JZ 0x1000:259f` →`09cf` (rel8 `+0x0b`: `09c2+2+0x0b = 09cf`; `0x259f−0x1bd0 = 09cf` ✓ — fix-wave correction: earlier render said `09cd`, an arithmetic error; live `read_memory(11bd:09c2,6)` = `740be4920c03` ↔ data `[116,11,…]` re-confirmed) | — |
| A20 leg + exit | `09c4`–`09cc` | `e492 IN AL,0x92`; `0c03 OR AL,3`; `eb00` (→`09ca`); `e692 OUT 0x92,AL`; `f4 HLT` @`09cc` — **cited exit** | — |
| halt-retry | `09cd`–`09ce` | `ebfd JMP 0x1000:259c` (→ `0x259c−0x1bd0` = `09cc` — jump BACK to the HLT: halt-retry idiom) — NO static predecessor: the `JZ`@`09c2` targets `09cf` (fixed arrow above) and the `HLT`@`09cc` terminates fall-in, which is why the block orphaned at create (Task-2 row `1000:259d..259e`) | — |
| JZ-leg block | `09cf`–`09d3` | `b0fe MOV AL,0xfe`; `e664 OUT 0x64,AL`; `f4 HLT` @`09d3` — REACHED from H8's own flow as the `JZ`@`09c2` taken leg (create absorbed `09cf..09d3` into `FUN_11bd_099b` — Task-2 H8 row); `09d4`–`09d6` (`e9751e JMP 0x1000:441c` →`284c`) is NOT reached by H8's flow — its only static feeder is H9's gate `09e2 JNC`→`09d4` — recorded, edge cited | → `284c` |
| return block | `0b6e`–`0b93` | entry cite = stored IP `0xb6e`; `b80010/8ed8 DS←0x1000`; `8e167c0f/8b267a0f SS/SP←[0xf7c]/[0xf7a]`; `b00d/ e670 OUT 0x70,AL`; `61 POPA`; `e471 IN AL,0x71`; `803e350000 CMP byte [0x35],0`; `e492 IN AL,0x92`; `7502 JNZ` →`0b8d`; `24fd AND AL,0xfd`; `24fe AND AL,0xfe`; `e692 OUT 0x92,AL`; `5b/58/c3 RET` @`0b93` — **static exit cite** | — |
| contacts | — | `[0x120c]`, `[0x9b6]`, `[0xf7c]/[0xf7a]`, `ES:[0x467]/[0x469]`, `[0x35]`; ports `0x92/0x70/0x71/0x64` | — |
| boundary | `[099b..09cc]` primary + `[0b6e..0b93]` return; JZ-leg block `[09cf..09d3]` + gate stub `[09d4..09d6]` recorded | both ranges inside row `1000:2508..2792`; return block is non-contiguous (separated by H9..H12 lands) — per-block proposal | — |

**H9 `0x09d7`/`0x09dc` (arg `0x3a7`)** — windows `dry_run` `11bd:09d7`
(100 B), `11bd:0a35` (41 B, exact).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `09d7`–`09dc` | `50 53 bb0010 fa` (`09dc CLI` = `w1`) | — |
| gate | `09dd`–`09e3` | `803e2f0003 CMP byte [0x2f],0x3`; `73f0 JNC 0x1000:25a4` → `0x25a4−0x1bd0` = `09d4` = the `JMP 284c` gate stub (`09d4..09d6`, H8-side, NOT the JZ-leg block — cross-handler edge, cite-only) | → `09d4`→`284c` |
| FPU leg | `09e4`–`09f0` | `53 PUSH BX`; `8b1e820f MOV BX,[0xf82]`; `0bdb OR BX,BX`; `7403 JZ 0x1000:25c0` →`09f0`; `dd37 FNSAVE [BX]`; `9b WAIT`; `5b POP BX` | — |
| body | `09f1`–`0a26` | `60 PUSHA`; `b80010/50/50`; `b83800/8ec0 ES←0x38`; `26c706fc03350a MOV ES:[0x3fc],0xa35`; `a1b609`; `26a3fe03 MOV ES:[0x3fe],AX` — return pair `0xa35:[0x9b6]`; `e402 IN AL,0x2`; `8ac8 CL←AL`; `e412 IN AL,0x12`; `8ae8 CH←AL`; `b0fb/e612`; `b0ff/e602` (counter restore legs); `b001`; `bab031 MOV DX,0x31b0`; `ee OUT DX,AL`; `e460 IN AL,0x60`; `c0e802 SHR AL,2`; `2407 AND AL,7` | — |
| PIC mask + exit | `0a27`–`0a34` | `60 PUSHA`; `891e7c0f/89267a0f` saves; `b009/e620 OUT 0x20,AL(9)`; `f4 HLT` @`0a34` — **cited exit** | — |
| return block | `0a35`–`0a5d` | entry cite = stored IP `0xa35`; `b080/e620 OUT 0x20,AL(0x80)`; `bb0010/8edb DS`; `8e167c0f/8b267a0f SS/SP`; `61 POPA`; `e660 OUT 0x60,AL`; `fa CLI`; `8ac5 AL←CH`; `e612`; `8ac1 AL←CL`; `e602`; `32c0 XOR AL,AL`; `bab031/ee`; `07 POP ES`; `1f POP DS`; `61 POPA`; `5b/58`; `c3 RET` @`0a5d` — **static exit cite**; ends one byte before landing `0a5e` (tiling ✓) | — |
| contacts | — | `[0x2f]`, `[0xf82]`, `[0x9b6]`, `[0xf7c]/[0xf7a]`, `ES:[0x3fc]/[0x3fe]`; ports `0x2/0x12/0x20/0x60/0x31b0` | — |
| boundary | `[09d7..0a5d]` (135 B, contiguous) | inside row `1000:2508..2792` ✓ | — |

**H10 `0x0a5e`/`0x0a63` (arg `0x71a`)** — window `dry_run` `11bd:0a5e`
(100 B `0a5e..0ac1`).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0a5e`–`0a63` | `50 53 bb0010 fa` (`0a63 CLI` = `w1`) | — |
| body | `0a64`–`0a83` | `60 PUSHA`; `b83800/8ec0 ES←0x38`; `26c706a204860a MOV ES:[0x4a2],0xa86`; `a1b609`; `26a3a404 MOV ES:[0x4a4],AX` — return pair `0xa86:[0x9b6]` (DIFFERENT cluster: `[0x4a2]/[0x4a4]`); `891e7c0f/89267a0f` saves; `ba003f MOV DX,0x3f00`; `ed IN AX,DX` | — |
| exit | `0a84` | `ebfe JMP 0x1000:2654` (→ `0x2654−0x1bd0` = `0a84` self) — **cited exit: self-spin tail-JMP** | — |
| return block | `0a86`–`0a9e` | entry cite = stored IP `0xa86`; `b80010/8ed8/8ec0 DS/ES←0x1000`; `8e167c0f/8b267a0f SS/SP`; `ba203f MOV DX,0x3f20`; `b000`; `ee OUT DX,AL`; `61 POPA`; `5b/58`; `c3 RET` @`0a9e` — **static exit cite** | — |
| contacts | — | `[0x9b6]`, `[0xf7c]/[0xf7a]`, `ES:[0x4a2]/[0x4a4]`; ports `0x3f00/0x3f20` | — |
| boundary | `[0a5e..0a9e]` (65 B, contiguous incl. return) | inside row `1000:2508..2792`; `0a9f` = next landing (tiling ✓) | — |

**H11 `0x0a9f`/`0x0aa4` (arg `0x8b2`)** — windows `dry_run` `11bd:0a9f`
(100 B emitted into H12 through `0aff`), `11bd:0b00` (96 B —
**off-alignment disclosed**: starts mid `0aff`'s 7-byte store; aligned
continuation `0b06..` verified consistent), `11bd:0b60` (return tail).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0a9f`–`0aa4` | `50 53 bb0010 fa` (`0aa4 CLI` = `w1`) | — |
| frame | `0aa5`–`0ac6` | `e421 IN AL,0x21`; `60 PUSHA`; `9c PUSHF`; `ff36b609 PUSH [0x9b6]`; `683f0b PUSH 0xb3f` — far frame `:0xb3f` (H11's own return IP); `2b26c409 SUB SP,[0x9c4]` (frame carve); `b0ff/e621 OUT 0x21,AL` (mask ALL); `b83800/8ec0 ES←0x38`; `26891e6904 MOV ES:[0x469],BX`; `2689266704 MOV ES:[0x467],SP` (pair = BX:SP under seg 0x38) | — |
| gate + exit | `0ac7`–`0ad4` | `803ed00e00 CMP byte [0xed0],0`; `7507 JNZ 0x1000:26a5` →`0ad5` (shared tail — **NOT** island `08d5`: `0x26a5−0x1bd0=0xad5` re-derived); `b0fe/e664 OUT 0x64,AL`; `f4 HLT` @`0ad2` — **cited exit**; `ebfd JMP 0x1000:26a2` @`0ad3..0ad4` (→`0ad2`) halt-retry | → `0ad5` |
| shared shutdown tail | `0ad5`–`0ae1` | `c706d0080000 MOV word [0x8d0],0`; `0f011ed008 LIDT word [0x8d0]`; `cdff INT 0xff` — fed by three static edges (`084f` H6, `0acc` H11, `0b0b` H12); contains the only LIDT-class cite of the band handlers; **cited exit of the tail: INT 0xff** (`cdff` ends `0ae1`); `0ae2` = H12 landing — falls through to the next handler's prelude (adjacency cite) | — |
| return block | `0b3f`–`0b5f` | entry cite = own pushed IP `0xb3f`; `fa CLI`; `e81df8 CALL 0x1000:1f30` (→`0360`); `61 POPA`; `e621 OUT 0x21,AL`; `c706d008ff07 MOV [0x8d0],0x7ff`; `803e350000 CMP byte [0x35],0`; `750d JNZ` →`0b60`; `803e3f0000 CMP byte [0x3f],0`; `7403 JZ` →`0b5d`; `e88600 CALL 0x1000:27b3` (→`0be3`, inside `FUN_11bd_0be9` body `0be3..0bef` — edge to DEFINED byte, one hop); `5b/58/c3 RET` @`0b5f` — **static exit cite** | → `0360`, `0be3` |
| delay-call tail | `0b60`–`0b6d` | entered by `0b51 JNZ 0x1000:2730` (→`0b60`): `51 PUSH CX`; `8b0e1000 MOV CX,[0x10]`; `90 NOP`; `e2fe LOOP 0x1000:2736` (→`0b66` self); `e87e00 CALL 0x1000:27b9` (→`0be9` = `FUN_11bd_0be9` ENTRY — defined-body edge); `59 POP CX`; `ebef JMP 0x1000:272d` (→`0b5d` — re-joins return block at `POP BX`) | → `0be9` |
| contacts | — | `[0x9b6]`, `[0x9c4]`, `[0xed0]`, `[0x8d0]`, `[0x35]`, `[0x3f]`, `[0x10]`, `ES:[0x467]/[0x469]`; ports `0x21/0x64` | — |
| boundary | `[0a9f..0ae1]` primary (incl. shared tail) + `[0b3f..0b6d]` return (contiguous, 47 B) | inside row `1000:2508..2792`; tail `0ae0..0ae1` ends one byte before landing `0ae2` ✓ | — |

**H12 `0x0ae2`/`0x0ae7` (arg `0x905`)** — windows `dry_run` `11bd:0a9f`
(100 B, emitted H12 `0ae2..0aff`), `11bd:0b00` (aligned `0b06..0b5f`).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `0ae2`–`0ae7` | `50 53 bb0010 fa` (`0ae7 CLI` = `w1`) | — |
| body | `0ae8`–`0b05` | `e421 IN AL,0x21`; `60 PUSHA`; `891e7c0f/89267a0f` saves; `b83800/8ec0 ES←0x38`; `a1b609 MOV AX,[0x9b6]`; `26a36904 MOV ES:[0x469],AX`; `26c7066704140b MOV ES:[0x467],0xb14` — return pair `0xb14:[0x9b6]` (7-byte store ends `0b05`) | — |
| gate + exit | `0b06`–`0b12` | `803ed00e00 CMP byte [0xed0],0`; `75c8 JNZ 0x1000:26a5` →`0ad5` (shared tail, `0x26a5−0x1bd0` ✓); `b0fe/e664`; `f4 HLT` @`0b11` — **cited exit**; `ebfd JMP 0x1000:26e1` (→`0x26e1−0x1bd0` = `0b11`) halt-retry | → `0ad5` |
| return block | `0b14`–`0b3b` | entry cite = stored IP `0xb14`; `b80010/8ed8 DS`; `8e167c0f/8b267a0f SS/SP`; `e83cf8 CALL 0x1000:1f30` (→`0360`); `b00d/e670 OUT 0x70,AL`; `61 POPA`; `e621 OUT 0x21,AL`; `c706d008ff07 MOV [0x8d0],0x7ff`; `813e35000080 CMP word [0x35],0x8000`; `7403 JZ` →`0b3c`; `5b/58`; `c3 RET` @`0b3b` — **static exit cite** | → `0360` |
| conditional tail | `0b3c`–`0b3e` | `e9bc1d JMP 0x1000:44cb` — `0x0b3f+0x1dbc` = `0x28fb` = pocket DEFINED orphan block `28ee..28fd` (within `FUN_11bd_2864` envelope, uncovered) — flow edge into defined bytes: collision recorded, boundary keeps own bytes | → `28fb` |
| contacts | — | `[0x9b6]`, `[0xf7c]/[0xf7a]`, `ES:[0x467]/[0x469]`, `[0xed0]`, `[0x8d0]`, `[0x35]`; ports `0x21/0x64/0x70` | — |
| boundary | `[0ae2..0b11]` primary + `[0b14..0b3e]` return+tail | inside row `1000:2508..2792`; `0b3f` = H11's return block (cross-handler adjacency: H12 range ends `0b3e`, H11 return starts `0b3f` — tiles with no gap) | — |

**H13 `0x2a5a`/`0x2a60` (arg `0x29bc` — the BLOCK handler)** — window
`dry_run` `11bd:2a5a` (128 B emitted `2a5a..2ad9`).

| element | address | evidence (bytes) | calls (address only) |
|---------|---------|------------------|----------------------|
| prelude | `2a5a`–`2a60` | `50 PUSH AX`; `53 PUSH BX`; `8b1eb409 MOV BX,[0x9b4]` (6-byte preamble — the cited `0x2a5a` form `50538b1eb409fa` ✓); `fa CLI` @`2a60` (= `w1`; explains the `+6` spacing) | — |
| 32-bit save | `2a61`–`2a67` | `6650 PUSH EAX`; `6652 PUSH EDX`; `6651 PUSH ECX`; `6656 PUSH ESI` | — |
| **MSW-clear shape** | `2a69`–`2a73` | `0f01e1 SMSW CX`; **`a14000 MOV AX,[0x40]` @`2a6c`**; `f7d0 NOT AX`; `23c1 AND AX,CX`; `0f01f0 LMSW AX` — the slice-16 flagged shape, REACHED by the walk (fallthrough from `2a60`, `+0xc`; from `2a5a`, `+0x12`) | — |
| body | `2a76`–`2a9f` | `a1700d MOV AX,[0xd70]`; `a35e0d MOV [0xd5e],AX`; `660fb70eb409 MOVZX ECX,[0x9b4]` (second cell read); `668bd4 MOV EDX,ESP`; `8c2e620d MOV [0xd62],GS`; `8c26600d MOV [0xd60],FS` (THE pocket `2880/2884` save cells); `6633c0 XOR EAX,EAX`; `8ee0 MOV FS,AX`; `8ee8 MOV GS,AX`; push storm BEGINS @`2a94`: `50`,`0fa8 PUSH GS`,`50`,`0fa0 PUSH FS`,`6651 PUSH ECX` ×2 (@`2a9a`/`2a9c`), `50`, `53 PUSH BX` @`2a9f` — storm completes `2aa0..2aaa`, next row | — |
| segment load + CALLF | `2aa0`–`2ac3` | storm completes (`2aa0..2aaa`, continuation of row above): `6652 PUSH EDX`@`2aa0`, `6650 PUSH EAX`@`2aa2`, `50`@`2aa4`, `ff36b609 PUSH [0x9b6]`@`2aa5`, `50`@`2aa9`, `68c42a PUSH 0x2ac4`@`2aaa` — far frame `:0x2ac4` (own return); `b82000/8ec0 ES←0x20`; `b83800/8ed8 DS←0x38`; `b80cde MOV AX,0xde0c`; `660fb7e4 MOVZX ESP,SP`; `6626ff1e5a0d CALLF word ptr ES:[0xd5a]` (far call through cell `ES(0x20):[0xd5a]` — dynamic target, cite-only) | → `ES:[0xd5a]` dynamic |
| return block | `2ac4`–`2ad8` | entry cite = own pushed IP `0x2ac4`; `268e2e660d MOV GS,ES:[0xd66]`; `268e26640d MOV FS,ES:[0xd64]` (the pocket `2824/2828` save cells, restored via seg-0x20); `665e POP ESI`; `6659 POP ECX`; `665a POP EDX`; `6658 POP EAX`; `5b POP BX`; `58 POP AX`; `c3 RET` @`2ad8` — **cited exit** | — |
| excluded tail | `2ad9`–`2ada` | `0000` = the live `CS:[0x2ad9]` data tail cell (`2adb` reader + `7739` writer per `## paging block 2978..2ada`) — NOT walked, NOT included (first cited external byte after the exit = `2ad9`) | — |
| contacts | — | `[0x9b4]` ×2 (16-bit + MOVZX), `[0x40]`, `[0x9b6]`, `[0xd5e]`, `[0xd60]/[0xd62]` saves, `ES:[0xd64]/[0xd66]` restores, `ES:[0xd5a]` CALLF cell, `CS:[0x2ad9]` adjacency (excluded); segment stores `ES←0x20`, `DS←0x38`, `FS/GS←0`, FS/GS reload | — |
| boundary | `[2a5a..2ad8]` (127 B) | first attributed entry in block `2978..2ada`: dynamic vector landing `w0`/`w1` from dedupe row = entry cites; range interior fully inside block row `1000:4548..46aa` (all undefined per slice-17/21 + this pass's no-function probes); tail `2ad9..2ada` excluded | — |

### Prelude signature comparison (Step 3)

| handler | prelude bytes cited | vs `0x0938` form `50 53 bb0010 fa` | diff |
|---------|---------------------|------------------------------------|------|
| H1..H12 (band, 12) | `50`,`53`,`bb0010`,`fa` identical at `040e/0491/05af/0697/076f/07e7/0938/099b/09d7/0a5e/0a9f/0ae2` (per-window emission rows above) | MATCH — zero diffs | — |
| H13 (block) | `50`,`53`,`8b1eb409 (MOV BX,[0x9b4])`,`fa` @`2a5a` | matches the `0x2a5a` cited form `50538b1eb409fa` | base from cell `[0x9b4]` (4-byte operand) not immediate `0x1000`; prelude 6 B → `+6` word spacing (slice-21's lone `+6` arg, now explained at the opcode level) |

`w1` check: the `fa` CLI byte is an instruction boundary of each `w0` decode
in all 13 (emitted `CLI` rows above) — the `FUN_11bd_0931` preamble-skip
entry (stub pre-pushes `50/53` at `0932/0933`) lands exactly on the post-
prelude `CLI` in every handler. Gate polarity: four band handlers
(H1/H2/H7 + H9 via the `09d4` hop) carry the `CMP byte [0x2f],0x3` +
`J(C/C)C` pair with the taken/skipped leg ending in `JMP 0x1000:441c` →
`11bd:284c` (pocket SS-rebase preamble FUN) when `[0x2f]>=3` — byte-cited
four distinct `e9..` edges (`041b`,`049e`,`0945`,`09d4`) to the same pocket
entry; the other nine go gate-free straight into the body.

### Block adjacency: `2a6c` verdict (Step 3)

- **H13's walk REACHES `2a6c`.** Fallthrough path `2a5a→2a60(CLI)→2a61..2a67
  (PUSH EAX/EDX/ECX/ESI)→2a69 SMSW CX→2a6c MOV AX,[0x40]→2a6f NOT AX→
  2a71 AND AX,CX→2a73 LMSW AX→2a76 …` — every byte emitted by the single
  `dry_run` window `11bd:2a5a` (128 B), zero branches between the landing
  and the shape. Distance: `+0x12` from `w0`, `+0xc` from `w1`.
- The slice-16 `2a6c..2a73` "MSW-clear locator" (`a14000 f7d0 23c1 0f…`,
  flagged-not-adopted as a *candidate entry*) is therefore **reached and
  statically attributed as inline mid-body code of dispatch handler
  `0x2a5a`** — the flag's own bytes are byte-exact this pass (`a14000`@`2a6c`,
  `f7d0`@`2a6f`, `23c1`@`2a71`, `0f01f0 LMSW`@`2a73`). Per Step 3's
  condition the shape "stays flagged-not-adopted until this slice's walk
  shows otherwise": the walk shows the shape is entered THROUGH `2a5a`/
  `2a60`, not as a separate entry candidate — `2a6c` needs no own feeder
  search beyond the vector landing. Adoption (create) is Task 2's decision;
  Task 1 records the reachability and the attribution basis.
- The SMSW→AND-NOT-`[0x40]`→LMSW op sequence, plus `[0xd60..0xd66]` FS/GS
  save/restore (the same cells the pocket stream `2880/2884` saves under,
  restored at `2ac4/2ac9` through segment-0x20) and the `CALLF ES:[0xd5a]`
  with self return frame `0x2ac4`, make H13 the block-side twin of the
  pocket's mode-transition machinery — mechanism-level naming leg exists
  (CR-access-class SMSW/LMSW ops cited inline), but naming is Task 2 per
  slice rules.
- Block attribution delta: `## paging block 2978..2ada`'s "ZERO attributed
  entries" deliverable is amended by this pass — `2a5a` (+ companion `2a60`)
  are now attributed vector entries with cited boundary `[2a5a..2ad8]`;
  `2978..2a59` and `2adb`-side legs stay as that section states (fall-in
  leg (i) now has a landing INSIDE the block; tail cell `2ad9..2ada`
  unchanged-flagged).
- H6/H11/H12 edges to `0ad5` and the `0x1000:26a5` renders: all three
  recompute `−0x1bd0 = 0xad5` — **none targets the `08c2..08d5` island**;
  the self-spin/halt-retry idioms never leave their handler's band row;
  NO walk touches `08c2`/`033c`/`0bc3`. The only defined-byte flow edges
  found: `JMP→284c` ×4 (entries at byte `041d/04a0/0947/09d6`-side,
  targets = created pocket FUN), `JMP→28fb` ×1 (H12 `0b3c`, defined orphan),
  calls into defined bodies (`0733` @H5 `07ad/07d1`, `0be3/0be9` @H11
  `0b5a/0b68`, pocket `284c`-edge above), and the unowned `0b94..0bc2`
  stub's `CALL 0x1000:3e7d`(→`11bd:22ad` `print_error_message`) +
  fallthrough to island `0bc3` at `0bc2` — recorded; that block is NOT a
  vector handler (no landing, no far-ret cite from the 26 edges) and stays
  unclaimed/deferred.

### Reads executed (ZERO-WRITE branch)

`disassemble_bytes` ×23 EXCLUSIVELY `dry_run=true` (13 primary windows,
6 extension/exit-exact windows incl. the `0697` 102-byte re-run, the
refused `04be` probe, and three disclosed off-alignment probes `07d2`/
`084d→087c`/`0b00` — the `084d` probe was aligned, the misalign list is
`07d2` (−2), `087c` (−1), `0b00` (−6)); `get_function_by_address` ×27
(26 unique offsets + `04be` collision probe, all verbatim above);
`find_code_gaps` ×1 (total 136); `analyze_data_region` ×1 (`04be` —
`current_name DAT_11bd_04be`, `current_type undefined2`, listing-state
fields only, xref_map sites `6bb6/6e99/7371/73a0/73e1/7501` cited as
listing fact not contact count); `inspect_memory_content` ×1 +
`read_memory` ×1 (`04be` 32 B/16 B — hex↔data reconciled ✓, the 16-B read
is a strict prefix of the inspect dump ✓ two-way agreement); NO
create/rename/comment/set_global/save; no transaction opened; pre-existing
bodies re-read only. `/media/felipe/FIFAPCCD/` untouched; `fifa96.rep`
churn left unstaged.

### Deferrals

- Callee bodies (`06fc`, `073c`, `0733`, `0360`, `0be3/0be9`, pocket
  `284c`, orphan `28fb`) and the `CALLF ES:[0xd5a]` dynamic target:
  one-hop cite-only.
- Cell ROLES (`[0x9b4]`, `[0x9b6]`, `[0x40]`, `[0x2f]`, `[0x35]`, `[0xf7c]/
  [0xf7a]`, `[0x467]/[0x469]`, `[0x4a2]/[0x4a4]`, `[0x3fc]/[0x3fe]`,
  `[0x160]/[0x162]`, `[0x404]/[0x406]`, `[0x9c4]/[0x9c6]/[0x9c8]/[0x9cc]`,
  `[0xd5e]/[0xd5a]/[0xd60..0xd66]`, `[0xf82]`, `[0x120c]`, `[0xed0]`,
  `[0x10]`, `[0x20]/[0x22]`, `[0x8d0]`): contacts LOGGED per handler above,
  not resolved (prior-slice dispositions stand — `[0x9b4]` slice-20,
  `[0xf7a]/[0xf7c]` slice-21 deferral, `[0xf50]`/`[0x996]` `## 02b7 twin`
  family — the `0bba` `MOV SP,[0xf50]` sighting in the unclaimed
  `0b94..0bc2` block re-cites that section's raw-scan list, no new claim).
- Unowned/recorded regions: `0852..08a3+` H6 installed-vector candidate,
  `09cf..09d3` (H8 JZ-leg block) / `09d4..09d6` (H9 gate-stub target),
  `0ad5..0ae1` shared
  shutdown tail (proposed under H11), `0b94..0bc2` non-vector stub,
  inter-handler fill (`045e..0490`, `04f3..05ae`, `0675..0696`,
  `06fc..0732`, `073c..076e`, `08d6..0937` incl. stub cluster,
  `0b60..` split per H11 above) — ownership is Task-2 create scope at the
  cited boundaries.
- `DAT_11bd_04be` (defined `undefined2` inside H2): clearing/reclassifying
  it is a listing mutation — Task 2 decides (stop-short boundary stands if
  not resolved).
- H2 true-alignment reconstruction at `04be..04c0` (`ba8404 = MOV DX,
  0x484`): flagged not adopted (data-unit carve makes it a hand decode from
  reconciled raw bytes); post-`04c1` alignment is tool-emit corroborated
  (both windows identical from `04c2`).
- Naming: NOTHING renamed this pass (Task 1 rule) — signature/gate/exit/
  return-block evidence above is the naming-leg raw material for Task 2's
  verdict rows (SMSW/LMSW-class cite exists ONLY in H13; band handlers'
  mechanism wording bounded by their port/PIC/CMOS operand cites).

### Writes (Task 2 — capped creates + verdicts, program `/fifa96.exe`)

Before-state (verbatim, all captured before the first mutation): all 13
landing offsets return the Task-1 dedupe-table owner errors live —
`get_function_by_address(11bd:040e)` → `{"error":"No function found for 11bd:040e"}`,
same form for `0491`/`05af`/`0697`/`076f`/`07e7`/`0938`/`099b`/`09d7`/`0a5e`/
`0a9f`/`0ae2`/`2a5a`; `get_function_count` → `{"function_count":300,"program":"fifa96.exe"}`;
covering gap rows byte-identical to Task 1's quotes (block
`{"start":"1000:4548","end":"1000:46aa","size":355,"has_undefined_bytes":true,"has_orphaned_instructions":false,…}`,
bands `1000:1f30..2302` 979 / `1000:230c..2491` 390 / `1000:2508..2792` 651,
total 136). No STOP-BLOCKED condition fired: no create landed on a defined
byte, no thunk collision at any landing.

REAL disassembly at the cited ranges (13 calls + 1 leg, all `success:true`,
emission byte-identical to the Task-1 dry-run cites above): `11bd:040e` len
53 → `040e..0442` (20 insns, ends on `f4 HLT`); `11bd:0491` len 45 →
`0491..04bd` (16 insns — window boundary = the `DAT_11bd_04be` stop-short,
H2 primary created to here only; the post-carve fragment `04c0..04d2` was
NOT disassembled — forcing a boundary through the defined unit is the
discouraged path; it stays undefined); `11bd:05af` len 182 → `05af..0664`
(98 insns); `11bd:0697` len 51 → `0697..06c9` (18, ends on the `ebf5` retry
JMP); `11bd:076f` len 74 → `076f..07b8` (30, ends on the `ebfe` self-spin);
`11bd:07e7` len 107 → `07e7..0851` (46, ends on the `e98302` tail-JMP);
`11bd:0938` len 99 → `0938..099a` (44 — note the 2-byte NON-EMIT gap at
`0974..0975`, raw `read_memory(11bd:0970,16)` →
`data[252,186,240,0,243,240,46,110,…]` ↔ hex `fcbaf000f3f02e6e…` ✓: bytes
`f3 f0` at `0974..0975` = an undecodable REP+LOCK prefix pair — the pocket
`28e7..28eb` skip-window class, the disassembler jumps `0973 → 0976`);
`11bd:099b` len 50 → `099b..09cc` (19); `11bd:09cd` len 2 → `09cd..09ce`
(the `ebfd` halt-retry bytes cited in the Task-1 H8 row — pre-defined for
neighborhood completeness; it has NO static predecessor — H8's `JZ`@`09c2`
targets `09cf`, fix-wave corrected — so it resolved to the orphan row
`1000:259d..259e` at create, see H8 post-bounds row); `11bd:09d7` len 94 → `09d7..0a34` (42);
`11bd:0a5e` len 40 → `0a5e..0a85` (15, ends on the `ebfe` self-spin);
`11bd:0a9f` len 67 → `0a9f..0ae1` (25, incl. the shared shutdown tail
`0ad5..0ae1`); `11bd:0ae2` len 48 → `0ae2..0b11` (18); `11bd:2a5a` len 127
→ `2a5a..2ad8` (52, incl. the `2a6c` MSW-clear shape and the `2ac4..2ad8`
return block).

Creates (order: `0ae2` → `0a9f` → `07e7` so the shared tail `0ad5..0ae1`
resolves under one owner before the three inbound edges; then batched).
Every response verbatim:

| create cmd | verbatim response |
|------------|--------------------|
| `create_function(11bd:0ae2)` | `{"success":true,"address":"11bd:0ae2","function_name":"FUN_11bd_0ae2","entry_point":"11bd:0ae2","body_size":61,"message":"Function created successfully at 11bd:0ae2"}` |
| `create_function(11bd:0a9f)` | `{"success":true,…,"function_name":"FUN_11bd_0a9f","entry_point":"11bd:0a9f","body_size":65,…}` |
| `create_function(11bd:07e7)` | `{"success":true,…,"function_name":"FUN_11bd_07e7","entry_point":"11bd:07e7","body_size":107,…}` |
| `create_function(11bd:040e)` | `{"success":true,…,"FUN_11bd_040e",…,"body_size":53,…}` |
| `create_function(11bd:0491)` | `{"success":true,…,"FUN_11bd_0491",…,"body_size":45,…}` |
| `create_function(11bd:05af)` | `{"success":true,…,"FUN_11bd_05af",…,"body_size":182,…}` |
| `create_function(11bd:0697)` | `{"success":true,…,"FUN_11bd_0697",…,"body_size":51,…}` |
| `create_function(11bd:076f)` | `{"success":true,…,"FUN_11bd_076f",…,"body_size":74,…}` |
| `create_function(11bd:0938)` | `{"success":true,…,"FUN_11bd_0938",…,"body_size":60,…}` (call-time envelope; final bounds below) |
| `create_function(11bd:099b)` | `{"success":true,…,"FUN_11bd_099b",…,"body_size":55,…}` (call-time envelope; final bounds below) |
| `create_function(11bd:0a5e)` | `{"success":true,…,"FUN_11bd_0a5e",…,"body_size":40,…}` |
| `create_function(11bd:09d7)` | `{"success":true,…,"FUN_11bd_09d7",…,"body_size":97,…}` |
| `create_function(11bd:2a5a)` | `{"success":true,…,"FUN_11bd_2a5a",…,"body_size":127,…}` |

Zero nudges: every create succeeded first-call; the cap's
`disassemble_first=false` retry was never engaged (the two boundary
deviations below are flow decisions on ALREADY-defined bytes — a nudge
cannot change them; ratify per rule).

Post-bounds vs proposal (all `get_function_by_address` verbatim read-backs;
envelope = body_start..body_end as returned, block-ownership quirks quoted
not fought, slice-17/21 precedent):

| H | FUN | proposal (Task 1) | post bounds | disposition |
|---|-----|-------------------|-------------|-------------|
| H1 | `FUN_11bd_040e` | `[040e..045d]` (80 B — Task-1 boundary row: primary + return) | `{"body_start":"11bd:040e","body_end":"11bd:0442"}` | DEVIATION → RATIFIED: a landing create yields the primary only — the first cited exit `0442 HLT` terminates flow and the far-ret half `0443..045d` is statically unreachable (entry cite `ES:[0x160]←0x443`); flow-following acceptance — see the `**` note below and the Deferrals far-ret bullet |
| H2 | `FUN_11bd_0491` | `[0491..04bd]` (split at `DAT_11bd_04be`) | `{"body_start":"11bd:0491","body_end":"11bd:04bd"}` | MATCH the split; fragments `04c0..04d2`/`04d3..04f2` left undefined, stop-short `04bd|04be` cited |
| H3 | `FUN_11bd_05af` | `[05af..0674]` (198 B contiguous — Task-1 boundary row) | `{"body_start":"11bd:05af","body_end":"11bd:0664"}` | DEVIATION → RATIFIED: first cited exit `0664 HLT` terminates the flow; far-ret half `[0665..0674]` (entry cite `PUSH 0x665`@`05c6`) stays undefined — `**` note/Deferrals |
| H4 | `FUN_11bd_0697` | `[0697..06fb]` (165 B — Task-1 boundary row) | `{"body_start":"11bd:0697","body_end":"11bd:06c9"}` | DEVIATION → RATIFIED: flow closes at the owned retry tail-JMP `06c8..06c9` (`ebf5→06bf`); the RET-bearing half `[06ca..06fb]` is far-ret-only (`ES:[0x467]←0x6ca`) — `**` note/Deferrals |
| H5 | `FUN_11bd_076f` | `[076f..07e6]` (120 B — Task-1 boundary row) | `{"body_start":"11bd:076f","body_end":"11bd:07b8"}` | DEVIATION → RATIFIED: self-spin `07b7..07b8` (`ebfe→07b7`) terminates flow; far-ret half `[07b9..07e6]` (`ES:[0x467]←0x7b9`) stays undefined — `**` note/Deferrals |
| H6 | `FUN_11bd_07e7` | `[07e7..0851]` | `{"body_start":"11bd:07e7","body_end":"11bd:0851"}` | MATCH — tail-JMP into `0ad5` (now owned by H11) resolves as an inter-function edge |
| H7 | `FUN_11bd_0938` | `[0938..099a]` | `{"body_start":"11bd:0938","body_end":"11bd:0973"}` | DEVIATION → RATIFIED: the undecodable `f3 f0` pair at `0974..0975` breaks linear flow; remainder `[0976..099a]` defined-but-unowned (post-probe `get_function_by_address(11bd:099a)` → `{"error":"No function found for 11bd:099a"}`) — listing-disposition item, no second create across an undecodable gap (the entry cite governs the landing `0938`, not `0976`) |
| H8 | `FUN_11bd_099b` | `[099b..09cc]` primary + `[0b6e..0b93]` return (Task-1 boundary row) | `{"body_start":"11bd:099b","body_end":"11bd:09d3"}` | DEVIATION → RATIFIED: analyzer flowed `09cf..09d3` INTO the body as the `JZ`@`09c2` TAKEN leg (fix-wave arrow correction: `0x259f−0x1bd0 = 09cf`, not `09cd`; probe `09d1` → `FUN_11bd_099b`); the halt-retry block `09cd..09ce` has NO static predecessor and reappears as an UNCOVERED row (`1000:259d..259e`, `has_orphaned_instructions:true`) despite the envelope — quoted as returned; return `[0b6e..0b93]` left undefined (far-ret half — `**` note/Deferrals) |
| H9 | `FUN_11bd_09d7` | `[09d7..0a5d]` (135 B contiguous — Task-1 boundary row) | `{"body_start":"11bd:09d4","body_end":"11bd:0a34"}` | DEVIATION → RATIFIED: (a) the `JNC→09d4` leg auto-disassembled + absorbed the 3-byte block `09d4..09d6` (`e9751e JMP 284c`) PRE-entry (probe `11bd:09d4` → `{"name":"FUN_11bd_09d7","entry_point":"11bd:09d7","body_start":"11bd:09d4","body_end":"11bd:0a34"}`); entry stays `09d7` ✓; (b) landing create yields primary only — cited exit `0a34 HLT`; far-ret half `[0a35..0a5d]` (`ES:[0x3fc]←0xa35`) — `**` note/Deferrals |
| H10 | `FUN_11bd_0a5e` | `[0a5e..0a9e]` (65 B, "contiguous incl. return" — Task-1 boundary row) | `{"body_start":"11bd:0a5e","body_end":"11bd:0a85"}` | DEVIATION → RATIFIED: self-spin `0a84..0a85` (`ebfe→0a84`) terminates; far-ret half `[0a86..0a9e]` (`ES:[0x4a2]←0xa86`) stays undefined — `**` note/Deferrals |
| H11 | `FUN_11bd_0a9f` | `[0a9f..0ae1]` (shared-tail proposal) | `{"body_start":"11bd:0a9f","body_end":"11bd:0ae1"}` | MATCH — tail outcome reversed from the interim read (H12 initially showed envelope `0ad5..0b11`; after `create_function(0a9f)` the block `0ad5..0ae1` resolves to H11: probe `11bd:0ad5` → `{"name":"FUN_11bd_0a9f",…}`; H12 final `{"body_start":"11bd:0ae2","body_end":"11bd:0b11"}`) — Task-1 proposal achieved; halt-retry fragment `0ad3..0ad4` left defined-unowned (probe `{"error":"No function found for 11bd:0ad3"}`) |
| H12 | `FUN_11bd_0ae2` | `[0ae2..0b11]` | `{"body_start":"11bd:0ae2","body_end":"11bd:0b11"}` | MATCH; the `0b3c JMP→0x28fb` edge now points into pocket-defined orphan bytes — no absorption (target owned by no function; slice-21 R7 orphan state unchanged); return `[0b14..0b3e]` left undefined |
| H13 | `clear_msw_and_callfar` (was `FUN_11bd_2a5a`) | `[2a5a..2ad8]` | `{"entry_point":"11bd:2a5a","body_start":"11bd:2a5a","body_end":"11bd:2ad8"}` | MATCH — the `CALLF ES:[0xd5a]` fall-through absorbed the `2ac4..2ad8` return block INTO the body (statically contiguous per listing — no separate create made, none needed); `2ad9..2ada` tail cell EXCLUDED ✓ |

** `**`: the six primary-only relabels (H1/H3/H4/H5/H9/H10 — and H8's return
half) are ONE deferred layer, not six defects: every far-return block is
entered only through the runtime far-ret pair its handler stores
(`[0x467]/[0x469]`, `[0x3fc]/[0x3fe]`, `[0x160]/[0x162]`, `[0x4a2]/[0x4a4]`
← stored IP + CS from `[0x9b6]`), so the landing create CANNOT statically
absorb them — the one-hop-out consumer of that pair is the deferred layer
(Deferrals: "Far-return blocks LEFT UNDEFINED"). Task-1 report concern 3
anticipated exactly this outcome.

Side-effect discloses (analyzer-created, RATIFIED+FLAGGED per slice-17/21
precedent — both are H4/H5 callee islands resolved from the `CALL` edges
during create; each stopped one byte BEFORE the next arg-pair source cell,
so the vector source DATA stays intact):
`get_function_by_address(11bd:06fc)` → `{"name":"FUN_11bd_06fc","signature":"byte FUN_11bd_06fc(void)","entry_point":"11bd:06fc","body_start":"11bd:06fc","body_end":"11bd:0715"}` (H4's `0x1000:22cc` callee; `0716/0718` = arg `0x71a` source cells, undefined ✓);
`get_function_by_address(11bd:073c)` → `{"name":"FUN_11bd_073c","entry_point":"11bd:073c","body_start":"11bd:073c","body_end":"11bd:0744"}` (H5's `0x1000:230c` callee; `0745/0747` = arg `0x749` source cells, undefined ✓).
`get_function_count` → 300 → **315** (+15 = 13 cited creates + 2 flow-effect
islands, exact arithmetic). No pocket/`092c`/`0931`/island body moved
(`FUN_11bd_2824`/`284c`/`2864`, `restore_fs_gs_and_resume`,
`dispatch_mode_vector`, `FUN_11bd_0931`, `08c2`/`033c`/`0bc3` unchanged —
post gap rows for their neighborhoods re-quoted identical).

Name + plate (Step 2): `rename_function(FUN_11bd_2a5a → clear_msw_and_callfar)`
→ `{"status":"success","message":"Success: Renamed function at FUN_11bd_2a5a
from 'FUN_11bd_2a5a' to 'clear_msw_and_callfar'",…}` (two PascalCase style
warnings quoted-as-returned; snake_case kept per repo convention — the
slice-21 `mode_vector_source_pair` naming-gate precedent). Naming bar:
the pre-flight ruling accepts msw-clear-class wording ONLY with cited
MSW-ops — satisfied IN-BODY: `0f01e1 SMSW CX`@`2a69`, `a14000 MOV AX,[0x40]`
@`2a6c`, `f7d0 NOT AX`, `23c1 AND AX,CX`, `0f01f0 LMSW AX`@`2a73` (the AND
of SMSW with the NOT of `[0x40]` clears the mask bits the cell names;
LMSW is the CR0-MSW write). Plate `set_comment(11bd:2a5a, plate)` →
`{"status":"success","message":"Set plate comment at 11bd:2a5a","warnings":[…"missing Algorithm/Parameters/Returns"…]}`
(full text in the first Deferrals bullet; `C: none — behavioral (…)` form
per slice-21). All twelve band handlers KEEP DEFAULT NAMES: the entry leg
holds (vector landing) but each ROLE leg needs the one-hop-out layer
(callee trees `06fc`/`073c`/`0360`/`0be3`/`0be9`, port-leg consumers, the
far-ret pair CONSUMER that re-enters the return blocks) — NOT-CONFIRMED-at-
role → no rename/plate. H11 additionally withholds despite the now in-body
LIDT (`0f011ed008` in the absorbed tail block): the tail is three-edge-
shared with H6/H12 and the `INT 0xff` consumer/IDT state is the deferred
layer — the missing leg is tail-specificity, recorded below.
`save_program` → `{"success":true,"program":"fifa96.exe","message":"Program saved successfully"}`.

Post-state gap carve (total 136 → **149**; the required block shrink):
`{"start":"1000:4548","end":"1000:4629","size":226,…,"before_function":"restore_fs_gs_and_resume","after_function":"clear_msw_and_callfar","after_function_address":"11bd:2a5a"}`
(= `2978..2a59` ✓ head still UNOWNED — slice-17 legs (i)/(ii)/(iii) stand,
and leg (i) "fall-in from the pocket" now has a named inside-block landing
at `+0xe2`) and
`{"start":"1000:46a9","end":"1000:46aa","size":2,…,"before_function":"clear_msw_and_callfar","after_function":"FUN_11bd_2adb"}`
(= `2ad9..2ada` ✓ — the `CS:[0x2ad9]` live tail-cell row, unchanged
undefined). Tiling: `226 + 127 + 2 = 355` ✓ = original row size. Band rows
split at every create: `1f30..1fdd` (174 = `0360..040d`), `2013..2060` (78
= `0443..0490`), `208e..217e` (241 = `04be..05ad`, `has_undefined_bytes:
false` = the DAT unit + the two untouched H2 fragments span, quoted as
returned), `2235..2266` (50 = `0665..0696`), `229a..22cb` (50 =
`06ca..06fb`), `22e6..2302` (29 = `0716..0732`), `2315..233e` (42 =
`0745..076e`), `2389..23b6` (46 = `07b9..07e6`), `2422..2491` (112 =
`0852..08c1`), `2544..256a` (39 = `0974..099a`, `has_orphaned_instructions:
true` — the H7 undecodable-gap + orphan-remainder row), `259d..259e` (2 =
`09cd..09ce` orphan row), `2605..262d` (41 = `0a35..0a5d`), `2656..266e`
(25 = `0a86..0a9e`), `26a3..26a4` (2 = `0ad3..0ad4` orphan row),
`26e2..2792` (177 = `0b12..0bc2` — all deferred return blocks + the
non-vector `0b94..0bc2` stub, untouched). `1ea4..1f0b`/`24a6..24f8` rows
unchanged.

### Verdicts (Task 2)

| FUN | address | evidence | new_name | C counterpart |
|-----|---------|----------|----------|---------------|
| `FUN_11bd_040e` | `11bd:040e` | vector landing `0x3d6:w0` (dedupe row); exit `0442 HLT f4`; return block `[0443..045d]` entry-cited by own `ES:[0x160]←0x443` store, left undefined | — (create-only) | role leg open: gate legs + stored-pair consumer unattributed |
| `FUN_11bd_0491` | `11bd:0491` | landing `0x462:w0`; split stop-short at `DAT_11bd_04be` (`04bd|04be` cited); return cite `ES:[0x3fc]←0x4d3` | — (create-only) | HLT@`04d2` beyond the carve stays unowned — role statement blocked by the split |
| `FUN_11bd_05af` | `11bd:05af` | landing `0x4f7:w0`; CMOS `0x368/0x36a` nibble-stream ops; exit `0664 HLT` | — (create-only) | operand-level only; `0x665`/`[0x9b6]` roles one hop out |
| `FUN_11bd_0697` | `11bd:0697` | landing `0x679:w0`; retry-loop exit `06c8 ebf5→06bf`; callee `06fc` auto-island ratified | — (create-only) | `AH=0xc0` callee semantics (`06fc` body) deferred |
| `FUN_11bd_076f` | `11bd:076f` | landing `0x749:w0`; exit `07b7 ebfe` self-spin; callee `073c`, calls into `FUN_11bd_0733` ×2 | — (create-only) | RTC-port arm role open |
| `FUN_11bd_07e7` | `11bd:07e7` | landing `0x8ac:w0`; PIT read/reprogram + `[0x20]/[0x22]` vector-save; tail-JMP `084f→0ad5` | — (create-only) | installed-vector region `0852..` unowned (no attributed entry) |
| `FUN_11bd_0938` | `11bd:0938` | landing `0x381:w0` (THE `0x0938` cite row); body split by undecodable `f3 f0` (`0974..0975`); `RET 099a` sits in the unowned remainder | — (create-only) | flow-split: whole-body role needs the `0976..099a` disposition first |
| `FUN_11bd_099b` | `11bd:099b` | landing `0x8da:w0`; A20 port `0x92` ops; exit `09cc HLT`; analyzer absorbed `09cf..09d3` as the `JZ`@`09c2` taken leg; `09cd..09ce` re-listed orphan | — (create-only) | `[0x120c]`-test branch semantics deferred |
| `FUN_11bd_09d7` | `11bd:09d7` | landing `0x3a7:w0`; FNSAVE `[BX]` via `[0xf82]`; pre-entry block `09d4..09d6` absorbed; exit `0a34 HLT` | — (create-only) | FPU-state consumer (`[0xf82]` role) deferred |
| `FUN_11bd_0a5e` | `11bd:0a5e` | landing `0x71a:w0`; ports `0x3f00/0x3f20`; exit `0a84 ebfe` spin | — (create-only) | `[0x4a2]/[0x4a4]` pair consumer deferred |
| `FUN_11bd_0a9f` | `11bd:0a9f` | landing `0x8b2:w0`; mask-all `OUT 0x21,0xff` + `SUB SP,[0x9c4]`; shared shutdown tail `0ad5..0ae1` (`LIDT [0x8d0]`→`INT 0xff`) resolved UNDER this body | — (create-only) | LIDT-class cite in-body → wording leg AVAILABLE, withheld: tail shared by H6@`084f`/H12@`0b0b` edges + `INT 0xff` consumer tree = deferred layer |
| `FUN_11bd_0ae2` | `11bd:0ae2` | landing `0x905:w0`; `ES:[0x467]←0xb14`; exit `0b11 HLT`; `0b3c JMP→28fb` edge into pocket orphan | — (create-only) | `[0xed0]`-gate semantics deferred |
| `clear_msw_and_callfar` | `11bd:2a5a` | landing `0x29bc:w0` — FIRST ATTRIBUTED BLOCK ENTRY; inline `2a6c..2a73` SMSW/AND-NOT-`[0x40]`/LMSW (slice-16 shape cited-then-adopted in-body); `CALLF ES:[0xd5a]`; RET `2ad8`; tail `2ad9..2ada` excluded | `clear_msw_and_callfar` (plate set) | verb-led mechanism from cited SMSW/LMSW ops; no direction/mode words — pre-flight bar cleared |

### Deferrals (Task 2 additions)

- Plate text (H13): `C: none — behavioral (vector dispatch landing handler,
  arg 0x29bc pair w0/w1: PUSH AX;PUSH BX;MOV BX,[0x9b4];CLI preamble;
  32-bit PUSH EAX/EDX/ECX/ESI; SMSW CX @2a69; AX←[0x40], NOT, AND AX,CX,
  LMSW AX @2a73 (clear MSW bits per cell [0x40] mask); GS/FS saved to
  [0xd62]/[0xd60] then zeroed; far frame push (:0x2ac4) + ES←0x20/DS←0x38;
  CALLF ES:[0xd5a]; return reloads GS/FS from ES:[0xd66]/[0xd64], pops
  ESI/ECX/EDX/EAX/BX/AX, RET @2ad8; tail cells 2ad9..2ada excluded)`.
- Listing-disposition items (defined, unowned, left so): H7 remainder
  `[0976..099a]` + the undecodable pair `f3 f0`@`0974..0975` (row
  `1000:2544..256a`); H8 halt-retry `[09cd..09ce]` (row `1000:259d..259e`);
  H11 retry `[0ad3..0ad4]` (row `1000:26a3..26a4`). No create forced across
  any of them; no delete made.
- Far-return blocks LEFT UNDEFINED (never disassembled this pass; entry
  cites in the Task-1 walk tables): H1 `[0443..045d]`, H2 `[04c0..04f2]`
  (post-carve, incl. the HLT@`04d2` and RET@`04f2`), H3 `[0665..0674]`,
  H4 `[06ca..06fb]`, H5 `[07b9..07e6]`, H8 `[0b6e..0b93]`, H9
  `[0a35..0a5d]`, H10 `[0a86..0a9e]`, H11 `[0b3f..0b6d]`, H12
  `[0b14..0b3e]` — each reachable only via the runtime far-ret pair
  (`[0x467]/[0x469]`, `[0x3fc]/[0x3fe]`, `[0x160]/[0x162]`,
  `[0x4a2]/[0x4a4]` ← stored IP + CS from `[0x9b6]`); creating them now
  would assert an entry the listing cannot show (one-hop rule).
- `DAT_11bd_04be` untouched (2 B defined data inside H2's true stream;
  its 6 xref sites `6bb6/6e99/7371/73a0/73e1/7501` remain cite-only);
  H2 true-alignment reconstruction at `04be..04c0` stays flagged-not-
  adopted.
- Callee trees one hop out: `06fc`/`073c` (auto-islands — bodies not
  examined), `0733`/`073c`/`0360`/`0be3`/`0be9`/`284c`/`28fb`/`22ad`,
  `CALLF ES:[0xd5a]` dynamic target. Cell consumers: `[0x9b4]` (slice-20
  disposition stands), `[0x9b6]`, `[0x40]` (slices 15/17 stand — H13 now
  adds a cited READER at `2a6c` INSIDE an owned function, noted, no
  direction claim), `[0xdfe]` (head `2978` byte — block still unowned,
  untouched), `[0xd5a]/[0xd5e]/[0xd60..0xd66]`, `[0xf82]`, `[0x9c4]`,
  `[0x9c6]/[0x9c8]/[0x9cc]`, `[0xed0]`, `[0x120c]`, `[0x8d0]`,
  `[0x20]/[0x22]`, `[0x10]`, `[0x2f]`, `[0x35]`, `[0x3f]` — logged per
  Task-1 tables, not resolved.
- Islands `08c2`/`033c`/`0bc3` and pocket FUNs `2824`/`284c`/`2864`:
  bodies untouched; only EDGES arrive (`JMP→284c` ×4 now inter-function;
  `JMP→28fb` inter-function; no island entered, none grown).
- Block interior: `2978..2a59` (226 B) stays NAMED-OPEN with the slice-17
  legs; `0b94..0bc2` non-vector stub untouched; `CS:[0x2ad9]` tail
  ownership unchanged (row `1000:46a9..46aa` re-quoted); `2a30` remains
  unreferenced by any walked flow.
- Vector selection logic between `092c`/`0931` (readers side) and the
  `3ed8` 14-arg arm cascade: untouched, prior dispositions stand.
- MSW-clear shape status: ADOPTED-IN-BODY per this slice's walk + create
  (was flagged-not-adopted): `2a6c..2a73` now sits inside
  `clear_msw_and_callfar`; the "candidate entry at `2a6c`" question is
  CLOSED (entered THROUGH the vector landing, no separate feeder needed);
  the `0ad5..0ae1` shutdown tail is owned by `FUN_11bd_0a9f` — Task-1's
  non-adoption flag discharged with the verbatim interim/ final envelope
  reads above.
- Suite: no test/tool/C changes; `cmake --build build && ctest` green
  post-write (docs-only diff). `/media/felipe/FIFAPCCD/` untouched;
  `fifa96.rep` churn left unstaged.

### Fix wave (final-review corrections, 2026-09-29 — docs-only, zero Ghidra writes)

Live verification (reads only, this wave): `read_memory(11bd:09c2,6)` →
`{"data":[116,11,228,146,12,3],"hex":"740be4920c03"}` — `JZ` rel8 `+0x0b`:
target `09c2+2+0x0b = 09cf` ✓ (and `0x259f−0x1bd0 = 0x09cf`);
`disassemble_bytes(11bd:09c2, length 16, dry_run=true)` re-emitted the same
render `JZ 0x1000:259f` with `b0fe MOV AL,0xfe` living at `09cf` ✓ — the
reviewer's finding stands: the Task-1 H8 body-row arrow `→09cd` was an
arithmetic error on the rendered operand. Spot-checked two re-quoted
proposals against the Task-1 boundary rows (H1 `[040e..045d]` 80 B; H10
`[0a5e..0a9e]` 65 B "contiguous incl. return") ✓ as printed in the
reviewer's finding. Changes made (this section only; prior sections
untouched):
(1) H8 walk table — `JZ` arrow corrected `09cd`→`09cf` (with the byte cite
above); halt-retry row's "also the JZ@`09c2` target" claim deleted,
replaced with the no-predecessor/orphan explanation; former "beyond-exit
block `09cf..09d6`" row split into "JZ-leg block `09cf..09d3`" (REACHED —
explains the create absorption) and the `09d4..09d6` gate stub (NOT
reached by H8 flow; H9-gate-only), boundary row and both deferrals-region
references updated to match;
(2) post-bounds table — H1/H3/H4/H5/H9/H10 relabeled MATCH →
DEVIATION → RATIFIED against their RE-QUOTED Task-1 boundary rows
(previously the proposal cells had been silently re-based to primary
only); H8/H9 proposal cells likewise re-quoted from Task-1; shared
`**` note added pointing the six far-ret halves at the one Deferrals
bullet (flow-following acceptance, anticipated by Task-1 report concern
3);
(3) Minors folded — H4 retry-loop range completed to `06c1..06c9`; H11
gate+exit range completed to `0ac7..0ad4`; H13 push-storm partition
cleaned (body row `2a76..2a9f` lists the storm head, segment row
`2aa0..2ac3` lists the tail with per-op addresses — no double-listing,
no address mislabels); `(call-time envelope; final bounds below)` added
to the `0938`/`099b` create rows; H2 window wording aligned with the
disclosed decoder-skip behavior ("emission ENDED at `04bd` because the
linear decoder does not carry across the `04be` defined unit"); Task-2
leg rationale for the `09cd` 2-byte pre-define reworded (it resolves to
the orphan row — the JZ does not reach it).
Program state and the prior commits' cites were NOT re-verified beyond
the two read-only probes above; nothing in the listing was mutated this
wave.

## block head 2978..2a59 (verified 2026-09-29, program `/fifa96.exe`)

Zero-Ghidra-write classification pass on the 226-byte block head — the
stretch `11bd:2978..2a59` left inside the former `2978..2ada` paging
block after slice-22's carve created `clear_msw_and_callfar`
(`2a5a..2ad8`) (gap row `1000:4548..1000:4629`). Headline: (1) the head
is TWO flow-disconnected CODE islands around ONE TABLE run — R1
`2978..29b7` (64 B): `[0xdfe]` gate → CR0 paging-enable (`6650 0f20c0
660d00000080 0f22c0`) → TSS-descriptor byte patch (`2999 MOV BX,
word [0xdfc]` → DS←`0x8` → `29a2 c64705 89 MOV byte [BX+5],0x89` → DS←
`0x20`) → `LTR` `0f00d8`@`29ae` + `CLTS` `0f06`@`29b1` → `POP EAX` →
tail `JMP 0x1000:1f07` (`e97fd9`@`29b5`, rel `d97f`=−`0x2681`,
`29b8−0x2681 = 0337` ✓, render `1f07−1bd0 = 0337` ✓) whose landing is
live-verified as the orphan-band relay `2eff26fa02` = `JMP word ptr
CS:[0x2fa]` (`0337..033b`, inside row `1000:1ea4..1000:1f0b`
`has_undefined_bytes:true`, `get_function_by_address(11bd:0337)` → no-
function error — exit cite only, one hop, not followed); R2
`29b8..29bb` (4 B): the `[0x9bc]`/`[0x9be]` pair source words for arg
`0x29bc` — `5a2a602a` (live `read_memory(29b8,4)` reconciled) =
`0x2a5a`/`0x2a60`, both INSIDE the owned handler body (slice-22 H13
boundary `2a5a..2ad8`), so the pair is a TABLE selecting handler
entries, NOT a pointer into the head; R3 `29bc..2a59` (158 B): PM-
transition save/restore island (GS/FS→`[0xd66]/[0xd64]` — the pocket
`2824/2828` twin cells; EAX→`[0xd6c]`, ESI→`[0xd68]`, `[0x8c8..0x8d4]`→
`[0xd4e..0xd58]` copy legs, SP→`[0xd78]/[0xdb0]`, ESI←`[0xd34]`,
`[0xd70]`→`[0xd5e]` (same cell pair H13 uses at `2a76/2a79`), FS/GS←0,
`b80cde MOV AX,0xde0c` + `cd67 INT 0x67`, DS←`0x20`, `2a1f TEST byte
[0x47],0x20`, `2a24 7513 JNZ`→`2a39` (`2a26+0x13`, `4609−1bd0=2a39` ✓),
restore legs `2a26..2a37` (`[0xd6c]/[0xd68]` reload, FS/GS←`[0xd60]/
[0xd62]`, `2a37 ffe3 JMP BX` DYNAMIC exit) and `2a39..2a58` (ESI←
`[0xde4]`, FS←`0x38`, `LES DI,[0xde8]`, `MOVZX EDI,DI`, CLD, TWO
`676664a5` = `MOVSD ES:EDI,FS:ESI` implicit string ops, `6697`
restore, `2a58 ebcc JMP`→`2a26` internal back-edge, `2a5a−0x34=2a26` ✓,
`45f6−1bd0=2a26` ✓). Dry-run window emitted 73 insns over 226/226 bytes
with ZERO decode-skips (contrast: pocket `2811..296c` had two skip
windows). (2) ZERO static entries into `2978..2a59` re-verified in the
post-carve state — the NEW `0x1000:45` class (4 hits, all
`FUN_11bd_2864` rel8/loop legs) recomputes to `2945/294a/2953/2957`
(pocket, below `2978`); `0x1000:46` = the same 8 slice-17 hits, every
target ≥ `2aeb`; `0x1000:29`/`0x1000:2a` reproduce 12/7 recomputing to
`0dxx`/`0ef4`; `11bd:29`/`11bd:2a` 0; operand `0x2978`/`0x2a30`/
`0x2a59` 0 each; `[0x9c2]` writer set still {`41ee`} value `0x296d`
(out of range); determinability re-scan: of the 28 arm cells
(`arg−4`/`arg−2` for all 14 args), exactly ONE pair lands in the head —
`0x29bc`→`29b8/29ba` — and it is a DATA source, its values pointing
into the handler, not the head; the `0x29bc` numeral is referenced only
by the two rejected frame-slot stores (`2f18`/`44ab`, slice-17 ruling +
`[0x9ba]` NONE-FROM-DISCIPLINE stand); no far-ret pair stores an IP in
range (the `0x2a` family's `2aaa PUSH 0x2ac4` is the handler's own
return frame — target `2ac4` inside `clear_msw_and_callfar`). (3)
`[0xdfe]` sweep — READERS among defined insns: ZERO; WRITERS FOUND:
`2892 c606fe0d00` + `28a0 c606fe0d01` = `MOV byte ptr [0xdfe],0x0/0x1`
inside `FUN_11bd_2864` (the pocket body created by slice-21 — the
writer set was never swept after creation; this pass supersedes the
slice-22 deferral wording "writers open" at the defined-instruction
layer). The program's only read of `[0xdfe]` is `2978` itself (`803efe0d01`),
in UNDEFINED bytes — invisible to defined-insn search by construction,
cited from raw bytes + the dry-run emission. Window arithmetic for the
`0xdfe..0xdff` reach class: constant-base census at this-slice time
(BX 46 / SI 62 / DI 35 / BP 1) × form partitions (`[BX + 0` envelope
320, sign runs 10/15/6, bare 140/40/35/0, pair 348/12, compound 4/3) —
ZERO constant-base site resolves into the window (per-candidate
arithmetic below); ES-override sites are NO-OP class (the cell is
DS-absolute; an ES-based window can't statically address it); dynamic
DS-relative sites are OPEN-WINDOW holes (never silently rejected).
Brief-hint reconciliation: the task hint "constant base [2df6,2e41]"
resolves live to the slice-20 anchor cluster `2df5 8b5e06 MOV BX,
[BP+0x6]` (RUNTIME base — its `2df8 ff7702 PUSH [BX+0x2]` is an
OPEN-WINDOW row reaching `0xdfe` iff runtime BX=`0xdfc`, not a constant)
and `2e41` = rel8 byte inside `2e40 75e7 JNZ`→`2e29` (no window site
there — dump-cited); the hint "partition `fe0d`@3059" does NOT
reproduce: raw `fe0d` = 4 hits {`1132`,`2894`,`28a2`,`297a`}, and all
three `3059` space-readings were probed negative (`1991:3059` = `e98200`
JMP rel16 head — no `fe0d` bytes; `11bd:3059` = `36` mid-`ff36`-push
operand; `1000:3059` = `11bd:1489` = `b0 08 MOV AL,0x8`) — reported as a
live-partition correction, not adopted. Quote protocol: 11/11
`read_memory` responses reconciled `hex`-vs-`data` before quoting (the
96+96+34 byte-truth chunks also reconcile insn-byte-for-insn-byte
against the dry-run emission at every boundary — three-way agreement,
no render glitch seen); 2 cap-truncated `[BP +` discovery runs are
class-rejected per the slice-20 method (1-hit BP census; stack-frame
disp sets observed `−0x62..−0x1`/`0x0..0x26`/`0xff00..0xff7e`; the only
census-constant reach disp is `[BP + 0xdfd]` and the targeted run
returns 0). Caveats standing: `search_instructions` is
defined-instruction-only (`instructions_scanned:14638` uniform on every
run at this-slice time — grew from slice-21's 14006/14170 as slice-22's
13 creates + islands landed), intra-gap relative flow is invisible
until decoded (the pocket skip bytes `28e7..28eb`/`28ed`, 7 B, were
again NOT disassembled this pass — residual fall-in hole), `get_xrefs_to`
and `list_data_items_by_xrefs` are dead channels for these forms
(probes on `2978`/`29b8`/`0dfe` all returned 0/empty — reported, not
relied), implicit string operands (`MOVS`/`STOS`/the head's own
`2a4e/2a52 MOVSD`) render no offset expression and escape every
operand/window sweep, and raw `search_byte_patterns` hits must be
context-reconciled (`fe0d`@`1132` is the rel16 high bytes of the
orphan-band `1130 e83bfe0d CALL` — `1134−0x1c5 = 0f6f`, a false
positive, not a data contact). No `create_function`, no rename, no
comment, no `set_global`/define, no `save_program`, no real
`disassemble_bytes`; pre-existing bodies (`restore_fs_gs_and_resume`,
`clear_msw_and_callfar`, `FUN_11bd_2864`, `FUN_11bd_033c`) re-read only;
`/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left unstaged.

### Confirmation rows (Step 1)

| Probe | Verbatim response | Reconciliation / math |
|-------|-------------------|------------------------|
| `get_function_by_address(11bd:2978)` | `{"error":"No function found for 11bd:2978"}` | head start UNOWNED ✓ (2978 = `1000:4548`−`0x1bd0`) |
| `get_function_by_address(11bd:2977)` | `{"name":"restore_fs_gs_and_resume","address":"11bd:296d","signature":"undefined restore_fs_gs_and_resume(void)","entry_point":"11bd:296d","body_start":"11bd:296d","body_end":"11bd:2977"}` | head = body_end+1 ✓; before-neighbor unchanged since slice-14 |
| `get_function_by_address(11bd:2a59)` | `{"error":"No function found for 11bd:2a59"}` | head end UNOWNED ✓ (2a59 = `1000:4629`−`0x1bd0`) |
| `get_function_by_address(11bd:2a5a)` | `{"name":"clear_msw_and_callfar",…,"entry_point":"11bd:2a5a","body_start":"11bd:2a5a","body_end":"11bd:2ad8"}` | head = body_start−1 ✓; after-neighbor is the slice-22 H13 handler (read-only context per scope guard) |
| `find_code_gaps` head row (total 149, offset 0/limit 100) | `{"start":"1000:4548","end":"1000:4629","size":226,"has_undefined_bytes":true,"has_orphaned_instructions":false,"before_function":"restore_fs_gs_and_resume","before_function_address":"11bd:296d","after_function":"clear_msw_and_callfar","after_function_address":"11bd:2a5a"}` | `0x4548−0x1bd0 = 0x2978` ✓, `0x4629−0x1bd0 = 0x2a59` ✓, `0x4629−0x4548+1 = 0xE2 = 226 = 0x2a59−0x2978+1` ✓ — byte-identical to slice-22's post-carve quote ✓ (total 149 = slice-22's post-state count ✓); tail row `{"start":"1000:46a9","end":"1000:46aa","size":2,…}` (= `2ad9..2ada`, `CS:[0x2ad9]` live tail cell) also re-quoted identical |
| tail-`JMP` re-cite | `read_memory(11bd:2975,3)` → `{"data":[233,61,217],"hex":"e93dd9"}` | data→hex ✓ (233=`0xe9`,61=`0x3d`,217=`0xd9`); byte-level re-cite of the `## 296d hook target` row "exit (tail JMP, cited) … `e93dd9`, 3 bytes → ends `2977`" — unconditional `JMP 0x1000:1e85`, NOT re-walked; the stub body ends AT `2977` (owner row above), so no fallthrough path into `2978` |
| head bytes | `read_memory(11bd:2978,5)` → `{"data":[128,62,254,13,1],"hex":"803efe0d01"}` | data→hex ✓ (128=`0x80`,62=`0x3e`,254=`0xfe`,13=`0x0d`) — `CMP byte ptr [0xdfe],0x1`, byte-exact vs the slice-14/17 cites, listing still UNDEFINED (`get_function_by_address(2978)` error row above) |

### Byte-run classification (Step 2) — tiles `2978..2a59` contiguously

Walk: dry-run `disassemble_bytes(11bd:2978, length 226, max_instructions
400)` → `{"dry_run":true,"success":true,"start_address":"11bd:2978",
"end_address":"11bd:2a59","bytes_disassembled":226,"instructions_total":
73,"truncated":false}` — emission contiguous `2978→2a59` (no gaps),
ZERO decode-skips (disclosed-by-absence vs the pocket precedent). Byte
truth: `read_memory(2978,96)`+`read_memory(29d8,96)`+`read_memory(2a38,34)`
= 96+96+34 = 226 B, each internally hex↔data reconciled AND matching
the emitted per-insn bytes at every instruction boundary (chunk-2/3
start mid-insn by design: `29d8` is the 4th byte of `66a34e0d`@`29d5`,
`2a38` is byte 2 of `ffe3`@`2a37` — reconciled against the emission).

| run | range | evidence (hex+data reconciled) | class | entry/exit or table-role |
|-----|-------|--------------------------------|-------|---------------------------|
| R1 | `2978..29b7` (64 B, 22 insns) | `803efe0d01` CMP→`[0xdfe]`; `0f853400` JNZ→`29b5` (`2981+0x34`, render `4585−1bd0`✓); `6650 0f20c0 660d00000080 0f22c0` CR0 PG set; `a1fc0d 0bc0 0f841b00` (`2994` JZ→`29b3`, `2998+0x1b`, `4583−1bd0`✓); `53 8b1efc0d b80800 8ed8 c6470589` (`29a2` DS-rel window store `[BX+5]←0x89`, BX=`[0xdfc]` loaded under original DS, DS←`0x8` before the store); `8bc3 bb2000 8edb 5b` (DS←`0x20`, POP BX); `0f00d8 LTR`, `0f06 CLTS`, `6658 POP EAX`, `e97fd9` JMP→`0x1000:1f07` (`29b8−0x2681 = 0337`✓) | CODE (island A) | entry: NONE static (reachability table); exit: unconditional tail-JMP `29b5`→`11bd:0337` = orphan-band relay `2eff26fa02` `JMP word ptr CS:[0x2fa]` (5 B, row `1000:1ea4..1f0b`, no-function probe quoted; one hop, not followed) |
| R2 | `29b8..29bb` (4 B) | `read_memory(11bd:29b8,4)` → `{"data":[90,42,96,42],"hex":"5a2a602a"}` ✓ internally reconciled; LE words `[29b8]=0x2a5a`, `[29ba]=0x2a60` — byte-identical to the determinability appendix `29b8` row (`5a2a602a`, `## 2811..296c pocket` table); linear stream emits `5a POP DX`+`2a602a SUB AH,[BX+SI+0x2a]` — post-unconditional-JMP emission artifact (no flow edge lands here, see Step 3); values are instruction-boundary entries of the OWNED handler (`2a5a PUSH AX` head, `2a60 CLI` post-preamble — slice-22 H13 rows) | TABLE | role: `[0x9bc]`/`[0x9be]` source pair for arg `0x29bc` — `6270 2e8b47fc`/`6277 2e8b47fe` CS-window loads (map rows `## 2811..296c pocket` R2 chain + live `## callee arg question` reads); pair selects HANDLER entries, does NOT point into the head; CODE-vs-TABLE conflict resolved to TABLE with flow evidence |
| R3 | `29bc..2a59` (158 B, 49 insns) | head `93 XCHG AX,BX`, `58 POP AX`; saves `8c2e660d 8c26640d 66a36c0d 668936680d 8bcc 66a1c808 66a34e0d a0cc08 8a26cf08 a3520d a1d208 a3560d a0d408 a2580d c706fe088000 c6061d0989 8926780d 8926b00d 668b36340d a1700d a35e0d 33c0 8ee0 8ee8 b80cde cd67 8be1 b92000 8ed9 f606470020 7513` (`2a24` JNZ→`2a39`, `2a26+0x13`, `4609−1bd0`✓); leg-1 `66a16c0d 668b36680d 8e26600d 8e2e620d ffe3` (`2a37 JMP BX` — DYNAMIC exit, BX runtime-valued); leg-2 `668b36e40d b83800 8ee0 6697 c43ee80d 660fb7ff fc 676664a5 676664a5 6697 ebcc` (`2a58` JMP→`2a26`, `2a5a−0x34`, `45f6−1bd0`✓ — internal back-edge) | CODE (island B) | entry: `29bc` NONE static (reachability table; note `29bc` = the arg numeral — pure coincidence, the stores of `0x29bc` are frame-slot, rejected slice-17); exits: `2a37 JMP BX` dynamic, `2a16 INT 0x67` IVT-mediated; back-edge `2a58→2a26` internal. Cell contacts (cite-only): `[0xd34]`, `[0xd4e..0xd58]`, `[0xd5e]`, `[0xd60..0xd6c]` (= H13 restore pair `2ac4/2ac9` twins + pocket `2824/2828`), `[0xd70]`, `[0xd78]`, `[0xdb0]`, `[0xde4]`, `[0xde8]`, `[0x8c8..0x8d4]`, `[0x8fe]`, `[0x91d]`, `[0x47]` |

Tiling check (mandated): `64 + 4 + 158 = 226` = `0x2a59 − 0x2978 + 1` ✓;
every byte classified exactly once. Emission cross-check: 73 insns =
22 (R1) + 2 (R2 artifact) + 49 (R3) ✓; bytes `64+4+158 = 226` = the
window's `bytes_disassembled` ✓. PADDING: none — no zero-fill run
(contrast pocket R1); the only `00` bytes are embedded operands (`8000`
imm@`29ef`, `0d` disp-hi bytes). UNKNOWN/skip: none — zero decode-skips
this window. The `29b8..29bb` region was checked against the CODE
stream claim: the overlap IS the conflict (stream emits those 4 bytes
as 2 artifacts); resolved in Step 3 by flow evidence, not by fiat —
see reachability table row "determinability pair source".

### Reachability (Step 3a) — `| entry candidate | evidence | verdict |`

Sweep scope: `search_instructions` over the 14638 defined instructions
(uniform at this-slice time), every hit recomputed `−0x1bd0` (code
space renders) / `−0x9910` (overlay bank `1991:` renders); far renders
`11bd:`-form probed separately (0). Head range in render space:
`1000:4548..4629`.

| entry candidate | evidence | verdict |
|-----------------|----------|---------|
| fall-through from `296d` | `## 296d hook target` row "exit (tail JMP, cited)" (`JMP 0x1000:1e85` @`2975`, ends `2977`, unconditional) + live `read_memory(2975,3)` → `e93dd9` ✓ + owner row `restore_fs_gs_and_resume` body `296d..2977` | REFUTED — no fallthrough path into `2978` |
| defined-insn `CALL`/`JMP`/`Jcc` target in `2978..2a59`, render family `0x1000:45` | 4 hits (`## paging block` era state was 0 — NEW post-carve class): `291b 742d`→`451a`= `294a`, `2922 7433`→`4527`= `2957`, `2945 e2fe`→`4515`= `2945` (self), `2951 eb00`→`4523`= `2953` — all `FUN_11bd_2864` rel8 legs (pocket) | OUT ×4 — every recomputed target `< 2978`; ZERO in-range |
| same, family `0x1000:46` | 8 hits (identical to slice-17's row, owners/bits re-verified live): `2ae2`→`46b8`=`2ae8`, `2b02`→`46da`=`2b0a`, `2b3e`→`46f8`=`2b28`, `2b45`→`46f0`=`2b20`, `2b68`→`46bb`=`2aeb`, `2bc9`→`46e1`=`2b11`, `7744`/`792b`→`46ab`=`2adb` | OUT ×8 — all ≥ `2aeb` (after the handler); head sub-window `4600..4629` (`2a30..2a59`): ZERO hits |
| same, families `0x1000:29` / `0x1000:2a` / `11bd:29` / `11bd:2a` | 12 / 7 / 0 / 0 — REPRODUCED from slice-17 byte-for-byte; the 12 recompute to `11bd:0d56..0dd3`, the 7 all → `2ac4`−`1bd0` = `0ef4` (`FUN_11bd_0ef4`) | OUT ×19; segment-forms negative ×2 |
| direct candidate-entry operands | `0x2978`: 0 hits (slice-17 row reproduced); `0x2a30` (slice-17 candidate): 0; `0x2a59`: 0 | NEGATIVE ×3 — no defined reference to any head boundary/candidate address |
| cell `[0x9c2]` (armed vector) | operand `0x9c2` = 2: `02b1 ff26c209` READ + `41ee c706c2096d29` WRITE (value `0x296d`); superset `9c2` = 2 (no extension) — slice-14/17/22 writer set {`41ee`} CONFIRMED unchanged | `0x296d < 0x2978` — OUT; arms the restore stub, NOT the head |
| cell `[0x9bc]`/`[0x9be]` (per-arg vector) | controls REPRODUCED exactly: `0x9bc` 2 / `9bc` 6 / `0x9be` 2 / `9be` 4 (full false-string set — slice-21's reconciled count; single runs this pass, no batch artifact); per-arg pair words = determinability appendix `## 2811..296c pocket` table (14 rows quoted there, in-map) | the `0x29bc` arm's values `0x2a5a`/`0x2a60` land INSIDE `clear_msw_and_callfar` (owned, boundary `2a5a..2ad8`) — NOT the head; other 13 arms' values all `< 2978` (037d..0905 band) or pocket `2820/2822` |
| determinability pair-source words resolving INTO `2978..2a59` | re-scan of all 28 arm cells `arg−4`/`arg−2` (arithmetic from the appendix rows): `037d/037f`, `08d6/08d8`, `2820/2822`, `03a3/03a5`, `0716/0718`, `08ae/08b0`, `0745/0747`, `0901/0903`, `29b8/29ba`, `0675/0677`, `08a8/08aa`, `03d2/03d4`, `045e/0460`, `04f3/04f5` — only `0x29bc−4 = 0x29b8` ≥ `2978` (and `29ba/29bc…`: cells `29b8..29bb` ⊆ head ✓) | EXACTLY ONE in-head contact: the R2 TABLE run. It is a DATA source (read by `6270/6277`), not a transfer target — its values leave the head (`2a5a/2a60` in-handler). The `0x29bc` numeral itself: only `2f18`/`44ab` frame-slot stores (`0x29` family re-run, 4 hits identical to slice-17) → rejected as entry writer per slice-17; `[0x9ba]` chain terminal per slice-20 NONE-FROM-DISCIPLINE |
| far-ret return frames (stored IPs) | handler-pair stores per slice-22 walk tables: `0x443/0x4d3/0x665/0x6ca/0x7b9/0xb6e/0xa35/0xa86/0xb14/0xb3f` + H13 `PUSH 0x2ac4`@`2aaa` (in the `0x2a` family re-run, 18 hits = slice-17's 17 + this new handler row) | none in `2978..2a59` — `2ac4` is inside the handler body; ZERO ret-frame entries into the head |
| internal head edges (self vs external) | stream edges enumerated: `297d→29b5`, `2994→29b3`, `29b5→0337` (external), `2a24→2a39`, `2a37→BX` (dynamic), `2a58→2a26` (internal back-edge) — every in-head Jcc/JMP accounted | islands reachable only from their own heads (`2978`, `29bc`) or dynamically; `29b8` NOT flow-reachable (unconditional JMP exits at `29b5`) — supports R2 TABLE |
| residual dynamic holes (legs (i)/(ii)/(iii) inherited) | (i) fall-in from pocket: pocket now defined (slice-21/22) and the `0x1000:45` family proves no pocket edge targets the head — except the 7 never-disassembled skip bytes `28e7..28eb`/`28ed` (defined-instr blindness); (ii) runtime-installed pointers ([0x9c0]-NOT-IN-EXE precedent); (iii) `CS:[0x2ad9]` tail cell: not an entry (data, row `1000:46a9..46aa`) | head entry verdict: ZERO static entries — DYNAMIC-ONLY (or never). The three slice-17 legs persist in dynamic form only |

### `[0xdfe]` sweep (Step 3b) — contact table + window arithmetic + controls

`| pattern run | match_count | hits + classification |` (all `search_instructions`
runs `truncated:false`, `instructions_scanned:14638` unless stated):

| pattern run | match_count | hits + classification |
|-------------|-------------|------------------------|
| operand `0xdfe` | 2 | `2892 c606fe0d00` `MOV byte ptr [0xdfe],0x0` WRITE; `28a0 c606fe0d01` `MOV byte ptr [0xdfe],0x1` WRITE — both in `FUN_11bd_2864` (pocket `[0x2e]`-switch stream). ZERO defined READS. The head reader `2978` (`803efe0d01`) is UNDEFINED-listing → invisible by construction (cite: raw read + dry-run emit rows above); `2978` IS a reader of `[0xdfe]` (class READ — conditional gate, the flag test before CR0 PG-set) |
| operand `dfe` (superset) | 3 | above 2 + 1 FALSE-STRING: `1991:34e5 7507 JNZ 0x1000:cdfe` — overlay-bank near target, recompute `34e5+2+0x07 = 34ee` ↔ `cdfe−9910 = 34ee` ✓ same-bank CODE target, not the cell |
| operand `[0xdfe]` (bracket form) | 2 | identical (render-form probe) |
| operand `DS:[0xdfe]` / `CS:[0xdfe]` / `ES:[0xdfe]` / `SS:[0xdfe]` | 0 / 0 / 0 / 0 | negative ×4 — no override render of the cell exists |
| byte pattern `803efe0d` | 1 | `11bd:2978` only — the CMP-byte form is program-unique (the head itself; undefined bytes ARE scanned by the raw pattern channel) |
| byte pattern `3efe0d` | 1 | `11bd:2979` only (offset+1 = the `3e fe 0d` window on the same instruction — consistent single occurrence) |
| byte pattern `fe0d` (raw, caution) | 4 | `1132` FALSE-POSITIVE (context `1128 e843fe0b e83ffe0c e83bfe0d e837fe0e` — `1132..1133` = rel16 displacement `fe3b` of `1130 CALL`: `1134−0x1c5 = 0f6f`, orphan-band defined bytes not in a body (`get_function_by_address(1132)` → error), branch displacement bytes — not a data contact); `2894` = disp16 field of `2892` WRITE ✓; `28a2` = disp16 field of `28a0` WRITE ✓; `297a` = disp16 field of `2978` READ ✓ — partition reconciles 1:1 against the operand/pattern runs above; hint-partition `fe0d`@`3059` NOT reproduced (all three space-readings probed negative, ¶ above) |
| family control `[0xd` (absolute `0xd00..0xdf` window) | 95 | `[0xdfe]` contributes exactly 2 (both WRITEs) — READS zero; adjacency live: `[0xdfc]` `28a5 STR word ptr [0xdfc]` (pocket, TR store — feeds the head's `2999` BX load), `[0xdff]` ×12 (fix-wave 1 — count corrected from the original ×9, live-bracket run `[0xdff]` → `match_count:12`, `truncated:false`: CMPs `2afd`/`786b`/`78d1`/`794f`/`796f`/`7b7d`/`7bcf`/`7c6f`/`1991:593d`, `77ef SETA SS:`, writes `7836`/`7875`) — other-cell, cite-only; the `[0xd60..0xd6c]`/`[0xd5e]`/`[0xd70]` cluster rows appear under both pocket/H13/head owners (head-internal contacts listed in R3 row) |

Constant-base census at this-slice time (supersedes slice-20's
14006-era counts; window arithmetic re-derived against the SAME
constant sets — slice-20's Base→window ledger structure reused, values
re-verified live): `MOV BX,0x…` 46 (was 33; +13 = slice-22-created band
preludes `0347/0410/0493/05b1/0699/0771/07e9/093a/099d/09d9/0a60/0aa1/
0ae4` all `bb0010 MOV BX,0x1000` + island `033c` — no NEW value in
window-reach position), `MOV SI,0x…` 62 (was 59; +3: `0340 SI=0x467`,
`0354 SI=0x3e0`, `096a SI=0x978`), `MOV DI,0x…` 35 (was 34; +1: `033d
DI=0xf56`), `MOV BP,0x…` 1 (identical: `1000:0000 BP=0x1` stub). No
constant equals `0xdfe/0xdff`; reach requires `base+disp ≡ 0xdfe..0xdff`:

| base (census value) | required disp for `0xdfe` | site present? | verdict |
|---------------------|---------------------------|---------------|---------|
| `0x98e` (`6a97`) | `+0x470` | `[BX + 0` 320-hit list: no `0x470` disp | REJECT (vacuous-owner basis: slice-20 body-scan `6a68..6aab` zero sites) |
| `0xf7d` (`7684`) | `−0x17f` (`3965−383 = 3582` ✓; as disp16 renders `+0xfe81`) | targeted probes `[BX + -0x17f` → 0 AND `[BX + 0xfe81` → 0; the complete `[BX + -` 10-hit list carries only `−0x1..−0x4` | REJECT (owner sites `7687/768e` are `−0x3/−0x2` → `0xf7a/0xf7b` ∉) — fix-wave 1: printed value was `+0x81`, a slip; corrected, verdict stands |
| `0xd12` (`2804`) | `+0xec` | absent | REJECT |
| `0x1000` ×14 (band preludes/islands) | `−0x202` | `[BX + -` full 10-hit list: no `−0x202` | REJECT ×14 (band-handler `[BX+disp]` sites: none in the 10; their cell ops are absolute) |
| `0x2824` (`626d`) | `−0x1a26` (`10276−6694 = 3582` ✓; as disp16 renders `+0xe5da`) | targeted probes `[BX + -0x1a26` → 0 AND `[BX + 0xe5da` → 0; absent from the complete negative lists | REJECT (sites `6270/6277` → `0x2820/0x2822` ∉ — the known mirror rows) — fix-wave 1: printed value was `−0x1a46`, digit transposition; corrected, verdict stands |
| `0x11e4` (`3465/363b/36a4`) | `−0x3e6` | absent | REJECT (sites `+0x2..+0x26` → `0x11e6..0x120a` ∉) |
| `0x2d0a` (`7697`) | `−0x1f0c` | absent | REJECT (sites → `0x2d07/0x2d08` ∉) |
| `0xe000/0xf000/0xfffe/0xffff/0xe822` (`0d7c/3f13/3f18/5ff7/1e6b`) | `+0x2dfe/+0x1dfe/+0xe00/+0xdff/+0x25dc` | all absent in the positive envelope + `[BX + -` list; fix-wave targeted probes on the `0xe822` value: `[BX + 0x25dc` → 0 AND `[BX + -0xda24` → 0 — recompute CONFIRMS `+0x25dc` (`0xe822 + 0x25dc = 0x10Dfe ≡ 0xdfe (mod 0x10000)` ✓; the review-suggested `+0xf5dc`/`−0xa24` gives `0xDE22` — refuted, original value stands) | REJECT ×5 |
| small constants `{0,1,2,3,4,5,8,0xa,0xb,0x10,0x40,0x200}` (census owners `12a1/14a4/1df7/23cf/3bea/5e9e/62ce/63c4/6995/69ac/6afe/6e4f/707f/711f/7730/78ec/795c/79ff/7a23/1991:2034/1991:3897` — fix-wave 1: `6e4f MOV BX,0x200` (`FUN_11bd_6e20`) added, omitted from the original owner list; its required disp is the already-printed `+0xbfe`, not `+0xdbe` which belongs to the `0x40` owners `711f/795c`) | `0xdfe−b ∈ {0xdee,0xdf3,0xdf4,0xdf6,0xdf9..0xdfd,0xdbe,0xbfe,…}` | NONE present in any partition list (no `+0xd??`/`+0xdf?`/`+0xbfe` render among the 320/68/70 positives; the `dfe`-substring runs already cover any `0xdfe` render; fix-wave targeted probe `[BX + 0xbfe` → 0) | REJECT per-value ×21 |
| SI/DI/BP constants (62/35/1 values, full list quoted in the census runs above) | required disps `{0x997,0xa1e,0x486,0xdde,0xdf9,0x536,0xdf6,0xd86,0xb2e5,0xdfd,0xdfe(ESI 0xf000 half),0x6ffe,0x7fe,0x9fe,0x8bfe,0xdee,0xdf4,0xdfb,0xdf3,0xdfc,0xdf5,0xdf8,0xddc,0xdc7,0xdb6,0xd66,0x4c6,0x4be,0xae8,0xa6bd,0x8dfe,0x382,0xa101,0x65e,0x99b4,0xc62,…}` and DI `{0xe72…0xdce,0x1de,0x215,0x203,0x3d2,0x66,0xd9e,0x53e,…}` and BP `0xdfd` | per-site check against the live partition lists (`[SI + 0` 68 hits — disps `0x0..0x68`; `[DI + 0` 70 — `0x0..0xc,0x1,0x3,0x66a,0x28,0x2a`; `[BP + 0` capped-500 discovery — observed `0x0..0x26`+`0xff00..0xff7e` group, targeted `[BP + 0xdf` = 0 closes the single required value): NONE match | REJECT ×all resolved; negative-side renders `[BX/SI/DI + -` (10/15/6 hits, complete lists quoted above) carry no `−(base−0xdfe)` value → REJECT |
| head-internal window site `29a2 c6470589 MOV byte [BX+5],0x89` (live re-cite `read_memory(11bd:299d,10)` → `{"data":[184,8,0,142,216,198,71,5,137,139],"hex":"b808008ed8c64705898b"}` ✓ — DS←`0x8` at `29a0` immediately before the store) | BX = `[0xdfc]` loaded `2999` under the entry DS; the value's only known writer is the pocket `28a5 STR word ptr [0xdfc]` (`0f000efc0d`, task-register selector — cited this pass in the `[0xd` family row); site linear = `0x80 + ((BX+5) mod 0x10000)` ∈ `0x80..0x1007F` | CELL SEGMENT MODEL DECIDES: DS=`0x1000` model → cell linear `0x10Dfe` > `0x1007F` — out of reach (the original rejection holds under THIS model only); DS=`0x20` model — live in-map (R3 `2a1d/2a1f` DS←`0x20` then byte-test of `[0x47]`, H13 `ES←0x20` restore of the `[0xd64]/[0xd66]` pair) → cell linear `0x2Dfe`, which IS inside `0x80..0x1007F`: hit ⟺ `BX+5 ≡ 0x2DFE−0x80 = 0x2D7E`, i.e. `BX = 0x2D79` (reviewer arithmetic ✓ reproduced); offset-wrap hit under the `0x1000` model would need `BX ≡ 0xD79` | **RECLASSIFIED (fix-wave 1) → BX-dependent OPEN-WINDOW, NOT REJECT.** Of the two reviewer-sanctioned remedies the reclassification is chosen because it is the honest enumeration posture (never silently reject): the selector-alignment argument (`0x2D79` = descriptor index `0x5AF`, RPL `1` — an LDT spanning `0x2D78` bytes — not a plausible STR result) establishes implausibility only, not static impossibility — the runtime value is unbounded. Downstream conclusions unchanged either way: the site is head-internal, byte-wide, UNDEFINED listing — invisible to the defined-insn sweep, no sweep negative binds it. The original "REJECT by segment arithmetic, BX-independent" wording is WITHDRAWN as over-generalizing one model (same failure class as slice-22's H8 arrow — false arithmetic justification inside otherwise-correct text) |

Reader/writer closure: `[0xdfe]` defined WRITEs `{2892, 28a0}` (sole
static writer set this pass — the pocket flag-legs: write-0 / write-1
inside `FUN_11bd_2864`'s `[0x2e]` switch stream, owners cited); defined
READs `∅`; the sole known READ is the unowned head `2978` (byte-cited).
Runtime writer status: OPEN-WINDOW holes stand — dynamic DS-relative
sites (base-not-static): `2df8 PUSH [BX+0x2]` (BX=`[BP+0x6]` caller
pointer; reaches `0xdfe` iff BX=`0xdfc`), `29a2 MOV byte [BX+5],0x89`
(head-internal, DS←`0x8`, BX=`[0xdfc]` STR value — reaches the cell
linear `0x2Dfe` under the DS=`0x20` model iff BX=`0x2D79`, reclassified
from REJECT in fix-wave 1 — arithmetic row above), `09ed FNSAVE [BX]` (H9 body,
BX←`[0xf82]` runtime, 108-byte write footprint), `0227 FNSAVE [BX]`,
`4629 CMP [BX]` (parse_config — read only), `0978` orphan-junk `[BX+SI]`
class, `1e73 ADC [BX+SI]`, `0bd4/0bd8` (`[BX+SI]` pair, SI=0x1028 →
BX=`0xfd6` needed — BX uncensored), struct-cursor owners (`1ab8/1d8c/
3986/4bdd/52ef/5686/6701/4ca1/4f83/6084` families per slice-20's OPEN
ledger — form sets re-verified identical or +1 row per the pair-run
deltas), every `1000:` stub-zone base (frame/stub class), overlay-bank
dynamic bases (`1991:321b/3253/35d0/362f/3689/3879/38b2/3977/453b/3a75/
3af1` + `1991:2f65`-class ES sites) — enumerated-as-class, NOT silently
rejected; `[ES+...]`/`[SS+...]` renders are no-op/frame classes for a
DS-absolute cell (segment math, cited by the `1dfe`-row precedent
`## [0x9ba] consumers`); implicit `MOVSD` legs `2a4e/2a52` (inside the
head) touch FS:ESI/ES:EDI at runtime values `[0xde4]/`[0xde8]` — no
offset render, out of sweep visibility (flagged, head-internal).

Controls (reported, NOT relied — dead channel per slices 16/18/19/20/21):
`get_xrefs_to(11bd:0dfe)` → `{"references":[],"count":0,…,"total":0}`
despite the two defined WRITEs; `list_data_items_by_xrefs(filter=all,
type_filter=all,min_xrefs=1,limit=30)` → `{"data_items":[],"count":0,
…,"total":0}`; `get_xrefs_to(11bd:2978)` and `get_xrefs_to(11bd:29b8)` →
both `{"references":[],"count":0,"total":0}` (head-entry + pair-source
probes). Raw-pattern caution row quoted above (`fe0d`@`1132` false
positive) is the demonstration that the raw channel needs context
reconciliation.

### `0x29b8/0x29ba` role (Step 3c)

| Item | Evidence | Claim |
|------|----------|-------|
| pair bytes | `read_memory(11bd:29b8,4)` → `{"data":[90,42,96,42],"hex":"5a2a602a"}` — hex↔data reconciled ✓; matches the determinability appendix `29b8` row (`0x29bc (44ab) | 29b8/29ba 5a2a602a`) and the pocket "0x29bc lead resolution" paragraph byte-for-byte | `w0 = 0x2a5a`, `w1 = 0x2a60` |
| load chain | `## 2811..296c pocket` R2 row (cited, in-map): `626d bb2428`/`6270 2e8b47fc MOV AX,CS:[BX+-0x4]`→`6274 a3bc09 MOV [0x9bc],AX` + `6277 2e8b47fe`→`627b a3be09` — with BX = the `0x29bc` ARG (non-override path, gates `6259`/`6266` per that section's reader-closure row); `0x29bc` arrives via `44ab` slot store → `452f PUSH [BP+-0x5a]` → `6252 8b5f02` (`## callee arg question` re-derives, cited) | arg `0x29bc` arms the pair from cells `29b8/29ba` |
| target homes | `0x2a5a` = `clear_msw_and_callfar` entry (`get_function_by_address(2a5a)` owner row, this pass) — `2a5a PUSH AX` `50` boundary; `0x2a60` = the post-preamble `CLI fa` (`w1` preamble-skipping entry via stub `0934`, slice-22 H13 rows) | BOTH values land INSIDE the owned handler `2a5a..2ad8` — NOT in the head |
| role + conflict disposition | bytes sit in the head's address span (`2978 ≤ 29b8 ≤ 2a59`); the linear stream emits them as 2 artifact insns (`5a`/`2a602a`) with NO flow edge landing on `29b8` (every in-head edge enumerated in the reachability table; `29b5 JMP` unconditional before them); the same words are the data source of a live per-arg vector chain | TABLE (not CODE): the determinability pair for arg `0x29bc`, sibling of `mode_vector_source_pair@2820..2823` (same `arg−4/arg−2` shape; there the pair feeds the pocket-override targets, here the `0x29bc`-arm handler entries). The Step-2 CODE-vs-TABLE overlap is RESOLVED to TABLE with this evidence — the CODE emission past the unconditional tail-JMP is an output artifact, not a flow claim (pocket precedents: slice-22 "beyond-exit emitted, NOT walked" rows) |

### Disposition-so-far (Step 4)

Per-class counts: CODE 2 runs / 222 B (R1 `2978..29b7` 64 B, R3
`29bc..2a59` 158 B), TABLE 1 run / 4 B (R2 `29b8..29bb`), PADDING 0,
UNKNOWN 0 — tiling sum 226 ✓. Head-entry verdict: **ZERO static entries
into `2978..2a59`** re-verified in the post-carve defined set
(`instructions_scanned:14638`); R1 `2978` and R3 `29bc` are DYNAMIC-ONLY
entry candidates (or never-entered); no attributed walk was created —
the two islands' bodies are classified, not adopted (Task 2's capped
path at the cited boundaries). `[0xdfe]`: reader FOUND (the unowned head
`2978` — sole READ program-wide); defined WRITERS FOUND `2892`/`28a0`
(`FUN_11bd_2864`, flag-0/flag-1 stores) — the prior "runtime writers
zero/unfound" state is superseded at the defined-instruction layer;
runtime-window writers remain enumerated holes (OPEN-WINDOW rows +
implicit-MOVSD caveat). Pair-role claim: `0x29b8/0x29ba` = TABLE
(arg-`0x29bc` `[0x9bc]`/`[0x9be]` source pair selecting handler entries
`0x2a5a`/`0x2a60`) — CODE conflict resolved with flow evidence, do NOT
treat the run as code at create time (Task 2's never-create-over-TABLE
rule engaged). R1 tail `JMP→0337` enters the orphan-band relay `JMP CS:
[0x2fa]` — cell `[0x2fa]` writer set NOT swept this pass (one hop,
named-deferred); R3 `2a37 JMP BX` exit dynamic. Tool inventory (this
pass, all read-only): `search_instructions` ×46 (reachability cell
families 13 + pocket controls 4 + `[0xdfe]` literals 7 + family
control 1 + censuses 4 + window partitions 17; two `[BP +` discovery
runs capped 500 → class-rejected + targeted `[BP + 0xdf` closure,
slice-20 method), `search_byte_patterns` ×3, `read_memory` ×11 (11/11
internally reconciled; three-way with emission on the head chunks),
`disassemble_bytes` ×3 ALL `dry_run=true` (226 B head window, `2de0..
2e48` context probe, `0337..033b` landing probe), `get_function_by_address`
×8 (4 edges + `0337` + `2a30` + `1132` + `1991:3059`), `find_code_gaps`
×1 (total 149), `get_xrefs_to` ×3 (0 each), `list_data_items_by_xrefs`
×1 (empty). NO create/rename/comment/define/save; no transaction
opened; `fifa96.rep` churn left unstaged; `/media/felipe/FIFAPCCD/`
untouched.

### Fix wave 1 (review 2026-09-29 — text-only, zero Ghidra writes beyond reads)

Live verification this wave (all new read calls, before any edit):
targeted `search_instructions` disp probes ×7 — `[BX + -0x17f` → 0,
`[BX + 0xfe81` → 0, `[BX + -0x1a26` → 0, `[BX + 0xe5da` → 0,
`[BX + -0xda24` → 0, `[BX + 0x25dc` → 0, `[BX + 0xbfe` → 0 (every
`match_count:0`, `truncated:false`, `instructions_scanned:14638`);
bracket re-run `[0xdff]` → `match_count:12` (`2afd`/`77ef`/`7836`/`786b`/
`7875`/`78d1`/`794f`/`796f`/`7b7d`/`7bcf`/`7c6f`/`1991:593d`, full
membership quoted in the family-control row);
`read_memory(11bd:299d,10)` → `{"data":[184,8,0,142,216,198,71,5,137,
139],"hex":"b808008ed8c64705898b"}` ✓ internally reconciled (confirms
DS←`0x8` at `29a0` immediately precedes the `29a2` window store).
Corrections applied (this section only; prior sections untouched):
(1) IMPORTANT — the `29a2` arithmetic row: original "max `0x80+0xFFFF =
0x1007F < 0x10Dfe`, BX-independent" rejection holds ONLY under the
DS=`0x1000` cell model; under the map-live DS=`0x20` model (`2a1d/2a1f`,
H13 `ES←0x20` restore pair) the cell linear `0x2Dfe` IS inside the site
reach and the hit condition is `BX = 0x2DFE−0x80−5 = 0x2D79` (reviewer
math reproduced). Row RECLASSIFIED to BX-dependent OPEN-WINDOW and added
to the reader/writer-closure hole list; the selector-alignment argument
(`0x2D79` → descriptor index `0x5AF`/RPL `1`) is kept as plausibility
framing only — reclassification (not alignment-rejection) chosen because
the runtime selector value is statically unbounded ("never silently
reject" discipline). Downstream conclusions unchanged: head-internal,
byte-wide, invisible to the defined-insn layer; no sweep negative
depended on it. Failure class recorded: false arithmetic justification
inside otherwise-correct text (same class as slice-22's H8 arrow).
(2) Census cells — `0xf7d` required disp corrected `+0x81` → `−0x17f`
(alt render `+0xfe81`; both targeted-probed 0, verdict REJECT stands);
`0x2824` corrected `−0x1a46` → `−0x1a26` (alt `+0xe5da`; probed 0,
verdict stands); `0xe822` — recomputed: the printed `+0x25dc` was
ALREADY CORRECT (`0xe822+0x25dc = 0x10Dfe ≡ 0xdfe (mod 0x10000)`), the
review-suggested `+0xf5dc`/`−0xa24` is refuted (`0xe822+0xf5dc − 0x10000
= 0xDE22 ≠ 0xdfe`) — row keeps its value, targeted-probe cites added.
(3) `[0xdff]` adjacency count ×9 → ×12 with full membership quoted
(the original parenthetical both undercounted the value and enumerated
inconsistently).
(4) Small-constant owner list gained the omitted `6e4f MOV BX,0x200`
(`FUN_11bd_6e20`); its required disp is the row's already-printed
`+0xbfe` (targeted probe 0; per-value count 20 → 21) — the review's
`+0xdbe` attribution is the `0x40` owners' (`711f/795c`) value, not
`6e4f`'s.
Program state unchanged beyond the read probes; no listing mutation;
`fifa96.rep` churn left unstaged.

### Writes (Task 2 — capped creates + TABLE define, executed 2026-09-29, program `/fifa96.exe`)

Before-state (verbatim, captured before the first mutation):
`get_function_by_address` → `{"error":"No function found for 11bd:2978"}`,
same form for `11bd:29bc` / `11bd:29b8` / `11bd:0337`;
`get_function_count` → `{"function_count":315,"program":"fifa96.exe"}`;
head gap row byte-identical to Task 1's quote (`1000:4548..1000:4629`,
size 226, `has_undefined_bytes:true`);
`audit_global(11bd:29b8)` pre → `{"name":"","type":"","length":0,"plate_comment":"","xref_count":0,"issues":["generic_name","untyped","missing_plate_comment"],"severity_summary":{"hard":3,"medium":0,"soft":0},"fully_documented":false}`;
`ushort[2]` type exists (slice-21, `mode_vector_source_pair` template).
No STOP-BLOCKED fired: no create landed on a defined byte and the
CODE-vs-TABLE conflict never engaged — neither create's flow reaches
`29b8..29bb` (R1 exits at the unconditional tail-JMP `29b5`; R3 starts
`29bc`), and the apply conflict-check ran against the POST-create
state (dry-run row below).

Real disassembly (2 calls, cited ranges, emission byte-identical to the
Task-1 dry-run rows): `disassemble_bytes(11bd:2978, len 64)` →
`{"success":true,"start_address":"11bd:2978","end_address":"11bd:29b6",
"bytes_disassembled":63,"instructions_total":22,"truncated":false}` —
envelope 63/`29b6` quoted-as-returned lag (last insn `29b5` len 3 covers
`29b7`; slice-21 envelope-lag precedent); `disassemble_bytes(11bd:29bc,
len 158)` → `{"success":true,…,"end_address":"11bd:2a58",
"bytes_disassembled":157,"instructions_total":49,…}` — same lag class
(`2a58` len 2 covers `2a59`).

Creates (order R1 → R3; every response verbatim):

| create cmd | verbatim response |
|------------|--------------------|
| `create_function(11bd:2978)` | `{"success":true,"address":"11bd:2978","function_name":"FUN_11bd_2978","entry_point":"11bd:2978","body_size":69,"message":"Function created successfully at 11bd:2978"}` |
| `create_function(11bd:29bc)` | `{"success":true,"address":"11bd:29bc","function_name":"FUN_11bd_29bc","entry_point":"11bd:29bc","body_size":158,"message":"Function created successfully at 11bd:29bc"}` |

Post-bounds vs proposal (all `get_function_by_address` verbatim
read-backs): R1 → `{"name":"enable_paging_and_load_tss","entry_point":"11bd:2978","body_start":"11bd:2978","body_end":"11bd:29b7"}` — MATCH
proposal `[2978..29b7]` (call-time envelope `body_size:69` = 64 B body +
the 5 B relay block `0337..033b` transiently flow-attached; final bounds
per read-back — slice-14 `body_size:13` precedent, quoted not fought);
R3 → `{"entry_point":"11bd:29bc","body_start":"11bd:29bc","body_end":"11bd:2a59"}` — MATCH proposal `[29bc..2a59]`; probes
`(11bd:2a58)`/`(11bd:2a59)` resolve in-body ✓. Zero nudges: both
creates succeeded first-call; `disassemble_first=false` never used — cap
compliance. `get_function_callees`: `enable_paging_and_load_tss` →
`{"callees":[{"name":"caseD_0","address":"11bd:0337"}],"total":1}` (the
tail-JMP edge — see side-effects (1)); `FUN_11bd_29bc` →
`{"callees":[],"total":0}` — `INT 0x67` (IVT) + `JMP BX` (dynamic)
record nothing static, matching the Task-1 walk.

Name + plate (Step 1 disposition): `rename_function(FUN_11bd_2978 →
enable_paging_and_load_tss)` → `{"status":"success","message":"Success:
Renamed function at FUN_11bd_2978 from 'FUN_11bd_2978' to
'enable_paging_and_load_tss'","warnings":["…not PascalCase…","…contains
underscores…"]}` (the two style warnings quoted-as-returned; snake_case
kept per repo convention — slice-21/22 precedent). Naming bar: entry leg
= DYNAMIC-ONLY (Task-1 reachability zero-static-entries, cited); role leg
holds on IN-BODY cites: `0f20c0 MOV EAX,CR0` + `660d00000080 OR
EAX,0x80000000` + `0f22c0 MOV CR0,EAX` (CR0-class cite per the
preflight's mode-word bar) + `0f00d8 LTR` + `0f06 CLTS` + descriptor
type-byte store `c64705 89`. `set_comment(11bd:2978, plate)` →
`{"status":"success","message":"Set plate comment at 11bd:2978","warnings":[…missing Algorithm/Parameters/Returns…]}`;
plate text `C: none — behavioral (block-head island, dynamic-only entry
(zero static entries per slice-23 sweep): gate CMP byte [0xdfe],0x1
(JNZ skips enable); PUSH EAX; MOV EAX,CR0; OR EAX,0x80000000; MOV
CR0,EAX (set PG); test [0xdfc]; BX←[0xdfc] (value written by pocket
STR@28a5), DS←0x8, descriptor type byte store MOV byte [BX+5],0x89,
DS←0x20, POP BX; LTR AX; CLTS; POP EAX; unconditional tail JMP
0x1000:1f07 (=11bd:0337) into cell [0x2fa] relay (one hop, not
followed); TSS/LDT descriptor interpretation of the patched byte
DEFERRED — opcode-level cites only)` (full text read back via
`get_comment` ✓ verbatim match). R3 (`29bc..2a59`) KEEPS DEFAULT NAME:
NOT-CONFIRMED-at-name — the role leg needs the `INT 0x67` IVT-handler
identity, the `JMP BX` target, and the `[0xd34]/[0xd4e..0xd58]/[0xd5e]/
[0xd60..0xd6c]/[0xd70]/[0xd78]/[0xdb0]/[0xde4]/[0xde8]/[0x8c8..0x8d4]/
[0x8fe]/[0x91d]/[0x47]` cluster consumers, all one hop out; no rename,
no plate per the write rule.

TABLE define (controller path per the `mode_vector_source_pair@2820`
template, post-creates = the conflict-checked state): (1)
`apply_data_type(11bd:29b8, ushort[2], dry_run=true)` → `{"dry_run":true,
"status":"success","message":"Successfully applied data type 'ushort[2]'
at 11bd:29b8 (size: 4 bytes)","size":4}` — clean at both edges;
(2) real apply → `{"status":"success","message":"Successfully applied
data type 'ushort[2]' at 11bd:29b8 (size: 4 bytes)"}`; (3)
`create_label(11bd:29b8, mode_29bc_source_pair)` → `{"status":"success",
"message":"Created label 'mode_29bc_source_pair' at address 11bd:29b8"}`
(name = the brief's role wording: near-offset pair for mode-`0x29bc`
args; g_+Hungarian gate conflicts as on the template row — mandated
wording kept, precedent recorded); (4)
`set_comment(11bd:29b8, plate)` → `{"status":"success","message":"Set
plate comment at 11bd:29b8","warnings":[…missing sections…]}`, text
`C: none — behavioral (near-offset pair words for mode-0x29bc args:
[0x9bc]/[0x9be] source pair cells 29b8/29ba read by publish_mode_vector
CS-window chain 6270 (BX-4)/6277 (BX-2) feeding stores 6274/627b;
values 0x2a5a/0x2a60 land on clear_msw_and_callfar entry ops; sibling
of mode_vector_source_pair at 2820, same arg-4/arg-2 shape)`.
Verification: `audit_global` post → `{"name":"mode_29bc_source_pair",
"type":"ushort[2]","length":4,"xref_count":0,"issues":["name_missing_g_prefix",
"plate_line_too_long"],"severity_summary":{"hard":1,"medium":0,"soft":1}}`
— the blank pre-state's 3 hard issues cleared to the same naming-convention
pair the template carries; `analyze_global_completeness` → `{"score":77.0,
"effective_score":80.0,"band":"COMPLETE_80","missing":["name"]}` —
template band reproduced; bytes read-back `inspect_memory_content(11bd:29b8,8)`
→ hex_dump `5A 2A 60 2A 93 58 8C 2E` ✓ = the pair words unchanged +
`FUN_11bd_29bc` head bytes (the tool's `is_likely_string:true` is a
printable-ratio heuristic artifact, not a claim).

Save (with archive-repair disclosure): the first two `save_program`
attempts and one `save_all_programs` returned `{"error":"/udf_
7f0019cf94341022393885 already exists."}` — a stale `ProgramUserData`
domain entry in the project's USER storage area (`fifa96.rep/user/`
index + `00/00000000.prp` both mtime Sep 28 21:26, from a prior-session
interrupted save; the open program's FileData treats the user file as
new, so every createFile collides). Repair (filesystem only — NO
Ghidra write): the four stale files backed up to
`/tmp/opencode/fifa96-rep-backup/`; `user/~index.dat` rewritten to the
empty form; orphan `user/00/00000000.prp` + stale `user/~journal.bak`
removed → retry `save_program` → `{"success":true,"program":"fifa96.exe",
"message":"Program saved successfully"}`. Persistence verified: entry DB
committed `db.25.gbf` (+32,768 B over `db.24`) at 20:42; user area
rebuilds consistent (empty — this slice created no bookmarks/user
metadata). Note: the FAILED first attempt had already committed its
entry DB (`db.24`) — the analyzer sweep below ran during the save
sequence and the final save captured the complete state; nothing was
rolled back.

Side-effect disclosures (analyzer-created, RATIFIED + FLAGGED per the
slice-17/21/22 precedent — quoted as returned, not fought, not reverted
(reverting would be further writes beyond the cited boundaries)):
(1) create-flow: `caseD_0` at `11bd:0337` — relay stub
(`2eff26fa02` = `JMP word ptr CS:[0x2fa]`, `0337..033b`, listed
`isThunk:true`, same-name collision with the stub-zone `caseD_0` kept
as created); became `enable_paging_and_load_tss`'s sole callee (edge
cite above); the covering gap row shrank `1000:1ea4..1f0b` (104) →
`1000:1ea4..1f06` (99), arithmetic `99 + 5 = 104` ✓. (2) SAVE-TIME
AUTO-ANALYSIS SWEEP: function count moved 318 → **329** (+11) across
the save attempts — **3 new `11bd:` functions + 8 new `1991:`
(overlay-bank) functions, every one attributable**; my first ledger said
"3 + 8 unattributable candidates" because the sweep's overlay half sits
beyond the page-0 `find_code_gaps` fetch (total 149/151, limit 100)
both Task passes used — misattribution corrected by the fix-wave 1
replay (fix-wave 1). The `11bd` half: `FUN_11bd_0ad5` (`0ad5..0ae1`) —
**split out of `FUN_11bd_0a9f`**, whose live re-read body is now
`0a9f..0ad2` (slice-22's H11 record said `0a9f..0ae1` "resolved UNDER
this body" — the prior slice's boundary claim is BREACHED by the sweep;
disclosed, next slice must ratify or re-merge; the halt-retry fragment
`0ad3..0ad4` stays defined-unowned and the row `1000:26a3..26a4`
after_function moved `0ae2` → `0ad5` — the proof the entry is new);
`FUN_11bd_0c9f` (entry `0c9f`, body `0c84..0d0b`) — carved the former
gap `1000:280c..28db` (208 = `0c3c..0d0b`) into rows
`1000:280c..2853` (72) + `1000:286b..286e` (size 4,
**`has_undefined_bytes:true`** — an UNDEFINED 4-byte hole INSIDE the
new body, not a defined island [fix-wave 1]); anomaly (fix-wave 1):
body_start `0c84` sits `0xc9f−0xc84 = 0x1B = 27 B` BEFORE
entry_point `0c9f` (= the 23 B covered lead `1000:2854..286a` + the
4 B hole), the H9 pre-entry-absorption shape (slice-22 `09d4..09d6`
precedent) — disclosed so a next-slice reviewer doesn't hit it cold;
`FUN_11bd_2d0a` (body `2d0a..2d3f`, 54 B) — carved the former gap row
`1000:4717..490f` (505) to `1000:4717..48d9` (451), arithmetic
`451 + 54 = 505` ✓. The overlay half (`1991:` entries; row addresses
in `1000:` space, physical = `1991:`+`9910`): row `1000:990e..a155`
SPLIT by `FUN_1991_0400` (entry `1991:0400` = `1000:9d10`, body
`0400..047b` = `1000:9d10..9d8b`, bounds live-verified) into
`{"1000:990e..9d0f",size:1026}` + `{"1000:9d8c..a155",size:970}`
(reassemble `1026+124+970 = 2120 = 0xa155−0x990e+1` ✓); row
`1000:de60..e657` SPLIT into three by `FUN_1991_4930` (body
`4930..495f` = `1000:e240..e26f`) + `FUN_1991_4b0a` (body
`4b0a..4b13` = `1000:e41a..e423`) into `{"1000:de60..e23f",size:992}`
+ `{"1000:e270..e419",size:426}` + `{"1000:e424..e657",size:564}`
(reassemble `992+48+426+10+564 = 2040 = 0xe657−0xde60+1` ✓), with
`FUN_1991_4542` (body `4542..454f` = `1000:de52..de5f`, live) at the
seam just below — cited as `before_function` of the first row; its own
split is not row-visible, its sweep-newness rests on count-closure +
the reviewer's live replay; row `1000:e685..e80d` SPLIT by
`FUN_1991_4e38` (body `4e38..4e8f` = `1000:e748..e79f`) into
`{"1000:e685..e747",size:195}` + `{"1000:e7a0..e80d",size:110}`; and
THREE rows VANISHED with functions created at the exact former
row-starts — `FUN_1991_21e2` (body `21e2..2281` = former row-start
`1000:baf2`), `FUN_1991_2999` (body `2999..2a0c` = `1000:c2a9`),
`FUN_1991_2b3f` (body `2b3f..2d11` = `1000:c44f`) — the current
page-1 has no rows at those spans (page steps `b860..bab2`/`babd` →
`c87e..`). Attribution closes exactly: 3 + 8 = +11. Listing-state
flips (no ownership): `1000:2235..2266` (50 = `0665..0696`, H3's
far-ret half) and `1000:2605..262d` (41 = `0a35..0a5d`, H9's) went
`has_undefined_bytes: true→false` — now DEFINED but still UNOWNED,
deferrals stand with the listing disposition moved; `1000:80cf..8118`
same flip; `1000:3011..3066` + `1000:8045..8086` orphan flags flipped
false→true.
(3) Neighbors re-read identical to pre-state ✓: `restore_fs_gs_and_resume`
`296d..2977`, `clear_msw_and_callfar` `2a5a..2ad8`, `FUN_11bd_2adb`
`2adb..2ae8`, `FUN_11bd_2b11` `2b11..2b46`, `FUN_11bd_2864`
`2864..295c`, `FUN_11bd_0ae2` `0ae2..0b11`, `FUN_11bd_0bc3`
`0bc3..0bd0`.

Post-state gap arithmetic (head): the 226 B row is GONE, replaced by —
`{"start":"1000:4588","end":"1000:458b","size":4,"has_undefined_bytes":
false,"has_orphaned_instructions":false,"before_function":"enable_paging_and_load_tss",
"before_function_address":"11bd:2978","after_function":"FUN_11bd_29bc",
"after_function_address":"11bd:29bc"}` — `0x4588−0x1bd0 = 0x29b8` ✓
(the TABLE stays ROW-LISTED as defined data exactly like the slice-21
`1000:43e1..43f3` template row); tiling `64 + 4 + 158 = 226` ✓; total
gap rows `149 → 151` (Δ+2 decomposition per fix-wave 1 replay: +1
(`990e` split) +2 (`de60` split) +1 (`e685` split) +1 (`280c` split)
−3 (`baf2`/`c2a9`/`c44f` vanished) +0 (`4717` persists, shrank — my
original "`280c`/`4717` sweep splits" attribution was wrong: `4717`
contributes 0) +0 (`1ea4` shrink) +0 (head row → TABLE row) = **+2**
✓); function count `315 → 318 → 329` = 315 + 2 cited creates + 1
create-flow (`caseD_0`) + 11 save-sweep (3 `11bd` + 8 `1991`,
breakdown and provenance in the disclosures above).

### Verdicts (Task 2)

| FUN | address | evidence | new_name | C counterpart |
|-----|---------|----------|----------|---------------|
| `enable_paging_and_load_tss` | `11bd:2978..29b7` | R1 CODE run cited by Task 1; entry leg DYNAMIC-ONLY (zero static entries — reachability table); exit leg `JMP 0x1000:1f07`@`29b5` (`e97fd9`, unconditional, cited); role leg holds on in-body CR0-class cites (`0f20c0`/`660d00000080`/`0f22c0`) + `LTR`/`CLTS` (`0f00d8`/`0f06`) + descriptor type-byte store (`c6470589`); sole callee = relay `caseD_0@0337` | `enable_paging_and_load_tss` (plate set) | none — behavioral (conditional paging-enable gate + TSS-descriptor byte patch + LTR/CLTS + tail-relay); TSS/LDT semantics and the `[0x2fa]` relay target DEFERRED |
| `FUN_11bd_29bc` | `11bd:29bc..2a59` | R3 CODE run cited by Task 1; entry leg DYNAMIC-ONLY (cited); exits: `INT 0x67` (IVT-mediated), `JMP BX` (`2a37`, dynamic), internal back-edge `2a58→2a26`; zero static callees | — (create-only) | role leg open: IVT-`0x67` handler identity, BX target, save-cluster consumers — one hop out |
| `mode_29bc_source_pair` (DATA) | `11bd:29b8..29bb` | TABLE class won the Step-2 CODE-vs-TABLE conflict on Task-1 flow evidence; `ushort[2]` = `0x2a5a`/`0x2a60`; load chain `6270/6274` + `6277/627b` cited; audit `COMPLETE_80` band | — (label + plate) | sibling of `mode_vector_source_pair@2820` |

### Deferrals (Task 2 additions)

- `[0xdfe]` runtime writers: static set {`2892`, `28a0`} (pocket
  `FUN_11bd_2864` — Task 1 found); beyond that the enumerated
  OPEN-WINDOW holes stand (incl. the head-internal `29a2`
  BX-dependent row, fix-wave 1); NOT claimed closed.
- Cell consumers one hop out: `[0x2fa]` (relay target — writer set NOT
  swept this slice), `[0xd34]`/`[0xd4e..0xd58]`/`[0xd5e]`/
  `[0xd60..0xd6c]`/`[0xd70]`/`[0xd78]`/`[0xdb0]`/`[0xde4]`/`[0xde8]`/
  `[0x8c8..0x8d4]`/`[0x8fe]`/`[0x91d]`/`[0x47]`/`[0xdfc]` (head-side
  consumers now in-body-cited, roles unresolved); `[0x9b4]`/`[0x40]`
  prior dispositions stand.
- Created-handler callee trees: `caseD_0@0337` body followed ONE edge
  only (its `CS:[0x2fa]` target not chased); `INT 0x67` IVT leg;
  `JMP BX` dynamic leg.
- Six far-return halves: ownership stays deferred; H3 `0665..0696` and
  H9 `0a35..0a5d` listing moved undefined→defined-unowned by the
  save-sweep (disclosed; no ownership claim).
- Save-sweep ownership question (corrected ledger, fix-wave 1):
  `FUN_11bd_0ad5` split vs slice-22's H11 `0a9f..0ae1` record, the new
  band FUNs (`0c9f`/`2d0a`), and the OVERLAY half — `FUN_1991_0400`,
  `4542`, `4930`, `4b0a`, `4e38`, `21e2`, `2999`, `2b3f` (bounds +
  row-delta cites in the side-effect disclosure; prior-slice records
  covering the overlay rows `1000:990e..a155`, `de60..e657`,
  `e685..e80d`, `baf2..bb91`, `c2a9..c31c`, `c44f..c621` are the ones
  silently re-cut and need next-slice ratification) — next-slice
  review scope; this slice ratified per precedent and changed nothing
  back.
- Tail `2ad9..2ada`: `CS:[0x2ad9]` live data cell untouched; row
  `1000:46a9..46aa` re-quoted unchanged.
- Islands `08c2`/`033c`/`0bc3` + pocket FUNs `2824`/`284c`/`2864`
  bodies: unchanged (re-read rows above), except the analyzer-side
  listing flips disclosed.
- Pair-bytes conflict: RESOLVED for the TABLE claim (Task 1 Step-3
  evidence); no CODE create was allowed to touch `29b8..29bb` — both
  proposals stop at the edges and the post read-backs confirm exact
  match.
- Suite: no test/tool/C changes; build + ctest green post-write
  (docs-only diff); `grep -c "block head 2978"` nonzero; prior rows
  byte-identical (fix-wave-1 rows untouched by this append);
  `/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left
  unstaged.

### Fix wave 1 (Task 2 review 2026-09-29 — docs-only, zero Ghidra writes beyond reads)

Live verification this wave (reads only, before any edit):
`get_function_by_address` ×8 on the overlay functions —
`1991:0400` → `{"body_start":"1991:0400","body_end":"1991:047b"}`,
`1991:4542` → `4542..454f`, `1991:4930` → `4930..495f`, `1991:4b0a` →
`4b0a..4b13`, `1991:4e38` → `4e38..4e8f`, `1991:21e2` → `21e2..2281`,
`1991:2999` → `2999..2a0c`, `1991:2b3f` → `2b3f..2d11` (all verbatim,
reviewer bounds reproduced exactly); `find_code_gaps` offset 100
limit 100 (total 151) — the page-0 blind spot that caused the original
misattribution: current rows `1000:990e..9d0f`/`9d8c..a155` straddle
the `0400` body, `1000:de60..e23f`/`e270..e419`/`e424..e657` straddle
`4930`/`4b0a` (with `FUN_1991_4542` as `1000:de60` row's
`before_function`), `1000:e685..e747`/`e7a0..e80d` straddle `4e38`,
and NO rows exist at the `21e2`/`2999`/`2b3f` entry spans
(`baf2`/`c2a9`/`c44f` vanished; page steps `b860..bab2`/`babd` →
`c87e..`); reassembly math `1026+124+970=2120=0xa155−0x990e+1` ✓,
`992+48+426+10+564=2040=0xe657−0xde60+1` ✓; row `1000:286b..286e`
re-checked in the saved page-0: `"has_undefined_bytes":true` ✓.
Corrections applied (this section only; prior sections untouched):
(1) CRITICAL — the side-effect ledger: "+3 proven + 8 unattributable
11bd candidates (`0bd1`/`0d15`/…)" replaced by **3 `11bd` (0ad5/0c9f/
2d0a, row-proven) + 8 `1991` overlay (0400/4542/4930/4b0a/4e38/21e2/
2999/2b3f, row-split + vanish + count-closure provenance as printed)**
= +11 exact; the Δ+2 row-count sentence re-derived truthfully (+1
`990e` +2 `de60` +1 `e685` +1 `280c` −3 vanished +0 `4717` +0 `1ea4`
+0 head→TABLE = +2; the original "`280c`/`4717`" attribution was
wrong — `4717` persists shrunk, contributes 0); the reviewer's replay
of the PRE-state overlay rows (`990e..a155`, `de60..e657`,
`e685..e80d`, `baf2..bb91`, `c2a9..c31c`, `c44f..c621`) is the prior
snapshot behind the splits/vanishes (my pages were page-0-only and
never contained them — disclosed, not silently adopted); the
deferrals bullet names the overlay half as the silently-re-cut
prior-slice records needing next-slice ratification.
(2) `1000:286b..286e` mislabel: "a 4 B defined-island row" → a 4 B
`has_undefined_bytes:true` UNDEFINED hole INSIDE `FUN_11bd_0c9f`'s
body (flag quoted).
(3) New anomaly note: `FUN_11bd_0c9f` entry-before-body —
`body_start 0c84` is `0xc9f−0xc84 = 27 B` before the entry (the
reviewer's "60 B" did not reproduce; live math printed instead:
23 B covered lead `2854..286a` + 4 B hole = 27), the slice-22 H9
pre-entry-absorption shape, disclosed for the next reviewer.
Ghidra state untouched by this wave (reads only); rep churn left
unstaged.

## sweep-aftermath ratification (verified 2026-09-29, program `/fifa96.exe`)

Read-only drift-enumeration pass over the state the two save-time auto-analysis
sweeps (slices 22/23) left in the saved program: live re-proof of every drift
item named in `## block head 2978..2a59 ### Writes` (side-effects (1)/(2), the
3+8=11 ledger, Δ+2, count 329) and the `## vector dispatch handlers` H11
record, plus the ONE decisive new question — does anything statically branch
INTO `11bd:0ad5`? Answer: YES — three defined-instruction edges, two fully
external to both H11 halves; census row (2) below carries the verbatim
authority runs. Outcome: ACCEPTED-SPLIT (zero program writes needed).
Headline live states: `get_function_count` → 329 (unchanged since slice-23);
`find_code_gaps` total 151 (FULL pagination this time — see Reads executed);
`search_instructions` scope uniform `instructions_scanned:15589` this slice
(vs slice-23's quoted 14638 — drift, census row (7)); all eight
`FUN_1991_*` bodies and all split/vanish rows reproduce exactly; the three
far-ret-half rows confirmed `has_undefined_bytes:false` + no-function at their
starts; `caseD_0` name resolves to exactly ONE function per name-keyed lookup
(the `1000:0018` instance) while three instances exist — ambiguity
demonstrated, census row (5).

### Drift census (ratified record vs live state)

Delta conventions: gap rows render in `1000:` space, `11bd:` = render −
`0x1bd0`; overlay-bank physical = `1991:` + `0x9910`. All "live state" cells
are verbatim tool responses captured THIS pass (2026-09-29, zero writes —
`disassemble_bytes` was not needed; `disassemble_function` read-only used).

(1) H11 pair + `0ad3` island:

| item | ratified record (quoted, slice-22/23) | live state (verbatim) | class |
|------|----------------------------------------|------------------------|-------|
| `FUN_11bd_0a9f` bounds — slice-22 post-bounds row, EXACT QUOTE (bounds cell) | ``{"body_start":"11bd:0a9f","body_end":"11bd:0ae1"}`` | `{"name":"FUN_11bd_0a9f",…,"entry_point":"11bd:0a9f","body_start":"11bd:0a9f","body_end":"11bd:0ad2"}` — envelope BREACHED: `0ad5..0ae1` no longer inside | SPLIT (supersedes the envelope claim; slice-23 disclosed, now re-proved) |
| same item — slice-22 post-bounds row, disposition cell key clause | "after `create_function(0a9f)` the block `0ad5..0ae1` resolves to H11" | `get_function_by_address(11bd:0ad5)` now returns an OWN body (next row) — resolution moved from H11 to the new FUN | bounds attribution changed; the disposition's FLOW facts (tail resolves as one block, entry `0ad5`) still true |
| same item — slice-22 Verdicts row, evidence cell EXACT QUOTE | "shared shutdown tail `0ad5..0ae1` (`LIDT [0x8d0]`→`INT 0xff`) resolved UNDER this body" | same live pair as rows above | verdict text REMAINS VALID (landings, tail content, LIDT-withheld rationale) — only the body-ownership clause is superseded |
| second half | (none — slice-22 had no `FUN_11bd_0ad5`) | `{"name":"FUN_11bd_0ad5","address":"11bd:0ad5","signature":"undefined FUN_11bd_0ad5(void)","entry_point":"11bd:0ad5","body_start":"11bd:0ad5","body_end":"11bd:0ae1"}` | NEW function = split partner |
| plate/comments | Verdicts H11: "`— (create-only)`" (band handlers KEEP default names + no plate per Writes(Name+plate)) | `get_comment(11bd:0a9f)` → `{"plate":null,…,"has_comment":false}`; `get_comment(11bd:0ad5)` → same no-comment form | NOT drift — no plate was ever set; consistent |
| `0ad3` retry bytes | H11 walk gate+exit row: "`ebfd JMP 0x1000:26a2` @`0ad3..0ad4` (→`0ad2`) halt-retry"; post-bounds H11: "halt-retry fragment `0ad3..0ad4` left defined-unowned (probe `{"error":"No function found for 11bd:0ad3"}`)" | `get_function_by_address(11bd:0ad3)` → `{"error":"No function found for 11bd:0ad3"}` | UNCHANGED (defined-unowned island) |
| island row | slice-22 post-carve: "`26a3..26a4` (2 = `0ad3..0ad4` orphan row)" | `{"start":"1000:26a3","end":"1000:26a4","size":2,"has_undefined_bytes":false,"has_orphaned_instructions":true,"before_function":"FUN_11bd_0a9f","before_function_address":"11bd:0a9f","after_function":"FUN_11bd_0ad5","after_function_address":"11bd:0ad5"}` — `after_function` `0ae2`→`0ad5` (the new-entry proof) re-verified | MOVED neighbor = split proof |
| `2693..26a2`-family context | (no such row — `0ac3..0ad2` was inside the `0a9f..0ae1` envelope) | absent from the 151-row dump; region covered: body_end `11bd:0ad2` = `1000:26a2` exactly (disasm last rows `11bd:0ad0 OUT 0x64,AL`, `11bd:0ad2 HLT`) — the orphan row starts at `26a3` one byte after | TILING consistent |

Both function disassemblies verbatim (read-only `disassemble_function`):
`FUN_11bd_0a9f` → 21 insns `PUSH AX`@`0a9f` … `CMP byte ptr [0xed0],0x0`@`0ac7`,
`JNZ 0x1000:26a5`@`0acc`, `MOV AL,0xfe`@`0ace`, `OUT 0x64,AL`@`0ad0`,
`HLT`@`0ad2`; `FUN_11bd_0ad5` → 3 insns `MOV word ptr [0x8d0],0x0`@`0ad5`,
`LIDT word ptr [0x8d0]`@`0adb`, `INT 0xff`@`0ae0` — byte-identical content to
the slice-22 H11 walk rows (`gate + exit 0ac7..0ad4`, `shared shutdown tail
0ad5..0ae1 = c706d0080000 / 0f011ed008 / cdff`); the tail is now its own body.

(2) THE decisive question — external in-flow into `11bd:0ad5` (authority runs;
every run `scope:program`, `truncated:false`, `instructions_scanned:15589`,
defined-instructions-only, at this-slice time):

| run (pattern) | match_count | hits + arithmetic |
|---------------|-------------|-------------------|
| `search_instructions` operand `26a5` (render family for `11bd:0ad5` = `0x26a5−0x1bd0=0xad5`) | 3 | ① `11bd:084f JMP 0x1000:26a5` bytes `e98302` (3 B) in `FUN_11bd_07e7` (`07e7..0851`): `0x0852 + 0x0283 = 0x0AD5` ✓ — H6 tail-JMP, EXTERNAL; ② `11bd:0acc JNZ 0x1000:26a5` bytes `7507` (2 B) in `FUN_11bd_0a9f`: `0x0ACE + 0x07 = 0x0AD5` ✓ — H11 gate, inter-function edge INTO the new entry; ③ `11bd:0b0b JNZ 0x1000:26a5` bytes `75c8` (2 B) in `FUN_11bd_0ae2` (`0ae2..0b11`): `0x0B0D + 0xFFC8(−0x38) = 0x0AD5` ✓ — H12 gate, EXTERNAL |
| `search_instructions` operand `0ad5` (catches `11bd:0ad5`-segment-form / bare renders) | 0 | negative — scoped "defined-instructions-only, at this-slice time" |
| operand `0x9bc` (dispatch cell, `ff26` holder) | 2 | `11bd:092d ff26bc09 JMP word ptr [0x9bc]` (`dispatch_mode_vector`) + `11bd:6274 a3bc09 MOV [0x9bc],AX` (`publish_mode_vector`) — writer cited; values = the 26 dedupe landings (slice-22 table: `040e…0ae2`, `2a5a` w0-set; no `0ad5`) |
| operand `0x9be` (w1 cell, `ff26` holder) | 2 | `11bd:0934 ff26be09 JMP word ptr [0x9be]` (`FUN_11bd_0931`) + `11bd:627b a3be09 MOV [0x9be],AX` — writer cited; w1-set `0413…0ae7`, `2a60` (no `0ad5`) |
| far-ret pair-cell IPs (writers cited in map) | — | stored/pushed return IPs per slice-22 walks: `0x443`,`0x4d3`,`0x665`,`0x6ca`,`0x7b9`,`0x97b`,`0xb6e`,`0xa35`,`0xa86`,`0xb14`,`0xb3f`,`0x2ac4` — none `0ad5`-class |
| `JMP word ptr CS:[0x2fa]` (relay read at `0337`) | — | cell writer set NOT swept (slice-23 deferral stands); listing-resolved target = `caseD_0@1000:0018` (callees row below) — no cited writer holding `0ad5`, per rule cell-holders counted ONLY with cited writers; stays OPEN-WINDOW class |
| CONTROL (quoted, not relied): `get_xrefs_to(11bd:0ad5)` | 3 | `[{"from_address":"11bd:0b0b","type":"CONDITIONAL_JUMP","from_function":"FUN_11bd_0ae2"},{"from_address":"11bd:0acc","type":"CONDITIONAL_JUMP","from_function":"FUN_11bd_0a9f"},{"from_address":"11bd:084f","type":"UNCONDITIONAL_CALL","from_function":"FUN_11bd_07e7"}]` — alive THIS time (function-entry targets do carry refs); note the `084f` ref type label `UNCONDITIONAL_CALL` while the instruction bytes are `e98302` = `JMP` (disassembly wins; quoted as returned) |

Byte re-verification (quote protocol, all internally hex↔data reconciled):
`read_memory(11bd:084f,3)` → `{"data":[233,131,2],"hex":"e98302"}` ✓
(233=`0xe9`,131=`0x83`); `read_memory(11bd:0acc,2)` →
`{"data":[117,7],"hex":"7507"}` ✓; `read_memory(11bd:0b0b,2)` →
`{"data":[117,200],"hex":"75c8"}` ✓ (200=`0xc8`).
Additional in-map candidate edges NOT counted: the H6 beyond-exit "gate chain
to `0ad5`/`088d`/`0885`" (slice-22 walk row) lives in `0852..08c1` = row
`1000:2422..2491`, live `{"size":112,"has_undefined_bytes":true,…}` —
undefined bytes, invisible to the defined-instruction authority layer by
construction (no handler re-walk per scope guard).
OUTCOME: ≥1 hit → the split is legitimate flow; two of the three edges
(`084f`, `0b0b`) originate OUTSIDE `FUN_11bd_0a9f` entirely.

(3) The 8 `FUN_1991_*` sweep-overlay bodies (bounds `get_function_by_address`
verbatim, all reproduce the slice-23 fix-wave read-backs exactly) vs the
prior-slice gap rows each re-cut (pre-state row quotes = slice-23 side-effect
ledger ¶2 + fix-wave-1 reviewer replay lines; current rows = this pass's FULL
2-page `find_code_gaps` fetch):

| FUN | live body | re-cut prior-slice row(s) (quoted) | current live row(s) | math |
|-----|-----------|-------------------------------------|---------------------|------|
| `FUN_1991_0400` | `0400..047b` | row `1000:990e..a155` SPLIT by it (ledger ¶2) | `{"start":"1000:990e","end":"1000:9d0f","size":1026,…,"after_function":"FUN_1991_0400"}` + `{"start":"1000:9d8c","end":"1000:a155","size":970,…,"before_function":"FUN_1991_0400",…}` | body `9d10..9d8b` = 124; `1026+124+970=2120=0xa155−0x990e+1` ✓ |
| `FUN_1991_4542` | `4542..454f` | own split not row-visible (ledger); novelty via count-closure | `before_function` of `1000:de60..e23f` ✓ (re-verified live) | seam `de52..de5f` |
| `FUN_1991_4930` | `4930..495f` | row `1000:de60..e657` SPLIT into three (ledger) | `1000:de60..e23f` (992) + `1000:e270..e419` (426) + `1000:e424..e657` (564) straddle it + `FUN_1991_4b0a` ✓ | `992+48+426+10+564=2040=0xe657−0xde60+1` ✓ |
| `FUN_1991_4b0a` | `4b0a..4b13` | same three-way split (ledger) | as above (`after_function` of `e270..e419`, `before` of `e424..e657`) ✓ | body 10 B ✓ |
| `FUN_1991_4e38` | `4e38..4e8f` | row `1000:e685..e80d` SPLIT (ledger) | `{"1000:e685..e747",195}` + `{"1000:e7a0..e80d",110}` ✓ | body `e748..e79f` = 88; `195+88+110=393=0xe80d−0xe685+1` ✓ |
| `FUN_1991_21e2` | `21e2..2281` | row at `1000:baf2` VANISHED (`baf2..bb91` per replay) | NO row at the span: page steps `…bab2`/`babd` → `c87e…` (full pages ✓) | body 160 = `0xbb91−0xbaf2+1` ✓ |
| `FUN_1991_2999` | `2999..2a0c` | row at `1000:c2a9` VANISHED (`c2a9..c31c`) | NO row ✓ | body 116 ✓ |
| `FUN_1991_2b3f` | `2b3f..2d11` | row at `1000:c44f` VANISHED (`c44f..c621`) | NO row ✓ | body 467 ✓ |

`1000:e270..e419` etc. neighbor fields: `de60` row `before_function
FUN_1991_4542 after_function FUN_1991_4930`; `e270` row `before 4930 after
4b0a`; `e424` row `before 4b0a after FUN_1991_4d48`; `e685` row `before 4d48
after 4e38`; `e7a0` row `before 4e38 after FUN_1991_4efe` — all verbatim from
the page-1 fetch (rows `990e/9d8c/de60/e270/e424/e685/e7a0` families,
per brief Step 2).

(4) The three defined-unowned halves (row verbatims live; `has_undefined_bytes`
+ neighbors quoted; slice-22 deferral-recorded ranges with delta arithmetic):

| live row (verbatim) | `11bd:` range (delta −`0x1bd0`) | prior record (quoted) | start probe |
|---------------------|--------------------------------|------------------------|-------------|
| `{"start":"1000:2235","end":"1000:2266","size":50,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_05af",…,"after_function":"FUN_11bd_0697",…}` | `0665..0696` (50 ✓) = H3 far-ret half `[0665..0674]` (Deferrals "Far-return blocks LEFT UNDEFINED" bullet) + arg-cell fill `0675..0696` (slice-22 inter-handler bullet) | `get_function_by_address(11bd:0665)` → `{"error":"No function found for 11bd:0665"}` — UNOWNED confirmed (entry cite `PUSH 0x665`@`05c6`) |
| `{"start":"1000:2605","end":"1000:262d","size":41,"has_undefined_bytes":false,…,"before_function":"FUN_11bd_09d7",…,"after_function":"FUN_11bd_0a5e",…}` | `0a35..0a5d` (41 ✓) = H9 far-ret half `[0a35..0a5d]` (slice-22 `ES:[0x3fc]←0xa35`) | `get_function_by_address(11bd:0a35)` → no-function error ✓ |
| `{"start":"1000:80cf","end":"1000:8118","size":74,"has_undefined_bytes":false,…,"before_function":"FUN_11bd_64b7",…,"after_function":"FUN_11bd_6549",…}` | `64ff..6548` (74 ✓) — far-ret-half family (no slice-22 own record; prior record = slice-23 ledger "`1000:80cf..8118` same flip") | `get_function_by_address(11bd:64ff)` → no-function error ✓ |

(5) `caseD_0` triple + name-resolution ambiguity demo:

| instance | live bounds/flags | evidence |
|----------|-------------------|----------|
| `1000:0018` | `{"name":"caseD_0",…,"body_start":"1000:0018","body_end":"1000:031e"}`; `list_functions_enhanced` row `{"address":"1000:0018","name":"caseD_0","isThunk":false,"isExternal":false}` | stub-zone instance |
| `11bd:0337` | `{"name":"caseD_0",…,"body_start":"11bd:0337","body_end":"11bd:033b"}`; dump row `{"address":"11bd:0337","name":"caseD_0","isThunk":true,…}` | the `2eff26fa02 JMP CS:[0x2fa]` relay (slice-23 side-effect (1)) |
| `1991:4f40` | `{"name":"caseD_0",…,"body_start":"1991:4f40","body_end":"1991:4f96"}`; dump row `isThunk:false` | overlay-bank instance; rows `1000:e81e..e84f`/`e8a7..f05f` neighbors re-verified on page 1 |

Name-keyed behavior demo (verbatim): `get_function_callers(name="caseD_0")` →
`{"callers":[{"name":"caseD_0","address":"11bd:0337"}],"count":1,…,"total":1}`
— resolves to EXACTLY ONE instance: the caller-set returned is `1000:0018`'s
(address-key control `get_function_callers(1000:0018)` → the identical list;
`get_function_callees(11bd:0337)` → `{"callees":[{"name":"caseD_0","address":"1000:0018"}],"count":1,…}`
proves the relay is a recorded CALLER of `0018`, not the query target).
Address-keyed disambiguation: `callers(11bd:0337)` →
`[{"name":"enable_paging_and_load_tss","address":"11bd:2978"}]`;
`callers(1991:4f40)` → `[]`. Name grep: `search_functions(name_pattern="caseD")`
→ `["caseD_0 @ 1000:0018","caseD_0 @ 11bd:0337","caseD_0 @ 1991:4f40"]`,
total 3 ✓. DEGRADATION DEMONSTRATED: a name-keyed query silently binds one of
three instances — the other two records are unreachable by name. Tool-behavior
note (not relied on): `search_functions_enhanced` returned `total:0` for every
caseD pattern variant (×4 runs, incl. `regex:true`) while listing the rest of
the DB — flagged for the tooling backlog, census uses
`list_functions_enhanced`/`search_functions` instead (brief named
`list_functions_enhanced` — used ✓, count field 328 vs
`get_function_count` 329, Δ1 not reconciled this pass, no claim made).

(6) `0c9f` anomaly + `286b` hole, current rows: `get_function_by_address(11bd:0c9f)`
→ `{"name":"FUN_11bd_0c9f",…,"entry_point":"11bd:0c9f","body_start":"11bd:0c84","body_end":"11bd:0d0b"}`
— entry-before-body reproduced: `0xc9f−0xc84 = 0x1B = 27 B` (23 B covered lead
`1000:2854..286a` = `0c84..0c9a` + 4 B hole). Rows:
`{"start":"1000:280c","end":"1000:2853","size":72,"has_undefined_bytes":false,"has_orphaned_instructions":false,"before_function":"FUN_11bd_0c28",…,"after_function":"FUN_11bd_0c9f",…}`
(defined-unowned remainder `0c3c..0c83` of the former 208 B row) and
`{"start":"1000:286b","end":"1000:286e","size":4,"has_undefined_bytes":true,…,"before_function":"FUN_11bd_0c28",…,"after_function":"FUN_11bd_0c9f",…}`
(4 B UNDEFINED hole `0c9b..0c9e` inside the body) — both as slice-23 fix-wave
disclosed. `FUN_11bd_2d0a` carve row `{"start":"1000:4717","end":"1000:48d9","size":451,…,"after_function":"FUN_11bd_2d0a"}`
re-verified (tiling `451+54=505` ✓).

(7) Scan-size / count drift: `get_function_count` → `{"function_count":329,
"program":"fifa96.exe"}` = slice-23 post-state (NO drift since the last save);
`search_instructions` scope now `instructions_scanned:15589` on EVERY run this
pass (uniform), vs slice-23's quoted "instructions_scanned:14638 uniform on
every run at this-slice time" → Δ+951 (drift disclosed; the +11 sweep-created
bodies + their defined neighborhoods). `[0xdfe]` operand re-render (operand
`0xdfe`, truncated:false): 3 hits — `11bd:2892 MOV byte ptr [0xdfe],0x0` WRITE
(`FUN_11bd_2864`), `11bd:28a0 MOV byte ptr [0xdfe],0x1` WRITE (`FUN_11bd_2864`),
NEW: `11bd:2978 CMP byte ptr [0xdfe],0x1` bytes `803efe0d01` — class **READ**
(conditional paging gate), now DEFINED INSIDE `enable_paging_and_load_tss`
(`2978..29b7`) — supersedes slice-23's "ZERO defined READS / reader invisible by
construction" state at the defined-instruction layer (the slice-23 row itself
already named `2978` as the program's only read, cited from raw bytes; errata
material for Task 2's `### Errata` — prior sections not edited here).

### Rulings (proposed — Task 2 executes)

| item | proposed decision | evidence row carrying it |
|------|-------------------|--------------------------|
| H11 split | **ACCEPTED-SPLIT** (legitimate flow; ZERO writes; slice-22 envelope record `[0a9f..0ae1]` superseded by the pair — name of branching instruction verbatim: `11bd:084f JMP 0x1000:26a5` (`e98302`, `FUN_11bd_07e7`) and `11bd:0b0b JNZ 0x1000:26a5` (`75c8`, `FUN_11bd_0ae2`); intra-family edge `11bd:0acc JNZ 0x1000:26a5` (`7507`) rides inside `FUN_11bd_0a9f`) | census (2) — operand `26a5` match_count 3, arithmetic reconciled both ways, bytes read back; cell-forms excluded with cited writers (`0x9bc`/`0x9be` runs, far-ret IP list); re-merge is NOT admissible (rule: ANY hit → accepted) |
| `caseD_0@11bd:0337` | **RENAME → `FUN_11bd_0337`** (hygiene, not a semantic claim per the slice's naming rule; condition "ambiguity demonstrably degrades lookups" MET: name-keyed callers query binds `1000:0018` and hides the relay + overlay instances) | census (5) — verbatim name-resolution demo vs three address-keyed controls |
| 8 `FUN_1991_*` overlays | **RATIFY status quo** (sweep-created, bounds + row math reproduce exactly; roles NOT-CONFIRMED, naming out of scope) | census (3) — bounds ×8 + split/vanish rows + reassembly sums ✓ |
| 3 defined-unowned halves | **RATIFY status quo** (defined, unowned; far-ret attribution stays deferred per one-hop rule) | census (4) — row verbatims + no-function start probes |
| `FUN_11bd_0c9f` + `1000:286b..286e` hole | **RATIFY status quo** (entry-before-body anomaly and 4 B hole are disclosed listing states; not fought, not recreated) | census (6) — live bounds + both rows |
| `0ad3..0ad4` island / `2a6c` / tail cell / `caseD_0@1000:0018` / `@1991:4f40` | **RATIFY status quo** (no ownership claim; the two non-`11bd` `caseD_0` instances keep their names — rename capped at the one map-edge-carrying instance) | census (1) last rows, (5) |
| scan drift `14638→15589`, `[0xdfe]` 3-hit re-render, `0xDE22` slip | **RATIFY-with-disclosure** (counts scoped "at this-slice time"; errata to be recorded append-only in Task 2 `### Errata` — the slip row: `0xe822+0xf5dc−0x10000 = 0xDDFE` (re-verified: `0xE822+0xF5DC = 0x1DDFE`), NOT `0xDE22` as printed in slice-23 fix wave 1; refutation STANDS since `0xDDFE ≠ 0xdfe`; the kept `+0x25dc` value re-verified `0xE822+0x25DC = 0x10DFE ≡ 0xdfe` ✓) | census (7) + slice-23 fix-wave-1 `0xe822` row (quoted there) |

### Reads executed (ZERO-WRITE branch)

`get_function_by_address` ×20 (H11 `0a9f/0ad5/0ad3`; overlays ×8; `0c9f`; half
starts `0665/0a35/64ff`; caseD triple `1000:0018/11bd:0337/1991:4f40`; edge
owners `07e7/0ae2`); `disassemble_function` ×2 (H11 pair — read-only listing
walk); `get_comment` ×2; `search_instructions` ×5 (operands `26a5`,`0ad5`,
`0x9bc`,`0x9be`,`0xdfe` — every response carries pattern + `match_count` +
`scope:program` + `truncated:false` + `instructions_scanned:15589`, all quoted
above); `get_xrefs_to` ×1 (control); `read_memory` ×3 (edge bytes, 3/3
hex↔data reconciled before quoting); `find_code_gaps` ×2 — FULL pagination
disclosed: `offset 0 limit 100` (100 rows) + `offset 100 limit 100` (51 rows),
`total:151` on both fetches, 151/151 rows consumed (the slice-23 page-0-only
blind spot explicitly avoided this pass); `get_function_callers` ×4 (name-key
1 + address-key 3); `get_function_callees` ×1; `get_function_count` ×1;
`search_functions` ×1; `search_functions_enhanced` ×4 (tool-behavior probes,
0 each — disclosed in census (5)); `list_functions_enhanced` ×1 (limit 10000,
328 rows returned). NO create/delete/rename/comment/set_global/define/
`save_program`; NO `disassemble_bytes` calls at all (nothing needed the
dry-run path); no transaction opened; pre-existing bodies re-read only;
`/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left unstaged.
Scope caveats standing: every search negative is
"defined-instructions-only, at this-slice-time (2026-09-29,
`instructions_scanned:15589`)"; intra-gap relative flow (e.g. the H6
beyond-exit chain in `1000:2422..2491`, still `has_undefined_bytes:true`) is
invisible until decoded; `get_xrefs_to` quoted-as-control (alive for
function-entry targets this time, NOT relied on for the ruling — the operand
runs are the authority).

### Writes (Task 2 — executed 2026-09-29, program `/fifa96.exe`)

Step 1 contradiction check (the ONE ruling-critical read, single repeat):
`search_instructions(operand="26a5")` → `match_count:3`,
`instructions_scanned:15589`, `truncated:false`, sites `11bd:084f`
(`e98302` JMP, `FUN_11bd_07e7`), `11bd:0acc` (`7507` JNZ, `FUN_11bd_0a9f`),
`11bd:0b0b` (`75c8` JNZ, `FUN_11bd_0ae2`) — **3/3 same sites as Task 1,
NO contradiction** → rulings stand: H11 = ACCEPTED-SPLIT (zero writes — the
delete/create branch is NOT executed; the `0acc` edge is noted as INTERNAL to
H11's own body, the `084f`/`0b0b` edges are fully external).

RENAME branch — attempted, **not applicable cleanly**, executed to the brief's
enumerated fallback (RATIFY-with-disclosure, leave `caseD_0`). Verbatim
sequence, every write and every undo recorded:

| # | command | verbatim response | verification |
|---|---------|--------------------|--------------|
| 0 | pre-states `get_function_by_address(11bd:0337)/(1000:0018)/(1991:4f40)` | `{"name":"caseD_0",…,"body_start":"11bd:0337","body_end":"11bd:033b"}` / `{"name":"caseD_0",…,"body_start":"1000:0018","body_end":"1000:031e"}` / `{"name":"caseD_0",…,"body_start":"1991:4f40","body_end":"1991:4f96"}`; `get_function_count` → 329 | three instances `caseD_0`; `1000:0018`/`11bd:0337` share signature `undefined caseD_0(char, undefined2)`, `1991:4f40` differs (`undefined4, short`) |
| 1 | `rename_function(old_name="caseD_0", new_name="FUN_11bd_0337")` | `{"status":"success","message":"Success: Renamed function at caseD_0 from 'caseD_0' to 'FUN_11bd_0337'"}` | MIS-FIRE: post-probe shows `11bd:0337` AND `1000:0018` both `FUN_11bd_0337` (collateral: the stub-zone instance — and a segment-wrong name on it); `1991:4f40` untouched. The name-keyed selector is name-class scoped, not single-instance |
| 2 | `rename_function(old_name="1000:0018", new_name="caseD_0")` (address-keyed revert) | `{"status":"success","message":"Success: Renamed function at 1000:0018 from 'FUN_11bd_0337' to 'caseD_0'","warnings":["…not PascalCase. Expected: Cased0","…contains underscores…","…too short (main part 'caseD_0' is 7 chars, minimum 8)…"]}` | REVERT EXPANDED TO THE NAME-CLASS TOO: post-probe `1000:0018` AND `11bd:0337` both back to `caseD_0` — full restore, NO half-renamed state (style warnings quoted-as-returned; they re-apply the original auto-name) |
| 3 | `rename_symbol(target="11bd:0337", new_name="FUN_11bd_0337")` (kind auto) | `{"status":"success","message":"Created label 'FUN_11bd_0337' at address 11bd:0337","warnings":["…not snake_case…","…not snake_case…"]}` | WRONG PRIMITIVE (label route, not function rename): `get_function_by_address(11bd:0337)` still `caseD_0`; label did not land visibly — `delete_label` single-form `{"success":false,"deleted_count":0,"deleted_names":[]}` + batch-form `{"success":true,"labels_deleted":0,"labels_skipped":0,"errors_count":0}` (nothing to remove = no artifact persisted); `can_rename_at_address(11bd:0337)` → `{"can_rename":true,"type":"function","suggested_operation":"rename_function","current_name":"caseD_0"}` (all four fields — fix wave 1 re-quote, live-re-verified identical; the original cell dropped `suggested_operation` without an ellipsis); gap row `1000:1ea4..1f06` `after_function` still `caseD_0 @ 11bd:0337` (re-paged post-attempt) |
| 4 | `rename_symbol(target="11bd:0337", new_name="FUN_11bd_0337", kind="global")` | `{"error":"Global variable '11bd:0337' not found"}` | refusal quoted; no transaction |

CONCLUSION: no available tool route renames EXACTLY ONE of the two
identically-named identically-signed `caseD_0` instances (name-class scope on
`rename_function` in both key modes; `rename_symbol` auto→label route with no
persisted effect, global→refusal). Per the cap rule ("if rename cannot apply
cleanly, RATIFY-with-disclosure — never leave a half-renamed state"):
**disposition = leave `caseD_0`; net program change = ZERO.**

CAP INTERPRETATION (acknowledged per review): the plan's "at most ONE
`rename_function` for the collision" was executed as one LOGICAL rename
attempt whose route-discovery sub-steps are rows 1–4 (name-key, address-key
revert, `rename_symbol` auto + global) — each write-capable sub-step is
separately disclosed above, and every non-revert call either refused or was
inert on the listing. Within that reading no cap was exceeded; under a strict
per-call-cap reading rows 3–4 are two extra attempts — disclosed here as the
reviewer's important finding, net effect nil. The label-creating call (row 3)
is the ONE transient artifact of the attempt; its removal probes and
listing-state verification stay quoted in that row.

Post-read-backs (all captured after the last write-transaction, i.e. after row
2's revert; rows 3–4 changed nothing):
- triple re-probe: `11bd:0337` `{"name":"caseD_0","signature":"undefined caseD_0(char param_1, undefined2 param_2)",…,"body_end":"11bd:033b"}`; `1000:0018` `caseD_0` body `0018..031e`; `1991:4f40` `caseD_0` body `4f40..4f96` — all byte- and name-identical to pre-states ✓
- `get_function_callers(name="caseD_0")` now → `{"callers":[{"name":"caseD_0","address":"11bd:0337"}],"count":1,…,"total":1}` — binds exactly as before, meaning precisely: the name query again silently resolves to the `1000:0018` instance and returns ITS caller-set, whose single record is `caseD_0@11bd:0337` (census (5) semantics — the relay is a caller OF the resolved instance, not the binding itself; the `Drift` bullet's "still binds `1000:0018`" states the same fact from the resolution side); does NOT bind cleanly — the degradation census (5) documented remains live, unfixed-by-necessity
- collision grep now: `search_functions(name_pattern="caseD")` → 3, total 3; `list_functions_enhanced` caseD_0 rows = 3 (`1000:0018` false, `11bd:0337` **true**, `1991:4f40` false) — the brief's "expect 2" applied to the rename-success branch; under RATIFY-with-disclosure 3 is the correct outcome
- `get_function_count` → `{"function_count":329}` — delta **329→329** ✓ (rename not create, and not applied)
- edge integrity: `get_function_callees(11bd:2978)` → `{"callees":[{"name":"caseD_0","address":"11bd:0337"}],"count":1,…,"total":1}` (slice-23's recorded callee edge reproduced after the churn ✓)
- gap re-page: `find_code_gaps` total **151** (both pages re-fetched: offset 0 ×100 + offset 100, full consumption re-verified); `1000:26a3..26a4` row UNCHANGED (`after_function FUN_11bd_0ad5`); `1000:4588..458b` TABLE row UNCHANGED ✓ — no new save-time-sweep movement, no new rows

`save_program`: **NOT called** — there is nothing net-new to persist (every
write was reverted in-session; the on-disk program stays at slice-23's
committed state). The archive-repair recurrence check is therefore moot (no
save attempted ⇒ no `already exists` failure observed; disclosed rather than
exercised). `/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left
unstaged.

### Ratifications (Task 2)

| item | verdict | evidence |
|------|---------|----------|
| H11 `FUN_11bd_0a9f` + `FUN_11bd_0ad5` | **ACCEPTED-SPLIT** — final bounds `0a9f..0ad2` + `0ad5..0ae1`; superseded slice-22 record: the post-bounds H11 row (`"body_end":"11bd:0ae1"`, quoted split into exact-cell quotes in census (1) this wave) — the Verdicts-row verdict text stays valid (landings/tail content/LIDT-withheld rationale), only the "resolved UNDER this body" ownership clause is superseded; plate disposition: NONE (both halves `plate:null` live — slice-22 create-only stands) | census (1)+(2) + `### Writes` Step 1 re-run (3/3 sites: `e98302`@`084f` ext, `7507`@`0acc` internal, `75c8`@`0b0b` ext) |
| `0ad3..0ad4` halt-retry island | RATIFIED status quo (defined-unowned orphan row `1000:26a3..26a4`, `has_orphaned_instructions:true`, neighbors `0a9f`/`0ad5`) | census (1) rows 6–7 |
| 8 overlays `FUN_1991_0400/21e2/2999/2b3f/4542/4930/4b0a/4e38` | **RATIFIED** — re-cut list: splits `990e..a155`→2 (by `0400`), `de60..e657`→3 (by `4930`+`4b0a`, seam `4542`), `e685..e80d`→2 (by `4e38`); vanishes `baf2..bb91`(`21e2`)/`c2a9..c31c`(`2999`)/`c44f..c621`(`2b3f`); reassembly sums 2120/2040/393 ✓; roles NOT-CONFIRMED-at-name, naming out of scope | census (3) |
| 3 defined-unowned halves `1000:2235..2266`/`2605..262d`/`80cf..8118` = `11bd:0665..0696`/`0a35..0a5d`/`64ff..6548` | RATIFIED status quo — `has_undefined_bytes:false` + no-function at starts (`0665`/`0a35`/`64ff` errors quoted); attribution stays deferred (one-hop far-ret rule) | census (4) |
| `FUN_11bd_0c9f` entry-before-body + `1000:286b..286e` hole | RATIFIED status quo — live `{entry 0c9f, body 0c84..0d0b}` (27 B lead = 23 B covered + 4 B hole); hole row `has_undefined_bytes:true`; both not fought, not recreated | census (6) |
| `caseD_0` triple (`1000:0018`/`11bd:0337`/`1991:4f40`) | RATIFIED-with-disclosure — rename PROPOSED (Task 1) but NOT APPLICABLE to a single instance with the current tool set (`### Writes` rows 1–4 verbatim); all three keep `caseD_0`; degradation is real and unfixed — carried as Deferrals | census (5) + `### Writes` |

### Errata (append-only — prior sections quoted, never edited)

(a) The `0xDE22` slip — slice-23 fix-wave-1 text verbatim: "`0xe822` —
recomputed: the printed `+0x25dc` was ALREADY CORRECT
(`0xe822+0x25dc = 0x10Dfe ≡ 0xdfe (mod 0x10000)`), the review-suggested
`+0xf5dc`/`−0xa24` is refuted (`0xe822+0xf5dc − 0x10000 = 0xDE22 ≠ 0xdfe`) —
row keeps its value, targeted-probe cites added." — CORRECTION (this slice,
re-computed): `0xE822 + 0xF5DC = 0x1DDFE`, so `− 0x10000 = 0xDDFE`;
equivalently `0xE822 − 0xA24 = 0xDDFE`. The printed intermediate `0xDE22` is
a slip; the REFUTATION STANDS unchanged (`0xDDFE ≠ 0x0DFE`); the kept value
re-verified: `0xE822 + 0x25DC = 0x10DFE ≡ 0x0DFE (mod 0x10000)` ✓ (original
conclusion — REJECT for that base, value `+0x25dc` — unchanged).

(b) Scan-size drift — slice-23 verbatim: "Caveats standing:
`search_instructions` is defined-instruction-only
(`instructions_scanned:14638` uniform on every run at this-slice time — grew
from slice-21's 14006/14170 as slice-22's 13 creates + islands landed)" —
DRIFT: live scope this slice is `instructions_scanned:15589`, uniform across
every run (Task-1 ×5 + Task-2 contradiction re-run; Δ+951, at this-slice time
2026-09-29; the 14638 numbers remain true as-of-then). Consequential
re-render: operand `0xdfe` now **3 hits** (slice-23 quoted 2): `11bd:2892`
WRITE + `11bd:28a0` WRITE (`FUN_11bd_2864`) + `11bd:2978 CMP byte ptr
[0xdfe], 0x1` bytes `803efe0d01` — class READ, now DEFINED inside
`enable_paging_and_load_tss` (`2978..29b7`) — slice-23's "READERS among
defined insns: ZERO / the reader `2978` … invisible by construction" state is
superseded at the defined-instruction layer by this slice's own create
(its raw-byte + dry-run citation of `2978` as the program's sole read is
confirmed by the new hit).

(c) Task-1 reviewer minors folded (this wave, own-section edits only):
(1) census (1) fused quote cell re-printed as SEPARATE exact quotes — the
slice-22 post-bounds row (bounds cell + disposition key clause, one row each)
and the Verdicts-row evidence cell (own row); the earlier single cell inlined
the source row's table delimiter. (2) the `328 vs 329` list/count gap —
live full-page walk THIS wave: `list_functions_enhanced` exhausted at 328
entries (`offset 328` fetch → `{"functions":[],"count":0}`, limit 10000 page
returned all), `list_methods` → `total:328`, `list_functions` → `count:328`
(three enumeration routes, same 328-row set, pagination ruled out) vs
`get_function_count` → `329` and `get_metadata` → `function_count:329` (DB
counter corroborated). Δ1 is an enumeration-vs-DB gap, NOT a page-0 blind
spot; the 329th object's identity is NOT chased (out of scope) — disclosed
unresolved with both numbers; candidate class (external-backed functions, cf.
`thunk_EXT_FUN_0000_242d`) noted as UNVERIFIED hypothesis only.

### Drift (post-Task-2 live state)

Function count `329 → 329` (rename not applied; zero net writes);
`find_code_gaps` total `151 → 151` (re-paged full, both fetches);
`search_instructions` scope `15589` unchanged since Task 1 (no save occurred
⇒ no sweep ⇒ no new restructure — the slice-22/23 save-sweep failure mode
was NOT re-armed this slice precisely because the net-zero outcome skipped
`save_program`); `caseD_0` triple state: unchanged names/bodies/flags
(`isThunk` `false/true/false`), name-keyed lookup still binds `1000:0018`;
H11 pair bounds unchanged (`0a9f..0ad2` / `0ad5..0ae1`); no new functions,
no vanished rows, no neighbor-field moves.

### Deferrals (Task 2)

- Auto-analyze-on-save setting: NOT changed (a program-option write is outside
  this slice's capped envelope) — RECOMMENDATION carried: disabling it (or
  snapshot-before-save) removes the slice-22/23 silent-restructure class at
  its root.
- `caseD_0` disambiguation: rename retry deferred until a tool route exists
  that targets ONE of two identically-named identically-signed functions
  (name-class scope proven live in `### Writes` rows 1–2); relay `11bd:0337`
  and stub `1000:0018` stay coupled.
- Far-ret-half attribution (`0665..0696`/`0a35..0a5d`/`64ff..6548` + the six
  slice-22 deferral-recorded halves): one-hop rule unchanged; status-only.
- Overlay roles/naming (`FUN_1991_*` ×8 + `2d12`-class neighbors):
  NOT-CONFIRMED-at-name; no consumer sweep this slice.
- Cell consumers: `[0x2fa]` (relay target of `0337`; listing-resolved callee
  `caseD_0@1000:0018` cited; writer set still unswept); `[0x9b4]`/`[0x40]`
  prior dispositions stand; `[0xdfe]` runtime writers (OPEN-WINDOW rows incl.
  head-internal `29a2`) not claimed closed.
- R3 IVT cluster (`INT 0x67` handler identity, `2a37 JMP BX` dynamic leg):
  unchanged.
- The 329th-function identity (erratum (c)) — tooling backlog alongside
  `search_functions_enhanced` total-0 behavior and `get_function_labels`
  name-fold (`"0xcased_0"` error verbatim). Reviewer-measured data point
  (re-verified live in fix wave 1): `search_functions_enhanced(is_thunk=true)`
  → `{"total":3,"results":[{"name":"thunk_FUN_11bd_61ee","address":"11bd:607c",…,"isThunk":true},{"name":"thunk_FUN_11bd_61ee","address":"11bd:6080",…,"isThunk":true},{"name":"thunk_FUN_11bd_75be","address":"11bd:7591",…,"isThunk":true}]}`
  — MISSING two `isThunk:true` rows the `list_functions_enhanced` full dump
  carries (`caseD_0@11bd:0337` and `thunk_EXT_FUN_0000_242d@1991:3ea5`): the
  enhanced-thunk enumeration under-reports by 2 (5 dump rows vs 3 tool rows),
  same tool-family reliability cluster as the caseD total-0 anomaly.

### Fix wave (Task 2 review — docs-only trailer)

This section's census (1) fused quote cell was split into three rows of exact
separate quotes (post-bounds bounds cell / post-bounds disposition key clause /
Verdicts evidence cell) per the reviewer minor; erratum (c)(2) replaced the
Task-1 "Δ1 not reconciled" wording with the live three-route enumeration probe
result (328 full-page ×3 vs 329 DB ×2). No other prior row of this section was
altered; everything through `## block head 2978..2a59` remains byte-identical
(appends only). Ghidra state: net-zero (see `### Writes`).

### Fix wave 1 (Task 2 review round 1 — docs-only, reads-only verification, zero Ghidra writes)

Reviewer Approved-with-minors; folded items, each verified live before
re-quoting (this wave's only tool calls: `can_rename_at_address(11bd:0337)`
and `search_functions_enhanced(is_thunk=true)`, both read-only):
(1) IMPORTANT — cap wording: `### Writes` gained a CAP INTERPRETATION
paragraph — the sanctioned rename cap was read as one LOGICAL attempt whose
route-discovery sub-steps (name-key, address-key revert, `rename_symbol`
auto/global) are separately disclosed; the label-creating call named as the
one transient artifact with its removal/verification cites; strict per-call
reading of rows 3–4 acknowledged (net effect nil).
(2) Minor 1 — `### Writes` row 3 `can_rename_at_address` quote now prints ALL
FOUR fields verbatim (`suggested_operation":"rename_function"` restored);
live re-read this wave returned the identical full response, confirming the
original cell had dropped the field without an ellipsis.
(3) Minor 2 — the "binds exactly as before" bullet now states the semantics
inline (name query silently resolves the `1000:0018` instance and returns its
caller-set — the listed `caseD_0@11bd:0337` is a CALLER of the resolved
instance per census (5); the same fact as the `Drift` bullet's "still binds
`1000:0018`" from the resolution side) — contradiction-without-recall
removed.
(4) Minor 3 — tooling-backlog deferral bullet extended with the
reviewer-measured, live-re-verified enhanced-thunk under-report:
`search_functions_enhanced(is_thunk=true)` → total 3 vs the
`list_functions_enhanced` dump's 5 `isThunk:true` rows (missing
`caseD_0@11bd:0337`, `thunk_EXT_FUN_0000_242d@1991:3ea5`), response quoted.
All edits confined to this section; no program write; program state re-read
as net-zero (`caseD_0` triple intact per row-3's `current_name` re-quote);
`fifa96.rep` churn left unstaged.

## vector selection logic (verified 2026-09-30, program `/fifa96.exe`)

Zero-Ghidra-write census closing the slice-22 spine deferral — who calls
`dispatch_mode_vector@092c` vs `FUN_11bd_0931`, what the `0931` body walk
exposes, and what statically decides which vector cell the arm consumes
(near-offset pair `[0x9bc]/[0x9be]` vs far cell `[0x9c2]`). Headline:
(1) authority `search_instructions` operand runs find **6 near-`CALL` sites
resolving to `11bd:092c`** (nextIP+rel arithmetic below: `0cf4`/`2cc5`/
`7a76`/`7b24`/`7d15`/`7d1e` — two owners beyond slice-21's 3-callee control:
`FUN_11bd_0c9f` (sweep-created body `0c84..0d0b`, slice-24 census (6)) and a
**defined-unowned** site `2cc5` (`get_function_by_address` no-function error
quoted)), **0 `JMP`-form sites**, and **1 site resolving to `11bd:0931`**
(`0da6` in `FUN_11bd_0d80`, body `0d80..0db1` — the slice-22 leg, bytes
`e888fb` verified live: `0da9 − 0x478 = 0931` ✓); no far-form caller exists
(`9a`/`ea` byte-patterns 0/0 for `0931`, 1 hit for `ea2c09` at `1991:0477`
RECONCILED-OUT — raw bytes `ea 2c 09 18 00` = `JMPF 0x0018:0x092C` =
physical `0x0180+0x092C = 0x0AAC`; the listing render `JMPF 0x0000:0aac` =
physical `0x0AAC` — BOTH views converge on the same physical target (the
divergence is render-only: seg/offset decomposition of one physical,
re-checked this fix wave); `0x0AAC` ≠ the dispatcher's `0x124FC` either way
— not a caller); no `ff16/ff26` cell statically
holds a `092c`-class value (indirect-transfer enumeration complete — table
below; every abs-cell content reconciled, pair tables and stub addresses
cited). (2) The `0931` **walk exposes ZERO new contiguous CODE runs** — body
`0931..0937` is `NOP; PUSH AX; PUSH BX; JMP word ptr [0x9be]`, the `ff26`
transfer has no fall-through, and both neighbours are owned (last-owned
`0930` = `dispatch_mode_vector` body end; first-foreign `0938` = the live
entry of `FUN_11bd_0938`, body `0938..0973` — the slice-22 `0938` envelope
row re-derived live; the plan's `0938`/`stage_ss_selector` parenthetical is
superseded by these bounds + map rows per the ledger ruling). Task-2 write
window = EMPTY (enumerated legal outcome). (3) Cell census
`{0x9bc, 0x9be, 0x9c2}`: **zero delta** vs the slice-14/20/21/23/24 records
— `0x9bc` 2 / `9bc` 6 / `0x9be` 2 / `9be` 4 / `0x9c2` 2 / `9c2` 2 / `[0x…]`
bracket forms same-set / `CS:` forms 0 / `0x296d` 1 (`41ee` sole value site),
every false-string recomputed identical at scope `instructions_scanned:
15589` uniform on every program-scope run AT THIS-SLICE TIME; the
`[BX + -` window run **grew 10→24** — the +14 split honestly by set:
**12-family** = the sweep-created overlay `FUN_1991_0400`'s window sites in
this run (11 store sites `0412…045e` + the `0461 LEA` base-setter); +
`1991:111e` (LEA — NON-LOAD) + `1991:49aa` (TEST — OPEN-WINDOW, no base
enumerated) = 14 new run hits total. The 11 stores are **REJECTED via the
DS-CLAMP**, not a constant store base: `0406 8edb MOV DS,BX` with BX = the
`0402 bb2000` constant `0x20` pins DS, unrestored through the `0477 ea` tail
(42-insn listing: no `POP DS`/DS-write between) → every physical target of
the body's window sites ∈ `[0x20<<4, 0x20<<4+0xFFFF] = [0x200, 0x101FF]`,
and the `1000:`-paragraph cell views sit at physical `0x109BC/0x109BE/
0x109C2` > `0x101FF` — excluded regardless of the BX value; BX itself is
RUNTIME after `040e 8b1e9609 MOV BX,[0x996]` (following `0408 812e9609 8001
SUB [0x996],0x180`) — the register's own value stays OPEN-WINDOW-class, the
DS pin is what rejects the sites. NO window hit with a reachable base
reaches `0x9bc/0x9be/0x9c2`. (4) NEW records: the stub
address operands `[0x92c]`/`[0x931]` are STORE TARGETS, not calls — `0229`
`MOV byte ptr CS:[0x92c],0x9b` + `022f` `MOV byte ptr CS:[0x931],0x9b`
(both in `FUN_11bd_016c`, a recorded `7c62` callee — the x87 WAIT opcode
`0x9b` patched over BOTH stub head NOPs, gated `[0x3e]≠0` at `0216`/`021b`),
plus `575f` `MOV byte ptr [0x92c],CL` (`FUN_11bd_5686`, DS-relative);
head bytes still `90 90` in the listing at this-slice time (runtime patch,
trace-blocked like the other armed-value layers). (5) The **static image**
content of the cell cluster (DS=`0x1000` paragraph view, raw read at
`1000:09b0`) pre-arms `[0x9b4]=0x1000`, `[0x9b6]=0x11BD`, `[0x9ba]=0x08B2`,
`[0x9bc]=0x0A9F`, `[0x9be]=0x0AA4` — exactly the arg-`0x8b2` state (its pair
words = `0x0a9f`/`0x0aa4` per the determinability table), `[0x9c0]=0x02FC`,
`[0x9c2]=0x02B5` (the landing pad — default non-armed continuation),
`[0x9c4]=0x0022`; recorded as image bytes, not a runtime claim. (6) The
selection question is ANSWERED at the static layer: which dispatcher is
bound AT THE CALL SITE (no selector instruction anywhere inspects the cells
— the only readers of `[0x9bc]`/`[0x9be]` are the two stub transfers);
`092c` = bare w0 transfer (target carries its own `PUSH AX;PUSH BX`
preamble), `0931` = pre-pushing w1 transfer (lands on the post-preamble
`CLI` — why two dispatchers exist); which pair the arm consumes = the
`[BP+-0x5a]` arg (`440e`/`44ab` cascade stores → `452f` sole push → `4536`
publish) + `publish_mode_vector`'s gates (`6259`/`625e` carry and `6266`/
`626b`, override `626d` iff `[0x2f]>=3 AND [0x2e]==2` → pocket pair
`2820/2822`, else `CS:[arg−4]/[arg−2]`); pair-vs-far = two statically
distinct consumers — the near pair serves the stub dispatch, `[0x9c2]` serves
`execute_mode_switch`'s tail `02b1`, armed at `41ee` iff `[0x2f]>=3`
(`41e7`/`41ec` JL-skip re-derived byte-exact). Runtime-armed values, the
`[0x9ba]` chain, `[0x9c0]` installer identity and the `CS:[0x2fa]` relay
stay OPEN by standing scope guard; `2cc5`/`0e3c` owner neighborhoods listed
as defined-unowned, not chased. Quote protocol: 14/14 `read_memory`
responses internally reconciled hex-vs-data ✓ (the `1991:0473`/`0477`
raw-vs-listing seg/offset DIVERGENCE RENDER-ONLY per fix wave 1 — physicals
converge); `disassemble_bytes` ran
EXCLUSIVELY `dry_run=true` (2 calls); no create/rename/comment/define/`save_program`;
`find_code_gaps` not needed (no exposure claim rests on a gap row —
ownership cites govern); controls (`get_function_callers`/`get_function_xrefs`/
`get_function_callees`) quoted not relied; `get_function_xrefs(092c)` alive
this pass (6/6 = the authority set) vs the historical dead-channel 0 — drift
disclosed, not used for the census.

### Dispatcher bounds + caller census (Step 1)

Bounds verbatim (`get_function_by_address`, this pass):

| Probe | Verbatim response |
|-------|--------------------|
| `11bd:092c` | `{"name":"dispatch_mode_vector","address":"11bd:092c","signature":"undefined dispatch_mode_vector(void)","entry_point":"11bd:092c","body_start":"11bd:092c","body_end":"11bd:0930"}` |
| `11bd:0931` | `{"name":"FUN_11bd_0931","address":"11bd:0931","signature":"undefined FUN_11bd_0931(void)","entry_point":"11bd:0931","body_start":"11bd:0931","body_end":"11bd:0937"}` |
| `11bd:0934` (interior resolve) | `{"name":"FUN_11bd_0931","address":"11bd:0931",…,"body_start":"11bd:0931","body_end":"11bd:0937"}` — the `0934` load leg resolves THROUGH the stub FUN (slice-21 row reproduced) |
| `11bd:0938` (after-neighbor) | `{"name":"FUN_11bd_0938","address":"11bd:0938","signature":"undefined FUN_11bd_0938(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:0938","body_start":"11bd:0938","body_end":"11bd:0973"}` — FINAL bounds; the slice-22 create row's call-time envelope was `body_size:60` (H7 row, `(call-time envelope; final bounds below)` annotation) |
| `11bd:0d80` | `{"name":"FUN_11bd_0d80","address":"11bd:0d80","signature":"undefined2 FUN_11bd_0d80(void)","entry_point":"11bd:0d80","body_start":"11bd:0d80","body_end":"11bd:0db1"}` |

Authority runs (all `search_instructions`, `scope:program`,
`truncated:false`, `instructions_scanned:15589`, defined-instructions-only,
at this-slice time): CALL+`0x1000:24fc` = 6; JMP+`0x1000:24fc` = 0; all-
mnemonic `0x1000:24fc` = 6 (set identical — no other transfer form renders
that target); CALL+`0x1000:2501` = 1; JMP+`0x1000:2501` = 0; all-mnemonic
`0x1000:2501` = 1; operand `11bd:092c` = 0; operand `11bd:0931` = 0
(segment-form probes, negative ×2). Render delta `0x1000:xxxx − 0x1BD0 =
11bd:xxxx` applies to every target column.

| caller insn | bytes | nextIP + rel (arith shown) | resolves to | owner (function field as returned) |
|-------------|-------|---------------------------|-------------|-------------------------------------|
| `11bd:0cf4 CALL 0x1000:24fc` | `e835fc` | `0cf7 + 0xFC35(−0x3CB) = 092C` | `11bd:092c` | `FUN_11bd_0c9f` (`0c84..0d0b` — sweep-created, slice-24 census (6) neighbor; NEW owner vs slice-21 control) |
| `11bd:2cc5 CALL 0x1000:24fc` | `e864dc` | `2cc8 + 0xDC64(−0x239C) = 092C` | `11bd:092c` | NONE — `get_function_by_address(11bd:2cc5)` → `{"error":"No function found for 11bd:2cc5"}` (defined-unowned site; invisible to any function-keyed control) |
| `11bd:7a76 CALL 0x1000:24fc` | `e8b38e` | `7a79 + 0x8EB3(−0x714D) = 092C` | `11bd:092c` | `FUN_11bd_79fc` (control-listed caller reproduced) |
| `11bd:7b24 CALL 0x1000:24fc` | `e8058e` | `7b27 + 0x8E05(−0x71FB) = 092C` | `11bd:092c` | `FUN_11bd_7a88` (body `7a88..7b33` live-probed — control-listed caller reproduced) |
| `11bd:7d15 CALL 0x1000:24fc` | `e8148c` | `7d18 + 0x8C14(−0x73EC) = 092C` | `11bd:092c` | `execute_exit_arm` — the slice-12 verify leg, bytes re-derived (`e8148c` ↔ `−0x73EC` ✓) |
| `11bd:7d1e CALL 0x1000:24fc` | `e80b8c` | `7d21 + 0x8C0B(−0x73F5) = 092C` | `11bd:092c` | `execute_exit_arm` — the fail-path leg, bytes re-derived ✓ |
| `11bd:0da6 CALL 0x1000:2501` | `e888fb` | `0da9 + 0xFB88(−0x478) = 0931` | `11bd:0931` | `FUN_11bd_0d80` — the slice-22 known leg, bytes + owner verified live ✓ |

Direct-far (`9a`/`ea`) + indirect-through-cell probes: byte patterns
`9a2c09` 0, `9a3109` 0, `ea3109` 0, `ea2c09` **1** = `1991:0477` — raw read
`read_memory(1991:0473,10)` → `{"data":[71,28,52,15,234,44,9,24,0,142],
"hex":"471c340fea2c0918008e"}` ✓ internally reconciled = bytes `ea 2c 09 18
00` (JMPF seg-field `0x0018`); the `disassemble_function(1991:0400)` listing
emits at `0477` `JMPF 0x0000:0aac`. PHYSICAL ARITHMETIC (fix wave 1): raw
`0x0018:0x092C = 0x0018<<4 + 0x092C = 0x180 + 0x92C = 0x0AAC`; listing
`0x0000:0x0AAC = 0x0AAC` — the two views DECOMPOSE DIFFERENTLY BUT RESOLVE
TO THE SAME PHYSICAL TARGET; the divergence is RENDER-ONLY (seg/offset split
of one physical address), not a content contradiction. `0x0AAC ≠
0x11BD0+0x092C = 0x124FC` (the dispatcher) under both readings —
RECONCILED-OUT, not a caller; overlay-bank body, roles NOT-CONFIRMED
(slice-24 ratification stands). Cell-value census for `ff16/ff26` forms (complete indirect
enumeration, both runs `scope:program`, 15589, `truncated:false`):

| indirect site | cell | static content (source) | = 092c/0931-class? |
|---------------|------|-------------------------|--------------------|
| `11bd:092d JMP [0x9bc]` `ff26bc09` | `[0x9bc]` | runtime: sole writer `6274` value = `CS:[BX−4]` pair word; image initial `0x0A9F` (`1000:09bc` read) | NO (handler offsets only per determinability) |
| `11bd:0934 JMP [0x9be]` `ff26be09` | `[0x9be]` | runtime: sole writer `627b` value = `CS:[BX−2]`; image initial `0x0AA4` | NO |
| `11bd:02b1 JMP [0x9c2]` `ff26c209` | `[0x9c2]` | `41ee` value `0x296d`; image initial `0x02B5` | NO |
| `11bd:0249 CALL [0x97a]` `ff167a09` (`FUN_11bd_0246`) | `[0x97a]` | image `0x2F4C` (`1000:0970` read ✓ reconciled) | NO |
| `11bd:0294/02b8 CALL [0x9c0]` `ff16c009` | `[0x9c0]` | image `0x02FC`; NO-IN-EXE writer (slice-14 exhausted) | NO |
| `11bd:183e CALL [0xac2]` `ff16c20a` (`FUN_11bd_17f3`) | `[0xac2]` | image `0x2422` (`1000:0ac0` read ✓; region is text `0x6520 0x7272` = " err" tail bytes) | NO |
| `11bd:2368/2376 CALL [0xe6c]` `ff166c0e` (`print_error_message`) | `[0xe6c]` | image `0x17F3` (`1000:0e68` read ✓) | NO |
| `11bd:25ea JMP [0xd10]` `ff26100d` (`FUN_11bd_2460`) | `[0xd10]` | image `0x0000` (`1000:0d0c` read ✓) | NO |
| `11bd:4fd3 CALL [BP+0x4]` `ff5604` (`FUN_11bd_4f83`) | stack slot | runtime (frame value) | OPEN-WINDOW (runtime arg frame; per rule counted only with cited writers — none enumerated) |
| `11bd:0337 JMP CS:[0x2fa]` `2eff26fa02` | `CS:[0x2fa]` | slice-23/24 deferral stands (writer set unswept; listing-resolved callee `caseD_0@1000:0018` cited there) | OPEN-WINDOW (unchanged) |
| `1991:0cf7/1f7d/33a5/33e1/453b` `CS:[…]` window forms; `1991:3bbf/3bf5` `CALL [0xaa4]/[0xaa6]` | dynamic/overlay | overlay cells image `0x17C0`/`0x4E89` (`1991:0aa4` read ✓) | NO / OPEN-WINDOW (dynamic bases) |
| pair tables `mode_vector_source_pair@2820` = `64284c28` (`0x2864`/`0x284c`), `mode_29bc_source_pair@29b8` = `5a2a602a` (`0x2a5a`/`0x2a60`) | CS-space | words cited (determinability table; slice-21/23 applies) | NO — the tables feed `[0x9bc]/[0x9be]` via `6270/6277`, they are not caller cells |

No `ff162c09`-style cell-mediated CALL of the stub addresses exists
(pattern `ff262c09` 0, `ff263109` 0; operand renders below complete).

Controls (quoted, NOT authority): `get_function_callers(092c)` → `FUN_11bd_0c9f`,
`FUN_11bd_7a88`, `FUN_11bd_79fc`, `execute_exit_arm` (count 4 — slice-21's
3 + the sweep-created `0c9f`; misses the defined-unowned `2cc5` site:
6 authority sites vs 4 keyed entries); `get_function_callers(0931)` →
`FUN_11bd_0d80` count 1 ✓; `get_function_xrefs(092c)` → 6 refs, exactly the
authority set (`0cf4`/`2cc5` [no `from_function` — unowned] `/7a76`/`7b24`/
`7d15`/`7d1e`, all `UNCONDITIONAL_CALL`) — channel ALIVE this pass, drift vs
slices 16–20's 0-ref quotes, disclosed; `get_function_xrefs(0931)` → 1 ref
(`0da6`) ✓; `get_function_callees(092c)`/`(0931)` → `{"callees":[]}` both
(the transfers are dynamic — records nothing static, consistent with the
stub shape); `get_function_callees(execute_exit_arm)` → 6 = {`FUN_11bd_016c`,
`FUN_11bd_199a`, `FUN_11bd_79fc`, `clear_slot_entries@1df7`,
`dispatch_mode_vector@092c`, `stage_ss_selector@0290`} — slice-12's "exactly
the known six, no seventh" reproduced.

0d80 caller context (one hop out): its own callers are `11bd:0e3c CALL
0x1000:2950` (`e841ff`: `0e3f + 0xFF41(−0xBF) = 0D80` ✓ — site DEFINED-
UNOWNED, probe error quoted) and `11bd:11c2` (`e8bbfb`: `11c5 +
0xFBBB(−0x445) = 0D80` ✓) in `FUN_11bd_11c1` (body `11c1..11c6` verbatim) —
reproduces the paging-block `0x1000:29` family row (`## paging block` sweep,
12-hit era list including `0e3c`/`11c2`).

### Walk of FUN_11bd_0931 (Step 2)

`disassemble_function(11bd:0931)` verbatim: 4 insns — `0931 NOP`; `0932
PUSH AX`; `0933 PUSH BX`; `0934 JMP word ptr [0x9be]` — matches the stub-
cluster read re-quoted live this pass: `read_memory(11bd:092c,12)` →
`{"data":[144,255,38,188,9,144,80,83,255,38,190,9],"hex":"90ff26bc09905053ff26be09"}`
✓ (NOP;JMP`[0x9bc]`;NOP;PUSH AX;PUSH BX;JMP`[0x9be]` — head bytes `90` at
`0931` at this-slice time, i.e. the `022f` WAIT-patch below has NOT applied
to the listing).

Dry-run windows (`disassemble_bytes`, `dry_run=true` only — 2 calls this
pass): start `11bd:092c` len 16 → emitted `092c..093a` (9 insns): `092c NOP
90`; `092d JMP [0x9bc] ff26bc09`; `0931 NOP 90`; `0932 PUSH AX 50`; `0933
PUSH BX 53`; `0934 JMP [0x9be] ff26be09`; `0938 PUSH AX 50`; `0939 PUSH BX
53`; `093a MOV BX,0x1000 bb0010` (H7 prelude head, in the owned
`FUN_11bd_0938`). Start `11bd:0931` len 12 → identical from `0931` (7
insns). Every emitted instruction classified:

| insn | address | class | owner/flow note |
|------|---------|-------|-----------------|
| `NOP` | `0931` | CODE (body entry) | `FUN_11bd_0931` entry byte |
| `PUSH AX` | `0932` | CODE | preamble pre-push (w1 path byte 1) |
| `PUSH BX` | `0933` | CODE | preamble pre-push (w1 path byte 2) |
| `JMP word ptr [0x9be]` | `0934..0937` | CODE — **transfer exit** | dynamic near transfer (`ff26`); no fall-through defined past `0937` by this flow |
| `PUSH AX` / `PUSH BX` / `MOV BX,0x1000` | `0938`–`093c` | CODE (beyond, LINEAR emission only) | already DEFINED+OWNED (`FUN_11bd_0938` body `0938..0973`); static landing set of the `0934` transfer = the 13 w1 values `0413…0ae7`+`2a60` (slice-22 dedupe/determinability rows; each `CLI` byte, all inside slice-22/23-created handler bodies) |

Cited exit: the body's own terminator is the `ff26` dynamic transfer (exit
class of `092c`'s `ff26` twin; no `c3`/`cb`/`ea`/tail-`e9` byte exists
inside `0931..0937` — emission complete to the boundary). **Exposed NEW
contiguous CODE runs: NONE.** Boundaries with stop-short cites: before —
last-owned `0930` (`dispatch_mode_vector` body_end, bounds row) | entry
`0931`; after — last-owned `0937` | first-foreign `0938` = `FUN_11bd_0938`
entry (bounds row above; no undefined byte between — the 12-byte cluster
read is fully emitted/owned). Task-2 consequence: the write window is EMPTY;
zero creates are admissible from this walk.

### Cell census {0x9bc, 0x9be, 0x9c2} (Step 3)

Literal family (every run `scope:program`, `truncated:false`,
`instructions_scanned:15589` uniform — all "at this-slice time"):

| pattern run | match_count | hits + classification | prior record |
|-------------|-------------|------------------------|--------------|
| operand `0x9bc` | 2 | `092d ff26bc09 JMP [0x9bc]` TRANSFER-READER (`dispatch_mode_vector`) + `6274 a3bc09 MOV [0x9bc],AX` WRITE (`publish_mode_vector`) | slice-20/21/23 `2` — REPRODUCED exactly |
| operand `9bc` | 6 | above 2 + FALSE-STRINGS `2dd6`/`2de7 75xx JNZ 0x1000:49bc` (→`2dec`, `FUN_11bd_2d9c`) + `2f18 c746e8bc29` + `44ab c746a6bc29` (imm `0x29bc` renders; `44ab` bytes re-read below) | slice-20/21/23 `6` — REPRODUCED (same 4 false strings, same owners) |
| operand `0x9be` | 2 | `0934 ff26be09 JMP [0x9be]` TRANSFER-READER (`FUN_11bd_0931`) + `627b a3be09 MOV [0x9be],AX` WRITE | slice-20/21/23 `2` — REPRODUCED |
| operand `9be` | 4 | above 2 + `4dcd eb1f JMP 0x1000:69be` (→`4dee`, `FUN_11bd_4ca1`) + `1991:20a6 7506 JNZ 0x1000:b9be` (same-bank target) | slice-20/21 `4` reconciled set — REPRODUCED (single run, no batch artifact this pass) |
| operand `0x9c2` | 2 | `02b1 ff26c209 JMP [0x9c2]` READ (`execute_mode_switch` tail) + `41ee c706c2096d29 MOV [0x9c2],0x296d` WRITE (`FUN_11bd_3ed8`) | slice-14/17/22 `{41ee}`+read `02b1` — **CONFIRMED, NOT EXTENDED** (brief's `02b1?` hedge resolved: `02b1` is a READ, `41ee` the sole static writer, value `0x296d` — matching the prior records exactly, no re-proving needed) |
| operand `9c2` | 2 | identical (superset check) | slice-14 `2/2` ✓ |
| operand `[0x9bc]` / `[0x9be]` / `[0x9c2]` | 2 / 2 / 2 | same pairs (bracket-render probe) | slice-21 rows ✓ |
| operand `CS:[0x9bc]` / `CS:[0x9be]` / `CS:[0x9c2]` | 0 / 0 / 0 | negative ×3 — no CS-override render exists for any of the three | slice-21 negatives for `9bc/9be` extended to `9c2` this pass (new probe, same class) |
| operand `0x296d` | 1 | `41ee` only | slice-14 `1` — value program-unique REPRODUCED |
| operand `0x92c` (stub-address probe) | 2 | `0229 2ec6062c099b` `MOV byte CS:[0x92c],0x9b` WRITE + `575f 880e2c09` `MOV byte [0x92c],CL` WRITE — **NEW: store-targets, zero transfer-forms** (see patch row below) | never swept before (prior sweeps covered the `0x9b8..0x9bf` family and the stub as CODE only) |
| operand `0x931` | 1 | `022f 2ec60631099b` `MOV byte CS:[0x931],0x9b` WRITE — same patch family | NEW |
| operand `92c` (superset) | 3 | above 2 + FALSE-STRING `0d38 7522 JNZ 0x1000:292c` (→`0d5c`, `FUN_11bd_0d26` — the same hit as the paging-block `0x1000:29` row ✓ reproduced) | — |
| operand `931` (superset) | 5 | above 1 + FALSE-STRINGS `774a e301 JCXZ→0x1000:931d`, `7754 74ea`, `775e 75e0`, `7767 75d7` (all →`0x1000:9310` = `11bd:7740`-class, `setup_memory_hardware` branch-target renders — substrings, not cell refs) | — |

Window forms (`[base+disp]` discipline per slice-20/22): `[BX + -` run =
**24** hits (`truncated:false`, 15589) — **DELTA vs slice-20/21's 10: +14,
all enumerated below, NONE reaches a cell**:

| new/known hit | base cited | resolved | verdict |
|---------------|-----------|----------|---------|
| `6270 2e8b47fc` / `6277 2e8b47fe` (CS:[BX−4]/[BX−2]) | `626d bb2428 MOV BX,0x2824` (override path) | `0x2820/0x2822` | PAIR-SOURCE reads of the pocket TABLE (the `mode_vector_source_pair@2820` words) — known chain, live-verified in this run |
| `4cc3 268b47fe ES:[BX−2]`; `1991:2f65`; `1991:3872 LEA` | dynamic / non-load | — | OPEN-WINDOW ×2 + NON-LOAD (slice-20 ledger rows stand) |
| `7687/768e/769a/76a4` (hook-patch cluster, BX=`0xf7d`/`0x2d0a`) | `7684`/`7697` constant loads | `0xf7a`/`0x2d07` etc | REJECT (slice-20 arithmetic reproduced; owners render `FUN_11bd_7670` this pass) |
| `6198 8d47ff LEA` | — | — | NON-LOAD (`find_substring`, as slice-20) |
| **NEW family ×12** (`FUN_1991_0400` sites in this run — 11 stores `0412/0415/0418/041b/041e/0426/0429/042f/0435/0455/045e` + `0461 LEA`; body dump `0400..0477`, 42 insns this pass) | STORE BASE IS RUNTIME: `0402 bb2000 MOV BX,0x20` → `0405 1e PUSH DS` → `0406 8edb MOV DS,BX` (DS pinned `0x20`) → `0408 812e96098001 SUB [0x996],0x180` → `040e 8b1e9609 MOV BX,[0x996]` — every store site (all ≥ `0412`) executes with BX = the RUNTIME cell value `[0x996]−0x180`; the `0461 8d5fce LEA BX,[BX−0x32]` rebases again (runtime) for the post-`0461` sites (`046a` = bare-`[BX]` no-disp form, `0472 [BX+0x1c]` = positive-disp form — neither in this run's negative-disp set, same clamp) — class label folded per Task-2 fold-in | bytes all reconciled live this wave: `read_memory(1991:0400,16)` → `fa53bb20001e8edb812e9609 80018b1e` ✓ + `read_memory(1991:040e,4)` → `8b1e9609` ✓ | **REJECT ×11 stores — DS-CLAMP basis (fix wave 1; supersedes the earlier "constant base resolves `0xFFD0..0xFEEE`" wording, which was false on its own premise AND wrong-based)**: DS pinned `0x20` at `0406` and NEVER restored before the `0477 ea` tail (no DS write/`POP DS` in the listing between) → physical ∈ `[0x20<<4, 0x20<<4 + 0xFFFF] = [0x200, 0x101FF]` for any effective address; the cells in the `1000:`-paragraph view sit at physical `0x109BC/0x109BE/0x109C2` > `0x101FF` — unreachable regardless of BX; `0461 LEA` = NON-LOAD; REMAINS OPEN: the `[0x996]` value itself and the sites' effective addresses (runtime register — the clamp closes the PHYSICAL question for these cells only; the window is rejected against the `0x9bc/0x9be/0x9c2` view, and under DS=`0x20` no rendering of these disps is that cell set at any BX) |
| **NEW** `1991:111e 8d47e0 LEA` | none (defined-unowned site, function field absent) | — | NON-LOAD (LEA computes an address, never touches memory) |
| **NEW** `1991:49aa f647ff02 TEST byte [BX−0x1],0x2` | none enumerated (site ownerless in the run response) | — | **OPEN-WINDOW** (no base or segment pin citable for this site — class-OPEN, never silently rejected) |

Constant-BX census (`MOV BX,imm` family) needs no re-run for the cell
window: slice-20's 33-hit BX census and nearest-miss arithmetic (`6a97
BX=0x98e`) stands; the only BX-constant sites reaching the `0x9b8..0x9bf`
window in any run this pass are the cited pair-source reads via the
`0x2824` override (window −4/−2 = the TABLE, not the cells). Positive-disp
envelope (`[BX + 0`, 320 hits at 14006 in slice-20/21) NOT re-enumerated
per-site this pass (no new claim rests on it; the window question for the
three cells is closed by the ledger + this pass's negative-disp run — the
envelope's scope number would now drift at 15589; recorded, not re-derived
— fix-forwardable if a later slice needs the per-hit set).

READER/WRITER CLOSURE (unchanged vs slice-21): `[0x9bc]` = 1 write
(`6274`) + 1 defined transfer-reader (`092d`); `[0x9be]` = 1 write (`627b`)
+ 1 transfer-reader (`0934`); `[0x9c2]` = 1 write (`41ee`) + 1 transfer-
reader (`02b1`); zero conditional/test readers of the three cells among
defined instructions ("not attributable from the enumerated sweeps,
defined-insn-only, at this-slice time"). Cell-adjacency NEW data contacts
at the STUB addresses (`0229`/`022f`/`575f`) are stores into offset
`0x92c/0x931` — not the three cells; they belong to the patch row below.

Static image view of the cluster (recorded NEW; raw channel):
`read_memory(1000:09b0,32)` → `{"data":[128,0,40,0,0,16,189,17,0,0,178,8,
159,10,164,10,252,2,181,2,34,0,0,0,1,0,0,0,0,0,0,0],"hex":"800028000010bd11
0000b2089f0aa40afc02b502220000000100000000000000"}` ✓ reconciled →
`[0x9b0]=0x0080, [0x9b2]=0x0028, [0x9b4]=0x1000, [0x9b6]=0x11BD, [0x9ba]
=0x08B2, [0x9bc]=0x0A9F, [0x9be]=0x0AA4, [0x9c0]=0x02FC, [0x9c2]=0x02B5,
[0x9c4]=0x0022, [0x9c6]=0, [0x9c8]=0x0001, [0x9ca]=0, [0x9cc]=0, [0x9ce]=0`
(fix wave 1 — the earlier tuple `{0,0,0,0x0001,0}` mis-placed the `0x0001`
at `0x9cc`; the data-array index 24 = `1` = byte at offset `0x9b0+24 =
0x9c8`, word `[0x9c8]=0x0001` — mapping re-derived word-by-offset from the
re-read `1000:09b0`/32 this wave, hex↔data ✓). The DS=`0x1000` paragraph
image pre-arms the cluster with the arg-`0x8b2` dispatch state (`0x9ba` =
that arg; `0x9bc/0x9be` = its pair words `0x0a9f/0x0aa4` = H11 w0/w1 per the
determinability table — byte-for-byte ✓), the CS paragraph in `[0x9b6]`,
the band base in `[0x9b4]`, the landing-pad default continuation `0x02b5` in
`[0x9c2]`, and a static initial `0x02FC` in the hook cell `[0x9c0]` (new
observation vs the "runtime-written, identity unidentified" framing — an
IMAGE initial exists in this paragraph view; the runtime-writer identity
question stays open unchanged). Caveat recorded: these are image bytes at
the `1000:`-paragraph view; runtime DS staging varies per path (see the
`0da3/0da5 PUSH 0x20/POP DS` row) — not a runtime-value claim.

### Selection trace — one hop from `execute_exit_arm@7c62` (Step 3)

| leg | cite | what it decides |
|-----|------|-----------------|
| arm → stub dispatch | `CALL 0x1000:24fc` at `7d15` (`e8148c` — verify path) and `7d1e` (`e80b8c` — fail path), both in `execute_exit_arm`; 092c-leg of the six-callee list (control reproduced) | which DISPATCHER the arm uses — statically bound at the call site: the arm consumes the **near pair** via `092c` (bare w0 transfer). No selector insn tests the cells anywhere (census above) |
| arm → mode-switch core → far cell | `CALL 0x1000:1e60` at `7ced` (= `1e60−0x1bd0 = 0290` ✓) → `stage_ss_selector` `0290..0292` fall-through `execute_mode_switch` `0293..02b4` (slice-12 fall-through row) → `JMP word ptr [0x9c2]` at `02b1` (`ff26c209` — live in the `0x9c2` run) | pair-vs-far consumption: the FAR cell serves the mode-switch tail exit, the NEAR pair serves the stub dispatch — two statically distinct consumers, both one hop from `7c62`'s recorded callees |
| selector helper → stub patch | `FUN_11bd_016c` (recorded callee, site `7c6c`): `0216 803e3e0000`-family gate `CMP byte [0x3e],0x0` + `021b JZ→0245` (skip on 0); inside the FPU block `021d FNINIT`/`021f MOV BX,[0xf82]`/`0227 FNSAVE [BX]`: `0229 2ec6062c099b MOV byte CS:[0x92c],0x9b` + `022f 2ec60631099b MOV byte CS:[0x931],0x9b` (`0x9b` = the WAIT opcode emitted at `0242` in the same body — 60-insn dump this pass) | NEW leg: when `[0x3e]≠0` the helper patches `0x9b` (WAIT) over BOTH stub head NOPs — an x87-busy sync prepended to each dispatcher entry, not a selection decision; listing heads still `90` (runtime patch, trace-blocked); the other DS-view store `575f 880e2c09 MOV byte [0x92c],CL` (`FUN_11bd_5686`, config-string family per slice-20 owner row) recorded as a data contact under runtime DS — semantics deferred |
| cascade → publish | `440e c746a62428` `MOV word [BP+−0x5a],0x2824` (bytes re-read: `read_memory(11bd:440e,6)` → `c746a62428c6` ✓) and `44ab c746a6bc29` `…,0x29bc` (within `read_memory(11bd:44a3,14)` → `c746a60509e96dffc746a6bc29eb` ✓ — the `44a3`(0x905) store, `44a8 e96dff` and the `44b0 eb` leg exactly as the slot-consumers rows); sole slot read `452f ff76a6 PUSH word [BP+−0x5a]` (`read_memory(11bd:452f,12)` → `ff76a6898614ffe8171d5b80` ✓ reconciled); direct call `4536 e8171d` → `0x7e20−0x1bd0 = 6250` ✓ `publish_mode_vector` | which PAIR the cells receive: the 14-arg cascade value (`0x2824` = pocket override pair; `0x29bc` = block pair; 12 band args) — reproduced live byte-exact |
| publish gate (pair content) | `read_memory(11bd:6250,46)` → `8bdc8b5f02891eba09803e2f000372102ec6061f0366803e2e00027503bb24282e8b47fca3bc092e8b47fea3be09` ✓ reconciled; decode `6250 8bdc MOV BX,SP` / `6252 8b5f02 MOV BX,[BX+0x2]` (arg) / `6255 891eba09 MOV [0x9ba],BX` / `6259 803e2f0003 CMP byte [0x2f],0x3` / `625e 7210 JC→6270` / `6260 2ec6061f0366 MOV byte CS:[0x31f],0x66` (side store in the `[0x2f]≥3` branch — recorded, role not claimed) / `6266 803e2e0002 CMP byte [0x2e],0x2` / `626b 7503 JNZ→6270` / `626d bb2428 MOV BX,0x2824` / `6270 2e8b47fc` / `6274 a3bc09` / `6277 2e8b47fe` / `627b a3be09` | the OVERRIDE decision: `[0x2f]>=3 AND [0x2e]==2` ⇒ BX=0x2824 ⇒ pocket-table pair (`2820/2822` = `64284c28` → cells `0x2864`/`0x284c`); ELSE BX=arg ⇒ `CS:[arg−4]/[arg−2]`; the 6270/6277 CS-window pair-source reads and the 6274/627b stores live-verified in the byte read AND the `[BX + -` run ✓ |
| far-cell arming | `read_memory(11bd:41e0,20)` → `fe027503e88024803e2f00037c69c706c2096d29` ✓; decode `41e2 7503 JNZ→41e7` / `41e4 e88024 CALL` / `41e7 803e2f0003 CMP byte [0x2f],0x3` / `41ec 7c69 JL→4257` (skip) / `41ee c706c2096d29 MOV word [0x9c2],0x296d` | the FAR decision, statically: armed iff `[0x2f]>=3` with value `0x296d` = `restore_fs_gs_and_resume` (`296d..2977`, slice-14 verdict — sole value, `0x296d` run 1 hit ✓); default image value `0x02b5` = the POPA+RET landing pad (non-armed continuation) — the near/far split is gate-bound at `[0x2f]`, both legs in `FUN_11bd_3ed8` |
| `0931` caller context (`0d80`) | `FUN_11bd_0d80` dump (26 insns, body `0d80..0db1`): `0d85 MOV AX,0x8`/`0d88 MOV DS,AX` → 3× `CALL 0x1000:2932` (`e89dff`-family at `0d8d/0d95/0d9b` — target `2932−0x1bd0 = 0d62` = `FUN_11bd_0d62` `0d62..0d7f` (probe cited, one hop, no dive); `0da0 XCHG word [BP+0x4],AX` (arg swap with `0d62`-AX); `0da3 6a20 PUSH 0x20`/`0da5 1f POP DS` (**DS←0x20 staged immediately before the dispatcher call**); `0da6 e888fb CALL 0x1000:2501` (=0931 ✓); exits `0dab/0db1 RETF` | what distinguishes the `0931` path caller-side: a far-returning selector helper that arms DS=0x20 for the stub (the `[0x9be]` read happens under that DS view — runtime-segment note, trace-blocked); no cell-inspection either — the stub choice is again compile-time call-site binding |
| `2864` pocket role | `search_instructions(function="FUN_11bd_2864")`: operand `0xdfe` → 2 hits = `2892 c606fe0d00` WRITE + `28a0 c606fe0d01` WRITE (`instructions_scanned:90` — its own ops ✓); operand `0x282` → 0 hits; `BX + -` → 0 hits (both 90 scanned) | VERDICT: `FUN_11bd_2864` WRITES `[0xdfe]` (2/2 live) and is **NOT a `2820`-pair consumer** (0/90 pair reads, 0/90 window reads) — it is the pair's VALUE (w0 landing `0x2864 PUSH AX` per the determinability row), answering the brief's compound question in the negative for the consumer leg; the program-wide `[0xdfe]` reader stays `2978` (`enable_paging_and_load_tss`, READ — the slice-24 3-hit re-render reproduced exactly: 2 WRITE + 1 READ) |

### Disposition-so-far

Caller counts: `dispatch_mode_vector@092c` — **6 authority sites / 5 owners**
(`FUN_11bd_0c9f` `0cf4`; defined-unowned `2cc5`; `FUN_11bd_79fc` `7a76`;
`FUN_11bd_7a88` `7b24`; `execute_exit_arm` `7d15`+`7d1e`); `FUN_11bd_0931` —
**1 site / 1 owner** (`FUN_11bd_0d80` `0da6`). No JMP/far/indirect caller of
either stub exists at the defined-instruction layer ("not attributable from
the enumerated sweeps, defined-insn-only, at this-slice time"); the `9a`/`ea`
probes and the complete indirect-transfer/cell-content enumeration are
negative with per-hit cites above. Walk outcome: **ZERO new contiguous CODE
runs exposed** — the body is a 7-byte pre-push-transfer stub boxed between
two owned neighbours; the Task-2 write window is empty. Cell census:
**zero delta** vs slice-14/20/21/23/24 (all counts reproduced at scope 15589;
writer `{41ee}`→`0x296d` + reader `02b1` for `[0x9c2]` re-confirmed, `02b1`
READ — the brief's hedge resolved); disclosed deltas: `[BX + -` 10→24 (all
+14 constant-base-rejected or class-OPEN, none reaches a cell), `get_function_xrefs`
aliveness, and the two NEW stub-head store families (`0229`/`022f` CS-patch,
`575f` DS byte) plus the image-state cluster record (`0x02fc` initial of
`[0x9c0]` noted as observation; the NOT-IN-EXE writer finding itself
unchanged). SELECTION ANSWER — CLOSED at the static layer: (i) WHICH
dispatcher = the call site (arm `7d15/7d1e` → `092c` = bare w0 transfer to
the handler head; `0d80@0da6` → `0931` = AX/BX pre-push w1 transfer to the
post-prelude `CLI` — the two dispatchers exist because each vector pair
stores a preamble-entrance `w0` and a preamble-skipped `w1`, slice-22
prelude rows; no runtime selector inspects the cells); (ii) WHICH pair the
cells hold = the `[BP+-0x5a]` arg (14-store cascade, byte-verified legs
`440e`/`44ab`) + `publish_mode_vector`'s override gate (`[0x2f]>=3 AND
[0x2e]==2` → pocket pair `0x2864/0x284c`, else `CS:[arg−4]/[arg−2]`);
(iii) pair-vs-far consumption = statically distinct consumers (stubs
`092d`/`0934` read the near pair; `02b1` reads `[0x9c2]`), with the far cell
armed to the restore-epilogue `0x296d` iff `[0x2f]>=3` (`41ee`, gate bytes
re-derived) and pre-armed `0x02b5` (landing pad) in the image. What STAYS
OPEN (exact legs named): the runtime armed VALUES of `[0x9ba]`/`[0x9bc]`/
`[0x9be]` per dispatch (runtime DS/arg values — trace-blocked per standing
scope guard); the `[0x9c0]` runtime-writer identity; the `CS:[0x2fa]` relay
content; which of the six `092c` call paths executes at runtime (path
selection is `[0xdff]`/`[0xe00]`/`[0x47]`-style runtime state, slice-12 rows
stand); the runtime effect of the `016c` WAIT patch (heads unpatched in the
listing at this-slice time).

### Reads executed (ZERO-WRITE branch)

Tool inventory, this pass (all reads; no transaction opened):
`get_function_by_address` ×15 (`092c`/`0931`/`0934`/`0938`/`0d80`/`2cc5`/
`0e3c`/`0d62`/`11c1`/`7a88` + the bounds table rows; all quoted above);
`search_instructions` ×33 runs (6 caller-form operand runs, 2 segment-form
probes, 1 `0x1000:2950`, 16 cell-literal family runs incl. bracket/CS forms
and the `0x92c/0x931` probes, `CALL [`/`JMP [` enumeration ×2, `[BX + -` ×1,
`[0xdfe]` program ×1 + function-scoped ×3, `0x296d`/`0x282`/`BX + -` scoped
— every response carries pattern + `match_count` + scope +
`truncated:false`; program-scope `instructions_scanned:15589` UNIFORM on
every program-scope run at this-slice time — re-read, matches slice-24's
number; `find_code_gaps` NOT used); `search_byte_patterns` ×6
(`9a2c09`/`9a3109`/`ea2c09`/`ea3109`/`ff262c09`/`ff263109` — 1 hit
(`ea2c09`), reconciled in-line); `disassemble_function` ×4 (`0931` walk ×1,
`0d80` context ×1, `016c` patch context ×1, `1991:0400` reconciliation ×1 —
read-only listing walks); `disassemble_bytes` ×2 **EXCLUSIVELY
`dry_run=true`** (the two walk windows); `read_memory` ×14 (`11bd:092c/12`
stub cluster, `11bd:09b0/32` (band-bytes view), `1000:09b0/32` (cell image
view), `1000:0970/16`, `1000:0ac0/8`, `1000:0e68/8`, `1000:0d0c/8`,
`1991:0aa4/4`, `1991:0473/10`, `11bd:440e/6`, `11bd:44a3/14`, `11bd:452f/12`,
`11bd:6250/46`, `11bd:41e0/20`) — 14/14 internally hex↔data reconciled;
`get_function_callers` ×2, `get_function_xrefs` ×2, `get_function_callees`
×3 (controls, quoted-not-relied). Program state: NO create/rename/comment/
set_global/define/delete/`save_program`; pre-existing bodies (`092c`/`0931`/
`0938`/`0d80`/`7a88`/`79fc`/`016c`/`FUN_1991_0400`) re-read only; unmoved
proof: the stub-cluster read byte-identical to the slice-21 quote
(`90ff26bc09905053ff26be09` ✓) and the `0938`/`0d80` bounds match the
slice-22/23 records. `/media/felipe/FIFAPCCD/` untouched; `fifa96.rep`
churn left unstaged. Suite: build + `ctest` → 10/10 (docs-only diff).

### Deferrals

- Runtime armed values of `[0x9ba]/[0x9bc]/[0x9be]/[0x9c2]` per dispatch and
  the per-path DS/CS staging (`0x1000` image view vs the `DS←0x20` stage at
  `0d80`, `DS←0x38/0/0x8` stagings in handler bodies) — trace-blocked
  runtime layer, prior disposition stands.
- `FUN_11bd_016c`'s WAIT-patch semantics beyond the op-cites (why `0x9b`,
  which dispatch sees a patched head) — second layer; `FUN_11bd_2081` callee
  (site `01e4`) untouched.
- Handler callee trees (`0d62`, `06fc`-family, `284c` pocket body, `28fb`
  orphan, the far-ret pair consumers) — one-hop rule; `575f`'s owner-role
  (`FUN_11bd_5686`) per slice-20 config-string disposition, not reopened.
- Defined-unowned neighbourhoods of `2cc5`/`0e3c` (nearest owners quoted
  only); `1991:0477` raw-vs-listing view divergence (tooling backlog class
  alongside slice-24's `caseD`/enhanced-search items).
- `[0x9c0]` runtime-writer identity (image initial `0x02FC` recorded this
  pass as static fact — installer still unidentified); `CS:[0x2fa]` writer
  set; `6260` side-store `CS:[0x31f]←0x66` role.
- R3 `INT 0x67` cluster (slice 26 queued); `[0x29bc]`-coincidence rows —
  unchanged.
- `0931` naming: the walk adds mechanism evidence (w1 pre-push-transfer —
  the body's own 4 insns) but the rename is Task-2 verdict territory per the
  plan's bar; nothing renamed this pass.

### Fix wave 1 (review 2026-09-30 — storm-ledger correction; own-section edits only, zero Ghidra writes beyond reads)

Reviewer finding IMPORTANT + minors 2–4 folded; every correction verified
live this wave before printing (reads only: `read_memory` ×4 new —
`1991:0400/16` → `{"data":[250,83,187,32,0,30,142,219,129,46,150,9,128,1,139,30],
"hex":"fa53bb20001e8edb812e960980018b1e"}` ✓ internally reconciled,
`1991:040e/4` → `{"data":[139,30,150,9],"hex":"8b1e9609"}` ✓,
`1991:0461/3` → `{"data":[141,95,206],"hex":"8d5fce"}` ✓ (the `LEA`
rebaser bytes), re-read `1000:09b0/32` — byte-identical to the Task-1 quote
✓; the `FUN_1991_0400`
42-insn listing is the Task-1 pass's own `disassemble_function` dump, re-used
for the no-DS-restore scan). Corrections:
(1) IMPORTANT — storm row rejection basis: the Task-1 row resolved the 11
negative-disp stores from a "constant base `0402` BX=`0x20`" — FALSE:
`0406 8edb` consumes that BX for `MOV DS,BX` and `040e 8b1e9609` RELOADS
BX from `[0x996]` (after `0408 812e9609 8001 SUB [0x996],0x180`) — store
base is RUNTIME. The quoted range was also wrong on its own premise
(`0x20−0x30 = 0xFFF0`, `0x20−0x12 = 0x000E`, not `0xFFD0..0xFEEE`). The
rejection now rests on the DS-CLAMP: DS pinned `0x20` at `0406`, never
restored before the `0477 ea` tail (listing has no DS write/`POP DS`
between; `046e MOV SS,DX` re-pins SS, not DS — per the 42-insn dump) → physical ∈ `[0x20<<4,
0x20<<4 + 0xFFFF] = [0x200, 0x101FF]` for every effective address, and the
`0x9bc/0x9be/0x9c2` cells in the DS=`0x1000` image view sit at physical
`0x109BC/0x109BE/0x109C2` > `0x101FF` — excluded at any BX value. What
REMAINS OPEN is stated in the row: the `[0x996]` value / BX register itself
(runtime) — the clamp closes the cells' physical reachability, not the
register. The `0461/0402`-derived `0xFFEE`/`0x000A` LEA resolutions were
reworded out. Slice-26 inherits this ledger with the corrected basis.
(2) Minor 2 — recount by set: the run's new hits are 14 = `FUN_1991_0400`
FAMILY 12 (11 stores `0412/0415/0418/041b/041e/0426/0429/042f/0435/0455/
045e` + `0461 LEA`) + `1991:111e` LEA (NON-LOAD) + `1991:49aa` TEST
(OPEN-WINDOW, ownerless); the Task-1 labels "NEW ×13" and "REJECT ×14"
undercounted/misbundled — replaced with family-scope/run-scope rows above
and headline (3) reworded; the earlier "(`0439 [BX+0x2a]`-family is in the
positive-disp class)" parenthetical double-counted a site outside this run
and is gone.
(3) Minor 3 — `1991:0477` physical arithmetic corrected: `JMPF
0x0018:0x092C` = `0x0180 + 0x092C = 0x0AAC` (NOT `0xAA4`); with the fix the
raw bytes and the listing render `0x0000:0aac` converge on the SAME physical
target — the divergence is render-only (seg/offset decomposition), headline
(1), the Step-1 paragraph and the quote-protocol parenthetical reworded; the
RECONCILED-OUT conclusion is unaffected (`0x0AAC ≠ 0x124FC`).
(4) Minor 4 — image-cluster tuple: `0x0001` is at `[0x9c8]` (data index 24),
not `0x9cc`; the tuple is printed word-by-offset corrected, mapping
re-derived from the live re-read.
State: prior sections untouched (all edits confined to this section —
diff-mechanics wording folded per Task-2, hunk evidence lives in the task
report); no program write; `fifa96.rep` churn left unstaged.

### Writes (Task 2 — zero-writes branch, executed 2026-09-30, program `/fifa96.exe`)

The Task-1 walk table's exposed-bodies list is **NONE**, so the conditional
create path is skipped and the binding ruling for this task is ZERO
PROGRAM WRITES: no `create_function`, no `rename_function` (execute), no
plate, no `set_global`, no `save_program`, no transaction opened. The
rename-branch bar test for `0931` is executed honestly in the verdict row
below — its premise FAILS and the tool-side probe ran `dry_run=true` only
(read-only, no mutation recorded). This section is the disclosure's
byte-level proof, re-run live THIS task (all responses verbatim, Task-2
time):

| proof leg | verbatim live response (this task) | reconciliation |
|-----------|-----------------------------------|----------------|
| before-neighbor | `get_function_by_address(11bd:092c)` → `{"name":"dispatch_mode_vector","address":"11bd:092c","signature":"undefined dispatch_mode_vector(void)","entry_point":"11bd:092c","body_start":"11bd:092c","body_end":"11bd:0930"}` | body ends `0930` — the byte immediately before the stub |
| the stub | `get_function_by_address(11bd:0931)` → `{"name":"FUN_11bd_0931","address":"11bd:0931","signature":"undefined FUN_11bd_0931(void)","entry_point":"11bd:0931","body_start":"11bd:0931","body_end":"11bd:0937"}` | bounds BYTE-IDENTICAL to the Task-1 quote — unmoved |
| after-neighbor | `get_function_by_address(11bd:0938)` → `{"name":"FUN_11bd_0938","address":"11bd:0938","signature":"undefined FUN_11bd_0938(undefined2 param_1, undefined2 param_2)","entry_point":"11bd:0938","body_start":"11bd:0938","body_end":"11bd:0973"}` | starts `0938` = `0937+1` ✓ — the walk's first-foreign byte is OWNED |
| boundary gaps | between `0930|0931` and between `0937|0938` there is NO unowned byte — the three returned `body_end`/`body_start` pairs are exact adjacencies (arith above); full-pagination `find_code_gaps` (below) carries NO row covering `1000:24f9..2543` (`0929..0973` minus the four bodies: zero) | boxed walk proven: no gap row inside the cluster span |
| caller census re-verify | `search_instructions` CALL + `0x1000:24fc` → `match_count:6`, sites `0cf4/2cc5/7a76/7b24/7d15/7d1e`, bytes `e835fc/e864dc/e8b38e/e8058e/e8148c/e80b8c` — **byte-identical to Task 1**; CALL + `0x1000:2501` → `match_count:1`, `0da6 e888fb` in `FUN_11bd_0d80` ✓ | authority stable Task-1→Task-2; `instructions_scanned:15589` uniform (scope UNCHANGED — no save occurred, so no sweep and no drift: the slice-24 `Drift`/`15589` precedent holds exactly) |
| gap census (FULL pagination, mandated since a state read backs the zero-writes claim) | `find_code_gaps offset 0 limit 100` → 100 rows, `total:151`; `offset 100 limit 100` → 51 rows, `total:151` — 151/151 consumed; neighborhood rows verbatim: `{"start":"1000:24a6","end":"1000:24f8","size":83,…,"before_function":"FUN_11bd_08c2",…,"after_function":"FUN_11bd_0929",…}` and `{"start":"1000:2544","end":"1000:256a","size":39,…,"before_function":"FUN_11bd_0938",…,"after_function":"FUN_11bd_099b",…}` | `24f8−0x1bd0 = 0928`, `2544−0x1bd0 = 0974` ✓; total 151 = slice-24 post-state ✓ (no drift); between the rows the whole `0929..0973` span is owned by the four bodies — no new contiguous CODE run exists anywhere the walk could have exposed |
| function count | `get_function_count` → `{"function_count":329,"program":"fifa96.exe"}` | 329 = slice-24 → 329, Δ0 ✓ (and Δ0 Task 1 → Task 2 — this task wrote nothing) |

NEW SURFACED ADJACENCY (disclosed, NOT created here): the gap row above
names `after_function FUN_11bd_0929` — probe: `get_function_by_address(11bd:0929)`
→ `{"name":"FUN_11bd_0929","address":"11bd:0929","signature":"undefined FUN_11bd_0929(void)","entry_point":"11bd:0929","body_start":"11bd:0929","body_end":"11bd:092b"}` —
a 3-byte body PRECEDING `dispatch_mode_vector`. Provenance: the count is
stable at 329 across slice-24/Task-1/Task-2 quotes ⇒ it PRE-DATES this
slice (never claimed by it); Task 1 simply never quoted that row's
`after_function` field. The walk proof is unaffected (the adjacency sits
one body BEFORE the `092c` end-boundary, and the boxed-walk claim concerns
`0930|0931` and `0937|0938` — both still exact).

Fold-ins executed (docs-only, own-section rows — Task-1 wording nits):
(1) the storm-row post-`0461` class label corrected — `046a` is a
BARE-`[BX]` no-disp form (the dump's `MOV word ptr [BX],0xef8`), `0472`
carries `[BX+0x1c]` positive-disp; neither belongs to this run's
negative-disp set (row now says so); (2) the `### Fix wave 1` trailer's
"diff hunks all `≥5417`" wording replaced by
"all edits confined to this section" (the hunk count is a report-side
artifact, not a map claim). No other prior row of this or any section was
touched; `git diff` shows the fold-ins plus this append.

### Verdicts (Task 2)

| verdict | address | evidence | disposition | C counterpart |
|---------|---------|----------|-------------|---------------|
| `dispatch_mode_vector` — **RATIFIED** (as recorded) | `11bd:092c` | slice-12 rename+plate row (`NOP`@`092c` + `JMP word ptr [0x9bc]`@`092d`, no RET — cited there; bounds re-read byte-identical THIS task); caller census count live re-verified: **6 sites / 5 owners** (`0cf4`,`2cc5`,`7a76`,`7b24`,`7d15`,`7d1e` — bytes identical to Task 1), controls 4-keyed/6-xrefs quoted; NEW patch-leg adjacency disclosed and deferred (`016c@0229/022f` WAIT stores, heads `90` un-applied) | no change — record stands + the new caller rows are this slice's census (Tables above) | none — behavioral (indirect tail transfer through `[0x9bc]`) — as set slice-12 |
| `FUN_11bd_0931` — **NOT-CONFIRMED-at-name** | `11bd:0931` | bar test shown: the rename branch requires the body's ops to BE the near-offset pair dispatch with pair-LOAD + far-consume — its own cited ops are `0931 NOP (90)`/`0932 PUSH AX (50)`/`0933 PUSH BX (53)`/`0934 JMP word ptr [0x9be] (ff26be09)`: there is NO load op (the pair-load is `publish_mode_vector`'s `6270/6277`, one hop out) and the consume is the NEAR `ff26` form, NOT `JMPF`/`ea` — premise fails; the mechanism the ops DO show (pre-push AX/BX replay + near tail transfer through the w1 cell) named cleanly at the gate on a read-only probe: `rename_function(old_name="FUN_11bd_0931", new_name="prepush_dispatch_mode_vector", dry_run=true)` → `{"dry_run":true,"status":"success","message":"Success: Renamed function at FUN_11bd_0931 from 'FUN_11bd_0931' to 'prepush_dispatch_mode_vector'","warnings":["…not PascalCase. Expected: PrepushDispatchModeVector","…contains underscores…"]}` — style warnings only, quoted-as-returned, snake_case precedent (`clear_msw_and_callfar`, `enable_paging_and_load_tss`); NOT EXECUTED per this task's zero-writes ruling | create-already-existing → disposition = keep `FUN_11bd_0931` default name, NO plate; role recorded at the mechanism the body itself shows; the dry-run-passed candidate carried in Deferrals should the controller lift the zero-writes ruling | none — behavioral (register-preamble replay + near tail transfer through `[0x9be]`), recorded wording only |
| **selection answer** — **CLOSED at the static layer** (runtime legs named) | chain | stub choice = CALL-SITE binding (092c: 6 sites incl. `execute_exit_arm@7d15/7d1e`; 0931: 1 site `0da6` in `FUN_11bd_0d80` `0d80..0db1`, DS←0x20 staged `0da3/0da5` immediately prior — cited in the walk context); pair content = `[BP+-0x5a]` arg (cascade stores `440e/44ab` bytes live-verified) + `publish_mode_vector` gates `6259/625e` carry ∧ `6266/626b`, override `626d` → `CS:[BX−4]/[BX−2]` → `6274/627b`; far cell `[0x9c2]` = consumer `02b1` (`execute_mode_switch` tail) + conditional writer `41ee` iff `[0x2f]>=3` (`41e7/41ec` byte-re-derived) + image default `0x02B5`; patch leg `016c@0229/022f` gated `[0x3e]≠0`, UN-APPLIED in the listing at this-slice time | answers the spine question at the static layer; NO runtime claim made | — |

### Deferrals (Task 2)

- R3 IVT cluster (`INT 0x67` handler identity at `2a16`, `2a37 JMP BX`
  dynamic leg) — slice-26 queued (standing scope guard).
- Handler callee trees (`0d62`, `06fc`/`073c`/`0733`-family, `284c`/`28fb`,
  the far-ret pair consumers, `CALLF ES:[0xd5a]`) — one-hop rule.
- `[0x9ba]` armed-value runtime — FU-blocked (prior disposition).
- `[0x2fa]` consumers + the `016c` patch-leg gate inputs (`[0x3e]`,
  `[0xf82]` runtime) — recorded, not traced.
- Runtime writers of `{[0x9bc], [0x9be], [0x9c2], [0x996]}` beyond the
  cited static writers — OPEN-WINDOW (armed-value layer, trace-blocked).
- `0931` NAME EXECUTION: the gate-passed candidate
  `prepush_dispatch_mode_vector` (dry-run verbatim in the verdict row) is
  carried forward — executing it is a controller decision, this slice's
  ruling is zero-writes; the `caseD_0` name-class route (slice-24 finding:
  no tool route renames exactly one of two identically-named instances)
  remains deferred.
- Δ1 (`list_functions_enhanced` 328 vs count 329) — unresolved, no claim
  (slice-24 erratum (c) stands).
- Storm ledger carry-forward to slice-26: rejection basis = **DS-clamp**
  (`0406 8edb` pin, never restored before the `0477 ea` tail; physical
  `[0x200,0x101FF]` excludes the `1000:`-view cells) — the fixed basis is
  what slice-26 inherits, per the fix-round ruling.
- Suite green (10/10, docs-only diff); no program write occurred this task
  — `save_program` NOT called (nothing to persist); on-disk program stays
  at slice-23's committed state (the net-zero precedent — slice-24 `### Writes`);
  `/media/felipe/FIFAPCCD/` untouched; `fifa96.rep` churn left unstaged.
