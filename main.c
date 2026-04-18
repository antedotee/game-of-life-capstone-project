#include "keyboard.h"
#include "life_patterns.h"
#include "math.h"
#include "memory.h"
#include "screen.h"
#include "string.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void shutdown_tty(void)
{
    keyboard_shutdown();
    screen_leave_app();
}

static char *g_grid = 0;
static char *g_staging = 0;
static int g_w = 0;
static int g_h = 0;
static int g_sim_w = 0;
static int g_sim_h = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static int g_generation = 0;
/* Arrow keys send escape sequences; 0 idle, 1 saw ESC, 2 saw ESC [, 3 saw ESC O */
static int g_esc_seq;
static int g_auto_run;
static unsigned g_anim_frame;
static volatile sig_atomic_t g_stop_requested;
static unsigned long g_step_delay_us;
static unsigned long g_ui_refresh_us;
static unsigned long g_step_elapsed_us;
static int g_term_rows = 24;
static int g_term_cols = 80;
static int g_board_left;
static const LifePattern *g_start_pattern;

enum {
    STEP_DELAY_MIN_US = 30000,
    STEP_DELAY_MAX_US = 800000,
    STEP_DELAY_STEP_US = 10000,
    LAYOUT_SIDE_MARGIN = 4,
    LAYOUT_HEADER_ROWS = 3,
    LAYOUT_FOOTER_ROWS = 3,
    GRID_MAX_W = 240,
    GRID_MAX_H = 160,
    OFFSCREEN_MARGIN = 16
};

static void on_sigint(int sig)
{
    (void)sig;
    g_stop_requested = 1;
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

static int default_auto_run(void)
{
    return 0;
}

static unsigned long default_step_delay_us(void)
{
    return 180000ul;
}

static unsigned long default_ui_refresh_us(void)
{
    return 30000ul;
}

static void derive_grid_size(int term_rows, int term_cols, int *gw, int *gh)
{
    int max_w;
    int max_h;

    if (!gw || !gh) {
        return;
    }
    max_w = math_idiv(term_cols - LAYOUT_SIDE_MARGIN - 2, 2);
    max_h = term_rows - LAYOUT_HEADER_ROWS - LAYOUT_FOOTER_ROWS - 2;

    if (max_w < 1) {
        max_w = 1;
    }
    if (max_h < 1) {
        max_h = 1;
    }

    *gw = math_clamp(max_w, 1, GRID_MAX_W);
    *gh = math_clamp(max_h, 1, GRID_MAX_H);
}

static void move_cursor_bounded(int *x, int *y, int w, int h, int dx, int dy)
{
    if (!x || !y || w <= 0 || h <= 0) {
        return;
    }
    *x = math_clamp(*x + dx, 0, w - 1);
    *y = math_clamp(*y + dy, 0, h - 1);
}

static int count_live_neighbors(const char *grid, int w, int h, int x, int y)
{
    int dx;
    int dy;
    int count = 0;

    if (!grid || w <= 0 || h <= 0) {
        return 0;
    }
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int nx;
            int ny;
            if (dx == 0 && dy == 0) {
                continue;
            }
            nx = x + dx;
            ny = y + dy;
            if (math_in_bounds(nx, ny, w, h) && grid[math_imul(ny, w) + nx]) {
                count++;
            }
        }
    }
    return count;
}

static void step_life_once(const char *current, char *next, int w, int h)
{
    int x;
    int y;

    if (!current || !next || w <= 0 || h <= 0) {
        return;
    }
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int idx = math_imul(y, w) + x;
            int n = count_live_neighbors(current, w, h, x, y);
            if (current[idx]) {
                next[idx] = (char)((n == 2 || n == 3) ? 1 : 0);
            } else {
                next[idx] = (char)((n == 3) ? 1 : 0);
            }
        }
    }
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
    g_sim_w = 0;
    g_sim_h = 0;
}

static int sim_width_for(int visible_w)
{
    return visible_w + math_imul(OFFSCREEN_MARGIN, 2);
}

