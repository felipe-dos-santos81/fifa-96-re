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
