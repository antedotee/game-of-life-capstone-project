#include "keyboard.h"
#include "math.h"
#include "memory.h"
#include "modes.h"
#include "screen.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "modes.c"

static void shutdown_tty(void)
{
    keyboard_shutdown();
}

static char *g_cur = 0;
static char *g_next = 0;
static char *g_blk_a = 0;
static char *g_blk_b = 0;
static int g_w = 0;
static int g_h = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static int g_running = 0;
static int g_generation = 0;
static int g_frame_delay_us = 120000;
static unsigned int g_prng = 2463534242u;

static unsigned int prng_next(void)
{
    g_prng ^= g_prng << 13;
    g_prng ^= g_prng >> 17;
    g_prng ^= g_prng << 5;
    return g_prng;
}

static int terminal_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *rows = (int)ws.ws_row;
        *cols = (int)ws.ws_col;
        return 1;
    }
    *rows = 24;
    *cols = 80;
    return 0;
}

static int derive_grid_size(int term_rows, int term_cols, int *gw, int *gh)
{
    int reserve_rows = 4;
    int reserve_cols = 2;
    int w = term_cols - reserve_cols;
    int h = term_rows - reserve_rows;
    w = math_clamp(w, 12, 160);
    h = math_clamp(h, 8, 48);
    *gw = w;
    *gh = h;
    return 1;
}

static void cells_zero(char *p, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        p[i] = 0;
    }
}

static int grid_buffers_alloc(int w, int h)
{
    unsigned long need = (unsigned long)w * (unsigned long)h;
    char *a;
    char *b;
    char *old_a = g_blk_a;
    char *old_b = g_blk_b;
    a = memory_alloc(need);
    b = memory_alloc(need);
    if (!a || !b) {
        if (a) {
            memory_dealloc(a);
        }
        if (b) {
            memory_dealloc(b);
        }
        return 0;
    }
    cells_zero(a, math_imul(w, h));
    cells_zero(b, math_imul(w, h));
    if (old_a) {
        memory_dealloc(old_a);
    }
    if (old_b) {
        memory_dealloc(old_b);
    }
    g_blk_a = a;
    g_blk_b = b;
    g_cur = a;
    g_next = b;
    g_w = w;
    g_h = h;
    g_cursor_x = math_idiv(w, 2);
    g_cursor_y = math_idiv(h, 2);
    g_generation = 0;
    return 1;
}

static int count_neighbors(int x, int y)
{
    int dx;
    int dy;
    int c = 0;
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int nx;
            int ny;
            if (dx == 0 && dy == 0) {
                continue;
            }
            nx = math_wrap_index(x + dx, g_w);
            ny = math_wrap_index(y + dy, g_h);
            if (g_cur[math_imul(ny, g_w) + nx]) {
                c++;
            }
        }
    }
    return c;
}

static void step_once(void)
{
    int x;
    int y;
    for (y = 0; y < g_h; y++) {
        for (x = 0; x < g_w; x++) {
            int idx = math_imul(y, g_w) + x;
            int n = count_neighbors(x, y);
            if (g_cur[idx]) {
                g_next[idx] = (char)((n == 2 || n == 3) ? 1 : 0);
            } else {
                g_next[idx] = (char)(n == 3 ? 1 : 0);
            }
        }
    }
    {
        char *t = g_cur;
        g_cur = g_next;
        g_next = t;
    }
    g_generation++;
}

static void randomize_field(int density_permille)
{
    int x;
    int y;
    int d = math_clamp(density_permille, 50, 400);
    for (y = 0; y < g_h; y++) {
        for (x = 0; x < g_w; x++) {
            unsigned int u = prng_next() & 1023u;
            g_cur[math_imul(y, g_w) + x] = (char)(u < (unsigned int)d ? 1 : 0);
        }
    }
    g_generation = 0;
}

static void clear_field(void)
{
    cells_zero(g_cur, math_imul(g_w, g_h));
    g_generation = 0;
}

static void draw_frame(void)
{
    int x;
    int y;
    char genbuf[32];
    screen_clear();
    screen_cursor(0, 0);
    screen_puts("C capstone Game of Life | Enter run  P pause  Space toggle  WASD move  R random  C clear");
    screen_puts(" | 0-9 presets  F/Z  +/-  : cmd  Q  | modes: gun pulse gates cpu");
    screen_cursor(0, 2);
    str_from_int(g_generation, genbuf, 32);
    screen_puts("Gen: ");
    screen_puts(genbuf);
    screen_puts(g_running ? "  RUN" : "  EDIT");
    screen_cursor(0, 3);
    for (y = 0; y < g_h; y++) {
        for (x = 0; x < g_w; x++) {
            int on = g_cur[math_imul(y, g_w) + x];
            int cx = g_cursor_x;
            int cy = g_cursor_y;
            if (!g_running && x == cx && y == cy) {
                screen_putc(on ? '@' : '+');
            } else {
                screen_putc(on ? '#' : '.');
            }
        }
        screen_putc('\n');
    }
    screen_flush();
}

