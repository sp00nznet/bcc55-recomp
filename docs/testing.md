# Testing

## The conformance harness

```
py -3 tools\bcc_diff.py [--bin game\Bin] [--recomp build\bcc32recomp.exe] [--keep] [name ...]
```

It builds each program in `tests/` twice, once with the original tools in
`game\Bin` and once with the recompiled hosts, under
`out\diff\<name>\{orig,recomp}`, and compares:

- the `.obj` bcc32 wrote, byte for byte;
- the `.exe` ilink32 wrote, byte for byte with the clock stamps masked;
- the tools' console output;
- what each built program prints, against `tests\<name>.expect`.

`--recomp` names the recompiled bcc32 (ilink32recomp is taken from beside it;
netlab passes its build-farm copy), and `--keep` leaves the files there. On a machine without
`game\Bin\bcc32.exe` it prints SKIP and exits 0.

Today: **6/6 IDENTICAL**.

## The corpus (`tests/`)

| Program | What it exercises |
|---|---|
| `hello.c` | C, stdio, the command line |
| `fp.c` | x87 floating point, libm, `long double`, printf formatting |
| `templates.cpp` | class and function templates, `vector`/`string`/`map`/`sort` from the bundled Rogue Wave STL, iostreams |
| `except.cpp` | C++ exceptions: unwinding through destructors, rethrow, catch by base, Borland's `__try`/`__except` |
| `mt.c` | the multithreaded RTL (`cw32mt.lib`), `_beginthread`, `__thread` |
| `gui.c` | a Win32 GUI program (`c0w32.obj`, `-tW`): a window class, a window, timers and a message loop; never shown, so it runs headless |

## What is masked, and why

ilink32 stamps the current time into the COFF header, the export directory
and every resource directory; the harness zeroes exactly those before
comparing ([bringup.md](bringup.md#what-the-harness-masks)). Nothing in a
`.obj` comes from the clock, so `.obj` files compare as they are; the harness
copies each source with its mtime so the dependency records match.

## Not covered yet

`bcc32` without `-c` (it starts the original `ilink32.exe`), optimization
and debug switches, DLLs, resources (`RLINK32.DLL`) and the other tools in
`Bin\` ([ROADMAP.md](../ROADMAP.md)).
