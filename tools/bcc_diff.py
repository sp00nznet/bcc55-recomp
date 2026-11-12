#!/usr/bin/env python3
"""BCC 5.5 conformance harness: the recompiled bcc32/ilink32 against the originals.

For each program in tests/: compile it with the original bcc32.exe and with
build/bcc32recomp.exe, link each .obj with the matching ilink32, then

  1. the two .obj files must be byte-identical,
  2. the two .exe files must be byte-identical once the time stamps are
     masked (COFF header, export and resource directories: the only fields
     that depend on the clock; see docs/bringup.md),
  3. the tools' own console output must match,
  4. both programs must run and print what tests/<name>.expect says.

    py -3 tools/bcc_diff.py [--bin game/Bin] [--keep] [name ...]

Exit 0 when every test passes. Without the original tools it prints SKIP.
"""
import argparse
import os
import shutil
import subprocess
import sys

import pefile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(HERE, 'tests')
BUILD = os.path.join(HERE, 'build')

# name -> (bcc32 switches, startup obj, RTL, ilink32 app type, how the result is read)
CASES = {
    'hello':     ([],      'c0x32.obj', 'cw32.lib',   '-ap', 'stdout'),
    'fp':        ([],      'c0x32.obj', 'cw32.lib',   '-ap', 'stdout'),
    'templates': ([],      'c0x32.obj', 'cw32.lib',   '-ap', 'stdout'),
    'except':    ([],      'c0x32.obj', 'cw32.lib',   '-ap', 'stdout'),
    'mt':        (['-tWM'], 'c0x32.obj', 'cw32mt.lib', '-ap', 'stdout'),
    'gui':       (['-tW'], 'c0w32.obj', 'cw32.lib',   '-aa', 'file'),
}


def run(cmd, cwd, env=None, timeout=120):
    p = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, timeout=timeout)
    return p.returncode, p.stdout.decode('latin-1').replace('\r\n', '\n')


def mask_pe(data):
    """Zero the stamps ilink32 takes from the clock: the COFF header's
    TimeDateStamp, the export directory's, and every resource directory's
    (one per level of the .rsrc tree). Nothing else in the image moves."""
    if data[:2] != b'MZ':
        return data
    pe = pefile.PE(data=data)
    stamps = [pe.FILE_HEADER]
    if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT'):
        stamps.append(pe.DIRECTORY_ENTRY_EXPORT.struct)
    todo = [pe.DIRECTORY_ENTRY_RESOURCE] if hasattr(pe, 'DIRECTORY_ENTRY_RESOURCE') else []
    while todo:
        d = todo.pop()
        stamps.append(d.struct)
        todo.extend(e.directory for e in d.entries if hasattr(e, 'directory'))
    b = bytearray(data)
    for st in stamps:
        off = st.get_field_absolute_offset('TimeDateStamp')
        b[off:off + 4] = bytes(4)
    return bytes(b)


def first_diff(a, b):
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i
    return n if len(a) != len(b) else -1


def build(tool_dir, recomp, name, src, out, env):
    """Compile and link one test in out/; returns (log, obj bytes, exe bytes)."""
    os.makedirs(out, exist_ok=True)
    shutil.copy2(src, out)                   # copy2 keeps the mtime the .obj records
    flags, c0, rtl, app, _ = CASES[name]
    inc, lib = os.path.join(tool_dir, '..', 'Include'), os.path.join(tool_dir, '..', 'Lib')
    inc, lib = os.path.normpath(inc), os.path.normpath(lib)
    bcc = os.path.join(BUILD, 'bcc32recomp.exe') if recomp else os.path.join(tool_dir, 'bcc32.exe')
    ilink = os.path.join(BUILD, 'ilink32recomp.exe') if recomp else os.path.join(tool_dir, 'ilink32.exe')
    base = os.path.basename(src)
    rc1, log1 = run([bcc, '-c'] + flags + ['-I' + inc, base], out, env)
    rc2, log2 = run([ilink, '-L' + lib, '-L' + os.path.join(lib, 'PSDK'), '-Tpe', app, '-x', '-c', '-Gn',
                     c0, name + '.obj,', name + '.exe,', ',', 'import32.lib', rtl], out, env)
    rd = lambda f: open(os.path.join(out, f), 'rb').read() if os.path.exists(os.path.join(out, f)) else b''  # noqa: E731
    return 'bcc32 rc=%d\n%silink32 rc=%d\n%s' % (rc1, log1, rc2, log2), rd(name + '.obj'), rd(name + '.exe')


