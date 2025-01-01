#include "hyprland.h"

const char *hyprland_active_monitor =
    "hyprctl monitors -j | jq -r '.[] | select(.focused) | .name'";

const char *hyprland_active_window =
    "hyprctl activewindow -j | jq -r '\"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'";

const char *hyprland_windows =
    // lists windows in Hyprland
    "hyprctl clients -j"
    // filters window outside of the current workspace
    " | jq -r --argjson workspaces \"$(hyprctl monitors -j | jq -r 'map(.activeWorkspace.id)')\""
    "   'map(select([.workspace.id] | inside($workspaces)))'"
    // get the position and size of each window and turn it into format slurp can read
    " | jq -r '.[] | \"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'"
    // and those informations will be used by slurp to automatically snap selection of
    // region to the windows
    ;
