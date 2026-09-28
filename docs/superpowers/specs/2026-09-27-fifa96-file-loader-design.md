# FIFA96 File/Data Loading — Design (Slice 1 of EXE recreation in C)

Date: 2026-09-27
Status: Approved in chat (scope / components / errors+testing), pending file review
Goal: Recreate `fifa96.exe` logic in portable C, starting with file/data loading, as 1:1 matching functions with tests.

## 1. Context

- Target: `/fifa96.exe` in Ghidra project `fifa96` (`/home/felipe/code/fifa96-reversed/fifa96`).
  - 16-bit DOS Real Mode (`x86:LE:16:Real Mode Ex`), compiler `windows`.
  - 291 functions (all `FUN_*` except `entry`), 635 symbols.
  - Entry `11bd:2382`; segments `CODE_0` (1000:0000, 7120 B), `CODE_1` (11bd:0000, 32064 B), `CODE_2` (1991:0000, 22884 B).
  - Exactly 1 external import placeholder; file I/O expected via `INT 21h`, not imports.
- Reference data (read-only): `/media/felipe/FIFAPCCD/`
  - `fifa96.exe`, `cdrom.dat`, `autorun.*`, `univbe.exe`
  - `fedata/*.pog` (lang000-005, pccd, pcindex), `fegfx/`, `art/*.{qfs,pvi}` (fw1-12.qfs, gameart0.pvi, gamefld1-4.pvi, gamestd1-4.pvi, playart.pvi)
  - `soccer/fnames.dat, lengths.dat, crcvals.dat, instdat/instgrps.dat, iart/, readme/`
  - `sound/*.{viv,bnk,asf,par,crd}` (545 entries: plr_t*.viv / tem_t*.viv chants, que_*.asf, sfx_*.bnk, par_*.par)
  - `video/*.tgv` (73 entries), `modem/`, `fegfx/`
- Runner: `game/FIFAPCCD96.iso` + `run-fifa96.sh` (DOSBox-X, `svga_s3`, 16 MB) mounts ISO as D:, runs `D:FIFA96.EXE`.
- Workspace: `/home/felipe/code/fifa96-reversed/` (no git repo at design time).

## 2. Scope (approved)

In-scope for this slice:
- DOS file wrappers in EXE: open/read/seek/close (INT 21h AH=3Dh/3Fh/42h/3Eh), error/carry handling, segment:offset buffer management.
- Format decoders reached from loaders:
  - `fedata/*.pog` + `pcindex.pog` index/container
  - `art/*.qfs` + `*.pvi` stills/fields/stadiums
  - `sound/*.viv` + `*.bnk` + `*.asf` + `*.par` + `*.crd`
  - `video/*.tgv` framing only (no full video decode)
  - `soccer/fnames.dat, lengths.dat` tables (crcvals.dat CRC check deferred to follow-up FU-2 — needs CRC polynomial identified first)
- New portable C library + golden tests + Ghidra rename-backflow.

Out-of-scope (later slices): VGA/SVGA blit, match sim/AI/physics, audio synthesis/playback, full video decode, menus/FE, modem/serial, installer.

## 3. Approaches considered

- (A) EXE-only tracing: anchor on INT 21h sites, expand outward. Pro: exact function boundaries. Con: format semantics stay implicit, slow.
- (B) Format-first only: probe CD files, write parsers, map back later. Pro: fast testable wins. Con: risks diverging from original function split.
- (C) Hybrid (chosen): INT 21h-anchored EXE pass + data-driven decoder pass + `src/fifa96_loader/` harness with golden vectors. Every C function cites its source `FUN_11bd_*` address. Meets "matching functions" success criterion while staying testable on modern Linux.

## 4. Architecture

New layout (not yet created — implementation phase):

```
src/fifa96_loader/
  fifa96_file.{h,c}     # DOS shim: open/read/seek/close, seg:off -> flat, POSIX backend
  fifa96_pog.{h,c}
  fifa96_qfs.{h,c}
  fifa96_viv.{h,c}      # covers .viv/.bnk/.asf/.par/.crd framing
  fifa96_tgv.{h,c}      # framing only
  fifa96_tables.{h,c}   # fnames/lengths/crcvals
include/fifa96_loader/  # public headers (same names)
tests/
  golden/               # manifests + vectors copied from /media/felipe/FIFAPCCD (never moved)
  test_file.c, test_pog.c, test_qfs.c, test_viv.c, test_tgv.c, test_tables.c, test_load_order.c
tools/
  probe/                # throwaway header dumpers feeding golden manifests
docs/superpowers/specs/ # this file
```

