#include "capture.h"
#include "grim.h"
#include "memplus.h"
#include "prog.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define DEFAULT_OUTPUT_SIZE 1024

// NOTE: replaces last character of `region` into newline
bool cache_region(Config *config, char *region, size_t nbytes) {
    if (config->no_cache_region) return true;

    bool  result            = true;
    FILE *region_cache_file = NULL;

    region_cache_file = fopen(config->last_region_file, "w+");
    if (region_cache_file == NULL) {
        eprintf("Failed to open %s: %s\n", config->last_region_file, strerror(errno));
        return_defer(false);
    }
    // bring back the newline before writing the region to the file
    // because some editors automatically inserts newline to the file when you save it
    region[nbytes - 1] = '\n';
    if (fputs(region, region_cache_file) < 0) {
        eprintf("Failed to write to %s\n", config->last_region_file);
        return_defer(false);
    }

defer:
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_full(Config *config) {
    if (config->verbose) printf("*Capturing fullscreen*\n");
    if (!grim(config, NULL)) return false;
    return true;
}

bool capture_region(Config *config) {
    if (config->verbose) printf("*Capturing region*\n");

    char *region = mp_allocator_alloc(&config->alloc, DEFAULT_OUTPUT_SIZE);

    if (config->verbose) {
        if (config->compositor_supported) {
            printf("Snap selection to windows is enabled\n");
        } else {
            printf("Snap selection to windows is disabled\n");
        }
    }

    char *cmd = NULL;
    switch (config->compositor) {
        case COMP_HYPRLAND : {
            // TODO: when you switch workspace while on slurp
            // selection still snaps onto the position of windows in initial workspace
            cmd =
                // lists windows in Hyprland
                "hyprctl clients -j | "
                // filters window outside of the current workspace
                "jq -r --argjson workspaces \"$(hyprctl monitors -j | jq -r 'map(.activeWorkspace.id)')\" "
                "'map(select([.workspace.id] | inside($workspaces)))' | "
                // get the position and size of each window and turn it into format slurp can read
                "jq -r '.[] | \"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"' | "
                // and those informations will be used by slurp to automatically snap selection of
                // region to the windows
                "slurp \"$@\"";
        } break;
        case COMP_NONE : {
            cmd = "slurp";
        } break;
        case COMP_COUNT : {
            assert(0 && "unreachable");
        }
    }

    ssize_t bytes = run_cmd(cmd, region, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || region == NULL) {
        eprintf("Selection cancelled\n");
        return false;
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (config->verbose) printf("Selected region: %s\n", region);

    if (!grim(config, region)) return false;

    if (!cache_region(config, region, bytes)) return false;

    return true;
}

bool capture_last_region(Config *config) {
    if (config->verbose) printf("*Capturing last region*\n");

    bool  result            = true;
    FILE *region_cache_file = NULL;
    char *region            = mp_allocator_alloc(&config->alloc, DEFAULT_OUTPUT_SIZE);

    region_cache_file = fopen(config->last_region_file, "r");
    if (region_cache_file == NULL) {
        if (errno == ENOENT) {
            eprintf("Run `%s region` to set the region used for this mode\n", config->prog_name);
            return_defer(false);
        } else {
            eprintf("Failed to open %s: %s\n", config->last_region_file, strerror(errno));
            return_defer(false);
        }
    }

    size_t bytes = fread(region, 1, DEFAULT_OUTPUT_SIZE, region_cache_file);
    if (ferror(region_cache_file) != 0) {
        eprintf("Failed to read %s\n", config->last_region_file);
        eprintf("Remove it and run `%s region` to set the region\n", config->prog_name);
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final newline

    // TODO: check if the region format is correct

    if (config->verbose) printf("Selected region: %s\n", region);
    if (!grim(config, region)) return_defer(false);

defer:
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_active_window(Config *config) {
    if (!config->compositor_supported) {
        print_comp_support(config->compositor_supported);
        return false;
    }

    if (config->verbose) printf("*Capturing active window*\n");

    char *region = mp_allocator_alloc(&config->alloc, DEFAULT_OUTPUT_SIZE);

    char *cmd = NULL;
    switch (config->compositor) {
        case COMP_HYPRLAND : {
            cmd =
                "hyprctl activewindow -j | jq -r '\"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'";
        } break;
        case COMP_NONE :
        case COMP_COUNT : {
            assert(0 && "unreachable");
        }
    }

    ssize_t bytes = run_cmd(cmd, region, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || region == NULL) {
        eprintf("Failed to get information about window position\n");
        return false;
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (!grim(config, region)) return false;

    if (!cache_region(config, region, bytes)) return false;

    return true;
}

bool capture(Config *config) {
    if (config->verbose) {
        printf("====================\n");
        printf("Screenshot directory    : %s\n", config->screenshot_dir);
        printf("Last region cache       : %s\n", config->last_region_file);
        printf("Compositor              : %s\n", compositor2str(config->compositor));
        printf("Mode                    : %s\n", mode2str(config->mode));
        printf("Cursor                  : %s\n", config->cursor ? "Shown" : "Hidden");
        printf("Clipboard               : %s\n", config->no_clipboard ? "Disabled" : "Enabled");
        print_comp_support(config->compositor_supported);
        printf("====================\n");
    }

    switch (config->mode) {
        case MODE_FULL : {
            return capture_full(config);
        } break;
        case MODE_REGION : {
            return capture_region(config);
        } break;
        case MODE_LAST_REGION : {
            return capture_last_region(config);
        } break;
        case MODE_ACTIVE_WINDOW : {
            return capture_active_window(config);
        } break;
        case MODE_TEST : {
            eprintf("There's nothing here yet :)\n");
            return true;
        } break;
        case MODE_COUNT : {
            assert(0 && "unreachable");
        }
    }

    assert(0 && "unreachable");
}
