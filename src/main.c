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

#define DEFAULT_DIR       "Pictures/Screenshots"
#define LAST_REGION_FNAME "gripper-last-region"

mp_Allocator       *g_alloc;
const Config       *g_config;
static mp_Allocator alloc;
static Config       config;

int main(int argc, char *argv[]) {
    mp_Arena arena  = mp_arena_new();
    int      result = EXIT_SUCCESS;
    // variables that will be freed after defer must be initialized before any call to return_defer
    char *alt_dir               = NULL;
    char *last_region_file_path = NULL;

    config              = (Config){ 0 };
    config.prog_name    = PROG_NAME;
    config.prog_version = PROG_VERSION;
    g_config            = &config;

    alloc   = mp_arena_new_allocator(&arena);
    g_alloc = &alloc;

    config_init(&config);
    int parse_result = parse_args(argc, argv, &config);
    switch (parse_result) {
        case 2 : break;
        case 1 : return_defer(EXIT_SUCCESS);
        case 0 : {
            usage();
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
    if (!make_dir(config.screenshot_dir)) return_defer(EXIT_FAILURE);

    // Assign config.last_region_file
    config.cache_dir = getenv("XDG_CACHE_HOME");
    if (config.cache_dir == NULL) {
        config.cache_dir = alloc_strf("%s/.cache", home_dir).cstr;
    }
    last_region_file_path   = alloc_strf("%s/" LAST_REGION_FNAME, config.cache_dir).cstr;
    config.last_region_file = last_region_file_path;

    // Set the output (monitor) name
    if (config.output_name != NULL) {
        // Verify if output exists
        mp_String cmd =
            mp_string_newf(g_alloc, "grim -t jpeg -q 0 -o %s - >/dev/null", config.output_name);
        if (run_cmd(cmd.cstr, NULL, 0) == -1) {
            eprintf("Unknown output `%s`\n", config.output_name);
            return false;
        }
    } else if (config.mode == MODE_FULL && !config.all_outputs) {
        if (!set_current_output_name(&config)) return false;
    }

    if (config.save_mode & SAVEMODE_DISK) {
        set_output_path(&config);
        assert(config.output_path != NULL);
    }

    if (!capture()) return_defer(EXIT_FAILURE);

defer:
    mp_arena_free(&arena);
    return result;
}
