#ifndef MODES_H
#define MODES_H

#define PATTERN_COUNT 10

typedef enum {
    MODES_START_RANDOM = 0,
    MODES_START_GUN,
    MODES_START_PULSE,
    MODES_START_GATES,
    MODES_START_CPU
} ModesStartupKind;

ModesStartupKind modes_parse_startup(int argc, char **argv);
void modes_apply_startup(char *grid, int gw, int gh, ModesStartupKind k);
void pattern_load_preset(char *grid, int gw, int gh, int id);
const char *pattern_name(int id);

#endif
