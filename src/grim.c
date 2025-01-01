#include "grim.h"
#include "memplus.h"
#include "prog.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

bool notify(const Config *config, const char *fname) {
    if (!command_found("notify-send") || config->save_mode == SAVEMODE_NONE) return false;
    const char *name = NULL;
    if (config->save_mode == SAVEMODE_DISK) {
        name = fname;
    } else if (config->save_mode == SAVEMODE_CLIPBOARD) {
        name = "clipboard";
    } else if (config->save_mode == (SAVEMODE_DISK | SAVEMODE_CLIPBOARD)) {
        name = alloc_strf("%s and clipboard", fname).cstr;
    }

    // TODO: maybe print the region?
    // TODO: add actions to the notification that maybe brings an option to view/edit the image
    char *cmd = alloc_strf("notify-send -t 3000 -a Gripper 'Screenshot taken (%s)' 'Saved to %s'",
                           mode2str(config->mode),
                           name)
                    .cstr;

    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Failed to send notification\n");
        return false;
    }

    return true;
}

bool grim(const Config *config, const char *region) {
    char       *cmd   = NULL;
    const char *fname = NULL;
    if (config->save_mode & SAVEMODE_DISK) {
        fname = get_fname(config);
        assert(fname != NULL);
    }

    mp_String options =
        mp_string_newf(g_alloc, "-t %s", config->scale, imgtype2str(config->imgtype));
    // NOTE: too many allocs, but it may be just fine
    if (config->scale != 1.0) options = alloc_strf("%s -s %f", options.cstr, config->scale);
    if (config->jpeg_quality != DEFAULT_JPEG_QUALITY || config->png_level != DEFAULT_PNG_LEVEL) {
        switch (config->imgtype) {
            case IMGTYPE_PNG : {
                options = alloc_strf("%s -l %d", options.cstr, config->png_level);
            } break;
            case IMGTYPE_JPEG : {
                options = alloc_strf("%s -q %d", options.cstr, config->jpeg_quality);
            } break;
            case IMGTYPE_PPM : break;
        }
    }
    if (config->cursor) options = alloc_strf("%s -c", options.cstr);
    if (config->output_name != NULL && region == NULL)
        options = alloc_strf("%s -o %s", options.cstr, config->output_name);
    if (region != NULL) options = alloc_strf("%s -g \"%s\"", options.cstr, region);

    if (config->save_mode & SAVEMODE_DISK) {
        cmd = alloc_strf("grim %s - > %s", options.cstr, fname).cstr;
    } else if (config->save_mode == SAVEMODE_CLIPBOARD) {
        cmd = alloc_strf("grim %s - | wl-copy", options.cstr).cstr;
    } else if (config->save_mode == SAVEMODE_NONE) {
        cmd = alloc_strf("grim %s - >/dev/null", options.cstr).cstr;
    }

#ifdef DEBUG
    if (config->verbose) printf("$ %s\n", cmd);
#endif
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Failed to run grim\n");
        return false;
    }

    notify(config, fname);

    if (config->save_mode == (SAVEMODE_DISK | SAVEMODE_CLIPBOARD)) {
        cmd = alloc_strf("wl-copy < %s", fname).cstr;
#ifdef DEBUG
        if (config->verbose) printf("$ %s\n", cmd);
#endif
        if (run_cmd(cmd, NULL, 0) == -1) {
            eprintf("Failed to save image to %s\n", config->screenshot_dir);
            return false;
        }
    }

    return true;
}
