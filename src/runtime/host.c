/*
 * BCC 5.5 - static recompilation host for bcc32.exe and ilink32.exe.
 *
 * One source, built twice (CMakeLists.txt): BCC_TOOL is "bcc32" or
 * "ilink32". A 32-bit host on pcrecomp's runtime/native32; the lifted tool
 * has the Borland RTL linked in and calls the real kernel32/user32 through
 * the bridge. From the AmazingMIDI host.
 *
 * It behaves like the tool: `bcc32recomp -c foo.c` is `bcc32 -c foo.c`. The
 * command line after the host's own name goes to the guest verbatim, so host
 * knobs are environment variables, not flags:
 *   BCC55_BIN       folder with the original bcc32.exe/ilink32.exe
 *                   (default: ..\game\Bin next to this exe's folder)
 *   BCC55_WATCHDOG  seconds before the run is killed (exit 4)
 *   BCC55_TRACE     1 = native-call trace on stderr
 *   BCC55_DIAG      pcrecomp recomp_trace options, e.g. "--calltrace t.txt"
 *                   (a -DBCC_TRACE=ON build; see recomp_trace.h)
 * Nothing is printed on stdout but the tool's own output.
 *
 * Linked at /BASE:0x60000000 so 0x00400000 is free for the guest image.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native32.h"
#include "recomp_trace.h"

extern const uint32_t guest_entry_va;    /* recomp_dispatch.c */

#define GUEST_BASE 0x00400000u

static DWORD g_watchdog_s;
static char  g_guest_exe[MAX_PATH], g_guest_cmdline[32768];

#define ARG(n) MEM32(g_esp + 4 + 4 * (n))
#define RET(v, nargs) do { g_eax = (uint32_t)(v); g_esp += 4 + 4 * (nargs); } while (0)

/* The guest is the original exe, not this host. */
static void shim_GetModuleHandleA(void) {
    uint32_t name = ARG(0);
    RET(name ? (uint32_t)(uintptr_t)GetModuleHandleA((const char*)(uintptr_t)name) : GUEST_BASE, 1);
}

/* bcc32 finds bcc32.cfg (and ilink32 ilink32.cfg) next to its own exe. */
static void shim_GetModuleFileNameA(void) {
    uint32_t h = ARG(0), size = ARG(2);
    char* out = (char*)(uintptr_t)ARG(1);
    if (h == 0 || h == GUEST_BASE) {
        uint32_t n = (uint32_t)strlen(g_guest_exe);
        if (size) {
            if (n >= size) n = size - 1;
            memcpy(out, g_guest_exe, n);
            out[n] = 0;
        }
        RET(n, 3);
        return;
    }
    RET(GetModuleFileNameA((HMODULE)(uintptr_t)h, out, size), 3);
}

static void shim_GetCommandLineA(void) { RET((uintptr_t)g_guest_cmdline, 0); }

static native32_shim_t g_shims[] = {
    { "GetModuleHandleA", shim_GetModuleHandleA },
    { "GetModuleFileNameA", shim_GetModuleFileNameA },
    { "GetCommandLineA", shim_GetCommandLineA },
};

/* ------------------------------------------------------------ reports */

recomp_func_t recomp_lookup_manual(uint32_t va) { (void)va; return NULL; }

/* Added after native32's own handler, so callbacks are resolved first and
 * only real faults get here. */
