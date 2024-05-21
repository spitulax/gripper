#ifndef UTILS_H
#define UTILS_H

#include "prog.h"
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

char *alloc_strf(const char *fmt, ...);

const char *compositor2str(Compositor compositor);

char *get_fname(const char *dir);

bool grim(Config *config, Mode mode, const char *region);

// Returns the amounts of bytes written to buf
ssize_t run_cmd(char *buf, size_t nbytes, const char *cmd);

Compositor str2compositor(const char *str);

void usage(Config *config);

#endif /* ifndef UTILS_H */
