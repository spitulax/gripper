#include "utils.h"
#include "compositors.h"
#include "memplus.h"
#include "prog.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const char *compositor_name[COMP_COUNT] = {
    [COMP_NONE]     = "Not supported",
    [COMP_HYPRLAND] = "Hyprland",
    [COMP_SWAY]     = "sway",
};

static const char *mode_name[] = {
    [MODE_FULL]          = "Full",
    [MODE_REGION]        = "Region",
    [MODE_LAST_REGION]   = "Last Region",
    [MODE_ACTIVE_WINDOW] = "Active Window",
    [MODE_CUSTOM]        = "Custom",
    [MODE_TEST]          = "Test",
};

static const char *imgtype_name[IMGTYPE_COUNT] = {
    [IMGTYPE_NONE] = NULL,      //
    [IMGTYPE_PNG]  = "png",     //
    [IMGTYPE_PPM]  = "ppm",     //
    [IMGTYPE_JPEG] = "jpeg",    //
    [IMGTYPE_JPG]  = "jpg",     //
};

static const char *savemode_name[] = {
    [SAVEMODE_NONE]                      = "None",
    [SAVEMODE_DISK]                      = "Disk",
    [SAVEMODE_CLIPBOARD]                 = "Clipboard",
    [SAVEMODE_DISK | SAVEMODE_CLIPBOARD] = "Disk & Clipboard",
};

bool command_found(const char *command) {
    char *cmd = alloc_strf("command -v '%s' >/dev/null 2>&1", command).cstr;
    assert(cmd != NULL);
    int ret = system(cmd);
    return !ret;
}

const char *compositor2str(Compositor compositor) {
    assert(compositor != COMP_COUNT);
    return compositor_name[compositor];
}

const char *imgtype2str(Imgtype imgtype) {
    // `Imgtype` is different from `Compositor` that it shouldn't be unknown
    assert(!(imgtype == IMGTYPE_COUNT || imgtype == IMGTYPE_NONE));
    return imgtype_name[imgtype];
}

void imgtype_remap(Config *config) {
    // grim only accepts `jpeg`
    if (config->imgtype == IMGTYPE_JPG) config->imgtype = IMGTYPE_JPEG;
}

void set_output_path(Config *config) {
    if (config->output_path != NULL) return;

    time_t     now_time = time(NULL);
    struct tm *now      = localtime(&now_time);

    const char *ext = imgtype2str(config->imgtype);

    config->output_path = mp_string_newf(g_alloc,
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

    int pipe_fd[2];
    int read_pipe  = -1;
    int write_pipe = -1;
    int dev_null   = open("/dev/null", O_WRONLY | O_CREAT, 0666);
    assert(dev_null != -1);
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
    for (uint32_t i = 0; i < COMP_COUNT; ++i) {
        if (strcmp(str, compositor_name[i]) == 0) return i;
    }
    return COMP_NONE;
}

Imgtype str2imgtype(const char *str) {
    for (uint32_t i = 1; i < IMGTYPE_COUNT; ++i) {
        if (strcmp(str, imgtype_name[i]) == 0) return i;
    }
    return IMGTYPE_NONE;
}

bool verify_geometry(const char *geometry) {
    char *cmd = alloc_strf("grim -t jpeg -q 0 -g '%s' - >/dev/null", geometry).cstr;
    if (run_cmd(cmd, NULL, 0) == -1) {
        eprintf("Invalid region format `%s`\n", geometry);
        return false;
    }
    return true;
}

bool make_dir(const char *path) {
    struct stat s;
    if (stat(path, &s) != 0) {
        // FIXME: use `mkdir(2)`
        char *cmd = mp_string_newf(g_alloc, "mkdir -p '%s'", path).cstr;
        int   ret = system(cmd);
        if (ret == 0) {
            printf("Created %s\n", path);
        } else {
            eprintf("Failed to create directory %s\n", path);
            return false;
        }
    } else if (!S_ISDIR(s.st_mode)) {
        eprintf("%s already exists but it is not a directory\n", path);
        return false;
    }
    return true;
}

bool set_current_output_name(Config *config) {
    char *output_name = mp_allocator_alloc(g_alloc, DEFAULT_OUTPUT_SIZE);
    assert(g_config->compositor != COMP_COUNT);
    const char *cmd = comp_active_monitor_cmds[g_config->compositor];

    if (cmd != NULL) {
        ssize_t bytes = run_cmd(cmd, output_name, DEFAULT_OUTPUT_SIZE);
        if (bytes == -1 || output_name == NULL) {
            eprintf("Failed to get information about current monitor\n");
            return false;
        }
        output_name[bytes - 1] = '\0';    // trim the final newline
        config->output_name    = output_name;
    } else {
        config->output_name = NULL;
    }

    return true;
}

const char *file_ext(const char *path) {
    const char *dot_ptr = strrchr(path, '.');
    if (dot_ptr == NULL) return "";
    return dot_ptr + 1;
}

void print_valid_imgtypes(void) {
    for (uint32_t i = 1; i < IMGTYPE_COUNT; ++i) {
        if (i > 1) printf(", ");
        printf("%s", imgtype_name[i]);
    }
    printf("\n");
}
