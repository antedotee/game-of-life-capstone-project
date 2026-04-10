#ifndef C_CAPSTONE_MODES_C_INCLUDED
#define C_CAPSTONE_MODES_C_INCLUDED

#include "math.h"
#include "string.h"

ModesStartupKind modes_parse_startup(int argc, char **argv)
{
    ModesStartupKind k = MODES_START_RANDOM;
    int i;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (str_cmp(a, "mode=gun") == 0 || str_cmp(a, "--mode=gun") == 0) {
            k = MODES_START_GUN;
        } else if (str_cmp(a, "mode=pulse") == 0 || str_cmp(a, "--mode=pulse") == 0) {
            k = MODES_START_PULSE;
        } else if (str_cmp(a, "mode=gates") == 0 || str_cmp(a, "--mode=gates") == 0) {
            k = MODES_START_GATES;
        } else if (str_cmp(a, "mode=cpu") == 0 || str_cmp(a, "--mode=cpu") == 0) {
            k = MODES_START_CPU;
        }
    }
    return k;
}

static void grid_clear(char *grid, int gw, int gh)
{
    int i;
    int n = math_imul(gw, gh);
    for (i = 0; i < n; i++) {
        grid[i] = 0;
    }
}

static void stamp_live(char *grid, int gw, int gh, int px, int py)
{
    int wx = math_wrap_index(px, gw);
    int wy = math_wrap_index(py, gh);
    grid[math_imul(wy, gw) + wx] = 1;
}

static void stamp_pairs_centered(char *grid, int gw, int gh, const short *xy, int pairs,
                                 int center_x, int center_y)
{
    int ax = math_idiv(gw, 2) - center_x;
    int ay = math_idiv(gh, 2) - center_y;
    int k;
    for (k = 0; k < pairs; k++) {
        int dx = xy[math_imul(k, 2)];
        int dy = xy[math_imul(k, 2) + 1];
        stamp_live(grid, gw, gh, ax + dx, ay + dy);
    }
}

static void stamp_rle_at(char *grid, int gw, int gh, int ax, int ay, const char *rle)
{
    int px = 0;
    int py = 0;
    int i = 0;
    for (;;) {
        unsigned int run = 0;
        char c;
        while (rle[i] >= '0' && rle[i] <= '9') {
            run = run * 10u + (unsigned int)(rle[i] - '0');
            i++;
        }
        if (run == 0) {
            run = 1u;
        }
        c = rle[i];
        if (c == '\0') {
            break;
        }
        i++;
        if (c == '!') {
            break;
        }
        if (c == 'b') {
            px += (int)run;
        } else if (c == 'o') {
            unsigned int u;
            for (u = 0; u < run; u++) {
                stamp_live(grid, gw, gh, ax + px, ay + py);
                px++;
            }
        } else if (c == '$') {
            py += (int)run;
            px = 0;
        }
    }
}

static void stamp_rle_centered(char *grid, int gw, int gh, const char *rle, int cx, int cy)
{
    int ax = math_idiv(gw, 2) - cx;
    int ay = math_idiv(gh, 2) - cy;
    stamp_rle_at(grid, gw, gh, ax, ay, rle);
}

