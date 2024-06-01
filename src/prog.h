#ifndef PROG_H
#define PROG_H

#include "memplus.h"
#include <stdbool.h>

typedef enum {
    MODE_FULL,
    MODE_REGION,
    MODE_LAST_REGION,
    MODE_ACTIVE_WINDOW,
    MODE_TEST,
    MODE_COUNT,
} Mode;

typedef enum {
    COMP_NONE,
    COMP_HYPRLAND,
    COMP_COUNT,
} Compositor;

typedef struct {
    mp_Allocator alloc;

    const char *prog_name;
    const char *prog_version;
    const char *path;
    const char *screenshot_dir;
    const char *last_region_file;
    Compositor  compositor;
    bool        compositor_supported;
    Mode        mode;

    bool        verbose;
    bool        cursor;
    bool        no_cache_region;
    bool        no_clipboard;
    const char *output_name;
} Config;

int  parse_args(int argc, char *argv[], Config *config);
void prepare_options(Config *config);
// TODO: check if compositor supports needed wayland protocols
void print_comp_support(bool supported);

#endif /* ifndef PROG_H */
