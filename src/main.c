#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "capture.h"
#include "prog.h"
#include "utils.h"

#define PROG_NAME    "wayshot"
#define PROG_VERSION "v0.1.0"

#define DEFAULT_DIR       "Pictures/Screenshots"
#define LAST_REGION_FNAME "wayshot-last-region"

int main(int argc, char *argv[]) {
    int   result = EXIT_SUCCESS;
    char *cmd    = NULL;
    // variables that will be freed after defer must be initialized before any call to return_defer
    char *alt_dir               = NULL;
    char *last_region_file_path = NULL;

    static Config config;
    config.prog_name            = PROG_NAME;
    config.prog_version         = PROG_VERSION;
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
