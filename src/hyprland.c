#include "hyprland.h"

mp_String hyprland_get_active_monitor(mp_Allocator *alloc) {
    return mp_string_new(alloc, "hyprctl monitors -j | jq -r '.[] | select(.focused) | .name'");
}

mp_String hyprland_get_active_window(mp_Allocator *alloc) {
    return mp_string_new(
        alloc,
        "hyprctl activewindow -j | jq -r '\"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'");
}

mp_String hyprland_get_windows(mp_Allocator *alloc) {
    return mp_string_new(
        alloc,
        // lists windows in Hyprland
        "hyprctl clients -j | "
        // filters window outside of the current workspace
        "jq -r --argjson workspaces \"$(hyprctl monitors -j | jq -r 'map(.activeWorkspace.id)')\" "
        "'map(select([.workspace.id] | inside($workspaces)))' | "
        // get the position and size of each window and turn it into format slurp can read
        "jq -r '.[] | \"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'"
        // and those informations will be used by slurp to automatically snap selection of
        // region to the windows
    );
}