def run_program(name, out, env):
    exe = os.path.join(out, name + '.exe')
    if not os.path.exists(exe):
        return 'no exe'
    if CASES[name][4] == 'file':
        res = os.path.join(out, name + '.out')
        if os.path.exists(res):
            os.remove(res)
        rc, _ = run([exe, name + '.out'], out, env, timeout=30)
        return open(res).read() if os.path.exists(res) else 'rc=%d, no %s.out' % (rc, name)
    rc, text = run([exe, 'arg1'], out, env, timeout=30)
    return text + ('' if rc == 0 else 'rc=%d\n' % rc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bin', default=os.path.join(HERE, 'game', 'Bin'), help='the original BCC55\\Bin')
    ap.add_argument('--out', default=os.path.join(HERE, 'out', 'diff'))
    ap.add_argument('--keep', action='store_true', help='keep out/diff afterwards')
    ap.add_argument('names', nargs='*')
    args = ap.parse_args()
    tool_dir = os.path.abspath(args.bin)
    if not os.path.exists(os.path.join(tool_dir, 'bcc32.exe')):
        print('SKIP: no bcc32.exe in %s (README, Getting started)' % tool_dir)
        return 0
    for t in ('bcc32recomp.exe', 'ilink32recomp.exe'):
        if not os.path.exists(os.path.join(BUILD, t)):
            sys.exit('no build/%s -- run build.cmd' % t)
    env = dict(os.environ, BCC55_BIN=tool_dir, BCC55_WATCHDOG='120')
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    names = args.names or list(CASES)
    passed = 0
    for name in names:
        src = [os.path.join(TESTS, name + e) for e in ('.c', '.cpp') if os.path.exists(os.path.join(TESTS, name + e))][0]
        o_dir, r_dir = os.path.join(args.out, name, 'orig'), os.path.join(args.out, name, 'recomp')
        o_log, o_obj, o_exe = build(tool_dir, False, name, src, o_dir, env)
        r_log, r_obj, r_exe = build(tool_dir, True, name, src, r_dir, env)
        expect = open(os.path.join(TESTS, name + '.expect')).read()
        o_run, r_run = run_program(name, o_dir, env), run_program(name, r_dir, env)
        fails = []
        if not o_obj or not o_exe:
            fails.append('the original tools failed:\n' + o_log)
        if o_obj != r_obj:
            fails.append('.obj differs at 0x%X (%d vs %d bytes)' % (first_diff(o_obj, r_obj), len(o_obj), len(r_obj)))
        if mask_pe(o_exe) != mask_pe(r_exe):
            fails.append('.exe differs at 0x%X (%d vs %d bytes)' % (first_diff(mask_pe(o_exe), mask_pe(r_exe)),
                                                                   len(o_exe), len(r_exe)))
        if o_log != r_log:
            fails.append('tool output differs:\n--- original\n%s--- recomp\n%s' % (o_log, r_log))
        for who, got in (('original', o_run), ('recomp', r_run)):
            if got != expect:
                fails.append('%s-built program printed:\n%s' % (who, got))
        print('%-10s %s  obj %6d  exe %6d' % (name, 'FAIL' if fails else 'IDENTICAL', len(r_obj), len(r_exe)))
        for f in fails:
            print('    ' + f.replace('\n', '\n    '))
        passed += not fails
    print('%d/%d identical' % (passed, len(names)))
    if not args.keep:
        shutil.rmtree(args.out, ignore_errors=True)
    return 0 if passed == len(names) else 1


if __name__ == '__main__':
    sys.exit(main())
