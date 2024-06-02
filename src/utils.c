#include "utils.h"
#include "memplus.h"
#include "prog.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const char *compositor_name[] = {
    [COMP_NONE]     = "Not supported",
    [COMP_HYPRLAND] = "Hyprland",
};

static const char *mode_name[] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
    [MODE_CUSTOM]        = "Custom",
    [MODE_TEST]          = "Test",
};

static const char *imgtype_name[] = {
    [IMGTYPE_PNG]  = "png",
    [IMGTYPE_PPM]  = "ppm",
    [IMGTYPE_JPEG] = "jpeg",
};

static const char *savemode_name[] = {
    [SAVEMODE_NONE]                      = "None",
    [SAVEMODE_DISK]                      = "Disk",
    [SAVEMODE_CLIPBOARD]                 = "Clipboard",
    [SAVEMODE_DISK | SAVEMODE_CLIPBOARD] = "Disk & Clipboard",
};

char *malloc_strf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    assert(size >= 0 && "failed to count string size");
    va_end(args);

    char *ptr = malloc(size + 1);
    assert(ptr != NULL);

    va_start(args, fmt);
    int result_size = vsnprintf(ptr, size + 1, fmt, args);
    assert(result_size == size);
    va_end(args);

    return ptr;
}

bool command_found(const char *command) {
    char *cmd = malloc_strf("command -v '%s' >/dev/null 2>&1", command);
    assert(cmd != NULL);
    int ret = system(cmd);
    free(cmd);
    return !ret;
}

const char *compositor2str(Compositor compositor) {
    return compositor_name[compositor];
}

const char *imgtype2str(Imgtype imgtype) {
    return imgtype_name[imgtype];
}

const char *get_fname(Config *config) {
    if (config->output_path != NULL) return config->output_path;

    time_t     now_time = time(NULL);
    struct tm *now      = localtime(&now_time);

    char *ext = NULL;
    switch (config->imgtype) {
        case IMGTYPE_PNG : {
            ext = "png";
        } break;
        case IMGTYPE_JPEG : {
            ext = "jpg";
        } break;
        case IMGTYPE_PPM : {
            ext = "ppm";
        } break;
    }
    if (ext == NULL) assert(0 && "unreachable");

    return mp_string_newf(&config->alloc,
                          "%s/Screenshot_%04d%02d%02d_%02d%02d%02d.%s",
                          config->screenshot_dir,
                          now->tm_year + 1900,
                          now->tm_mon + 1,
                          now->tm_mday,
                          now->tm_hour,
                          now->tm_min,
                          now->tm_sec,
                          ext)
        .cstr;
}

const char *mode2str(Mode mode) {
    return mode_name[mode];
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
            eprintf("run_cmd: Failed to create pipes: %s\n", strerror(errno));
            return_defer(-1);
        }
        read_pipe  = pipe_fd[0];
        write_pipe = pipe_fd[1];
    }

    pid_t pid = fork();
    if (pid == -1) {
        eprintf("run_cmd: Failed to fork child process: %s\n", strerror(errno));
        return_defer(-1);
    } else if (pid == 0) {
        if (buf != NULL) {
            close(read_pipe);
            dup2(write_pipe, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
            close(write_pipe);
        } else {
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
        }
        close(dev_null);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        eprintf("run_cmd: Failed not run `%s`: %s\n", cmd, strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            eprintf("run_cmd: `%s` could not terminate: %s\n", cmd, strerror(errno));
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

const char *savemode2str(SaveMode save_mode) {
    return savemode_name[save_mode];
}

Compositor str2compositor(const char *str) {
    for (size_t i = 0; i < COMP_COUNT; ++i) {
        if (strcmp(str, compositor_name[i]) == 0) return i;
    }
    return COMP_NONE;
}

bool verify_geometry(const char *geometry) {
    char *cmd = malloc_strf("grim -t jpeg -q 0 -g '%s' - >/dev/null", geometry);
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Invalid region format `%s`\n", geometry);
        free(cmd);
        return false;
    }
    free(cmd);
    return true;
}
