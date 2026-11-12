/* Win32 GUI hello: registers a class, makes a window (never shown, so it
 * runs headless), pumps messages until a timer closes it. The result goes
 * to the file named on the command line, since a GUI app has no console. */
#include <windows.h>
#include <stdio.h>

static int g_created, g_timer, g_paint;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE: g_created = 1; SetTimer(h, 1, 10, NULL); return 0;
    case WM_TIMER:  g_timer++; if (g_timer == 3) DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(42); return 0;
    }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASS wc = {0};
    MSG msg;
    HWND h;
    char title[64];
    FILE *f;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "BccHello";
    RegisterClass(&wc);
    h = CreateWindow("BccHello", "Hello from BCC 5.5", WS_OVERLAPPEDWINDOW,
                     0, 0, 320, 200, NULL, NULL, hi, NULL);
    GetWindowText(h, title, sizeof title);
    while (GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
    f = fopen(*cmd ? cmd : "gui.out", "w");
    fprintf(f, "title=%s created=%d timers=%d quit=%d\n", title, g_created, g_timer, (int)msg.wParam);
    fclose(f);
    return 0;
}
