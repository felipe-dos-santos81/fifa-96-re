# FU-7: runtime trace harness plan

Goal: reach the POG/VIV/QFS decode entry points, which have no static tag
dispatch (see `FU6_manifest_and_io.md`), by breaking on the file-read wrappers
while the game reads those files and walking the return addresses.

## Step 1 — runtime addresses (done)

A guest-RAM dump taken while the game runs places the loaded image at
runtime = link + **0x1FC010** (entry-page banner `0x9FD12` -> physical
`0x29BD22`, entry `EB 76` at `0x29BD20`, tail bytes match the file).

| anchor | link | runtime |
| --- | --- | --- |
| entry | `0x9FD10` | `0x29BD20` |
| vgt_stream_poll | `0x67BA8` | `0x263BB8` |
| vgt_dispatch | `0xAE4BC` | `0x2AA4CC` |
| file_open_ro | `0xBABE0` | `0x2B6BF0` |
| game read wrapper | `0xBAFB1` | `0x2B6FC1` |
| game read `INT 21h` | `0xBAFB8` | `0x2B6FC8` |
| Watcom read `INT 21h` | `0xCD666` | `0x2C9676` |

`tools/fifa96_runtime.py DUMP` recovers the delta from any fresh dump and
prints the table (anchor checks: entry bytes; object 2/3 only if their pages
are resident).

## Step 2 — break and snapshot

The DOSBox-X debugger supports `BPLM <linear>` (breakpoint on a linear
address) and `MEMDUMPBIN <seg> <off> <count>` (writes `MEMDUMP.BIN` in the
working directory). The intended sequence, with the game already running:

```
BPLM 0x2B6FC8          # game-layer INT 21h read call site
BPLM 0x2C9676          # Watcom runtime INT 21h read call site
RUN
# on the first hit, inspect the PM stack (SS:ESP) and dump it:
MEMDUMPBIN SS ESP 8192
```

At the hit the client is in protected mode, so `[SS:ESP]` and the words above
it are return addresses into the calling decoder/loader; after the dump, walk
them against the link-time image (`addr - 0x1FC010`).

Environment notes from this host:

* DOSBox-X has to be the capability-free copy (`/tmp/opencode/dosbox-x-nocap`)
  to keep the process dumpable and `LD_PRELOAD`/ptrace usable; the packaged
  binary carries `cap_net_raw=ep`.
* There is a live X11 display, but no window manager: `xdotool
  windowactivate` cannot mark the window active (`_NET_WM_ACTIVE_WINDOW`
  unsupported). `xdotool windowfocus` + XTEST keys is the fallback, and the
  debugger hotkey is `Alt+Pause`; `-break-start` avoids the hotkey entirely
  (break at power-on, then set `BPLM` and `RUN`).
* Do not use `pkill -f dosbox` from the same shell command that mentions the
  pattern: it matches the shell itself.
* A suspended debugger leaves guest RAM intact, so the host-side dump
  (`process_vm_readv`, as in the FU-4 cross-check) is an alternative to
  `MEMDUMPBIN` when the in-debugger command syntax is inconvenient.

## Automation attempt and blocker (recorded)

Synthetic key injection into the DOSBox-X window was tried from this host with
xdotool (XTEST and XSendEvent, `-break-start` and the `Alt+Pause` hotkey,
`MEMDUMPBIN 0 0 100` as the probe): no keystroke reached the SDL debugger.
The X session runs mutter, and the DOSBox client window is either unmapped or
its frame holds the input focus (`xdotool windowfocus` returns failure and
`getwindowfocus` stays on the frame), so both the hotkey and the command line
are unreachable. The command form itself is fine.

The delta is deterministic across runs (`0x1FC010`, checked in two independent
guest-RAM dumps), so the breakpoint addresses in Step 1 are stable. Step 2
needs a real input path (run the three commands at the console) or the
patched-EXE variant (record `[ESP]` at the read wrapper into obj4 BSS and
recover the caller chain from a host-side RAM dump).
