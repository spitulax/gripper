#include "prog.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

bool parse_mode_args(size_t *i, int argc, char *argv[], Config *config) {
    const char *arg = argv[*i];
    if (strcmp(arg, "full") == 0) {
        config->mode = MODE_FULL;
    } else if (strcmp(arg, "region") == 0) {
        config->mode = MODE_REGION;
    } else if (strcmp(arg, "last-region") == 0) {
        config->mode = MODE_LAST_REGION;
    } else if (strcmp(arg, "active-window") == 0) {
        config->mode = MODE_ACTIVE_WINDOW;
    } else if (strcmp(arg, "custom") == 0) {
        if (*i + 1 >= (size_t)argc) return false;
        config->mode   = MODE_CUSTOM;
        config->region = argv[++*i];
    } else if (strcmp(arg, "test") == 0) {
        config->mode = MODE_TEST;
    } else {
        return false;
    }

    return true;
}

void print_comp_support(bool supported) {
    printf("Your compositor is ");
    printf(supported ? "supported.\n" : "not supported.\n");
    printf("    Mode `active-window` is unavailable for unsupported compositor and\n");
    printf("    mode `region` does not have snap to window.\n");
    printf("    Mode `full` is also unable to pick current output automatically.\n");
    printf("    You must specify it yourself with `-o` or it will capture all outputs\n");
    printf("    which is the behaviour of `--all`.\n");
}

void check_requirements(void) {
    const char *cmds[] = {
        "grim",
        "slurp",
        "jq",
    };
    const char *cmds_optional[] = {
        "wl-copy",
        "notify-send",
    };

    printf("Check if needed commands are found:\n");
    for (size_t i = 0; i < array_len(cmds); ++i)
        printf(" - %s: %s\n", cmds[i], command_found(cmds[i]) ? "found" : "not found");
    printf("Optional commands:\n");
    for (size_t i = 0; i < array_len(cmds_optional); ++i)
        printf(" - %s: %s\n",
               cmds_optional[i],
               command_found(cmds_optional[i]) ? "found" : "not found");
}

int atoui(const char *str) {
    if (str[0] == 48 && str[1] == '\0') return 0;
    int result = atoi(str);
    if (result == 0)
        return -1;
    else
        return result;
}

void usage(Config *config) {
    printf("Usage: %s <mode> [options]\n", config->prog_name);
    printf("\n");
    printf("Modes:\n");
    printf("    full                Capture fullscreen.\n");
    printf("    region              Capture selected region using slurp.\n");
    printf("    active-window       Capture active window.\n");
    printf("    last-region         Capture last selected region.\n");
    printf("    custom <region>     Capture custom region.\n");
    printf("                        The format must be 'X,Y WxH'.\n");
    printf("    --help, -h          Show this help.\n");
    printf("    --version, -v       Show version.\n");
    printf("    --check             Check compositor support and needed commands.\n");
    printf("Options:\n");
    printf("    -c                  Include cursor in the screenshot.\n");
    printf("    --all               Capture all outputs.\n");
    printf("                        Ignored outside of mode `full`.\n");
    printf("    --save              Save the captured image only to disk.\n");
    printf("    --copy              Save the captured image only to clipboard.\n");
    printf("    -d <dir>            Where the screenshot is saved (defaults to environment\n");
    printf("                        variable SCREENSHOT_DIR or ~/Pictures/Screenshots).\n");
    printf("    -f <path>           Where the screenshot is saved to (overrides -d).\n");
    printf("    -o <output>         The output/monitor name to capture.\n");
    printf("                        Ignored outside of mode `full`.\n");
    printf("    -w <sec>            Wait for given seconds before capturing.\n");
    printf("    -s <factor>         Scale the final image.\n");
    printf("    -t <png|ppm|jpeg>   The image type. Defaults to png.\n");
    printf("    --png-level <n>     PNG compression level from 0 to 9.\n");
    printf("                        Defaults to 6 (for -t png, ignored elsewhere).\n");
    printf("    --jpeg-quality <n>  JPEG quality from 0 to 100.\n");
    printf("                        Defaults to 80 (for -t jpeg, ignored elsewhere).\n");
    printf("    --no-save-region    Don't cache the region that would be captured.\n");
    printf("                        This means it will not override the region that\n");
    printf("                        should be used by last-region.\n");
    printf("                        Used in mode region and active-window.\n");
    printf("    --no-save           Don't save the captured image anywhere.\n");
    printf("    --verbose           Print extra output.\n");
}

