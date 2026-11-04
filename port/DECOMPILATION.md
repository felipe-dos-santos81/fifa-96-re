# Decompilation

The Ghidra project lives in `../data/fifa96.rep/` (git-ignored) and is opened as
project `fifa96`. The 16-bit loader is analysed as `x86:LE:16:Real Mode`, entry
`11bd:2382`; the appended 32-bit LE image is flat. Ghidra scripts are disabled;
analysis uses the standard MCP tools.

Human-readable derivations are recorded under `spec/` (`loader_rename_map.md`
and the `FU*.md` files). No generated decompiled C is committed.
