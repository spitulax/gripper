#include "grim.h"
#include "memplus.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define alloc_strf(fmt, ...) mp_string_newf(&config->alloc, fmt, __VA_ARGS__)

bool notify(Config *config, const char *fname) {
    if (!command_found("notify-send")) return false;

    // TODO: maybe print the region?
    // TODO: add actions to the notification that maybe brings an option to view the image
    char *cmd = alloc_strf("notify-send -a Waysnip 'Screenshot taken (%s)' 'Saved to %s'",
                           mode2str(config->mode),
                           fname)
                    .cstr;

    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Failed to send notification\n");
        return false;
    }

    return true;
}

bool grim(Config *config, const char *region) {
    char *cmd   = NULL;
    char *fname = get_fname(config);
    assert(fname != NULL);

    const char *options = config->cursor ? "-c" : "";

    if (region == NULL) {
        cmd = alloc_strf("grim %s - > %s", options, fname).cstr;
    } else {
        cmd = alloc_strf("grim %s -g \"%s\" - > %s", options, region, fname).cstr;
    }

    if (config->verbose) printf("$ %s\n", cmd);
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("grim exited\n");
        return false;
    }

    if (!notify(config, fname) && config->verbose) printf("Notification was not sent\n");

    if (!config->no_clipboard) {
        cmd = alloc_strf("wl-copy < %s", fname).cstr;
        if (config->verbose) printf("$ %s\n", cmd);
        if (run_cmd(cmd, NULL, 0) == -1) {
            eprintf("Failed to save image to %s\n", config->screenshot_dir);
            return false;
        }
    }

    return true;
}
