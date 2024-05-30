#ifndef UTILS_H
#define UTILS_H

#include "../prog.h"
#include <stdbool.h>
#include <stdio.h>

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

char *malloc_strf(const char *fmt, ...);

bool command_found(const char *command);

const char *compositor2str(Compositor compositor);

char *get_fname(Config *config);

const char *mode2str(Mode mode);

// Returns the amounts of bytes written to `buf`
// If failed returns -1 and set `buf` to NULL
// Set `buf` to NULL to discard the output altogether
//    In this case it returns 0 on success, returns -1 on failure
ssize_t run_cmd(const char *cmd, char *buf, size_t nbytes);

Compositor str2compositor(const char *str);

void usage(Config *config);

#endif /* ifndef UTILS_H */
