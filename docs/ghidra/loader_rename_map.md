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
