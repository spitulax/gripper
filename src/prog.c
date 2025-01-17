#include "prog.h"
#include "compositors.h"
#include "utils.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

void usage(void) {
    printf("Usage: %s <mode> [options]\n", g_config->prog_name);
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
    printf("    -f <path>           Where the screenshot is saved to.\n");
    printf("                        Overrides -d and --format.\n");
    printf("                        The extension must be a valid type as described below.\n");
    printf("    --format <str>      The format of the output file name.\n");
    printf("                        The format is described in `--help output-format`.\n");
    printf("    -t <type>           The image type. Defaults to png.\n");
    printf("                        Valid types: ");
    print_valid_imgtypes(stdout, true);
    printf("    -o <output>         The output/monitor name to capture.\n");
    printf("                        Ignored outside of mode `full`.\n");
    printf("    -w <sec>            Wait for given seconds before capturing.\n");
    printf("    -s <factor>         Scale the final image.\n");
    printf("    --png-level <n>     PNG compression level from 0 to 9.\n");
    printf("                        Defaults to 6 (for -t png, ignored elsewhere).\n");
    printf("    --jpeg-quality <n>  JPEG quality from 0 to 100.\n");
    printf("                        Defaults to 80 (for -t jpeg, ignored elsewhere).\n");
    printf("    --no-save-region    Don't cache the region that would be captured.\n");
    printf("                        This means it will not override the region that\n");
    printf("                        should be used by last-region.\n");
    printf("                        Used in mode region and active-window.\n");
    printf("    --no-save           Don't save the captured image anywhere.\n");
    printf("                        Overrides --save and --copy.\n");
    printf("    --verbose           Print extra output.\n");
}

void usage_output_format(void) {
    printf("Output filename format:\n");
    printf("    %%Y: Full year (4 digits)\n");
    printf("    %%y: Year since 2000 (2 digits)\n");
    printf("    %%M: Month of the year (2 digits)\n");
    printf("    %%d: Day of the month (2 digits)\n");
    printf("    %%h: Hour in 24-hour format (2 digits)\n");
    printf("    %%H: Hour in 12-hour format (2 digits)\n");
    printf("    %%p: 'AM' or 'PM'\n");
    printf("    %%m: Minute (2 digits)\n");
    printf("    %%s: Second (2 digits)\n");
    printf("    %%%%: Literal percent\n");
    printf("\n");
    printf("Example: 'Screenshot_%%y-%%M-%%d_%%h-%%m-%%s' => 'Screenshot_25-01-13_02-54-46.png'\n");
}

const char *next_arg(char ***it) {
    if (**it == NULL) return NULL;
    return *((*it)++);
}

bool parse_mode_args(char ***it, Config *config) {
    const char *arg = *(*it - 1);
    if (strcmp(arg, "full") == 0) {
        config->mode = MODE_FULL;
    } else if (strcmp(arg, "region") == 0) {
        config->mode = MODE_REGION;
    } else if (strcmp(arg, "last-region") == 0) {
        config->mode = MODE_LAST_REGION;
    } else if (strcmp(arg, "active-window") == 0) {
        config->mode = MODE_ACTIVE_WINDOW;
    } else if (strcmp(arg, "custom") == 0) {
        const char *subarg = next_arg(it);
        if (subarg == NULL) {
            eprintf("custom: Unspecified region\n");
            return false;
        }
        config->mode   = MODE_CUSTOM;
        config->region = subarg;
    } else if (strcmp(arg, "test") == 0) {
        config->mode = MODE_TEST;
    } else {
        eprintf("Unknown mode: %s\n", arg);
        return false;
    }

    return true;
}

typedef enum {
    POST_ARG_NONE    = 0,
    POST_ARG_NO_SAVE = 1 << 0,    // Overrides `save_mode`
} PostArg;

void post_parse_args(Config *config, uint32_t post_args) {
    if (post_args & POST_ARG_NO_SAVE) config->save_mode = SAVEMODE_NONE;

    imgtype_remap(config);
}

