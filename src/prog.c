#include "prog.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

bool parse_mode_args(const char *argv, Config *config) {
    if (strcmp(argv, "full") == 0) {
        config->mode = MODE_FULL;
    } else if (strcmp(argv, "region") == 0) {
        config->mode = MODE_REGION;
    } else if (strcmp(argv, "last-region") == 0) {
        config->mode = MODE_LAST_REGION;
    } else if (strcmp(argv, "active-window") == 0) {
        config->mode = MODE_ACTIVE_WINDOW;
    } else {
        return false;
    }

    return true;
}

void print_comp_support(bool supported) {
    printf("Your compositor is ");
    printf(supported ? "supported.\n" : "not supported.\n");
    printf("    Mode active-window is unavailable for unsupported compositor and\n");
    printf("    mode region does not have snap to window.\n");
}

void check_requirements() {
#define CMDS_COUNT 4
    const char *cmds[CMDS_COUNT] = {
        "grim",
        "slurp",
        "wl-copy",
        "notify-send",
    };

    printf("Check if needed commands are found:\n");
    for (size_t i = 0; i < CMDS_COUNT; ++i) {
        const char *msg      = command_found(cmds[i]) ? "found" : "not found";
        bool        optional = false;
        if (i == 3) optional = true;
        printf(" - %s: %s%s\n", cmds[i], msg, optional ? " (optional)" : "");
    }
}

void usage(Config *config) {
    printf("Usage: %s <mode> [options]\n", config->prog_name);
    printf("\n");
    printf("Modes:\n");
    printf("    full                Capture fullscreen\n");
    printf("    region              Capture selected region using slurp\n");
    printf("    active-window       Capture active window\n");
    printf("    last-region         Capture last selected region\n");
    printf("    --help, -h          Show this help\n");
    printf("    --version, -v       Show version\n");
    printf("    --check             Check compositor support and needed commands\n");
    printf("Options:\n");
    printf("    -d <dir>            Where the screenshot is saved (defaults to environment\n");
    printf("                        variable SCREENSHOT_DIR or ~/Pictures/Screenshots)\n");
    // TODO:
    // printf("    -f <path>           Where the screenshot is saved to (overrides -d)\n");
    // printf("    -c                  Include cursor in the screenshot\n");
    // printf("    -o <output>         The output name to capture\n");
    // printf("    --no-cache-region   Used in mode region and active-window\n");
    // printf("                        Don't cache the region that would be captured\n");
    // printf("                        This means it will not override the region that\n");
    // printf("                        should be used by last-region\n");
    // printf("     --no-clipboard     Don't copy the captured image into the clipboard\n");
    // TODO: automatically fallback to --no-clipboard if wl-copy is not found
    printf("    --verbose           Print extra output\n");
}

// 0 - arguments passed incorrectly
// 1 - arguments passed correctly and immediately end the program
// 2 - arguments passed correctly and proceed
int parse_args(int argc, char *argv[], Config *config) {
    if (argc < 2) {
        usage(config);
        return 0;
    }

    size_t i = 1;
    for (; i < (size_t)argc; i++) {
        if (i == 1) {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(config);
                return 1;
            } else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
                printf("%s %s\n", config->prog_name, config->prog_version);
                return 1;
            } else if (strcmp(argv[1], "--check") == 0) {
                print_comp_support(config->compositor_supported);
                check_requirements();
                return 1;
            }
            if (!parse_mode_args(argv[i], config)) break;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = true;
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->screenshot_dir = argv[++i];
        }
    }

    if (i == (size_t)argc) {
        return 2;
    } else {
        usage(config);
        return 0;
    }
}
