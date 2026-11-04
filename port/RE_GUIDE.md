# Reverse-engineering guide — FIFA 96 (DOS, 1995)

`FIFA96.EXE` is a 16-bit real-mode loader with a 32-bit protected-mode LE image
appended to the same file. The loader spine is documented in
`spec/loader_rename_map.md`; the LE-image derivations are the `spec/FU*.md`
records.

The Ghidra project is `../data/fifa96.rep/` (git-ignored), opened as project
`fifa96`. The program argument is exactly `/FIFA96.EXE` — case-sensitive.
Ghidra scripts are disabled; use the standard MCP tools only.

Address conventions:

- loader-map rows quote `11bd:`-segment addresses; `1000:` gap rows use the
  fixed delta `−0x1BD0`;
- LE derivations use flat image addresses;
- runtime mapping uses the load delta `0x1FC000`.