static void handle_command_line(void)
{
    char line[128];
    char *toks[8];
    int n;
    int i;
    screen_cursor(0, math_min(g_h + 4, 40));
    screen_puts("Cmd: ");
    screen_flush();
    keyboard_read_line(line, 128);
    n = str_split_inplace(line, ' ', toks, 8);
    for (i = 0; i < n; i++) {
        if (str_cmp(toks[i], "") == 0) {
            continue;
        }
        if (str_cmp(toks[i], "quit") == 0 || str_cmp(toks[i], "q") == 0) {
            exit(0);
        }
        if (str_cmp(toks[i], "help") == 0) {
            screen_cursor(0, math_min(g_h + 5, 42));
            screen_puts("Commands: help quit random clear faster slower (keys F/Z)");
            screen_flush();
            usleep(800000);
            return;
        }
        if (str_cmp(toks[i], "random") == 0) {
            randomize_field(180);
            return;
        }
        if (str_cmp(toks[i], "clear") == 0) {
            clear_field();
            return;
        }
        if (str_cmp(toks[i], "faster") == 0) {
            g_frame_delay_us = math_max(10000, g_frame_delay_us - 15000);
            return;
        }
        if (str_cmp(toks[i], "slower") == 0) {
            g_frame_delay_us = math_min(500000, g_frame_delay_us + 15000);
            return;
        }
        return;
    }
}

static int try_resize(int delta)
{
    int tr;
    int tc;
    int nw;
    int nh;
    terminal_size(&tr, &tc);
    derive_grid_size(tr, tc, &nw, &nh);
    nw = math_clamp(nw + delta, 12, 200);
    nh = math_clamp(nh + delta, 6, 60);
    return grid_buffers_alloc(nw, nh);
}

static void handle_key(char ch)
{
    if (!g_running && ch >= '0' && ch <= '9') {
        pattern_load_preset(g_cur, g_w, g_h, ch - '0');
        g_generation = 0;
        return;
    }
    if (ch == ':') {
        handle_command_line();
        return;
    }
    if (ch == 'q' || ch == 'Q') {
        exit(0);
    }
    if (ch == '\r' || ch == '\n') {
        g_running = 1;
        return;
    }
    if (ch == 'p' || ch == 'P') {
        g_running = 0;
        return;
    }
    if (ch == ' ') {
        if (!g_running && math_in_bounds(g_cursor_x, g_cursor_y, g_w, g_h)) {
            int idx = math_imul(g_cursor_y, g_w) + g_cursor_x;
            g_cur[idx] = (char)(g_cur[idx] ? 0 : 1);
        } else {
            g_running = 0;
        }
        return;
    }
    if (!g_running) {
        if (ch == 'w' || ch == 'W') {
            if (math_in_bounds(g_cursor_x, g_cursor_y - 1, g_w, g_h)) {
                g_cursor_y--;
            }
            return;
        }
        if (ch == 's' || ch == 'S') {
            if (math_in_bounds(g_cursor_x, g_cursor_y + 1, g_w, g_h)) {
                g_cursor_y++;
            }
            return;
        }
        if (ch == 'a' || ch == 'A') {
            if (math_in_bounds(g_cursor_x - 1, g_cursor_y, g_w, g_h)) {
                g_cursor_x--;
            }
            return;
        }
        if (ch == 'd' || ch == 'D') {
            if (math_in_bounds(g_cursor_x + 1, g_cursor_y, g_w, g_h)) {
                g_cursor_x++;
            }
            return;
        }
    }
    if (ch == 'r' || ch == 'R') {
        randomize_field(180);
        return;
    }
    if (ch == 'c' || ch == 'C') {
        clear_field();
        return;
    }
    if (ch == 'f' || ch == 'F') {
        g_frame_delay_us = math_max(10000, g_frame_delay_us - 15000);
        return;
    }
    if (g_running && (ch == 'z' || ch == 'Z')) {
        g_frame_delay_us = math_min(500000, g_frame_delay_us + 15000);
        return;
    }
    if (ch == '+') {
        try_resize(2);
        return;
    }
    if (ch == '-') {
        try_resize(-2);
        return;
    }
}

int main(int argc, char **argv)
{
    int tr;
    int tc;
    int gw;
    int gh;
    int tick = 0;
    memory_init();
    g_prng ^= (unsigned int)getpid();
    terminal_size(&tr, &tc);
    derive_grid_size(tr, tc, &gw, &gh);
    if (!grid_buffers_alloc(gw, gh)) {
        return 1;
    }
    keyboard_init();
    if (atexit(shutdown_tty) != 0) {
        keyboard_shutdown();
        return 1;
    }
    {
        ModesStartupKind sk = modes_parse_startup(argc, argv);
        if (sk == MODES_START_RANDOM) {
            randomize_field(160);
        } else {
            modes_apply_startup(g_cur, g_w, g_h, sk);
        }
    }
    while (1) {
        char ch;
        while (keyboard_key_pressed(&ch)) {
            handle_key(ch);
        }
        if (g_running) {
            tick++;
            if (tick >= 2) {
                tick = 0;
                step_once();
            }
        } else {
            tick = 0;
        }
        draw_frame();
        usleep((unsigned int)g_frame_delay_us);
    }
}
