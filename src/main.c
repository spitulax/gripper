#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PROG_NAME    "wayshot"
#define PROG_VERSION "v0.1.0"

#define DEFAULT_DIR       "Pictures/Screenshots"
#define LAST_REGION_FNAME "wayshot-last-region"

#define return_defer(v)                                                                            \
    do {                                                                                           \
        result = (v);                                                                              \
        goto defer;                                                                                \
    } while (0)

#define eprintf(...)                                                                               \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
    } while (0)

typedef enum {
    COMP_NONE,
    COMP_HYPRLAND,
    COMP_COUNT,
} Compositor;

const char *compositor_name[COMP_COUNT] = {
    [COMP_NONE]     = "Not supported",
    [COMP_HYPRLAND] = "Hyprland",
};

typedef enum {
    MODE_FULL,
    MODE_REGION,
    MODE_LAST_REGION,
    MODE_ACTIVE_WINDOW,
    MODE_COUNT,
} Mode;

const char *mode_name[MODE_COUNT] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
};

typedef struct {
    const char *path;
    const char *screenshot_dir;
    const char *last_region_file;
    Compositor  compositor;
    bool        compositor_supported;
    Mode        mode;
    bool        verbose;
} Config;

char      *alloc_strf(const char *fmt, ...);
Compositor str2compositor(const char *str);

void usage(Config *config);
int  parse_args(int argc, char *argv[], Config *config);
bool capture(Config *config);

int main(int argc, char *argv[]) {
    int   result = EXIT_SUCCESS;
    char *cmd    = NULL;
    // variables that will be freed after defer must be initialized before any call to return_defer
    char *alt_dir               = NULL;
    char *last_region_file_path = NULL;

    static Config config;
    config.compositor           = str2compositor(getenv("XDG_CURRENT_DESKTOP"));
    config.compositor_supported = config.compositor != COMP_NONE;
    config.screenshot_dir       = NULL;

    int parse_result = parse_args(argc, argv, &config);
    switch (parse_result) {
        case 2 :  break;
        case 1 :  return_defer(EXIT_SUCCESS);
        case 0 :  return_defer(EXIT_FAILURE);
        default : assert(0 && "unreachable");
    }

    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        eprintf("Environment variable HOME is not set\n");
        return_defer(EXIT_FAILURE);
    }

    // Asssign config.screenshot_dir
    const char *screenshot_dir = getenv("SCREENSHOT_DIR");
    if (config.screenshot_dir == NULL) {
        if (screenshot_dir == NULL) {
            alt_dir = alloc_strf("%s/" DEFAULT_DIR, home_dir);
            if (alt_dir == NULL) {
                eprintf("Could not allocate the path to screenshot directory\n");
                return_defer(EXIT_FAILURE);
            }
            config.screenshot_dir = alt_dir;
        } else {
            config.screenshot_dir = screenshot_dir;
        }
    }

    // Create the directory if it didn't exist
    {
        struct stat dir_stat;
        if (stat(config.screenshot_dir, &dir_stat)) {
            cmd     = alloc_strf("mkdir -p %s", config.screenshot_dir);
            int ret = system(cmd);
            free(cmd);
            if (ret == 0) {
                printf("Created %s\n", config.screenshot_dir);
            } else {
                eprintf("Could not create directory %s\n", config.screenshot_dir);
                return_defer(EXIT_FAILURE);
            }
        }
    }

    // Assign config.last_region_file
    const char *cache_dir = getenv("XDG_CACHE_HOME");
    if (cache_dir == NULL) {}
    last_region_file_path = alloc_strf("%s/" LAST_REGION_FNAME, cache_dir);
    if (last_region_file_path == NULL) {
        eprintf("Could not allocate the path to %s/" LAST_REGION_FNAME "\n", cache_dir);
        return_defer(EXIT_FAILURE);
    }
    config.last_region_file = last_region_file_path;

    // Create the file if it didn't exist
    {
        struct stat file_stat;
        if (stat(config.last_region_file, &file_stat) == -1) {
            if (errno != EACCES) {
                cmd     = alloc_strf("touch %s", config.last_region_file);
                int ret = system(cmd);
                free(cmd);
                if (ret != 0) {
                    eprintf("Could not create file %s\n", config.last_region_file);
                    return_defer(EXIT_FAILURE);
                }
                printf("Created %s\n", config.last_region_file);
            } else {
                eprintf("Could not access %s: %s\n", config.last_region_file, strerror(errno));
                return_defer(EXIT_FAILURE);
            }
        }
    }

    if (!capture(&config)) return_defer(EXIT_FAILURE);

defer:
    free(alt_dir);
    free(last_region_file_path);
    return result;
}

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
                printf(PROG_NAME " " PROG_VERSION "\n");
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

char *alloc_strf(const char *fmt, ...) {
    char   *result = NULL;
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    if (size < 0) {
        // NOTE: since path to screenshot directory is the only string costumizable by the user
        // we shouldn't need to specify all possibilities
        eprintf("Error encoding the screenshot directory path\n");
        return_defer(NULL);
    }
    va_end(args);

    va_start(args, fmt);
    char *ptr = malloc(size + 1);
    if (ptr == NULL) return_defer(NULL);
    int result_size = vsnprintf(ptr, size + 1, fmt, args);
    if (result_size != size) {
        eprintf("Could not write string to buffer: %s\n", strerror(errno));
        free(ptr);
        return_defer(NULL);
    }
    return_defer(ptr);

defer:
    va_end(args);
    return result;
}

