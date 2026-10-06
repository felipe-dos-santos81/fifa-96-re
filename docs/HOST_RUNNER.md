# Host runner (`make run`)

`tools/fifa96_play.c` is the single host executable behind `make run`. It has
four modes — explicit `video`, `audio`, `sprite`, and `auto`, where `auto` is
also the default whenever the first argument is a bare `FILE` path. It adds no
dependencies; all decoding goes through the `fifa96_*` libraries.

```bash
make build
./build/fifa96_play --help
make run ARGS="--help"                  # equivalent
make run FILE=game/FIFAPCCD96.iso       # flagship: auto mode on the real CD image
```

`make run FILE=PATH` runs `fifa96_play auto PATH`; `make run ARGS="..."` runs
the tool directly (use `ARGS` when you also need flags: `make run ARGS="auto
game/FIFAPCCD96.iso --list"`). Outputs are never written outside `build/` or
the path passed to `--out`.

## Auto: sniff any input (`auto FILE`, the `make run` default)

```bash
fifa96_play auto FILE [--list] [--class video|audio|sprite|crd] [--max-frames N]
               [--entry N | --name NAME] [--modex] [--all] [--out DIR]
```

`auto` sniffs the input and does the useful thing without a mode flag:

* **ISO9660 image** (PVD `CD001` at sector 16) — walks the directory tree with
  `fifa96_iso9660` and prints a deterministic class table
  (`video` = `/VIDEO/*.TGV`, `audio` = `/SOUND/*.BNK` + `/SOUND/*.VIV`,
  `crd` = `/SOUND/*.CRD`, `sprite` = `/ART/*.PVI`), then runs a **bounded smoke
  pass**: it decodes the first asset of each class through the ported pipeline
  and writes outputs under `--out` (default `build/run/`):
  * video → `OUT/video/<name>/frame-NNNN.ppm` for the first `--max-frames`
    frames (default 3); `--modex` writes `.modex` planes instead;
  * audio → the first `.BNK`'s id list plus its first entry (or `--entry ID`)
    as `OUT/audio/<name>.wav`; the first `.VIV` is listed (records + first
    names/sizes) and, with `--name NAME`, that record is decoded to WAV;
  * crd → `fifa96_crd` summary (version/tracks/events) of `/SOUND/CRD_*.CRD`;
  * sprite → the first `/ART/*.PVI` BIGF entry listing and the first decodable
    SHPI frame as `OUT/art/<name>.ppm`.
  Every line of the summary table is stable: source path, size, decoded
  geometry/format and output paths plus sha256 values. `--class` restricts the
  pass; `--list` prints only the tree and class counts; `--all` widens the pass
  but caps at 8 assets per class and says so on stdout.
* **container file** — detects `0xFB10` envelope, QFS, POG, `kVGT` TGV,
  VIV/BIGF/BNK/CRD/EACS/PVI with the shared `tools/fifa96_detect.c` sniff,
  prints the recognized headers, and decodes where the port can: `.TGV` →
  frames, `.BNK` → list + first-entry WAV, `.PVI` → sprite PPM, `.CRD` →
  summary, raw `EACS` → WAV; `0xFB10` records are unwrapped up to four codec
  layers before dispatch (e.g. `fw1.qfs` → SHPI → PPM). Unknown data prints
  the first 16 bytes.

The smoke pass is deterministic: the class table and the first-asset choice
are path-sorted, so the same input always yields the same outputs and hashes.

## Video: raw `.TGV` chunk stream -> PPM / Mode-X

```bash
fifa96_play video FILE [--max-frames N] [--frame K] [--out PATH] [--modex] [--print-summary]
```

* `FILE` is a raw TGV chunk stream (`[u32 tag][u32 length][payload...]`, as in
  `VIDEO/*.TGV`), decoded with `fifa96_tgv_walk` + `fifa96_vgt_player`
  (kVGT keyframes, fVGT deltas, rewind/skip sentinels).
* Each presented frame is written as a binary P6 PPM by palette lookup
  (256-triple palette from the last kVGT keyframe).
* `--modex` writes the raw 4-plane Mode-X image through `fifa96_blit_modex`
  instead (planes of `height * 80` bytes, in plane order).
* With no `--frame`, all frames are written into the `--out` directory
  (default `build/play`) as `frame-NNNN.ppm` / `frame-NNNN.modex`.
* `--frame K` writes only frame `K`; `--out` is then a file (or a directory,
  in which case `frame-NNNN.ext` is appended). `--frame` and `--max-frames`
  are exclusive.
* `--print-summary` prints one line per written frame:
  `frame 2: 320x240 ppm sha256=<hex>` (or `modex`).
* Any malformed chunk, a partial frame, or a missing `--frame K` exits
  non-zero with a message on stderr.

There is no committed raw `.TGV` stream fixture; the CTest wrapper
(`tests/test_play.py`) generates `build/play/test-video.tgv` from the
committed kVGT/fVGT vectors, so a working example is:

```bash
python3 tests/test_play.py -k sequence          # builds build/play/test-video.tgv
./build/fifa96_play video build/play/test-video.tgv \
    --print-summary --out build/play/seq
```

## Audio: EACS payload / `.BNK` entry / BIGF `.VIV` entry -> WAV

```bash
fifa96_play audio FILE [--out PATH] [--print-summary]
fifa96_play audio --bnk FILE --id N [--out PATH] [--print-summary]
fifa96_play audio --viv FILE (--name NAME | --index N) [--out PATH] [--print-summary]
```