static int sim_height_for(int visible_h)
{
    return visible_h + math_imul(OFFSCREEN_MARGIN, 2);
}

static int sim_index_from_visible(int x, int y, int sim_w)
{
    return math_imul(y + OFFSCREEN_MARGIN, sim_w) + x + OFFSCREEN_MARGIN;
}

static int grid_alloc(int w, int h)
{
    unsigned long need = (unsigned long)w * (unsigned long)h;
    char *p;
    char *q;
    int sim_w;
    int sim_h;

    if (w <= 0 || h <= 0) {
        return 0;
    }
    sim_w = sim_width_for(w);
    sim_h = sim_height_for(h);
    need = (unsigned long)sim_w * (unsigned long)sim_h;
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
    g_sim_w = sim_w;
    g_sim_h = sim_h;
    cells_zero(g_grid, math_imul(g_sim_w, g_sim_h));
    cells_zero(g_staging, math_imul(g_sim_w, g_sim_h));
    g_cursor_x = math_idiv(w, 2);
    g_cursor_y = math_idiv(h, 2);
    g_generation = 0;
    return 1;
}

/* UTF-8 glyphs: full block (alive), empty (dead), textured block (cursor) */
static const char k_glyph_live[] = "\xe2\x96\x88\xe2\x96\x88"; /* ██ */
static const char k_glyph_dead[] = "  ";
static const char k_border_h[] = "\xe2\x94\x80"; /* ─ */
static const char k_border_v[] = "\xe2\x94\x82"; /* │ */
static const char k_border_tl[] = "\xe2\x94\x8c"; /* ┌ */
static const char k_border_tr[] = "\xe2\x94\x90"; /* ┐ */
static const char k_border_bl[] = "\xe2\x94\x94"; /* └ */
static const char k_border_br[] = "\xe2\x94\x98"; /* ┘ */

static void draw_grid_outer_hrule(int row, int top)
{
    int x;
    screen_cursor(g_board_left, row);
    screen_style_border_hot();
    screen_puts(top ? k_border_tl : k_border_bl);
    for (x = 0; x < g_w; x++) {
        screen_puts(k_border_h);
        screen_puts(k_border_h); /* Two horizontal lines per cell because width is 2 */
    }
    screen_puts(top ? k_border_tr : k_border_br);
    screen_reset();
}

static void screen_style_alive_for_cell(int x, int y)
{
    int band = math_wrap_index(x + y + (int)g_anim_frame, 3);
    if (band == 0) {
        screen_style_alive();
    } else if (band == 1) {
        screen_style_alive_alt();
    } else {
        screen_style_alive_hot();
    }
}

static void draw_grid_cell_row(int row, int gy)
{
    int x;
    screen_cursor(g_board_left, row);
    screen_style_border();
    screen_puts(k_border_v);
    
    for (x = 0; x < g_w; x++) {
        int on = g_grid[sim_index_from_visible(x, gy, g_sim_w)];
        if (x == g_cursor_x && gy == g_cursor_y) {
            if (g_auto_run) {
                if (on) {
                    screen_style_alive();
                    screen_puts(k_glyph_live);
                } else {
                    screen_style_dead();
                    screen_puts(k_glyph_dead);
                }
            } else {
                screen_style_cursor_cell();
                if (on) {
                    screen_puts(k_glyph_live);
                } else {
                    screen_puts(k_glyph_live);
                }
            }
        } else if (on) {
            screen_style_alive_for_cell(x, gy);
            screen_puts(k_glyph_live);
        } else {
            if (math_wrap_index(x + gy, 2) == 0) {
                screen_style_dead();
            } else {
                screen_style_dead_alt();
            }
            screen_puts(k_glyph_dead);
        }
        screen_reset();
    }
    
    screen_style_border();
    screen_puts(k_border_v);
    screen_reset();
}

