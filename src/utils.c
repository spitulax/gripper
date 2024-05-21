#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const char *compositor_name[COMP_COUNT] = {
    [COMP_NONE]     = "Not supported",
    [COMP_HYPRLAND] = "Hyprland",
};

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

const char *compositor2str(Compositor compositor) {
    return compositor_name[compositor];
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

ssize_t run_cmd(char *buf, size_t nbytes, const char *cmd) {
    int     pipe_fd[2];
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

Compositor str2compositor(const char *str) {
    if (strcmp(str, "Hyprland") == 0) {
        return COMP_HYPRLAND;
    } else {
        return COMP_NONE;
    }
}
