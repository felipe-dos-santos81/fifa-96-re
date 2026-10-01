# FU-10: in-guest DPMI trace probe (plan)

Static analysis is exhausted for the resource path: dispatch and extension
tables are runtime-built, manifest strings have no code xrefs, and the chain
from the game I/O layer ends in the Watcom stdio FILE table (`0x158C0`,
stride `0x1A`). The remaining question is which game code calls the loader and
decompressor for each format; that needs call-site evidence at run time.

The DOSBox-X debugger cannot be automated on this host (xdotool cannot focus
the client window; windowed mode currently hangs after the WM frame helper
was killed). This plan avoids the debugger entirely by tracing from inside the
guest, reusing the proven serial capture rig.

## Method

1. **PM trampoline** in the obj1 cave at link `0x6728D` (the only in-range
   zero area): overwrite a function entry with `call cave`, save the return
   address (`[esp]`) and a site id, then execute the overwritten prologue and
   `jmp` back.
2. **Real-mode notification** via DPMI `INT 31h AX=0300h` (simulate real-mode
   interrupt): the cave prepares a 48-byte real-mode register structure in an
   obj4 BSS buffer (e.g. `0x162000`), puts a signature and the captured values
   in EAX/EBX/ECX/EDX, sets `BL=0x60`, and calls the service. Absolute
   references in the cave must use link + linear delta `0x2D1000` (the value
   verified from the relocated operand at `0x9FD8E`); the dump-space delta
   `0x1FC010` is physical placement only and must not be used here.
3. **TSR v2** adds an `INT 60h` handler that writes the passed words plus its
   own CS:IP to COM1 in the existing frame format; the `AH=3D`/`AH=3F`
   capture stays as-is.
4. **Targets, in order**: `FUN_0009e718` (decompressor; guaranteed to run for
   compressed assets), `FUN_0009e860`, `FUN_000c9d10`, `FUN_000cad30`. Move on
   only after a signature appears on the wire for the current target.
5. **Verification**: decompress the serial stream; require signatures with
   return addresses inside the code object (`0x10000..0xD1EA0`), then
   cross-check each against the static caller lists in FU-8/FU-9.

## Risks / fallbacks

* DOSBox-X may not implement `INT 31h AX=0300h`; if the cave faults or the
  signature never arrives, fall back to a PM-side ring buffer in obj4 BSS
  that the TSR drains on the next `INT 21h` file call (extender-serviced calls
  are known to reach the TSR).
* The cave must not clobber registers the target prologue needs; save/restore
  all volatile state and keep the instruction count minimal.
* ISO patching is done in place and restored by sha256 (as in the daytrace
  experiment); keep the pre/post hashes.

## Environment prerequisites

* Headless DOSBox-X (`-silent`) is sufficient; the interactive debugger is
  not needed for this plan.
* Windowed DOSBox-X remains unusable until the WM frame helper is restored
  (full session/WM restart); that only matters for the fallback debugger path.