Build: plain C11, `-Wall -Wextra -Werror`, `ctest`. No DOS toolchain required for slice 1.

## 5. Components

1. Ghidra pass (project `fifa96`, program `/fifa96.exe`):
   - Search `INT 21h` via `search_instructions(mnemonic=INT)`, filter AH=3Dh/3Fh/42h/3Eh.
   - `get_xrefs_to` / `get_xrefs_from` to collect wrapper callers; `decompile_function` each wrapper.
   - Rename wrappers to `file_open/file_read/file_seek/file_close`-style names, add plate comments citing C counterpart path.
   - Apply types for DOS structures (FCB/DTA as needed) with `apply_data_type`; tag loader set with `loader` function tag.
   - Save checkpoints with `save_program` after each rename batch.
2. `fifa96_file`: sole module doing I/O. POSIX `fopen/fread/fseek` backend; models DOS semantics (text/binary flag ignored, short-read vs EOF, carry-flag errors mapped to enum). Splits reads that would cross a 64 KiB segment boundary into two chunks to preserve original behavior.
3. Decoders (`pog/qfs/viv/tgv/tables`): pure `bytes-in -> struct-out`, no globals, no I/O, no allocation inside hot path except caller-provided buffers where the original used far buffers.
4. Probe tools: read-only dumpers emitting JSON manifests (magic, version, count, offsets, sizes, crc) used to freeze golden expectations.

## 6. Data flow

CD bytes (`/media/felipe/FIFAPCCD/...`) -> `fifa96_file_open/read` -> magic/extension dispatch -> specific decoder (`pog/qfs/viv/tgv/tables`) -> `{ header struct + payload pointer/len }` -> (future) sim/graphics callers.

Load-order oracle: DOSBox-X run + INT 21h trace gives expected open sequence; test `test_load_order` replays manifest order against `fifa96_file` shim log.

## 7. Error handling

Lib never aborts. Return enum:

```c
typedef enum {
  FIFA96_OK = 0,
  FIFA96_ERR_NOT_FOUND,
  FIFA96_ERR_SHORT_READ,
  FIFA96_ERR_BAD_MAGIC,
  FIFA96_ERR_TRUNCATED,
  FIFA96_ERR_CRC_MISMATCH,  // checked against soccer/crcvals.dat where applicable
  FIFA96_ERR_IO
} fifa96_err_t;
```

DOS carry-flag errors map to `NOT_FOUND/IO/SHORT_READ`. `BAD_MAGIC/TRUNCATED` from decoders. `CRC_MISMATCH` only where `crcvals.dat` applies. Segment-wrap handled by split, not error.

## 8. Testing

- Golden vectors from real CD (copied into `tests/golden/`, originals untouched).
- Per-format `ctest` suites assert byte-identical payloads + field-level header asserts.
- Matching-function check: each C function's doc comment cites `Ghidra: FUN_11bd_XXXX @ 11bd:XXXX` + golden file + decompile hash at time of port.
- Regression: any Ghidra rename must keep `FUN_11bd_* -> new_name` map in spec appendix during implementation.

## 9. Isolation

Each decoder answers: what it parses, how to call it (`fifa96_<fmt>_parse(const uint8_t*, size_t, struct*)`), dependency (only `fifa96_file` types + libc). Changing internals does not change callers. Files stay small; split on growth.

## 10. Non-goals restated

No rendering, no gameplay, no sound output, no TGV frame decode beyond header/framing, no writes to `/media/felipe/FIFAPCCD`, no changes to ISO or HDD.

## 11. Next step

Invoke `writing-plans` skill to produce the implementation plan from this spec. No `src/` code until plan approval.
Follow-ups outstanding after this slice: FU-1 decoder-citation Ghidra pass (spec §8) and FU-2 crcvals reader+CRC (spec §7).
