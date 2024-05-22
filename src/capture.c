#include "capture.h"
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

    bool  result = true;
    char *region = malloc(DEFAULT_OUTPUT_SIZE);
    if (region == NULL) {
        eprintf("Could not allocate output for running slurp\n");
        return_defer(false);
    }

    ssize_t bytes = run_cmd("slurp", region, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || region == NULL) {
        eprintf("Selection cancelled\n");
        return_defer(false);
    }
    region[bytes - 1] = '\0';    // trim the final endline

    if (!grim(config, MODE_REGION, region)) return_defer(false);

defer:
    free(region);
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
