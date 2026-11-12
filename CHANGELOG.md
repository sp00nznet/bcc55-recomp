# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-11-12

### Added
- `bcc32.exe` and `ilink32.exe` from Borland C++ 5.5.1 lifted to C (6,401
  and 2,635 functions, 0 errors) on pcrecomp's native32 host, one host per
  tool (`build/bcc32recomp.exe`, `build/ilink32recomp.exe`). The host passes
  its command line to the tool unchanged; its settings are environment
  variables (`BCC55_BIN`, `BCC55_WATCHDOG`, `BCC55_TRACE`, `BCC55_DIAG`).
- `run_lift.py <tool>`: lifts with `computed_ret` and seeds the arms of
  `push [table + reg*4]; ret` jumps (Borland's hand-written lexer).
- `tools/bcc_diff.py`, the conformance harness: six test programs (`tests/`:
  C, x87, templates and the STL, exceptions, the multithreaded RTL, a Win32
  GUI program) compiled and linked by the original and the recompiled tools.
  6/6 byte-identical `.obj` and `.exe` (clock stamps masked), and every
  program runs and prints what `tests/*.expect` says.
- `Setup.cmd` (`tools/setup.ps1`): one-click setup from your own copy; ends
  with a `BCC55 Recomp Prompt` shortcut.
- `launcher/launcher.json`, `docs/bringup.md`.

Needs pcrecomp branch `pcrecomp-bcc55` (jump tables indexed by a negative
signed char).
