#include "capture.h"
#include "grim.h"
#include "hyprland.h"
#include "memplus.h"
#include "prog.h"
#include "sway.h"
#include "unistd.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define DEFAULT_OUTPUT_SIZE 1024

// NOTE: replaces last character of `region` into newline
bool cache_region(Config *config, char *region, ssize_t nbytes) {
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

    mp_String cmd = { 0 };
    // TODO: costumize colors option?
    const char *slurp_cmd = "slurp -d -b '#101020aa' -c '#cdd6f4aa' -B '#31324450'";
    switch (config->compositor) {
        // FIXME: when you switch workspace while on slurp
        // selection still snaps onto the position of windows in initial workspace
        case COMP_HYPRLAND : {
            cmd = mp_string_new(&config->alloc, hyprland_windows);
        } break;
        case COMP_SWAY : {
            cmd = mp_string_new(&config->alloc, sway_windows);
        } break;
        case COMP_NONE : {
            cmd = mp_string_new(&config->alloc, "slurp");
        } break;
        case COMP_COUNT : {
            assert(0 && "unreachable");
        }
    }

    cmd = mp_string_newf(&config->alloc, "%s | %s", cmd.cstr, slurp_cmd);

    ssize_t bytes = run_cmd(cmd.cstr, region, DEFAULT_OUTPUT_SIZE);
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

    if (!verify_geometry(region)) return false;

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

    mp_String cmd = { 0 };
    switch (config->compositor) {
        case COMP_HYPRLAND : {
            cmd = mp_string_new(&config->alloc, hyprland_active_window);
        } break;
        case COMP_SWAY : {
            cmd = mp_string_new(&config->alloc, sway_active_window);
        } break;
        case COMP_NONE : {
            print_comp_support(config->compositor_supported);
            return false;
        }
        case COMP_COUNT : {
            assert(0 && "unreachable");
        }
    }

    ssize_t bytes = run_cmd(cmd.cstr, region, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || region == NULL) {
        eprintf("Failed to get information about window position\n");
        return false;
    }
    region[bytes - 1] = '\0';    // trim the final newline

    if (!grim(config, region)) return false;
    if (!cache_region(config, region, bytes)) return false;

    return true;
}

bool capture_custom(Config *config) {
    if (config->verbose) printf("*Capturing custom region*\n");

    if (!grim(config, config->region)) return false;
    if (!cache_region(config,
                      mp_string_newf(&config->alloc, "%s\0", config->region).cstr,
                      (ssize_t)strlen(config->region) + 1))
        return false;

    return true;
}

bool get_current_output_name(Config *config) {
    char     *output_name = mp_allocator_alloc(&config->alloc, DEFAULT_OUTPUT_SIZE);
    mp_String cmd         = { 0 };
    switch (config->compositor) {
        case COMP_HYPRLAND : {
            cmd = mp_string_new(&config->alloc, hyprland_active_monitor);
        } break;
        case COMP_SWAY : {
            cmd = mp_string_new(&config->alloc, sway_active_monitor);
        } break;
        case COMP_NONE : {
            config->output_name = NULL;
            return true;
        } break;
        case COMP_COUNT : {
            assert(0 && "unreachable");
        } break;
    }

    ssize_t bytes = run_cmd(cmd.cstr, output_name, DEFAULT_OUTPUT_SIZE);
    if (bytes == -1 || output_name == NULL) {
        eprintf("Failed to get information about current monitor\n");
        return false;
    }
    output_name[bytes - 1] = '\0';    // trim the final newline
    config->output_name    = output_name;

    return true;
}

bool capture(Config *config) {
    if (config->output_name != NULL) {
        // Verify if output exists
        mp_String cmd = mp_string_newf(
            &config->alloc, "grim -t jpeg -q 0 -o %s - >/dev/null", config->output_name);
        if (run_cmd(cmd.cstr, NULL, 0) == -1) {
            eprintf("Unknown output `%s`\n", config->output_name);
            return false;
        }
    } else if (config->mode == MODE_FULL && !config->all_outputs) {
        if (!get_current_output_name(config)) return false;
    }

    if (config->region != NULL)
        if (!verify_geometry(config->region)) return false;

    if (config->mode != MODE_FULL && (config->output_name != NULL || config->all_outputs)) {
        eprintf("\033[1;33m");
        eprintf("Warning: Flag -o and --all is ignored outside of mode `full`\n");
        eprintf("\033[0m");
    }

    if (config->verbose) {
        printf("====================\n");
        if (config->mode == MODE_FULL) {
            printf("Output                  : %s\n",
                   (config->output_name == NULL) ? "All" : config->output_name);
        }
        if (config->mode == MODE_CUSTOM) {
            printf("Region                  : %s\n", config->region);
        }
        printf("Screenshot directory    : %s\n", config->screenshot_dir);
        printf("Last region cache       : %s\n", config->last_region_file);
        printf("Compositor              : %s\n", compositor2str(config->compositor));
        printf("Mode                    : %s\n", mode2str(config->mode));
        printf("Cursor                  : %s\n", config->cursor ? "Shown" : "Hidden");
        printf("Save to                 : %s\n", savemode2str(config->save_mode));
        printf("Scale                   : %.1f\n", config->scale);
        printf("Image type              : %s\n", imgtype2str(config->imgtype));
        switch (config->imgtype) {
            case IMGTYPE_PNG : {
                printf("PNG compression level   : %d\n", config->png_level);
            } break;
            case IMGTYPE_JPEG : {
                printf("JPEG quality:           : %d\n", config->jpeg_quality);
            } break;
            case IMGTYPE_PPM : break;
        }
        printf("====================\n");
    }

    if (config->wait_time > 0) {
        if (config->verbose) printf("*Waiting for %d seconds...*\n", config->wait_time);
        sleep(config->wait_time);
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
        case MODE_CUSTOM : {
            return capture_custom(config);
        } break;
        case MODE_TEST : {
            eprintf("There's nothing here yet :)\n");
            return true;
        } break;
    }

    assert(0 && "unreachable");
}
