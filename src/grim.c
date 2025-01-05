#include "grim.h"
#include "memplus.h"
#include "prog.h"
#include "sys/stat.h"
#include "unistd.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

bool notify(void) {
    if (!command_found("notify-send") || g_config->save_mode == SAVEMODE_NONE) return false;
    const char *name = NULL;
    if (g_config->save_mode == SAVEMODE_DISK) {
        name = g_config->output_path;
    } else if (g_config->save_mode == SAVEMODE_CLIPBOARD) {
        name = "clipboard";
    } else if (g_config->save_mode == (SAVEMODE_DISK | SAVEMODE_CLIPBOARD)) {
        name = alloc_strf("%s and clipboard", g_config->output_path).cstr;
    }

    // TODO: maybe print the region?
    // TODO: add actions to the notification that maybe brings an option to view/edit the image
    char *cmd = alloc_strf("notify-send -t 3000 -a Gripper 'Screenshot taken (%s)' 'Saved to %s'",
                           mode2str(g_config->mode),
                           name)
                    .cstr;

    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Failed to send notification\n");
        return false;
    }

    return true;
}

bool grim(const char *region) {
    char *cmd = NULL;

    mp_String options =
        mp_string_newf(g_alloc, "-t %s", g_config->scale, imgtype2str(g_config->imgtype));
    // NOTE: too many allocs, but it may be just fine
    if (g_config->scale != 1.0) options = alloc_strf("%s -s %f", options.cstr, g_config->scale);
    if (g_config->jpeg_quality != DEFAULT_JPEG_QUALITY ||
        g_config->png_level != DEFAULT_PNG_LEVEL) {
        switch (g_config->imgtype) {
            case IMGTYPE_PNG : {
                options = alloc_strf("%s -l %d", options.cstr, g_config->png_level);
            } break;
            case IMGTYPE_JPEG : {
                options = alloc_strf("%s -q %d", options.cstr, g_config->jpeg_quality);
            } break;
            case IMGTYPE_PPM : break;
        }
    }
    if (g_config->cursor) options = alloc_strf("%s -c", options.cstr);
    if (g_config->output_name != NULL && region == NULL)
        options = alloc_strf("%s -o %s", options.cstr, g_config->output_name);
    if (region != NULL) options = alloc_strf("%s -g \"%s\"", options.cstr, region);

    if (g_config->save_mode & SAVEMODE_DISK) {
        if (access(g_config->output_path, F_OK) == 0) {
            struct stat s;
            if (stat(g_config->output_path, &s) != 0) {
                eprintf("Failed to stat %s\n", g_config->output_path);
                return false;
            }
            if (!S_ISREG(s.st_mode)) {
                eprintf("%s already exists and it is not a regular file\n", g_config->output_path);
                return false;
            }
            printf("Overriding %s, are you sure? [y/N] ", g_config->output_path);
#define BUFLEN 3    // enough for one character, a newline, and a '\0'
            char buf[BUFLEN];
            if (fgets(buf, BUFLEN, stdin) == NULL) {
                eprintf("Failed to read input\n");
                return false;
            }
            if (tolower(buf[0]) != 'y') return false;
#undef BUFLEN
        }
        cmd = alloc_strf("grim %s - > %s", options.cstr, g_config->output_path).cstr;
    } else if (g_config->save_mode == SAVEMODE_CLIPBOARD) {
        cmd = alloc_strf("grim %s - | wl-copy", options.cstr).cstr;
    } else if (g_config->save_mode == SAVEMODE_NONE) {
        cmd = alloc_strf("grim %s - >/dev/null", options.cstr).cstr;
    }

    if (g_config->wait_time > 0) {
        if (g_config->verbose) printf("*Waiting for %d seconds...*\n", g_config->wait_time);
        sleep(g_config->wait_time);
    }

#ifdef DEBUG
    if (g_config->verbose) printf("$ %s\n", cmd);
#endif
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Failed to run grim\n");
        return false;
    }

    notify();

    if (g_config->save_mode == (SAVEMODE_DISK | SAVEMODE_CLIPBOARD)) {
        cmd = alloc_strf("wl-copy < %s", g_config->output_path).cstr;
#ifdef DEBUG
        if (g_config->verbose) printf("$ %s\n", cmd);
#endif
        if (run_cmd(cmd, NULL, 0) == -1) {
            eprintf("Failed to save image to %s\n", g_config->screenshot_dir);
            return false;
        }
    }

    return true;
}
