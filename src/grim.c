#include "grim.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

bool notify(Mode mode, const char *fname) {
    if (!command_found("notify-send")) return false;

    // TODO: maybe print the region?
    // TODO: add actions to the notification that maybe brings an option to view the image
    bool  result = true;
    char *cmd    = alloc_strf(
        "notify-send -a Waysnip 'Screenshot taken (%s)' 'Saved to %s'", mode2str(mode), fname);

    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Could not send notification\n");
        return_defer(false);
    }

defer:
    free(cmd);
    return result;
}

bool grim(Config *config, const char *region) {
    bool  result  = true;
    char *cmd     = NULL;
    char *options = NULL;
    char *fname   = get_fname(config->screenshot_dir);
    if (fname == NULL) return_defer(false);

    options = alloc_strf("%s", config->cursor ? "-c" : "");

    if (region == NULL) {
        cmd = alloc_strf("grim %s - > %s", options, fname);
    } else {
        cmd = alloc_strf("grim %s -g \"%s\" - > %s", options, region, fname);
    }

    if (config->verbose) printf("$ %s\n", cmd);
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("grim exited\n");
        return_defer(false);
    }

    free(cmd);
    cmd = alloc_strf("wl-copy < %s", fname);
    if (config->verbose) printf("$ %s\n", cmd);
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Could save image to %s\n", config->screenshot_dir);
        return_defer(false);
    }

    if (!notify(config->mode, fname) && config->verbose) printf("Could not send notification\n");

defer:
    free(options);
    free(cmd);
    free(fname);
    return result;
}
