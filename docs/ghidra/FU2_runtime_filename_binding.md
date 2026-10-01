# FU-2 — runtime filename↔container binding

Date: 2026-10-01. Evidence class: **runtime-captured** (P0 capture rig), never
blended with static cites. Sessions are git-ignored (derived from copyrighted
content); this doc quotes parsed trace lines only.

Session: `captures/session-20261001T172400Z` (full run: boot → intro → menus →
match → quit; `end=END`, `lost=0`). Tool: `tools/fifa96_bind.py`.

## Method

- `make capture` → `trace.bin`; `tools/fifa96_bind.py` walks the raw FILE frames
  and tracks handle state in stream order. A final-state handle→name map is
  wrong: DOS reuses handles after AH=3E (the first cut bound only 2 handles).
- For each AH=3F record: read length `n = min(cx, ax_after)`, `head` = first
  `min(n,64)` bytes. The record's FNV-1a-32 `hash` is recomputed over the ISO
  file bytes at the candidate offset.
- Candidate files come from a pure-stdlib ISO9660 walk (primary volume
  descriptor → directory records), filtered by the 13-byte trace-name prefix
  captured by the TSR. Reads are usually sequential, so the matcher first
  checks the handle's next expected offset, then its last offset, then searches
  the file with an occurrence cap (repetitive data cannot cause an unbounded
  scan). xorriso is not used or required.

Command:

```
python3 tools/fifa96_bind.py captures/session-20261001T172400Z/trace.bin \
    --iso game/FIFAPCCD96.iso
```

## Result

```
open-records=78 opens=58 reads=7036 reads-bound=7036 unbound=0
```

| trace name | ISO file | opens | reads | bound |
|---|---|---:|---:|---:|
| D:\ART\GAMEAR | /ART/GAMEART0.PVI | 1 | 10 | 10 |
| D:\ART\GAMEFL | /ART/GAMEFLD3.PVI | 1 | 1 | 1 |
| D:\ART\GAMEST | /ART/GAMESTD1.PVI | 1 | 4 | 4 |
| D:\ART\PLAYAR | /ART/PLAYART.PVI | 1 | 99 | 99 |
| D:\FEDATA\LAN | /FEDATA/LANG000.POG | 1 | 1 | 1 |
| D:\FEDATA\PCC | /FEDATA/PCCD.POG | 2 | 18 | 18 |
| D:\FEGFX\HELV | /FEGFX/HELV12.POG | 2 | 2 | 2 |
| D:\FEGFX\LEGA | /FEGFX/LEGAL.POG | 1 | 4 | 4 |
| D:\FEGFX\MOD1 | /FEGFX/MOD10D.POG | 1 | 24 | 24 |
| D:\FEGFX\MOD2 | /FEGFX/MOD2.POG (+1) | 4 | 86 | 86 |
| D:\FEGFX\MOD5 | /FEGFX/MOD5.POG (+1) | 2 | 53 | 53 |
| D:\FEGFX\MOUS | /FEGFX/MOUSE.POG | 2 | 2 | 2 |
| D:\FEGFX\SLIC | /FEGFX/SLICK12.POG (+4) | 9 | 13 | 13 |
| D:\FEGFX\TITL | /FEGFX/TITLE.POG | 1 | 13 | 13 |
| D:\FEGFX\VS.P | /FEGFX/VS.POG | 2 | 8 | 8 |
| D:\FIFA96.EXE | /FIFA96.EXE | 2 | 402 | 402 |
| D:\SOUND\CHN_ | /SOUND/CHN_BRA2.BNK (+2) | 5 | 19 | 19 |
| D:\SOUND\CRD_ | /SOUND/CRD_CRD0.CRD | 1 | 4 | 4 |
| D:\SOUND\PAR_ | /SOUND/PAR_CLOS.PAR (+1) | 2 | 13 | 13 |
| D:\SOUND\PHR_ | /SOUND/PHR_FULL.VIV | 1 | 881 | 881 |
| D:\SOUND\PLR_ | /SOUND/PLR_T047.VIV (+1) | 2 | 105 | 105 |
| D:\SOUND\QUE_ | /SOUND/QUE_BASS.ASF (+1) | 4 | 3161 | 3161 |
| D:\SOUND\SFX_ | /SOUND/SFX_GAME.BNK | 1 | 11 | 11 |
| D:\SOUND\TEM_ | /SOUND/TEM_T047.VIV (+1) | 2 | 10 | 10 |
| D:\VIDEO\VID_ | /VIDEO/VID_CRED.TGV (+4) | 4 | 2092 | 2092 |
| MSCD001 | (device, not a file) | 1 | 0 | 0 |
| \FIFA96.EXE | (root-relative, no reads) | 2 | 0 | 0 |