void pattern_load_preset(char *grid, int gw, int gh, int id)
{
    static const short glider[] = {1, 0, 2, 1, 0, 2, 1, 2, 2, 2};
    static const short blinker[] = {0, 0, 1, 0, 2, 0};
    static const short toad[] = {1, 0, 2, 0, 3, 0, 0, 1, 1, 1, 2, 1};
    static const short beacon[] = {0, 0, 1, 0, 0, 1, 1, 1, 2, 2, 3, 2, 2, 3, 3, 3};
    static const short pent[] = {0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0,
                                 8, 0, 9, 0, 10, 0, 11, 0, 12, 0};
    static const char lwss[] = "bo2bo$o4bo$o3b2o$b4o!";
    static const char mwss[] = "3bo$bo3bo$o5bo$o5bo$o5bo$bo3bo$3bo!";
    static const char rpent[] = "b2o$2o2b$o2b!";
    static const char acorn[] = "bo3b$o2bo3b$o2o6b2o!";
    static const char gosper[] =
        "24bo$22bobo$12b2o6b2o12b2o$11bo3bo4bo3bo11b2o$"
        "6b2o2bobo2bo4bo2bobo2b2o$6b2o4bo2b2o2bo2bo4b2o$"
        "7bobo8bobo$8b2o8b2o!";

    if (!grid || gw <= 0 || gh <= 0) {
        return;
    }
    if (id < 0 || id >= PATTERN_COUNT) {
        id = 0;
    }
    grid_clear(grid, gw, gh);
    switch (id) {
    case 0:
        stamp_rle_centered(grid, gw, gh, gosper, 21, 3);
        break;
    case 1:
        stamp_pairs_centered(grid, gw, gh, glider, 5, 1, 1);
        break;
    case 2:
        stamp_pairs_centered(grid, gw, gh, blinker, 3, 1, 0);
        break;
    case 3:
        stamp_pairs_centered(grid, gw, gh, toad, 6, 1, 0);
        break;
    case 4:
        stamp_pairs_centered(grid, gw, gh, beacon, 8, 1, 1);
        break;
    case 5:
        stamp_rle_centered(grid, gw, gh, lwss, 2, 1);
        break;
    case 6:
        stamp_rle_centered(grid, gw, gh, mwss, 3, 3);
        break;
    case 7:
        stamp_pairs_centered(grid, gw, gh, pent, 12, 6, 0);
        break;
    case 8:
        stamp_rle_centered(grid, gw, gh, rpent, 1, 1);
        break;
    case 9:
        stamp_rle_centered(grid, gw, gh, acorn, 5, 1);
        break;
    default:
        stamp_rle_centered(grid, gw, gh, gosper, 21, 3);
        break;
    }
}

const char *pattern_name(int id)
{
    static const char *names[PATTERN_COUNT] = {
        "Gosper glider gun (period 30, emits gliders)",
        "Glider (spaceship)",
        "Blinker (period-2 oscillator)",
        "Toad (period-2 oscillator)",
        "Beacon (period-2 oscillator)",
        "Lightweight spaceship (LWSS)",
        "Middleweight spaceship (MWSS)",
        "Pentadecathlon (period-15 oscillator)",
        "R-pentomino (small chaotic methuselah)",
        "Acorn (long-running methuselah)",
    };
    if (id < 0 || id >= PATTERN_COUNT) {
        return names[0];
    }
    return names[id];
}

void modes_apply_startup(char *grid, int gw, int gh, ModesStartupKind k)
{
    static const char tumbler[] = "4bo6bo$2b2o6b2o$o10bo$b2o6b2o$3bo6bo!";
    static const short gates_scene[] = {
        0, 0, 1, 0, 0, 1, 2, 1, 1, 2, 2, 2, 3, 2,
        14, 1, 15, 2, 16, 0, 16, 1, 16, 2,
    };
    if (!grid || gw <= 0 || gh <= 0) {
        return;
    }
    switch (k) {
    case MODES_START_RANDOM:
        break;
    case MODES_START_GUN:
        pattern_load_preset(grid, gw, gh, 0);
        break;
    case MODES_START_PULSE:
        grid_clear(grid, gw, gh);
        stamp_rle_centered(grid, gw, gh, tumbler, 5, 2);
        break;
    case MODES_START_GATES:
        grid_clear(grid, gw, gh);
        stamp_pairs_centered(grid, gw, gh, gates_scene, 12, 8, 1);
        break;
    case MODES_START_CPU:
        pattern_load_preset(grid, gw, gh, 9);
        break;
    default:
        break;
    }
}

#endif
