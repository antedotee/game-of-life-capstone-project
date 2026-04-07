#include "screen.h"
#include "string.h"
#include <stdio.h>

void screen_clear(void)
{
    printf("\033[2J\033[H");
}

void screen_cursor(int col, int row)
{
    printf("\033[%d;%dH", row + 1, col + 1);
}

void screen_putc(char c)
{
    putchar(c);
}

void screen_puts(const char *s)
{
    unsigned long i;
    unsigned long n;
    if (!s) {
        return;
    }
    n = str_len(s);
    for (i = 0; i < n; i++) {
        putchar(s[i]);
    }
}

void screen_flush(void)
{
    fflush(stdout);
}