static LONG CALLBACK crash(EXCEPTION_POINTERS* ep) {
    EXCEPTION_RECORD* r = ep->ExceptionRecord;
    if ((r->ExceptionCode & 0xF0000000u) != 0xC0000000u) return EXCEPTION_CONTINUE_SEARCH;
    fprintf(stderr, "\n=== fault 0x%08lX at 0x%p ===\n", r->ExceptionCode, r->ExceptionAddress);
    if (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2) {
        ULONG_PTR op = r->ExceptionInformation[0];
        uint32_t at = (uint32_t)r->ExceptionInformation[1];
        fprintf(stderr, "  %s of 0x%08X%s\n", op == 0 ? "read" : op == 1 ? "write" : "execute", at,
                native32_in_guest(at) ? " (inside the guest image)" : at < 0x10000 ? " (null/low)" : "");
    }
    recomp_trace_flush();
    fprintf(stderr, "  in lifted sub_%08X, last native call %s\n", g_cur_func, g_cur_import);
    fprintf(stderr, "  eax=%08X ecx=%08X edx=%08X ebx=%08X esp=%08X ebp=%08X esi=%08X edi=%08X\n",
            g_eax, g_ecx, g_edx, g_ebx, g_esp, g_ebp, g_esi, g_edi);
    native32_dump_icalls(12);
    recomp_dump_trace("fault");
    TerminateProcess(GetCurrentProcess(), 3);
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD WINAPI watchdog(LPVOID unused) {
    (void)unused;
    Sleep(g_watchdog_s * 1000);
    fprintf(stderr, "\n[watchdog] %lu s: in sub_%08X, last native call %s\n",
            g_watchdog_s, g_cur_func, g_cur_import);
    TerminateProcess(GetCurrentProcess(), 4);
    return 0;
}

/* The raw command line minus the host's own argv[0] (same rules as the CRT). */
static const char* args_tail(void) {
    const char* p = GetCommandLineA();
    if (*p == '"') { p++; while (*p && *p != '"') p++; if (*p) p++; }
    else while (*p && *p != ' ' && *p != '\t') p++;
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

int main(void) {
    char bin[MAX_PATH], env[32];
    setvbuf(stderr, NULL, _IONBF, 0);
    if (!GetEnvironmentVariableA("BCC55_BIN", bin, sizeof bin)) {
        char me[MAX_PATH], *s;
        GetModuleFileNameA(NULL, me, sizeof me);
        if ((s = strrchr(me, '\\')) != NULL) *s = 0;          /* build\     */
        _snprintf(bin, sizeof bin - 1, "%s\\..\\game\\Bin", me);
    }
    if (GetEnvironmentVariableA("BCC55_WATCHDOG", env, sizeof env)) g_watchdog_s = strtoul(env, NULL, 0);
    if (GetEnvironmentVariableA("BCC55_TRACE", env, sizeof env)) native32_trace_native = atoi(env);
    static char diag[1024];
    if (GetEnvironmentVariableA("BCC55_DIAG", diag, sizeof diag)) {
        char* av[16] = { "host" };
        int ac = 1;
        for (char* t = strtok(diag, " "); t && ac < 16; t = strtok(NULL, " ")) av[ac++] = t;
        for (int i = 1; i < ac; i++) {
            int n = recomp_trace_arg(ac, av, i);
            if (!n) { fprintf(stderr, "BCC55_DIAG: unknown option %s\n", av[i]); return 1; }
            i += n - 1;
        }
    }
    char full[MAX_PATH];
    _snprintf(full, sizeof full - 1, "%s\\" BCC_TOOL ".exe", bin);
    GetFullPathNameA(full, MAX_PATH, g_guest_exe, NULL);
    _snprintf(g_guest_cmdline, sizeof g_guest_cmdline - 1, "\"%s\" %s", g_guest_exe, args_tail());

    /* ilink32 LoadLibrary's RLINK32.DLL and LNKDFM50.DLL by bare name, which
     * Windows looks for next to the exe: the original's folder, not ours.
     * They are not lifted; they run as native code through the bridge. */
    SetDllDirectoryA(bin);
    native32_init();
    AddVectoredExceptionHandler(0, crash);
    uint32_t span = native32_map(g_guest_exe, GUEST_BASE);
    if (!span) {
        fprintf(stderr, BCC_TOOL "recomp: cannot map %s (set BCC55_BIN to the BCC55\\Bin folder)\n", g_guest_exe);
        return 1;
    }
    int unresolved = native32_bind(GUEST_BASE, g_shims, (int)(sizeof g_shims / sizeof g_shims[0]));
    if (unresolved) { fprintf(stderr, BCC_TOOL "recomp: %d imports unresolved\n", unresolved); return 1; }
    if (g_watchdog_s) CloseHandle(CreateThread(NULL, 0, watchdog, NULL, 0, NULL));
    native32_call_guest(guest_entry_va, 0, NULL);
    return (int)g_eax;
}