static int grid_resize(int w, int h)
{
    unsigned long need;
    char *p;
    char *q;
    int sim_w;
    int sim_h;
    int old_sim_w;
    int minw;
    int minh;
    int x;
    int y;

    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (w == g_w && h == g_h) {
        return 1;
    }
    sim_w = sim_width_for(w);
    sim_h = sim_height_for(h);
    need = (unsigned long)sim_w * (unsigned long)sim_h;
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
    cells_zero(p, math_imul(sim_w, sim_h));
    cells_zero(q, math_imul(sim_w, sim_h));
    old_sim_w = g_sim_w;
    if (g_grid && g_staging && g_w > 0 && g_h > 0 && g_sim_w > 0) {
        minw = (g_w < w) ? g_w : w;
        minh = (g_h < h) ? g_h : h;
        for (y = 0; y < minh; y++) {
            for (x = 0; x < minw; x++) {
                p[sim_index_from_visible(x, y, sim_w)] =
                    g_grid[sim_index_from_visible(x, y, old_sim_w)];
                q[sim_index_from_visible(x, y, sim_w)] =
                    g_staging[sim_index_from_visible(x, y, old_sim_w)];
            }
        }
    }
    grid_free();
    g_grid = p;
    g_staging = q;
    g_w = w;
    g_h = h;
    g_sim_w = sim_w;
    g_sim_h = sim_h;
    if (g_cursor_x >= g_w) {
        g_cursor_x = g_w - 1;
    }
    if (g_cursor_y >= g_h) {
        g_cursor_y = g_h - 1;
    }
    if (g_cursor_x < 0) {
        g_cursor_x = 0;
    }
    if (g_cursor_y < 0) {
        g_cursor_y = 0;
    }
    return 1;
}

static void sync_terminal_layout(void)
{
    int tr;
    int tc;
    int gw;
    int gh;

    (void)terminal_size(&tr, &tc);
    g_term_rows = tr;
    g_term_cols = tc;
    derive_grid_size(tr, tc, &gw, &gh);
    if (gw != g_w || gh != g_h) {
        if (!grid_resize(gw, gh)) {
            return;
        }
    }
}

static void clear_grid(void)
{
    int n = math_imul(g_sim_w, g_sim_h);
    cells_zero(g_grid, n);
    if (g_staging) {
        cells_zero(g_staging, n);
    }
    g_generation = 0;
}

static void stamp_start_pattern(void)
{
    int ox;
    int oy;
    if (!g_start_pattern) {
        return;
    }
    ox = math_idiv(g_w - g_start_pattern->width, 2);
    oy = math_idiv(g_h - g_start_pattern->height, 2);
    life_stamp_pattern(g_grid, g_sim_w, g_sim_h, g_start_pattern,
                       ox + OFFSCREEN_MARGIN, oy + OFFSCREEN_MARGIN);
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
                g_grid[sim_index_from_visible(x, y, g_sim_w)] = (char)(alive ? 1 : 0);
            }
        }
    }
}

/*
 * One Conway generation on a finite board: live survives iff 2 or 3 neighbors;
 * dead becomes live iff exactly 3 neighbors; else dead.
 */
static void step_conway_once(void)
{
    step_life_once(g_grid, g_staging, g_sim_w, g_sim_h);
    {
        char *t = g_grid;
        g_grid = g_staging;
        g_staging = t;
    }
    g_generation++;
}

static void draw_centered(int row, const char *text)
{
    int len = (int)str_len(text);
    int col = math_idiv(g_term_cols - len, 2);
    if (col < 0) {
        col = 0;
    }
    screen_cursor(col, row);
    screen_puts(text);
}