// 0 - arguments passed incorrectly
// 1 - arguments passed correctly and immediately end the program
// 2 - arguments passed correctly and proceed
int parse_args(int argc, char *argv[], Config *config) {
    if (argc < 2) return 0;

    size_t i = 1;
    for (; i < (size_t)argc; ++i) {
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
            if (!parse_mode_args(&i, argc, argv, config)) break;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            config->cursor = true;
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->screenshot_dir = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->output_name = argv[++i];
            config->all_outputs = false;
        } else if (strcmp(argv[i], "--all") == 0) {
            config->output_name = NULL;
            config->all_outputs = true;
        } else if (strcmp(argv[i], "--no-save-region") == 0) {
            config->no_cache_region = true;
        } else if (strcmp(argv[i], "--save") == 0) {
            config->save_mode = SAVEMODE_DISK;
        } else if (strcmp(argv[i], "--copy") == 0) {
            config->save_mode = SAVEMODE_CLIPBOARD;
        } else if (strcmp(argv[i], "--no-save") == 0) {
            config->save_mode = SAVEMODE_NONE;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= (size_t)argc) break;
            const char *type = argv[++i];
            if (strcmp(type, "png") == 0) {
                config->imgtype = IMGTYPE_PNG;
            } else if (strcmp(type, "ppm") == 0) {
                config->imgtype = IMGTYPE_PPM;
            } else if (strcmp(type, "jpeg") == 0) {
                config->imgtype = IMGTYPE_JPEG;
            } else {
                eprintf("Invalid file type `%s`\n", type);
                return 0;
            }
        } else if (strcmp(argv[i], "--png-level") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->png_level = atoui(argv[++i]);
            if (config->png_level < 0 || config->png_level > 9) {
                eprintf("Input a number between 0-9\n");
                return 0;
            }
        } else if (strcmp(argv[i], "--jpeg-quality") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->jpeg_quality = atoui(argv[++i]);
            if (config->jpeg_quality < 0 || config->jpeg_quality > 100) {
                eprintf("Input a number between 0-100\n");
                return 0;
            }
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->output_path = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->scale = strtod(argv[++i], NULL);
            if (config->scale <= 0) {
                eprintf("Input a number greater than 0\n");
                return 0;
            } else if (config->scale == HUGE_VAL) {
                eprintf("Input number overflew\n");
                return 0;
            }
        } else if (strcmp(argv[i], "-w") == 0) {
            if (i + 1 >= (size_t)argc) break;
            config->wait_time = atoui(argv[++i]);
            if (config->wait_time < 0) {
                eprintf("Input a positive number\n");
                return 0;
            }
        } else {
            return 0;
        }
    }

#ifndef DEBUG
    if (config->mode == MODE_TEST) return 0;
#endif

    if (i == (size_t)argc) {
        return 2;
    } else {
        return 0;
    }
}

void prepare_options(Config *config) {
    config->compositor           = str2compositor(getenv("XDG_CURRENT_DESKTOP"));
    config->compositor_supported = config->compositor != COMP_NONE;
    config->imgtype              = IMGTYPE_PNG;
    config->png_level            = DEFAULT_PNG_LEVEL;
    config->jpeg_quality         = DEFAULT_JPEG_QUALITY;
    config->save_mode            = SAVEMODE_DISK | SAVEMODE_CLIPBOARD;
    config->scale                = 1.0;
    config->wait_time            = 0;
}
