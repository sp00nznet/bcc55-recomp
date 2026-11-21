# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.1.0] - 2026-11-20

### Added
- `CONTRIBUTING.md`, `CONTRIBUTORS.md`, `docs/building.md` (the setup steps
  by hand, the hosts' settings) and `docs/testing.md` (the harness).
- `setup.sh`, which says there is no Linux or macOS build yet and exits.
- `.github/workflows/installer.yml`, on push and v* tags.

### Changed
- Builds against pcrecomp main; the `pcrecomp-bcc55` branch is merged.
- README in the house order: Getting Started, Status (what the harness
  proves), Screenshots, Documentation, Contributors, License.
- `NOTICE` is now `NOTICE.md`, saying what the MIT licence covers and what it
  doesn't.

### Fixed
- `tools/bcc_diff.py` failed 0/6 in any folder whose path has a `-` in it
  (a ZIP download unpacks to `bcc55-recomp-main`): ilink32, the original
  too, reads the `-` in an absolute `-L` path as a switch (`Fatal: Illegal
  option`). The library path now goes in relative to the output folder.
  Found by the fresh-clone Setup test.

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
