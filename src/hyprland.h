#ifndef HYPRLAND_H
#define HYPRLAND_H

#include "memplus.h"

mp_String hyprland_get_active_monitor(mp_Allocator *alloc);
mp_String hyprland_get_active_window(mp_Allocator *alloc);
mp_String hyprland_get_windows(mp_Allocator *alloc);

#endif /* ifndef HYPRLAND_H */
