/* plain C, stdio and the command line */
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int i, sum = 0;
    for (i = 1; i <= 100; i++) sum += i;
    printf("hello from bcc %d\n", sum);
    printf("argc=%d %s\n", argc, argc > 1 ? argv[1] : "-");
    printf("%s|%5d|%-5s|%x\n", strrchr("a/b/c", '/'), 42, "ab", 0xBEEF);
    return 0;
}
