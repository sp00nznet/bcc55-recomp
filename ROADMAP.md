# Roadmap

## Done
- `bcc32` and `ilink32` lifted with 0 errors, on pcrecomp `main`.
- `tools/bcc_diff.py`: 6/6 programs byte-identical to the originals' output.
- `Setup.cmd`, from the ZIP download to a prompt shortcut.

## Next
- `bcc32` without `-c` links by starting `ilink32.exe` from its own folder,
  so today that step runs the original linker. Redirect it (a `CreateProcessA`
  shim that starts `ilink32recomp.exe`), and add a one-step `bcc32 hello.c`
  case to the harness.
- More of the corpus: optimization switches (`-O2`, `-6`), debug info (`-v`,
  which makes ilink32 write a `.tds`), a DLL (`-tWD`, `-Tpd`), a `.res` from
  `brcc32` linked in (the RLINK32 path), a `.lib` built with `tlib`, and
  compiling Borland's own `Examples\`.
- Track the harness figure over time and fail on regression (REPO_RULES
  section 9): a `conformance.json` baseline, like hover's.
- A netlab test-VM run of the harness (the recipe is
  `projects/bcc55-recomp.env` in recomp-netlab).
- A Linux build (`setup.sh` exits today): pcrecomp's win32hle under it, or
  the Windows build under Wine.

## Then
- Lift the other tools in `Bin\`: `cpp32` (the preprocessor), `tlib`,
  `implib`, `brcc32`, `tdump`, `make`, and the DLLs ilink32 loads
  (`rlink32.dll`, `lnkdfm50.dll`), so no original code runs natively.
- The 107 bcc32 and 30 ilink32 functions whose extent walk found no
  terminator: check none of them is reached.
- Move `push_tables` (the push/ret jump-table seeding in `run_lift.py`) into
  pcrecomp once a second title needs it.

## Out of scope
- Changing what the compiler does. The point is the original's output, byte
  for byte.
- A headless/record mode (REPO_RULES section 10): these are console tools
  with no display; the GUI test program never shows its window.
