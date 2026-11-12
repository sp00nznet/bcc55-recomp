# Bring-up notes

How the two tools went from lifted to byte-identical, in order. Each fix
names the address it was found at, so a later lift can be checked against it.

## Getting the files

The archive.org item `BCC55PubliclyAvailableBorlandCCompiler5.5` is a zip of
an installed `BCC55\` folder (SHA-1 `44a7705e...371d`, SHA-256
`8944076e...f7cc`), not the original `freecommandLinetools.exe`. CMake's CI
(`.gitlab/ci/borland.ps1`) pins a SHA-256 for BCC 5.5, but for its own
repackaged zip (`bcc5.5-1.zip`, internal to Kitware), so it can't check this
one. The Wayback copy of the installer was unreachable at the time. The
binaries identify themselves as 5.5.1 (`bcc32` banner, PE stamps
`0x3989C322` and `0x395BC444`), the version Borland's patch made, and
`run_lift.py` checks those stamps.

| File | SHA-256 |
|---|---|
| `bcc32.exe` | `e7f7853e8c71b120839fab6505516799ec0d4bd9ae0797dcf00a1a6656ab1e5b` |
| `ilink32.exe` | `ba9f744ed5328a7e76f54f2bbf0064cae72103da157e66cf1b4751c9c1f8d5b0` |

## The two programs

Both are single PE32 console executables at `0x00400000` with the Borland RTL
linked in statically: no `cc3250.dll`, only kernel32, user32, advapi32 and
ole32. Both have a `.tls` section (Borland threadvars), which native32
already maps. Since both want the same base address, each gets its own host
(`CMakeLists.txt` builds `host.c` twice with `BCC_TOOL` set).

Both banners worked on the first run.

## bcc32

1. **Jump tables indexed by a negative signed char** (toolkit,
   `generate._jump_table`). The preprocessor's lexer is hand-written assembly:
   `movsx eax, byte [class + c]; and al, al; jg ...; jmp cs:[eax*4 + 0x48DA3B]`,
   so the index runs from 0 down to -34 and the arms sit *below* the table
   base. The toolkit scanned 64 bytes down (16 arms; enough for the CRT
   memcpy it was written for), so arm -20, `0x0048E222`, was never a label:
   every `#include` looped forever on `ITAIL: unresolved VA 0x0048E222`. The
   scan now goes down 128 entries (the reach of a signed char).
2. **`push [table + reg*4]; ret` computed jumps** (`run_lift.py`). The number
   lexer at `0x0048EADF` picks its suffix handler with
   `push dword cs:[eax*4 + 0x48EA7B]; mov al, [esi-1]; ret`. Lifted as a
   plain return, the handler never ran, and the caller resumed with `ebx`
   zeroed: a read of `0x45` in `sub_0048DC85`. `run_lift.py` now lifts with
   `computed_ret=True` (a `ret` to an address the caller didn't push is a
   jump) and seeds the arms of every such table (`push_tables`: 5 arms in
   bcc32, none in ilink32).

After those two, all six test programs compiled to the same `.obj` bytes as
the original. Nothing in a Borland `.obj` comes from the clock: the
dependency records hold the source and header files' own time stamps, so
the harness copies each source with its mtime (`shutil.copy2`) and the `.obj`
files compare as they are.

## ilink32

1. **An indirect call to `0x004264B0`**, which the catalog missed (a function
   pointer). Seeded in `seeds/ilink32.json`.
2. **Large-address-aware hosts hang ilink32.** At `0x0040D654` it looks for
   room for its heaps: it walks `VirtualQuery` for the largest free region
   below `0xFFFF0000`, then aligns its base up by powers of two in a loop
   (`0x0040D6CA`). A large-address-aware process sees free space above 2 GB,
   so the power of two wraps to 0 and the loop never ends. The originals are
   not large-address-aware, so the hosts aren't either
   (`/LARGEADDRESSAWARE:NO`).
3. **`RLINK32.DLL` and `LNKDFM50.DLL`.** ilink32 loads them by bare name,
   which Windows looks for next to the exe: the host's folder. The host calls
   `SetDllDirectoryA(BCC55_BIN)`. They are not lifted; they run as native
   code, and only `RLINK32.DLL` (the resource linker) does any work, when
   `.res` files are linked.

## What the harness masks

`ilink32` stamps the output with the current time in three kinds of place:
the COFF header's `TimeDateStamp`, the export directory's (Borland programs
export `__GetExceptDLLinfo`), and every resource directory in `.rsrc` (one
per level). `tools/bcc_diff.py` zeroes exactly those with pefile before
comparing; with them masked, the original and recompiled linkers write the
same file. Without masking, a link that falls in the same second already
matches (it happened with `fp.exe` once). A fixed clock in the host would
only fix one side, so the harness masks instead.

`-Gn` keeps ilink32 from writing its incremental-link state files
(`.ilc`, `.ild`, ...), and `-x` from writing a map, so each run starts clean.
