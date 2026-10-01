# FU-8: the resource loader chain (protected-mode image)

Follow-up to FU-5/FU-6. Link-time addresses; runtime adds the stable linear
delta `0x2D1000` (physical placement varies per run, see FU-7).

## The chain

```
resource name
  -> FUN_000c9d10(name, type, flags)      0xC9D10
       FUN_000cab30(name, buf, ext)       build filename (extension table @0x4CB8)
       FUN_00099924(buf)                  cache lookup
       FUN_000cad30(buf, type, flags)     load + decode
          FUN_000cd595(...)               open (handle, size)
          FUN_000cac10(...)               probe: seek (FUN_000cd740), read up to
                                          0x100 bytes (FUN_000cd6f0), FUN_0009e890
          FUN_000cd6f0(handle, dst, n)    chunked read: INT 21h AH=3F in a
                                          0x4000-byte loop
          FUN_0009e890(buf)               raw-vs-compressed selector
          if 0: FUN_000cd390(buf, dst, n) raw copy
          else: FUN_0009e718(buf, dst,..) decompress (the FU-5 kVGT command
                                          stream engine)
          FUN_000cd6dc(handle)            close
  FUN_000c570b / FUN_000c5735             thin wrappers around FUN_000c9d10
```

Supporting loaders:

* `FUN_0009777c` — VFS table read: entry stride `0x24` at `0x111DC`; type 2
  reads through `FUN_000cd6f0`, otherwise from a cached buffer
  (`FUN_00096415`, advancing a per-entry offset at `+0x10`).
* `FUN_0009a0a8` — whole-file load into memory (`FUN_000cd595` + `FUN_0009e890`
  + `FUN_00098cd4`, close via `FUN_000cd6dc`).
* `FUN_0009e890` — reads a header field via `FUN_0009e3ec`/`FUN_0009e420`; when
  `0 < value < 0x1F` it returns a **big-endian 24-bit field at offset +2** (the
  same encoding `FUN_0009e718` uses for its literal-copy length). Non-zero
  selects the decompressor.

## Why the format tags were invisible statically

`FUN_000c570b` and `FUN_000c5735` have **no direct callers**: the resource
layer is reached through function-pointer tables. The POG/VIV/QFS payloads are
loaded by this generic path (raw or `FUN_0009e718`-decompressed) and then
parsed by registered consumers, so no `PCNX`/`SHPI` compare exists anywhere.
The two stdio wrappers (`FUN_000bafb1`, `FUN_000cd5d6`) are not on the asset
path; `FUN_000cd6f0` is.

## Runtime recorder experiment (patched EXE)

Method: trampolines patched into a working copy of `FIFA96.EXE` inside the ISO
(restored afterwards, sha256 verified) record the last 8 caller stacks plus a
per-page magic into object-4 BSS; run headless (`-silent`) and scan host RAM
dumps for the magic. The linear delta stayed `0x2D1000` in every run.
Outcome: the game reaches its video mode and the TSR captures 3385 `AH=3F`
reads, but none of the patched high-level wrappers fired — the reads observed
headless come from a path not covered (extender page-ins), and the asset reads
of a match need interactive input the headless run cannot provide. The
trampoline must not be placed past an object's virtual size (execution there
faults); the only in-range zero cave in object 1 is at link `0x6728D`.
