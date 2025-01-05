#include "capture.h"
#include "compositors.h"
#include "grim.h"
#include "memplus.h"
#include "prog.h"
#include "unistd.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// NOTE: replaces last character of `region` into newline
bool cache_region(char *region, ssize_t nbytes) {
    if (g_config->no_cache_region) return true;

    bool  result            = true;
    FILE *region_cache_file = NULL;

    if (!make_dir(g_config->cache_dir)) return_defer(false);

    region_cache_file = fopen(g_config->last_region_file, "w+");
    if (region_cache_file == NULL) {
        eprintf("Failed to open %s: %s\n", g_config->last_region_file, strerror(errno));
        return_defer(false);
    }
    // bring back the newline before writing the region to the file
    // because some editors automatically inserts newline to the file when you save it
    region[nbytes - 1] = '\n';
    if (fputs(region, region_cache_file) < 0) {
        eprintf("Failed to write to %s\n", g_config->last_region_file);
        return_defer(false);
    }

defer:
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_full(void) {
    if (g_config->verbose) printf("*Capturing fullscreen*\n");
    if (!grim(NULL)) return false;
    return true;
}

bool capture_region(void) {
    if (g_config->verbose) printf("*Capturing region*\n");

    char *region = mp_allocator_alloc(g_alloc, DEFAULT_OUTPUT_SIZE);

    if (g_config->verbose) {
        if (comp_supported(g_config->compositor)) {
            printf("Snap selection to windows is enabled\n");
        } else {
            printf("Snap selection to windows is disabled\n");
        }
    }

    const char *slurp_cmd = "slurp -d -b '" SLURP_BG_COLOUR "' -c '" SLURP_BORDER_COLOUR
                            "' -B '" SLURP_OPTION_BOX_COLOUR "' -s '" SLURP_SELECTION_COLOUR "'";
    assert(g_config->compositor != COMP_COUNT);
    const char *windows_cmd = mp_string_new(g_alloc, comp_windows_cmds[g_config->compositor]).cstr;

    const char *cmd;
    if (windows_cmd != NULL) {
        cmd = mp_string_newf(g_alloc, "%s | %s", windows_cmd, slurp_cmd).cstr;
    } else {
        cmd = mp_string_newf(g_alloc, "%s", slurp_cmd).cstr;
    }

    ssize_t bytes = run_cmd(cmd, region, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || region == NULL) {
        eprintf("Selection cancelled\n");
        return false;
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (g_config->verbose) printf("Selected region: %s\n", region);

    if (!grim(region)) return false;

    if (!cache_region(region, bytes)) return false;

    return true;
}

bool capture_last_region(void) {
    if (g_config->verbose) printf("*Capturing last region*\n");

    bool  result            = true;
    FILE *region_cache_file = NULL;
    char *region            = mp_allocator_alloc(g_alloc, DEFAULT_OUTPUT_SIZE);

    region_cache_file = fopen(g_config->last_region_file, "r");
    if (region_cache_file == NULL) {
        if (errno == ENOENT) {
            eprintf("Run `%s region` to set the region used for this mode\n", g_config->prog_name);
            return_defer(false);
        } else {
            eprintf("Failed to open %s: %s\n", g_config->last_region_file, strerror(errno));
            return_defer(false);
        }
    }

    size_t bytes = fread(region, 1, DEFAULT_OUTPUT_SIZE, region_cache_file);
    if (ferror(region_cache_file) != 0) {
        eprintf("Failed to read %s\n", g_config->last_region_file);
        eprintf("Remove it and run `%s region` to set the region\n", g_config->prog_name);
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (!verify_geometry(region)) return_defer(false);

    if (g_config->verbose) printf("Selected region: %s\n", region);
    if (!grim(region)) return_defer(false);

defer:
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_active_window(void) {
    comp_check_support(g_config->compositor, false);

    if (g_config->verbose) printf("*Capturing active window*\n");

    char *region = mp_allocator_alloc(g_alloc, DEFAULT_OUTPUT_SIZE);

    assert(!(g_config->compositor == COMP_NONE || g_config->compositor == COMP_COUNT));
    const char *cmd   = comp_active_window_cmds[g_config->compositor];
    ssize_t     bytes = run_cmd(cmd, region, DEFAULT_OUTPUT_SIZE);
    // NOTE: If no active window, the output will probably be "null,null nullxnull"
    // which will fail the third check
    if (bytes == -1 || region == NULL || !isdigit((int)region[0])) {
        eprintf("Failed to get information about window position\n");
        return false;
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (!grim(region)) return false;
    if (!cache_region(region, bytes)) return false;

    return true;
}

bool capture_custom(void) {
    if (g_config->verbose) printf("*Capturing custom region*\n");

    if (!grim(g_config->region)) return false;
    if (!cache_region(mp_string_newf(g_alloc, "%s\0", g_config->region).cstr,
                      (ssize_t)strlen(g_config->region) + 1))
        return false;

    return true;
}

bool capture(void) {
    if (g_config->region != NULL)
        if (!verify_geometry(g_config->region)) return false;

    if (g_config->mode != MODE_FULL && (g_config->output_name != NULL || g_config->all_outputs)) {
        eprintf("\033[1;33m");
        eprintf("Warning: Flag -o and --all is ignored outside of mode `full`\n");
        eprintf("\033[0m");
    }

    if (g_config->verbose) {
        printf("====================\n");
        if (g_config->mode == MODE_FULL) {
            printf("Output                  : %s\n",
                   (g_config->output_name == NULL) ? "All" : g_config->output_name);
        }
        if (g_config->mode == MODE_CUSTOM) {
            printf("Region                  : %s\n", g_config->region);
        }
        printf("Screenshot directory    : %s\n", g_config->screenshot_dir);
        printf("Output path             : %s\n", g_config->output_path);
        printf("Last region cache       : %s\n", g_config->last_region_file);
        printf("Compositor              : %s\n", compositor2str(g_config->compositor));
        printf("Mode                    : %s\n", mode2str(g_config->mode));
        printf("Cursor                  : %s\n", g_config->cursor ? "Shown" : "Hidden");
        printf("Save to                 : %s\n", savemode2str(g_config->save_mode));
        printf("Scale                   : %.1f\n", g_config->scale);
        printf("Image type              : %s\n", imgtype2str(g_config->imgtype));
        switch (g_config->imgtype) {
            case IMGTYPE_PNG : {
                printf("PNG compression level   : %d\n", g_config->png_level);
            } break;
            case IMGTYPE_JPG :
            case IMGTYPE_JPEG : {
                printf("JPEG quality:           : %d\n", g_config->jpeg_quality);
            } break;
            case IMGTYPE_PPM :   break;
            case IMGTYPE_NONE :
            case IMGTYPE_COUNT : {
                unreachable();
            }
        }
        printf("====================\n");
    }

    bool ok = false;
    switch (g_config->mode) {
        case MODE_FULL : {
            ok = capture_full();
        } break;
        case MODE_REGION : {
            ok = capture_region();
        } break;
        case MODE_LAST_REGION : {
            ok = capture_last_region();
        } break;
        case MODE_ACTIVE_WINDOW : {
            ok = capture_active_window();
        } break;
        case MODE_CUSTOM : {
            ok = capture_custom();
        } break;
        case MODE_TEST : {
            eprintf("There's nothing here yet :)\n");
            return true;
        } break;
    }

    if (!ok) return false;

    if (g_config->save_mode == SAVEMODE_NONE) return true;
    const char *name = NULL;
    if (g_config->save_mode == SAVEMODE_DISK) {
        name = g_config->output_path;
    } else if (g_config->save_mode == SAVEMODE_CLIPBOARD) {
        name = "clipboard";
    } else if (g_config->save_mode == (SAVEMODE_DISK | SAVEMODE_CLIPBOARD)) {
        name = alloc_strf("%s and clipboard", g_config->output_path).cstr;
    }
    printf("Saved to %s\n", name);

    return true;
}
