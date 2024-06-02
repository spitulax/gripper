#include "sway.h"
#include "memplus.h"

mp_String sway_get_active_monitor(mp_Allocator *alloc) {
    return mp_string_new(alloc,
                         "swaymsg -t get_outputs | jq -r '.[] | select(.focused)' | jq -r '.name'");
}

mp_String sway_get_active_window(mp_Allocator *alloc) {
    return mp_string_new(alloc,
                         "swaymsg -t get_tree"
                         " | jq -r 'recurse(.nodes[]?, .floating_nodes[]?) | select(.focused)'"
                         " | jq -r '.rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'");
}

mp_String sway_get_windows(mp_Allocator *alloc) {
    return mp_string_new(
        alloc,
        "swaymsg -t get_tree"
        " | jq -r '.. | select(.pid? and .visible?) | .rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'");
}
