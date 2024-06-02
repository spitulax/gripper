#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

#include <assert.h>
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

#define PROG_NAME    "gripper"
#define PROG_VERSION "v1.0.0"

#define DEFAULT_DIR       "Pictures/Screenshots"
#define LAST_REGION_FNAME "gripper-last-region"

#define alloc_strf(fmt, ...) mp_string_newf(&config.alloc, fmt, __VA_ARGS__)

int main(int argc, char *argv[]) {
    mp_Arena arena  = mp_arena_new();
    int      result = EXIT_SUCCESS;
    char    *cmd    = NULL;
    // variables that will be freed after defer must be initialized before any call to return_defer
    char *alt_dir               = NULL;
    char *last_region_file_path = NULL;
    char *cache_dir             = NULL;

    static Config config = { 0 };
    config.prog_name     = PROG_NAME;
    config.prog_version  = PROG_VERSION;

    mp_Allocator alloc = mp_arena_new_allocator(&arena);
    config.alloc       = alloc;

    prepare_options(&config);
    int parse_result = parse_args(argc, argv, &config);
    switch (parse_result) {
        case 2 : break;
        case 1 : return_defer(EXIT_SUCCESS);
        case 0 : {
            usage(&config);
            return_defer(EXIT_FAILURE);
        }
        default : assert(0 && "unreachable");
    }

    const char *home_dir = getenv("HOME");
    assert(home_dir != NULL && "HOME dir is not set");

    // Asssign config.screenshot_dir
    const char *screenshot_dir = getenv("SCREENSHOT_DIR");
    if (config.screenshot_dir == NULL) {
        if (screenshot_dir == NULL) {
            alt_dir               = alloc_strf("%s/" DEFAULT_DIR, home_dir).cstr;
            config.screenshot_dir = alt_dir;
        } else {
            config.screenshot_dir = screenshot_dir;
        }
    }

    // Create the directory if it didn't exist
    {
        struct stat dir_stat;
        if (stat(config.screenshot_dir, &dir_stat)) {
            cmd     = alloc_strf("mkdir -p %s", config.screenshot_dir).cstr;
            int ret = system(cmd);
            if (ret == 0) {
                printf("Created %s\n", config.screenshot_dir);
            } else {
                eprintf("Failed to create directory %s\n", config.screenshot_dir);
                return_defer(EXIT_FAILURE);
            }
        }
    }

    // Assign config.last_region_file
    cache_dir = getenv("XDG_CACHE_HOME");
    if (cache_dir == NULL) {
        cache_dir = alloc_strf("%s/.cache", home_dir).cstr;
    }
    last_region_file_path   = alloc_strf("%s/" LAST_REGION_FNAME, cache_dir).cstr;
    config.last_region_file = last_region_file_path;

    if (!capture(&config)) return_defer(EXIT_FAILURE);

defer:
    mp_arena_free(&arena);
    return result;
}
