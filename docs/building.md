# Building

`Setup.cmd` (`tools/setup.ps1`) runs exactly these steps, skipping any whose
output is already there. This is the same thing by hand.

## Prerequisites

Windows 10/11, **Python 3.10+** (`py -3 --version`), **git**, **Visual Studio
2022** (or its Build Tools) with *Desktop development with C++*, your own
**Borland C++ 5.5 Free Command-Line Tools** (the `BCC55` folder), and the
pcrecomp toolkit cloned **beside** this repository as `pcrecomp`, on branch
**`pcrecomp-bcc55`** (on `main` the lifted bcc32 loops forever in its
preprocessor, `ITAIL: unresolved VA 0x0048E222`; see bringup.md):

```
some-folder\
  pcrecomp\     <- git clone -b pcrecomp-bcc55 https://github.com/sp00nznet/pcrecomp
  bcc55\        <- this repository
```

The binaries must be 5.5.1 (`bcc32.exe` PE stamp `0x3989C322`, `ilink32.exe`
`0x395BC444`); `run_lift.py` checks.

## Steps

1. Python packages:
   ```
   py -3 -m pip install --user pefile capstone
   ```
2. Copy the `BCC55` folder (`Bin`, `Include`, `Lib`, ...) into `game\`, and
   the two tools into `work\` too:
   ```
   mkdir game work
   xcopy /e /y C:\Borland\BCC55 game\
   copy game\Bin\bcc32.exe work\
   copy game\Bin\ilink32.exe work\
   ```
3. The function catalogs (about a minute):
   ```
   py -3 ..\pcrecomp\tools\disasm\disasm32.py work\bcc32.exe -o work\bcc32.functions.json
   py -3 ..\pcrecomp\tools\disasm\disasm32.py work\ilink32.exe -o work\ilink32.functions.json
   ```
   Expected, at the end of each: `[*] Functions: 6390  (thunks=81, leaves=1332)`
   and `[*] Functions: 2633  (thunks=90, leaves=622)`.
4. Recompile to C, into `src\recomp\bcc32\` and `src\recomp\ilink32\`
   (gitignored):
   ```
   py -3 run_lift.py bcc32
   py -3 run_lift.py ilink32
   ```
   Expected: `lifted 6401 (+6 outside the catalog)   errors 0 ...` and
   `lifted 2635 (+1 outside the catalog)   errors 0 ...`.
5. Build (about 3 minutes; bcc32 is 1.7 million lines of C):
   ```
   build.cmd
   ```
   `build.cmd` uses the newest Visual Studio `vswhere` finds and its
   64-bit-hosted x86 compiler, and configures CMake with Ninja into `build\`.
   `set CMAKE_ARGS=-DPCRECOMP=<path>` points it at another toolkit checkout.
   Expected: `Linking C executable bcc32recomp.exe` and `ilink32recomp.exe`.
6. Check it:
   ```
   build\bcc32recomp.exe
   ```
   Expected: `Borland C++ 5.5.1 for Win32 Copyright (c) 1993, 2000 Borland`
   and the option list.
7. The conformance harness ([testing.md](testing.md)):
   ```
   py -3 tools\bcc_diff.py
   ```
   Expected: six `IDENTICAL` lines and `6/6 identical`.

The usual trip-ups: `python` opening the Microsoft Store (that is Windows'
alias placeholder; use `py -3`, or turn the alias off in *Settings > Apps >
Advanced app settings > App execution aliases*); a freshly installed Python
not being on `PATH` until you open a new window; the toolkit on `main`
instead of `pcrecomp-bcc55`; and ilink32 stopping with `Fatal: Illegal
option` when a path in its `-L` has a `-` in it (Borland's own linker does
the same): use a relative path, as `toolscc_diff.py` does.

## Using the hosts

The hosts take the tools' own command lines, unchanged:

```
set BCC55_BIN=C:\path\to\bcc55\game\Bin
bcc32recomp -c -IC:\path\to\bcc55\game\Include hello.c
ilink32recomp -LC:\path\to\bcc55\game\Lib -Tpe -ap -x -c -Gn c0x32.obj hello.obj, hello.exe, , import32.lib cw32.lib
hello
```

```
Borland C++ 5.5.1 for Win32 Copyright (c) 1993, 2000 Borland
hello.c:
Turbo Incremental Link 5.00 Copyright (c) 1997, 2000 Borland
hello from bcc 5050
```

Everything after the host's name goes to the tool, so the host's own settings
are environment variables:

| Variable | Meaning |
|---|---|
| `BCC55_BIN` | the folder holding the original `bcc32.exe`/`ilink32.exe` (default `..\game\Bin` from `build\`). The tool reads its `.cfg` from there, and ilink32 its DLLs |
| `BCC55_WATCHDOG` | seconds before a run is stopped (exit code 4), saying where it was |
| `BCC55_TRACE` | `1`: every call into Windows on stderr |
| `BCC55_DIAG` | pcrecomp `recomp_trace` options, such as `--calltrace t.txt`, for a build configured with `-DBCC_TRACE=ON` |

The host prints one line of its own, `[bind] ...`, on stderr; stdout carries
only the tool's output. The `BCC55 Recomp Prompt` shortcut Setup makes sets
`PATH` and `BCC55_BIN` for you.

## Linux and macOS

Not yet: `setup.sh` says so and exits. The hosts are MSVC-built and run on
`native32`, which binds the tools' imports to the real Windows DLLs.
