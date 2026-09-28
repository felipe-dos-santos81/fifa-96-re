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
