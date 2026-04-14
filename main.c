#include "keyboard.h"
#include "math.h"
#include "memory.h"
#include "screen.h"
#include "string.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void shutdown_tty(void)
{
    keyboard_shutdown();
}

static char *g_grid = 0;
static char *g_staging = 0;
static int g_w = 0;
static int g_h = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static int g_generation = 0;

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
    int reserve_rows = 6;
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

static void grid_free(void)
{
    if (g_grid) {
        memory_dealloc(g_grid);
        g_grid = 0;
    }
    if (g_staging) {
        memory_dealloc(g_staging);
        g_staging = 0;
    }
    g_w = 0;
    g_h = 0;
}

static int grid_alloc(int w, int h)
{
    unsigned long need = (unsigned long)w * (unsigned long)h;
    char *p;
    char *q;

    if (w <= 0 || h <= 0) {
        return 0;
    }
    p = memory_alloc(need);
    q = memory_alloc(need);
    if (!p || !q) {
        if (p) {
            memory_dealloc(p);
        }
        if (q) {
            memory_dealloc(q);
        }
        return 0;
    }
    grid_free();
    g_grid = p;
    g_staging = q;
    g_w = w;
    g_h = h;
    cells_zero(g_grid, math_imul(w, h));
    cells_zero(g_staging, math_imul(w, h));
    g_cursor_x = math_idiv(w, 2);
    g_cursor_y = math_idiv(h, 2);
    g_generation = 0;
    return 1;
}

static void stamp_live(int px, int py)
{
    int wx = math_wrap_index(px, g_w);
    int wy = math_wrap_index(py, g_h);
    g_grid[math_imul(wy, g_w) + wx] = 1;
}

/* Classic glider shape, centered on the board. */
static void stamp_static_glider(void)
{
    int ax = math_idiv(g_w, 2) - 1;
    int ay = math_idiv(g_h, 2) - 1;
    stamp_live(ax + 1, ay + 0);
    stamp_live(ax + 2, ay + 1);
    stamp_live(ax + 0, ay + 2);
    stamp_live(ax + 1, ay + 2);
    stamp_live(ax + 2, ay + 2);
}

/* Paint a square around the cursor (only in-bounds cells). rad 1 => 3x3. */
static void paint_cursor_block(int alive, int rad)
{
    int dx;
    int dy;
    for (dy = -rad; dy <= rad; dy++) {
        for (dx = -rad; dx <= rad; dx++) {
            int x = g_cursor_x + dx;
            int y = g_cursor_y + dy;
            if (math_in_bounds(x, y, g_w, g_h)) {
                g_grid[math_imul(y, g_w) + x] = (char)(alive ? 1 : 0);
            }
        }
    }
}

static int count_live_neighbors(int x, int y)
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
            if (g_grid[math_imul(ny, g_w) + nx]) {
                c++;
            }
        }
    }
    return c;
}

/*
 * One Conway generation on a torus: live survives iff 2 or 3 neighbors;
 * dead becomes live iff exactly 3 neighbors; else dead.
 */
static void step_conway_once(void)
{
    int x;
    int y;
    for (y = 0; y < g_h; y++) {
        for (x = 0; x < g_w; x++) {
            int idx = math_imul(y, g_w) + x;
            int n = count_live_neighbors(x, y);
            int alive;
            if (g_grid[idx]) {
                alive = (n == 2 || n == 3) ? 1 : 0;
            } else {
                alive = (n == 3) ? 1 : 0;
            }
            g_staging[idx] = (char)alive;
        }
    }
    {
        char *t = g_grid;
        g_grid = g_staging;
        g_staging = t;
    }
    g_generation++;
}

static void draw_frame(void)
{
    int x;
    int y;
    char xbuf[32];
    char ybuf[32];
    char genbuf[32];
    screen_clear();
    screen_cursor(0, 0);
    screen_puts("Conway (torus) | WASD Space B K C Q | Enter: one generation");
    screen_cursor(0, 1);
    screen_puts("cursor ");
    str_from_int(g_cursor_x, xbuf, 32);
    str_from_int(g_cursor_y, ybuf, 32);
    screen_puts(xbuf);
    screen_puts(",");
    screen_puts(ybuf);
    screen_puts("  gen ");
    str_from_int(g_generation, genbuf, 32);
    screen_puts(genbuf);
    screen_cursor(0, 4);
    for (y = 0; y < g_h; y++) {
        for (x = 0; x < g_w; x++) {
            int on = g_grid[math_imul(y, g_w) + x];
            if (x == g_cursor_x && y == g_cursor_y) {
                screen_putc(on ? '@' : '+');
            } else {
                screen_putc(on ? '#' : '.');
            }
        }
        screen_putc('\n');
    }
    screen_flush();
}

static void handle_key(char ch)
{
    if (ch == 'q' || ch == 'Q') {
        grid_free();
        exit(0);
    }
    if (ch == ' ') {
        if (math_in_bounds(g_cursor_x, g_cursor_y, g_w, g_h)) {
            int idx = math_imul(g_cursor_y, g_w) + g_cursor_x;
            g_grid[idx] = (char)(g_grid[idx] ? 0 : 1);
        }
        return;
    }
    if (ch == '\r' || ch == '\n') {
        step_conway_once();
        return;
    }
    if (ch == 'c' || ch == 'C') {
        {
            int n = math_imul(g_w, g_h);
            cells_zero(g_grid, n);
            if (g_staging) {
                cells_zero(g_staging, n);
            }
            g_generation = 0;
        }
        return;
    }
    if (ch == 'b' || ch == 'B') {
        paint_cursor_block(1, 1);
        return;
    }
    if (ch == 'k' || ch == 'K') {
        paint_cursor_block(0, 1);
        return;
    }
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

int main(void)
{
    int tr;
    int tc;
    int gw;
    int gh;

    memory_init();
    terminal_size(&tr, &tc);
    derive_grid_size(tr, tc, &gw, &gh);
    if (!grid_alloc(gw, gh)) {
        return 1;
    }
    stamp_static_glider();
    keyboard_init();
    if (atexit(shutdown_tty) != 0) {
        grid_free();
        keyboard_shutdown();
        return 1;
    }
    while (1) {
        char ch;
        while (keyboard_key_pressed(&ch)) {
            handle_key(ch);
        }
        draw_frame();
        usleep(80000);
    }
}