Sample bindings (path@file-offset+length) confirm seeks as well as sequential
streams, e.g. `D:\FIFA96.EXE -> /FIFA96.EXE@0x00f474+6` (the last 6 bytes of the
MZ image) and `D:\FEGFX\SLIC -> /FEGFX/SLICK18.POG@0x000000+7294`.

## Verbatim parsed lines (quote-only)

```
HEADER magic=FCAP version=1 patches=5
PATCH_OK site=5aa1 target=0x00010551 siglen=4
PATCH_OK site=5aa8 target=0x00010558 siglen=4
PATCH_OK site=5f6e target=0x00010a1e siglen=3
PATCH_OK site=5f89 target=0x00010a39 siglen=4
PATCH_OK site=5f4b target=0x000109fb siglen=4
END
SUMMARY opens=58 reads=7036 writes=108 other=80
SUMMARY patch: ok=5 skip=0
SUMMARY end=END lost=0 seqgaps=0
```

Per-read evidence (file, offset, length, FNV confirmation) is the binding table
above; the session's FILE lines are reproducible from the trace.

## Notes

- 78 AH=3D records, 58 successful opens: 20 failed opens carry `flags=0` and no
  name, and are not counted as opens by the parser.
- `MSCD001` is the CD device handle; `\FIFA96.EXE` is a root-relative second
  open. Neither produced reads, so neither binds to an ISO file.
- Trailers in the table (`+N`) are extra ISO files reached through reopened,
  reused handles in the same session (e.g. `SLIC` opened 9 times across
  `SLICK12/18/25I/64/…`).
- Trace names are truncated to 13 bytes by the TSR's `pre_name` buffer (by
  design); the binding extends them to full ISO names.
- The in-repo golden `FW1.QFS` fixture corresponds to `/ART/FW1.QFS`; that file
  was not read in this session.

## FU-3 status (preliminary)

No captured session emits a CODEC record — boot, menu, and full-match sessions
all show `patch: ok=5` and zero CODEC lines. The five cited instructions never
executed. A probe build (release TSR plus two extra trap records at the
function entries) produced:

```
PATCH_OK site=?0x05 target=0x00010472 siglen=4    (dispatch_object_load 11bd:5992)
PATCH_OK site=?0x06 target=0x000108b2 siglen=4    (load_mf_object 11bd:5dd2)
CODEC site=?0x05 ... ip=5992 cs=0aae ...
```

`dispatch_object_load` is entered once during boot and returns early through its
`[0xf21] > 0` / `[0xcee] < 3` guards before the cited CMP; `load_mf_object` is
never entered. FU-3 (codec binding via trace) cannot be produced from the
current trap set. The next FU-3 slice must either retarget traps to the driver
calls (`11bd:2e80` / `11bd:2e98`, per the FU-3 refined-targets table) or close
the funnel as runtime-unexercised.

## Reproduce

```
make tsr
make capture                      # boot -> menus -> match -> quit
./build/fifa96_trace captures/session-*/trace.bin | head -6
python3 tools/fifa96_bind.py <trace.bin> --iso game/FIFAPCCD96.iso
python3 tests/test_bind.py        # also wired as CTest test_bind
```
