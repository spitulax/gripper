#include "utils.h"
#include "prog.h"
#include <errno.h>
#include <fcntl.h>
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

const char *mode_name[MODE_COUNT] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
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

bool command_found(const char *command) {
    char *cmd = alloc_strf("which %s >/dev/null 2>&1", command);
    int   ret = system(cmd);
    free(cmd);
    return !ret;
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

bool grim(Config *config, const char *region) {
    bool  result = true;
    char *cmd    = NULL;
    char *fname  = get_fname(config->screenshot_dir);
    if (fname == NULL) return_defer(false);

    if (region == NULL) {
        cmd = alloc_strf("grim -c - > %s", fname);
    } else {
        cmd = alloc_strf("grim -g \"%s\" - > %s", region, fname);
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
    free(cmd);
    free(fname);
    return result;
}

const char *mode2str(Mode mode) {
    return mode_name[mode];
}

bool notify(Mode mode, const char *fname) {
    if (!command_found("notify-send")) return false;

    // TODO: maybe print the region?
    bool  result = true;
    char *cmd    = alloc_strf(
        "notify-send -a Wayshot 'Screenshot taken (%s)' 'Saved to %s'", mode2str(mode), fname);

    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Could not send notification\n");
        return_defer(false);
    }

defer:
    free(cmd);
    return result;
}

ssize_t run_cmd(const char *cmd, char *buf, size_t nbytes) {
    ssize_t result = -1;

    int     pipe_fd[2];
    int     read_pipe  = -1;
    int     write_pipe = -1;
    int     dev_null   = open("/dev/null", O_WRONLY | O_CREAT, 0666);
    ssize_t bytes_read = -1;

    if (buf != NULL) {
        if (pipe(pipe_fd) == -1) {
            eprintf("Could not create pipes: %s\n", strerror(errno));
            return_defer(-1);
        }
        read_pipe  = pipe_fd[0];
        write_pipe = pipe_fd[1];
    }

    pid_t pid = fork();
    if (pid == -1) {
        eprintf("Could not fork child process: %s\n", strerror(errno));
        return_defer(-1);
    } else if (pid == 0) {
        if (buf != NULL) {
            close(read_pipe);
            // NOTE: both stdout and stderr are redirected to the same pipe
            dup2(write_pipe, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
            close(write_pipe);
        } else {
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
        }
        close(dev_null);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        eprintf("Could not run `%s`: %s\n", cmd, strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            eprintf("`%s` could not terminate: %s\n", cmd, strerror(errno));
            return_defer(-1);
        }

        // NOTE: we don't give an f about what's in stderr
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) return_defer(-1);

        if (buf != NULL) {
            bytes_read      = read(read_pipe, buf, nbytes - 1);
            buf[bytes_read] = '\0';
            return_defer(bytes_read);
        } else {
            return_defer(0);
        }
    }

defer:
    if (read_pipe != -1 || write_pipe != -1) {
        close(read_pipe);
        close(write_pipe);
    };
    if (dev_null != -1) close(dev_null);
    return result;
}

Compositor str2compositor(const char *str) {
    for (size_t i = 0; i < COMP_COUNT; ++i) {
        if (strcmp(str, compositor_name[i]) == 0) return i;
    }
    return COMP_NONE;
}
