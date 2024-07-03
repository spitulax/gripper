#ifndef PROG_H
#define PROG_H

#include "memplus.h"
#include <stdbool.h>

#define DEFAULT_PNG_LEVEL    6
#define DEFAULT_JPEG_QUALITY 80

typedef enum {
    MODE_FULL,
    MODE_REGION,
    MODE_LAST_REGION,
    MODE_ACTIVE_WINDOW,
    MODE_CUSTOM,
    MODE_TEST,
} Mode;

typedef enum {
    COMP_NONE,
    COMP_HYPRLAND,
    COMP_SWAY,
    COMP_COUNT,
} Compositor;

typedef enum {
    IMGTYPE_PNG,
    IMGTYPE_PPM,
    IMGTYPE_JPEG,
} Imgtype;

typedef enum {
    SAVEMODE_NONE      = 0,
    SAVEMODE_DISK      = 1 << 0,
    SAVEMODE_CLIPBOARD = 1 << 1,
} SaveMode;

typedef struct {
    mp_Allocator alloc;

    const char *prog_name;
    const char *prog_version;
    const char *path;
    const char *screenshot_dir;
    const char *last_region_file;
    const char *region;
    Compositor  compositor;
    bool        compositor_supported;
    Mode        mode;
    SaveMode    save_mode;

    bool        verbose;
    bool        cursor;
    bool        no_cache_region;
    double      scale;
    uint32_t    wait_time;
    const char *output_name;
    Imgtype     imgtype;
    int         png_level;
    int         jpeg_quality;
    const char *output_path;
    bool        all_outputs;
} Config;

int  parse_args(int argc, char *argv[], Config *config);
void prepare_options(Config *config);
// TODO: check if compositor supports needed wayland protocols
void print_comp_support(bool supported);

#endif /* ifndef PROG_H */