static void draw_frame(void)
{
    char xbuf[32];
    char ybuf[32];
    char genbuf[32];
    char delaybuf[32];
    int board_cols;
    int base_row;
    int r;
    int y;
    static const char k_spin[] = "|/-\\";

    g_anim_frame++;
    screen_clear();
    screen_hide_cursor();

    board_cols = math_imul(g_w, 2) + 2;
    g_board_left = math_idiv(g_term_cols - board_cols, 2);
    if (g_board_left < 0) {
        g_board_left = 0;
    }
    base_row = LAYOUT_HEADER_ROWS;

    screen_style_title();
    draw_centered(0, "CONWAY'S GAME OF LIFE");
    screen_reset();

    screen_cursor(g_board_left, 1);
    screen_style_panel();
    screen_puts(" ");
    screen_puts("gen ");
    str_from_int(g_generation, genbuf, 32);
    screen_puts(genbuf);
    screen_puts("  ");
    if (g_auto_run) {
        screen_style_run();
        screen_puts("RUN");
    } else {
        screen_style_paused();
        screen_puts("PAUSED");
    }
    screen_style_hint();
    screen_puts("  ");
    screen_putc(k_spin[math_wrap_index((int)g_anim_frame, 4)]);
    screen_puts("  cursor ");
    str_from_int(g_cursor_x, xbuf, 32);
    screen_puts(xbuf);
    screen_puts(",");
    str_from_int(g_cursor_y, ybuf, 32);
    screen_puts(ybuf);
    screen_puts("  ");
    str_from_int(math_idiv((int)g_step_delay_us, 1000), delaybuf, 32);
    screen_puts(delaybuf);
    screen_puts("ms ");
    screen_reset();

    r = base_row;
    draw_grid_outer_hrule(r, 1);
    r++;

    for (y = 0; y < g_h; y++) {
        draw_grid_cell_row(r, y);
        r++;
    }

    draw_grid_outer_hrule(r, 0);
    r++;

    screen_cursor(g_board_left, r + 1);
    screen_style_hint();
    screen_puts("WASD/arrows move   Space draw   B/K brush/erase   C clear   R reload preset   Enter run   N step   [ ] speed   Q quit");
    screen_reset();
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
            int idx = sim_index_from_visible(g_cursor_x, g_cursor_y, g_sim_w);
            g_grid[idx] = (char)(g_grid[idx] ? 0 : 1);
        }
        return;
    }
    if (ch == '\r' || ch == '\n') {
        g_auto_run = !g_auto_run;
        return;
    }
    if (ch == 'n' || ch == 'N') {
        step_conway_once();
        return;
    }
    if (ch == 'c' || ch == 'C') {
        clear_grid();
        return;
    }
    if (ch == 'r' || ch == 'R') {
        clear_grid();
        stamp_start_pattern();
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
        move_cursor_bounded(&g_cursor_x, &g_cursor_y, g_w, g_h, 0, -1);
        return;
    }
    if (ch == 's' || ch == 'S') {
        move_cursor_bounded(&g_cursor_x, &g_cursor_y, g_w, g_h, 0, 1);
        return;
    }
    if (ch == 'a' || ch == 'A') {
        move_cursor_bounded(&g_cursor_x, &g_cursor_y, g_w, g_h, -1, 0);
        return;
    }
    if (ch == 'd' || ch == 'D') {
        move_cursor_bounded(&g_cursor_x, &g_cursor_y, g_w, g_h, 1, 0);
        return;
    }
    if (ch == '[') {
        if (g_step_delay_us <= STEP_DELAY_MAX_US - STEP_DELAY_STEP_US) {
            g_step_delay_us += STEP_DELAY_STEP_US;
        } else {
            g_step_delay_us = STEP_DELAY_MAX_US;
        }
        return;
    }
    if (ch == ']') {
        if (g_step_delay_us >= STEP_DELAY_MIN_US + STEP_DELAY_STEP_US) {
            g_step_delay_us -= STEP_DELAY_STEP_US;
        } else {
            g_step_delay_us = STEP_DELAY_MIN_US;
        }
        return;
    }
}

/* CSI / SS3 arrows: ESC [ A|B|C|D or ESC O A|B|C|D — same moves as WASD */
static void handle_csi_arrow(char c)
{
    switch (c) {
    case 'A':
        handle_key('w');
        break;
    case 'B':
        handle_key('s');
        break;
    case 'C':
        handle_key('d');
        break;
    case 'D':
        handle_key('a');
        break;
    default:
        break;
    }
}

