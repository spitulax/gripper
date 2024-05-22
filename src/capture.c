#include "capture.h"
#include "prog.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_OUTPUT_SIZE 1024

const char *mode_name[MODE_COUNT] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
};

// NOTE: appends newline into `region`
bool cache_region(Config *config, char *region, size_t nbytes) {
    bool  result            = true;
    FILE *region_cache_file = NULL;

    region_cache_file = fopen(config->last_region_file, "w+");
    if (region_cache_file == NULL) {
        eprintf("Could not open %s: %s\n", config->last_region_file, strerror(errno));
        return_defer(false);
    }
    // bring back the newline before writing the region to the file
    // because some editors automatically inserts newline to the file when you save it
    region[nbytes - 1] = '\n';
    if (fputs(region, region_cache_file) < 0) {
        eprintf("Could not write to %s\n", config->last_region_file);
        return_defer(false);
    }

defer:
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_full(Config *config) {
    if (config->verbose) printf("*Capturing fullscreen*\n");
    if (!grim(config, MODE_FULL, NULL)) return false;
    return true;
}

bool capture_region(Config *config) {
    if (config->verbose) printf("*Capturing region*\n");

    bool  result = true;
    char *region = malloc(DEFAULT_OUTPUT_SIZE);
    if (region == NULL) {
        eprintf("Could not allocate output for running slurp\n");
        return_defer(false);
    }

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
            // BUG: when you switch workspace while on slurp
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
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (config->verbose) printf("Selected region: %s\n", region);

    if (!grim(config, MODE_REGION, region)) return_defer(false);

    if (!cache_region(config, region, bytes)) return_defer(false);

defer:
    free(region);
    return result;
}

bool capture_last_region(Config *config) {
    if (config->verbose) printf("*Capturing last region*\n");

    bool  result            = true;
    FILE *region_cache_file = NULL;
    char *region            = malloc(DEFAULT_OUTPUT_SIZE);
    if (region == NULL) {
        eprintf("Could not allocate for %s\n", config->last_region_file);
        return_defer(false);
    }

    region_cache_file = fopen(config->last_region_file, "r");
    if (region_cache_file == NULL) {
        if (errno == ENOENT) {
            eprintf("Run `%s region` to set the region used for this mode\n", config->prog_name);
            return_defer(false);
        } else {
            eprintf("Could not open %s: %s\n", config->last_region_file, strerror(errno));
            return_defer(false);
        }
    }

    size_t bytes = fread(region, 1, DEFAULT_OUTPUT_SIZE, region_cache_file);
    if (ferror(region_cache_file) != 0) {
        eprintf("Could not read %s\n", config->last_region_file);
        eprintf("Remove it and run `%s region` to set the region\n", config->prog_name);
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final newline

    // TODO: check if the region format is correct

    if (config->verbose) printf("Selected region: %s\n", region);
    if (!grim(config, MODE_REGION, region)) return_defer(false);

defer:
    free(region);
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_active_window(Config *config) {
    if (!config->compositor_supported) {
        print_comp_support(config->compositor_supported);
        return false;
    }

    if (config->verbose) printf("*Capturing active window*\n");

    bool  result = true;
    char *region = malloc(DEFAULT_OUTPUT_SIZE);
    if (region == NULL) {
        eprintf("Could not allocate for region\n");
        return_defer(false);
    }

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
        eprintf("Could not get information about window position\n");
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (!grim(config, MODE_REGION, region)) return_defer(false);

    if (!cache_region(config, region, bytes)) return_defer(false);

defer:
    free(region);
    return result;
}

bool capture(Config *config) {
    if (config->verbose) {
        printf("====================\n");
        printf("Screenshot directory    : %s\n", config->screenshot_dir);
        printf("Last region cache       : %s\n", config->last_region_file);
        printf("Compositor              : %s\n", compositor2str(config->compositor));
        printf("Mode                    : %s\n", mode_name[config->mode]);
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
        case MODE_COUNT : {
            assert(0 && "unreachable");
        }
    }

    return false;
}