Compositor str2compositor(const char *str) {
    if (strcmp(str, "Hyprland") == 0) {
        return COMP_HYPRLAND;
    } else {
        return COMP_NONE;
    }
}

void print_comp_support(bool supported) {
    printf("Your compositor is ");
    printf(supported ? "supported.\n" : "not supported.\n");
    printf("Mode active-window is unavailable for unsupported compositor and\n");
    printf("mode region does not have snap to window.\n");
}

void usage(Config *config) {
    printf("Usage: " PROG_NAME " <mode> [options]\n");
    printf("\n");
    printf("Modes:\n");
    printf("    full                Capture fullscreen\n");
    printf("    region              Capture selected region using slurp\n");
    printf("    last-region         Capture last selected region\n");
    printf("    active-window       Capture active window\n");
    printf("    --help, -h          Show this help\n");
    printf("    --version, -v       Show version\n");
    printf("Options:\n");
    printf("    -d <dir>            Where the screenshot is saved (defaults to environment\n");
    printf("                        variable SCREENSHOT_DIR or ~/Pictures/Screenshots)\n");
    // TODO:
    // printf("    -f <path>           Where the screenshot is saved to (overrides -d)\n");
    printf("    --verbose           Print extra output\n");
    printf("\n");
    print_comp_support(config->compositor_supported);
}

ssize_t run_cmd(char *buf, size_t nbytes, const char *cmd) {
    int     pipe_fd[2] = { 0 };
    ssize_t bytes_read = -1;

    if (pipe(pipe_fd) == -1) {
        eprintf("Could not create pipes: %s\n", strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        eprintf("Could not fork child process: %s\n", strerror(errno));
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    } else if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[1]);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        eprintf("Could not run `%s`: %s\n", cmd, strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        close(pipe_fd[1]);

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            eprintf("`%s` could not terminate: %s\n", cmd, strerror(errno));
            return -1;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            return -1;
        }

        bytes_read      = read(pipe_fd[0], buf, nbytes - 1);
        buf[bytes_read] = '\0';
        close(pipe_fd[0]);
    }

    return bytes_read;
}

char *get_fname(const char *dir) {
    time_t    now_time = time(NULL);
    struct tm now;

    if (localtime_r(&now_time, &now) == NULL) {
        eprintf("Could not get time\n");
        return NULL;
    }

    char *ptr = alloc_strf("%s/Screenshot_%04d%02d%02d_%02d%02d%02d.png",
                           dir,
                           now.tm_year + 1900,
                           now.tm_mon + 1,
                           now.tm_mday,
                           now.tm_hour,
                           now.tm_min,
                           now.tm_sec);

    if (ptr == NULL) return NULL;

    return ptr;
};

bool grim(Config *config, Mode mode, const char *region) {
    bool  result = true;
    char *cmd    = NULL;
    char *fname  = NULL;

    switch (mode) {
        case MODE_FULL : {
            cmd = alloc_strf("grim -c - | wl-copy");
        } break;
        case MODE_REGION : {
            cmd = alloc_strf("grim -g \"%s\" - | wl-copy", region);
        } break;
        default : {
            assert(0 && "unimplemented");
        }
    };

    if (config->verbose) printf("$ %s\n", cmd);
    if (system(cmd) != 0) {
        eprintf("Could not run grim\n");
        return_defer(false);
    }

    fname = get_fname(config->screenshot_dir);
    if (fname == NULL) return_defer(false);

    free(cmd);
    cmd = alloc_strf("wl-paste > %s", fname);
    if (config->verbose) printf("$ %s\n", cmd);
    if (system(cmd) != 0) {
        eprintf("Could save image to %s\n", config->screenshot_dir);
        return_defer(false);
    }

defer:
    free(cmd);
    free(fname);
    return result;
}

bool capture_full(Config *config) {
    if (config->verbose) printf("*Capturing fullscreen*\n");
    if (!grim(config, MODE_FULL, NULL)) return false;
    return true;
}

bool capture_region(Config *config) {
    bool         result      = true;
    const size_t region_size = 1024;
    char        *region      = malloc(region_size);
    if (region == NULL) {
        eprintf("Could not allocate output for running slurp\n");
    }

    if (config->verbose) printf("*Capturing region*\n");

    ssize_t bytes = run_cmd(region, region_size, "slurp");
    if (bytes == -1) {
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
    (void)config;
    printf("Capture last region\n");
    return true;
}

bool capture_active_window(Config *config) {
    (void)config;
    printf("Capture active window\n");
    return true;
}

bool capture(Config *config) {
    if (config->verbose) {
        printf("====================\n");
        printf("Screenshot directory    : %s\n", config->screenshot_dir);
        printf("Last region cache       : %s\n", config->last_region_file);
        printf("Compositor              : %s\n", compositor_name[config->compositor]);
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
