// C++ exceptions: unwinding with destructors, rethrow, catch by base, Borland's __try
#include <stdio.h>
#include <string.h>
#include <windows.h>

static int live = 0;
struct Guard {
    const char *name;
    Guard(const char *n) : name(n) { live++; }
    ~Guard() { live--; printf("  ~%s\n", name); }
};
struct Base { virtual const char *what() const { return "base"; } virtual ~Base() {} };
struct Derived : Base { int code; Derived(int c) : code(c) {} const char *what() const { return "derived"; } };

static void deep(int n)
{
    Guard g(n == 0 ? "g0" : "gn");
    if (n == 0) throw Derived(7);
    deep(n - 1);
}

static int middle()
{
    try { deep(2); }
    catch (Base &) { printf("  middle rethrows\n"); throw; }
    return 0;
}

int main()
{
    try { middle(); }
    catch (Derived &d) { printf("caught %s %d live=%d\n", d.what(), d.code, live); }
    try { throw "text"; }
    catch (int) { printf("wrong\n"); }
    catch (const char *s) { printf("caught string %s\n", s); }
    __try { int *p = 0; *p = 1; }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) { printf("SEH caught access violation\n"); }
    printf("done live=%d\n", live);
    return 0;
}