static void feed_input(char ch)
{
    if (g_esc_seq == 0) {
        if (ch == '\033') {
            g_esc_seq = 1;
            return;
        }
        handle_key(ch);
        return;
    }
    if (g_esc_seq == 1) {
        if (ch == '[') {
            g_esc_seq = 2;
            return;
        }
        if (ch == 'O') {
            g_esc_seq = 3;
            return;
        }
        g_esc_seq = 0;
        if (ch == '\033') {
            g_esc_seq = 1;
        } else {
            handle_key(ch);
        }
        return;
    }
    if (g_esc_seq == 2) {
        g_esc_seq = 0;
        if (ch >= 'A' && ch <= 'D') {
            handle_csi_arrow(ch);
            return;
        }
        if (ch == '\033') {
            g_esc_seq = 1;
        } else {
            handle_key(ch);
        }
        return;
    }
    if (g_esc_seq == 3) {
        g_esc_seq = 0;
        if (ch >= 'A' && ch <= 'D') {
            handle_csi_arrow(ch);
            return;
        }
        if (ch == '\033') {
            g_esc_seq = 1;
        } else {
            handle_key(ch);
        }
        return;
    }
}

static void print_presets(void)
{
    int i;
    screen_puts("Available presets:\n");
    for (i = 0; i < life_pattern_count(); i++) {
        const LifePattern *p = life_pattern_at(i);
        if (p) {
            screen_puts("  ");
            screen_puts(p->name);
            screen_puts(" - ");
            screen_puts(p->title);
            screen_puts("\n");
        }
    }
}

static int parse_args(int argc, char **argv)
{
    int i;
    g_start_pattern = 0;

    for (i = 1; i < argc; i++) {
        if (str_compare(argv[i], "--list-presets") == 0) {
            print_presets();
            return 0;
        }
        if (str_compare(argv[i], "--preset") == 0 || str_compare(argv[i], "-p") == 0) {
            i++;
            if (i >= argc) {
                screen_puts("Missing preset name after --preset.\n\n");
                print_presets();
                return -1;
            }
            g_start_pattern = life_find_pattern(argv[i]);
            if (!g_start_pattern) {
                screen_puts("Unknown preset: ");
                screen_puts(argv[i]);
                screen_puts("\n\n");
                print_presets();
                return -1;
            }
            continue;
        }
        g_start_pattern = life_find_pattern(argv[i]);
        if (!g_start_pattern) {
            screen_puts("Unknown option or preset: ");
            screen_puts(argv[i]);
            screen_puts("\n\n");
            print_presets();
            return -1;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    int tr;
    int tc;
    int gw;
    int gh;
    int parsed;

    parsed = parse_args(argc, argv);
    if (parsed <= 0) {
        if (parsed == 0) {
            return 0;
        }
        return 1;
    }
    memory_init();
    terminal_size(&tr, &tc);
    g_term_rows = tr;
    g_term_cols = tc;
    derive_grid_size(tr, tc, &gw, &gh);
    if (!grid_alloc(gw, gh)) {
        return 1;
    }
    g_auto_run = default_auto_run();
    g_step_delay_us = default_step_delay_us();
    g_ui_refresh_us = default_ui_refresh_us();
    stamp_start_pattern();
    screen_enter_app();
    keyboard_init();
    if (atexit(shutdown_tty) != 0) {
        grid_free();
        keyboard_shutdown();
        screen_leave_app();
        return 1;
    }
    if (signal(SIGINT, on_sigint) == SIG_ERR) {
        grid_free();
        keyboard_shutdown();
        screen_leave_app();
        return 1;
    }
    while (!g_stop_requested) {
        char ch;
        sync_terminal_layout();
        while (keyboard_key_pressed(&ch)) {
            feed_input(ch);
        }
        if (g_auto_run) {
            g_step_elapsed_us += g_ui_refresh_us;
            if (g_step_elapsed_us >= g_step_delay_us) {
                step_conway_once();
                g_step_elapsed_us = 0;
            }
        } else {
            g_step_elapsed_us = 0;
        }
        draw_frame();
        usleep(g_ui_refresh_us);
    }
    grid_free();
    keyboard_shutdown();
    return 0;
}
