#ifndef SWAY_H
#define SWAY_H

#include "memplus.h"

mp_String sway_get_active_monitor(mp_Allocator *alloc);
mp_String sway_get_active_window(mp_Allocator *alloc);
mp_String sway_get_windows(mp_Allocator *alloc);

#endif /* ifndef SWAY_H */