* `FILE` is a raw EACS payload (32-byte header + PCM), e.g.
  `tests/golden/eacs/bank-h.eacs`.
* `--bnk FILE --id N` selects SFX id `N` (0..127) from a `SOUND/*.BNK`
  descriptor table; `--viv FILE` selects a record from a BIGF `.VIV` bank by
  `--name` or 0-based `--index`.
* Output is a 16-bit PCM RIFF WAV at `--out` (default
  `build/play/audio.wav`) using the declared rate and channels. All proven
  formats decode: PCM16 stereo/mono, signed PCM8 stereo, and the `f10==2`
  adaptive-delta stereo/mono arms.
* `--print-summary` prints
  `audio: frames=8741 rate=16000 channels=1 sha256=<hex>`.
* An unreadable container, an absent id/name/index, an unsupported EACS
  format or a truncated header exits non-zero with a message on stderr.

```bash
./build/fifa96_play audio tests/golden/eacs/bank-h.eacs \
    --out build/play/bank-h.wav --print-summary
./build/fifa96_play audio --bnk tests/golden/sfx_game.bnk --id 1 \
    --out build/play/bnk1.wav --print-summary
./build/fifa96_play audio --viv tests/golden/eacs/bank-t019.viv --name tmt01901.spc \
    --out build/play/t019.wav --print-summary
```

## Sprite: BIGF `.pvi` entry -> SHPI bank frame -> PPM

```bash
fifa96_play sprite FILE [--entry N | --name NAME] [--frame K] [--out PATH]
                         [--palette FILE] [--print-summary]
```

* `FILE` is a sprite bank container: a raw `BIGF`/`SHPI` file, or a
  codec-wrapped record (`[selector, 0xFB, BE24 size, payload]`). The record
  chain is followed to its SHPI bank with `fifa96_record_decode`
  (refpack/huff/tree, any nesting) and `fifa96_sprite` (FU-86). `PLAYART.PVI`
  is refpack -> BIGF, whose `.qfs` entries decode huff(`0x31`) ->
  refpack(`0x10`) -> tree(`0x46`) -> SHPI; `tests/golden/gameart0.pvi` is
  refpack -> BIGF with raw `.fsh`/one-stage `.qfs` entries.
* `--entry N` (default 0) or `--name NAME` selects the BIGF record; `--frame K`
  (default 0) selects the SHPI frame.
* The frame is written as a P6 PPM at `--out` (default
  `build/play/sprite.ppm`). Without `--palette` the 8-bit pixel indices are
  written as grayscale; `--palette FILE` supplies an RGB table and the first
  768 bytes are used. The frame's optional second chunk (palette candidate,
  FU-86 open leg 1) is not consumed.
* `--print-summary` prints
  `sprite: entries=91 entry=4 name=jump.qfs frames=30 frame=0 36x36 ppm sha256=<hex>`.
* A missing entry/name/frame, a palette shorter than 768 bytes, or a record
  that does not reach SHPI exits non-zero with a message on stderr. The game's
  huff decoder over-reads the record for 10 `.qfs` entries (`stumble`,
  `kneesld`, `vollkcka`, `gldsa`, `dummyrfa`, `collairb`, `duckflpa`,
  `duckflpb`, `bodychk`, `xrready`; FU-86 §3 caveat/leg 9); the port is
  bounds-checked and rejects those exact slices.

```bash
./build/fifa96_play sprite tests/golden/gameart0.pvi --name Net.fsh \
    --frame 0 --out build/play/net.ppm --print-summary
# ART/PLAYART.PVI lives in the read-only ISO; extract the extent once:
python3 -c "import sys; sys.path.insert(0,'tools'); import fifa96_bind as b; \
e,d=b.iso_files('game/FIFAPCCD96.iso'); o,s=e['/ART/PLAYART.PVI']; \
open('build/play/playart.pvi','wb').write(d[o:o+s])"
./build/fifa96_play sprite build/play/playart.pvi --name jump.qfs \
    --frame 0 --out build/play/jump.ppm --print-summary
```

## Tests

`tests/test_play.py` (CTest `test_play`) builds a temp 2-chunk stream from the
committed kVGT/fVGT vectors and checks PPM/Mode-X bytes against independently
computed expectations, the raw EACS-to-WAV bytes against the fixture PCM, and
the BNK/VIV decode headers plus sample pins and sha256 values pinned from
`tests/test_bnk.c` / `tests/test_bigf.c`. It also covers malformed inputs.
The sprite cases build a minimal BIGF+SHPI fixture at runtime and check the
grayscale/palette PPM bytes, `--entry`/`--name`/`--frame` selection and the
error paths exactly; the committed `tests/golden/gameart0.pvi` covers a real
raw `.fsh` bank and a nested `.qfs` record, and when
`game/FIFAPCCD96.iso` is present the suite extracts `/ART/PLAYART.PVI` and runs
the full 3-stage chain (`jump.qfs`, pinned 30 frames) plus the `xstandd.fsh`
geometry pin (20×49). `TestAutoContainer` exercises `auto` over the committed
fixtures (TGV/EACS/BNK/CRD/PVI/QFS), and the CTest `test_iso9660` target
unit-tests the ISO9660 reader on a synthetic in-memory image under
ASan/UBSan. When `game/FIFAPCCD96.iso` is present, `TestAutoIso` pins the ISO
class counts and the bounded smoke-pass outputs (video frame, BNK WAV, sprite
PPM sha256 values). Run it directly with `python3 tests/test_play.py` (the
tool must be built).
