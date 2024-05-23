#ifndef PROG_H
#define PROG_H

#include "utils/arena.h"
#include <stdbool.h>

typedef enum {
    MODE_FULL,
    MODE_REGION,
    MODE_LAST_REGION,
    MODE_ACTIVE_WINDOW,
    MODE_COUNT,
} Mode;

typedef enum {
    COMP_NONE,
    COMP_HYPRLAND,
    COMP_COUNT,
} Compositor;

typedef struct {
    Arena arena;

    const char *prog_name;
    const char *prog_version;
    const char *path;
    const char *screenshot_dir;
    const char *last_region_file;
    Compositor  compositor;
    bool        compositor_supported;
    Mode        mode;

    bool verbose;
    bool cursor;
} Config;

int  parse_args(int argc, char *argv[], Config *config);
void print_comp_support(bool supported);

#endif /* ifndef PROG_H */
