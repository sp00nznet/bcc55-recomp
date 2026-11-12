# Contributing

Issues and pull requests are welcome.

## The one rule

**Nothing from Borland C++ goes in the repository.** No executables, DLLs,
headers or libraries (`game/`), no lifted C (`src/recomp/bcc32/`,
`src/recomp/ilink32/`), no catalogs (`work/`), no disassembly listings, and no
`.obj` or `.exe` the tools wrote (`out/`). The tool ships; its output is made
on each person's machine from their own copy. `.gitignore` covers the usual
paths; check `git status` before you commit anyway.

## Where your code comes from

Contributions must be your own or MIT-compatible. Don't port code from a GPL
or LGPL project (Wine, binutils, a GPL compiler or linker); describing what a
Borland tool does from its own behavior and output is fine.

## Before a pull request

1. Build from a clean lift: `py -3 run_lift.py bcc32` and
   `py -3 run_lift.py ilink32` (expected: `errors 0`), then `build.cmd`.
2. Run the conformance harness: `py -3 tools\bcc_diff.py`. It must print
   `6/6 identical`. A new behavior comes with a program in `tests/` and its
   `.expect` file that shows it.

## Where a fix belongs

- A bug in the lifter, the catalog or `native32` belongs in
  [pcrecomp](https://github.com/sp00nznet/pcrecomp), not here.
- A fix specific to these two tools is a seed or a patch in `run_lift.py`, or
  a shim in `src/runtime/host.c`, with a comment that says what it fixes and
  why.
- A wall you got past goes in `docs/bringup.md`: what it looked like, what it
  was, what moved it.
