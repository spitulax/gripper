#ifndef UTILS_H
#define UTILS_H

#include "prog.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#define return_defer(v)                                                                            \
    do {                                                                                           \
        result = (v);                                                                              \
        goto defer;                                                                                \
    } while (0)

#define eprintf(...)                                                                               \
    do {                                                                                           \
        fprintf(stderr, __VA_ARGS__);                                                              \
    } while (0)

#define array_len(array) (sizeof(array) / sizeof(array[0]))

#define alloc_strf(fmt, ...) mp_string_newf(g_alloc, fmt, __VA_ARGS__)

#define DEFAULT_OUTPUT_SIZE 1024

bool command_found(const char *command);

const char *compositor2str(Compositor compositor);

const char *imgtype2str(Imgtype imgtype);

void imgtype_remap(Config *config);

void set_output_path(Config *config);

bool set_current_output_name(Config *config);

const char *mode2str(Mode mode);

// Returns the amounts of bytes written to `buf`
// If failed returns -1 and set `buf` to NULL
// Set `buf` to NULL to discard the output altogether
//    In this case it returns 0 on success, returns -1 on failure
ssize_t run_cmd(const char *cmd, char *buf, size_t nbytes);

const char *savemode2str(SaveMode save_mode);

Compositor str2compositor(const char *str);

Imgtype str2imgtype(const char *str);

void usage(void);

bool verify_geometry(const char *geometry);

bool make_dir(const char *path);

const char *file_ext(const char *path);

void print_valid_imgtypes(void);

#endif /* ifndef UTILS_H */
