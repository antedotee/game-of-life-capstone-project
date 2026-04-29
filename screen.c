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

void screen_enter_app(void)
{
    printf("\033[?1049h\033[?25l");
}

void screen_leave_app(void)
{
    printf("\033[?25h\033[?1049l\033[0m");
    fflush(stdout);
}

void screen_hide_cursor(void)
{
    printf("\033[?25l");
}

void screen_reset(void)
{
    printf("\033[0m");
}

void screen_style_banner(void)
{
    printf("\033[1m\033[38;2;130;210;255m");
}

void screen_style_hint(void)
{
    printf("\033[38;2;140;150;175m");
}

void screen_style_title(void)
{
    printf("\033[1m\033[38;2;245;250;255m");
}

void screen_style_panel(void)
{
    printf("\033[48;2;12;18;32m\033[38;2;178;200;230m");
}

void screen_style_alive(void)
{
    printf("\033[1m\033[38;2;90;255;190m");
}

void screen_style_alive_alt(void)
{
    printf("\033[1m\033[38;2;72;205;255m");
}

void screen_style_alive_hot(void)
{
    printf("\033[1m\033[38;2;255;105;205m");
}

void screen_style_dead(void)
{
    printf("\033[38;2;20;26;44m");
}

void screen_style_dead_alt(void)
{
    printf("\033[38;2;25;32;54m");
}

void screen_style_cursor_cell(void)
{
    printf("\033[1m\033[38;2;255;220;90m");
}

void screen_style_border(void)
{
    printf("\033[38;2;98;120;190m");
}

void screen_style_border_hot(void)
{
    printf("\033[1m\033[38;2;180;120;255m");
}

void screen_style_run(void)
{
    printf("\033[1m\033[38;2;100;255;180m");
}

void screen_style_paused(void)
{
    printf("\033[38;2;200;120;160m");
}
