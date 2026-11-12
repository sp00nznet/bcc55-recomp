#!/bin/sh
# There is no Linux or macOS build of the BCC 5.5 recomp yet: the hosts are
# built with MSVC and run on pcrecomp's native32, which calls the real
# kernel32 and user32. Use Setup.cmd on Windows. (ROADMAP.md)
echo "Borland C++ 5.5 recomp: no Linux or macOS build yet. Run Setup.cmd on Windows." >&2
exit 1
