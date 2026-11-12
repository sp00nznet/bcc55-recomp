# Notice

The MIT licence in [LICENSE](LICENSE) covers what is ours in this repository:
the hosts, the lift driver, the setup scripts, the conformance harness, the
test programs in `tests/` and the documentation. It does not cover Borland
C++.

*Borland C++ 5.5* is © 1993-2000 Borland (later Inprise, CodeGear and
Embarcadero), and is not ours to license. Its executables, DLLs, headers,
libraries and help files are not in this repository and are excluded by
`.gitignore`.

- You need your own copy of the Borland C++ 5.5 Free Command-Line Tools to
  use any of this. Setup copies your `BCC55` folder into `game/`; the
  pipeline reads it from there.
- Lifted code is a derivative work of the binary it came from. Anything the
  pipeline writes into `work/` or `src/recomp/bcc32/` and `src/recomp/ilink32/`
  carries whatever licence the original carries, not this one. The MIT grant
  stops at our own work.
- The host and lift driver started from the AmazingMIDI recomp's (themselves
  from the MechWarrior 3 and Bully recomps), all ours and MIT. There is no
  third-party code in this repository.
