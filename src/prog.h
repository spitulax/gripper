#ifndef PROG_H
#define PROG_H

#include "compositors.h"
#include "memplus.h"
#include <stdbool.h>
#include <stdint.h>

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
    const char *prog_name;
    const char *prog_version;
    const char *path;
    const char *screenshot_dir;
    const char *cache_dir;
    const char *last_region_file;
    const char *region;
    Compositor  compositor;
    Mode        mode;
    uint32_t    save_mode;    // bitmask of `SaveMode`

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

extern mp_Allocator *g_alloc;
extern const Config *g_config;

int  parse_args(int argc, char *argv[], Config *config);
void config_init(Config *config);

#endif /* ifndef PROG_H */
