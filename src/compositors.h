#ifndef COMPOSITORS_H
#define COMPOSITORS_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    COMP_NONE,
    COMP_HYPRLAND,
    COMP_SWAY,
    COMP_COUNT,
} Compositor;

extern const char *comp_active_monitor_cmds[COMP_COUNT];
extern const char *comp_active_window_cmds[COMP_COUNT];
extern const char *comp_windows_cmds[COMP_COUNT];

// TODO: check if compositor supports needed wayland protocols
#define comp_supported(comp)     (comp != COMP_NONE)
#define comp_print_support(comp) _comp_print_support(comp, stdout)
#define comp_check_support(comp, ret)                                                              \
    do {                                                                                           \
        if (!comp_supported(comp)) {                                                               \
            _comp_print_support(comp, stderr);                                                     \
            return (ret);                                                                          \
        }                                                                                          \
    } while (0)
void _comp_print_support(Compositor compositor, FILE *stream);

#endif /* ifndef COMPOSITORS_H */
