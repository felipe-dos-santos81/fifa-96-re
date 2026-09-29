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
