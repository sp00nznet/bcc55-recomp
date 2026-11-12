# Borland C++ 5.5, recompiled

Static recompilation of the **Borland C++ 5.5.1 Free Command-Line Tools**
(2000): the compiler `bcc32.exe` and the linker `ilink32.exe`, from their
shipping Win32 executables to native C. No emulator, no wrapper DLLs.

Built on the [pcrecomp](https://github.com/sp00nznet/pcrecomp) toolchain and
following its shared house style (layout, conformance harness), like the
other [pcrecomp projects](https://github.com/sp00nznet/pcrecomp#the-projects-that-built-this).

## What it is

pcrecomp's first compiler, and its first program built by a compiler other
than MSVC: both tools were built with Borland's own compiler and have the
Borland run-time library linked in (Borland's startup code, threadvars, its
own exception handling, hand-written assembly in the preprocessor). Each is
lifted to C by pcrecomp, compiled with MSVC, and run on pcrecomp's `native32`
host, which calls the real kernel32 and user32. It runs Borland's code; it is
not a rewrite.

A compiler is the easiest program there is to check: give the original and
the recompiled one the same input, and their output has to be the same bytes.

## Status: v0.1.0, alpha. It compiles and links, and its output matches the original byte for byte

| Stage | State |
|---|---|
| Version recompiled | Borland C++ 5.5.1 (`bcc32.exe` PE stamp `0x3989C322`, `ilink32.exe` `0x395BC444`) |
| Function catalog (`disasm32`) | bcc32 6,390 functions, ilink32 2,633 |
| Lift (`run_lift.py`) | bcc32 6,401, ilink32 2,635, 0 errors |
| Hosts (`build/bcc32recomp.exe`, `build/ilink32recomp.exe`, 32-bit, `native32`) | 70 and 81 imports bound to real Windows, 0 unresolved |
| Compile C and C++ (templates, the bundled STL, exceptions, x87, threadvars) | works |
| Link console and GUI programs, single- and multithreaded RTL | works |
| `.obj` output vs the original | **byte-identical**, 6/6 |
| `.exe` output vs the original | **byte-identical** with the clock stamps masked, 6/6 |
| Programs built by the recompiled tools | all 6 run and print the expected output |
| `bcc32` without `-c` (it starts `ilink32` itself) | runs the original `ilink32.exe`, not the recompiled one ([ROADMAP.md](ROADMAP.md)) |
| `RLINK32.DLL`, `LNKDFM50.DLL` (loaded by ilink32) | not lifted; they run as native code |
| Conformance harness | `tools/bcc_diff.py`: **6/6** IDENTICAL on a machine with the tools; SKIP without them |

Built and tested against pcrecomp branch `pcrecomp-bcc55` (on `pcrecomp-retailplus`).

### The test corpus (`tests/`)

| Program | What it exercises |
|---|---|
| `hello.c` | C, stdio, the command line |
| `fp.c` | x87 floating point, libm, `long double`, printf formatting |
| `templates.cpp` | class and function templates, `vector`/`string`/`map`/`sort` from the bundled Rogue Wave STL, iostreams |
| `except.cpp` | C++ exceptions: unwinding through destructors, rethrow, catch by base, Borland's `__try`/`__except` |
| `mt.c` | the multithreaded RTL (`cw32mt.lib`), `_beginthread`, `__thread` |
| `gui.c` | a Win32 GUI program (`c0w32.obj`, `-tW`): a window class, a window, timers and a message loop; never shown, so it runs headless |

## What is not in this repo

Nothing of Borland C++: no executable, header, library, and **no generated
source**. The recompiled C is produced on your machine from your own copy by
`run_lift.py`, into `src/recomp/bcc32/` and `src/recomp/ilink32/`, which are
gitignored. The test programs in `tests/` are this project's own.

## Getting Started

You need **your own copy of the Borland C++ 5.5 Free Command-Line Tools**
(`freecommandLinetools.exe`, a free registration download from Borland, later
Embarcadero, with no serial). Its installer puts them in `C:\Borland\BCC55`.
The installed folder is also on the Internet Archive as item
`BCC55PubliclyAvailableBorlandCCompiler5.5` (a zip of `BCC55\`, SHA-1
`44a7705e577e2d5c120f37f23c58cf176b84371d`). Nothing is downloaded for you.

### Quick start: Setup.cmd

1. Download this repository (the green **Code** button, then **Download ZIP**)
   and unzip it somewhere with 1 GB free.
2. Double-click **`Setup.cmd`**.

It checks for Python 3.10+, the `pefile` and `capstone` packages, the pcrecomp
toolkit and the Visual Studio 2022 C++ compiler, and **asks** before
installing any of them, saying what and how big. It finds Borland C++ in
`C:\Borland\BCC55`, or asks where it is (`Setup.cmd -GameDir <the BCC55
folder>` skips the question, `-Yes` answers yes to every install). Then it
copies the tools into `game\` (and `bcc32.exe` and `ilink32.exe` into
`work\`), finds every function, recompiles both tools, builds them and runs
the conformance harness. A rerun skips the finished steps. If it stops, it
says why in one sentence, and the details are in `setup.log`.

It ends with a **`BCC55 Recomp Prompt`** shortcut in the folder: a command
prompt with `bcc32recomp` and `ilink32recomp` on `PATH`.

### Step by step

Prerequisites: Windows 10/11, **Python 3.10+** (`py -3 --version`), **git**,
**Visual Studio 2022** (or its Build Tools) with *Desktop development with
C++*, and the pcrecomp toolkit cloned **beside** this repository as
`pcrecomp`, on branch `pcrecomp-bcc55`:

```
some-folder\
  pcrecomp\                     <- git clone -b pcrecomp-bcc55 https://github.com/sp00nznet/pcrecomp
  bcc55\                        <- this repository
```

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
4. Recompile to C:
   ```
   py -3 run_lift.py bcc32
   py -3 run_lift.py ilink32
   ```
   Expected: `lifted 6401 (+6 outside the catalog)   errors 0 ...` and
   `lifted 2635 (+1 outside the catalog)   errors 0 ...`.
5. Build (about 10 minutes; bcc32 is 1.7 million lines of C):
   ```
   build.cmd
   ```
   Expected: `Linking C executable bcc32recomp.exe` and `ilink32recomp.exe`.
6. Check it:
   ```
   build\bcc32recomp.exe
   ```
   Expected: `Borland C++ 5.5.1 for Win32 Copyright (c) 1993, 2000 Borland`
   and the option list.
7. The conformance harness:
   ```
   py -3 tools\bcc_diff.py
   ```
   Expected: six `IDENTICAL` lines and `6/6 identical`.

The usual trip-ups: `python` opening the Microsoft Store (that is Windows'
alias placeholder; use `py -3`, or turn the alias off in *Settings > Apps >
Advanced app settings > App execution aliases*); a freshly installed Python
not being on `PATH` until you open a new window; `run_lift.py` refusing a
different version (it checks the 5.5.1 PE stamps); and a toolkit on `main`
instead of `pcrecomp-bcc55`, which lifts a bcc32 that loops forever in its
preprocessor (`ITAIL: unresolved VA 0x0048E222`).

## Usage

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
only the tool's output.

The harness, `py -3 tools\bcc_diff.py [--keep] [name ...]`, builds each
program in `tests/` twice (original tools, recompiled tools) under
`out\diff\<name>\{orig,recomp}`, and compares the `.obj`, the `.exe`, the
tools' console output, and what each built program prints against
`tests\<name>.expect`. `--keep` leaves the files there.
[docs/bringup.md](docs/bringup.md) says what is masked and why.

## Building from source

Steps 3 to 5 above. `build.cmd` uses the newest Visual Studio it finds
(`vswhere`) and its 64-bit-hosted x86 compiler, configures CMake with Ninja
into `build\`, and builds both hosts. `CMAKE_ARGS=-DPCRECOMP=<path>` points
it at another toolkit checkout (the default is `..\pcrecomp`).

## License

MIT for this repository's code (the hosts, the lift driver, the harness and
the test programs); see [LICENSE](LICENSE) and [NOTICE](NOTICE). Borland C++
5.5 is (c) 1993-2000 Borland (later Inprise, CodeGear, Embarcadero); none of
it is included.