ParseArgsResult parse_args(int argc, char *argv[], Config *config) {
#define FAILED    PARSE_ARGS_RESULT_FAILED
#define TERMINATE PARSE_ARGS_RESULT_TERMINATE
#define OK        PARSE_ARGS_RESULT_OK

    assert(argv[argc] == NULL);
    char **it = argv;
    // The first argument should be present
    if (next_arg(&it) == NULL) return FAILED;

    const char *mode = next_arg(&it);
    if (mode == NULL) {
        eprintf("Unspecified mode\n");
        return FAILED;
    }

    if (strcmp(mode, "--help") == 0 || strcmp(mode, "-h") == 0) {
        const char *arg = next_arg(&it);
        if (arg == NULL) {
            usage();
        } else if (streq(arg, "output-format")) {
            usage_output_format();
        } else {
            usage();
        }
        return TERMINATE;
    } else if (strcmp(mode, "--version") == 0 || strcmp(mode, "-v") == 0) {
        printf("%s %s\n", config->prog_name, config->prog_version);
        return TERMINATE;
    } else if (strcmp(mode, "--check") == 0) {
        comp_print_support(config->compositor);
        check_requirements();
        return TERMINATE;
    }
    if (!parse_mode_args(&it, config)) return FAILED;

    uint32_t post_args           = POST_ARG_NONE;
    bool     specified_save_mode = false;

    const char *arg;
    while ((arg = next_arg(&it)) != NULL) {
        if (streq(arg, "--verbose")) {
            config->verbose = true;
        } else if (streq(arg, "-c")) {
            config->cursor = true;
        } else if (streq(arg, "-d")) {
            if ((config->screenshot_dir = next_arg(&it)) == NULL) {
                eprintf("-d: Unspecified directory\n");
                return FAILED;
            }
        } else if (streq(arg, "-o")) {
            if ((config->output_name = next_arg(&it)) == NULL) {
                eprintf("-o: Unspecified output name\n");
                return FAILED;
            }
            config->all_outputs = false;
        } else if (streq(arg, "--all")) {
            config->output_name = NULL;
            config->all_outputs = true;
        } else if (streq(arg, "--no-save-region")) {
            config->no_cache_region = true;
        } else if (streq(arg, "--save")) {
            if (!specified_save_mode) {
                config->save_mode   = SAVEMODE_DISK;
                specified_save_mode = true;
            } else {
                config->save_mode |= SAVEMODE_DISK;
            }
        } else if (streq(arg, "--copy")) {
            if (!specified_save_mode) {
                config->save_mode   = SAVEMODE_CLIPBOARD;
                specified_save_mode = true;
            } else {
                config->save_mode |= SAVEMODE_CLIPBOARD;
            }
        } else if (streq(arg, "--no-save")) {
            post_args |= POST_ARG_NO_SAVE;
        } else if (streq(arg, "-t")) {
            const char *type = next_arg(&it);
            if (type == NULL) {
                eprintf("-t: Unspecified file type\n");
                eprintf("Valid file types: ");
                print_valid_imgtypes(stderr, true);
                return FAILED;
            }
            if ((config->imgtype = str2imgtype(type)) == IMGTYPE_NONE) {
                eprintf("-t: Invalid file type: %s\n", type);
                eprintf("Valid file types: ");
                print_valid_imgtypes(stderr, true);
                return FAILED;
            }
        } else if (streq(arg, "--png-level")) {
            const char *level_str = next_arg(&it);
            if (level_str == NULL) {
                eprintf("--png-level: Unspecified level\n");
                return FAILED;
            }
            config->png_level = atoui(level_str);
            if (config->png_level < 0 || config->png_level > 9) {
                eprintf("--png-level: Input a number between 0-9\n");
                return FAILED;
            }
        } else if (streq(arg, "--jpeg-quality")) {
            const char *quality_str = next_arg(&it);
            if (quality_str == NULL) {
                eprintf("--jpeg-quality: Unspecified quality\n");
                return FAILED;
            }
            config->jpeg_quality = atoui(quality_str);
            if (config->jpeg_quality < 0 || config->jpeg_quality > 100) {
                eprintf("--jpeg-quality: Input a number between 0-100\n");
                return FAILED;
            }
        } else if (streq(arg, "-f")) {
            if ((config->output_path = next_arg(&it)) == NULL) {
                eprintf("-f: Unspecified path\n");
                return FAILED;
            }
            if (!containing_dir_exists(config->output_path)) return FAILED;
            const char *type = file_ext(config->output_path);
            if ((config->imgtype = str2imgtype(type)) == IMGTYPE_NONE) {
                if (*type == '\0')
                    eprintf("-f: Unspecified file extension. Add extension to the file name\n");
                else
                    eprintf("-f: Invalid file extension: %s\n", type);
                eprintf("Valid file types/extensions: ");
                print_valid_imgtypes(stderr, true);
                return FAILED;
            }
        } else if (streq(arg, "-s")) {
            const char *scale_str = next_arg(&it);
            if (scale_str == NULL) {
                eprintf("-s: Unspecified scale\n");
                return FAILED;
            }
            config->scale = strtod(scale_str, NULL);
            if (config->scale <= 0) {
                eprintf("-s: Input a number greater than 0\n");
                return FAILED;
            } else if (config->scale == HUGE_VAL) {
                eprintf("-s: Input number is too big\n");
                return FAILED;
            }
        } else if (streq(arg, "-w")) {
            const char *time_str = next_arg(&it);
            if (time_str == NULL) {
                eprintf("-w: Unspecified time\n");
                return FAILED;
            }
            int wait_time = atoui(time_str);
            if (wait_time < 0) {
                eprintf("-w: Input a positive number\n");
                return FAILED;
            } else {
                config->wait_time = (uint32_t)wait_time;
            }
        } else if (streq(arg, "--format")) {
            if ((config->output_format = next_arg(&it)) == NULL) {
                eprintf("--foramt: Unspecified format\n");
                return FAILED;
            }
        } else {
            eprintf("Unknown argument: %s\n", arg);
            return FAILED;
        }
    }

#ifndef DEBUG
    if (config->mode == MODE_TEST) return FAILED;
#endif

    post_parse_args(config, post_args);
    return OK;

#undef FAILED
#undef TERMINATE
#undef OK
}

void config_init(Config *config) {
    config->compositor    = str2compositor(getenv("XDG_CURRENT_DESKTOP"));
    config->output_format = "Screenshot_%Y%M%d_%h%m%s";
    config->imgtype       = IMGTYPE_PNG;
    config->png_level     = DEFAULT_PNG_LEVEL;
    config->jpeg_quality  = DEFAULT_JPEG_QUALITY;
    config->save_mode     = SAVEMODE_DISK | SAVEMODE_CLIPBOARD;
    config->scale         = 1.0;
    config->wait_time     = 0;
}
