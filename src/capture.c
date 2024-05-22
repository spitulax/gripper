#include "capture.h"
#include "prog.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_OUTPUT_SIZE 1024

const char *mode_name[MODE_COUNT] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
};

bool capture_full(Config *config) {
    if (config->verbose) printf("*Capturing fullscreen*\n");
    if (!grim(config, MODE_FULL, NULL)) return false;
    return true;
}

bool capture_region(Config *config) {
    if (config->verbose) printf("*Capturing region*\n");

    bool  result            = true;
    FILE *region_cache_file = NULL;
    char *region            = malloc(DEFAULT_OUTPUT_SIZE);
    if (region == NULL) {
        eprintf("Could not allocate output for running slurp\n");
        return_defer(false);
    }

    char *cmd = NULL;
    switch (config->compositor) {
        case COMP_HYPRLAND : {
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
    region[bytes - 1] = '\0';    // trim the final endline

    if (config->verbose) printf("Selected region: %s\n", region);

    if (!grim(config, MODE_REGION, region)) return_defer(false);

    region_cache_file = fopen(config->last_region_file, "w+");
    fputs(region, region_cache_file);

defer:
    free(region);
    if (region_cache_file != NULL) fclose(region_cache_file);
    return result;
}

bool capture_last_region(Config *config) {
    assert(0 && "unimplemented");
    if (config->verbose) printf("Capture last region\n");
}

bool capture_active_window(Config *config) {
    assert(0 && "unimplemented");
    if (config->verbose) printf("Capture active window\n");
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
