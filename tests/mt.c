/* the multithreaded RTL (cw32mt): _beginthread, threadvars */
#include <stdio.h>
#include <process.h>
#include <windows.h>

static int __thread tls_counter;
static long total;
static HANDLE done[4];

static void worker(void *arg)
{
    int i, id = (int)arg;
    for (i = 0; i <= id * 10; i++) tls_counter += i;
    InterlockedExchangeAdd(&total, tls_counter);
    SetEvent(done[id]);
}

int main(void)
{
    int i;
    for (i = 0; i < 4; i++) { done[i] = CreateEvent(NULL, TRUE, FALSE, NULL); _beginthread(worker, 0, (void *)i); }
    WaitForMultipleObjects(4, done, TRUE, INFINITE);
    printf("total=%ld main tls=%d\n", total, tls_counter);
    return 0;
}
