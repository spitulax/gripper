#include "compositors.h"
#include <stdio.h>

// Commands to get the name of the active monitor
const char *comp_active_monitor_cmds[COMP_COUNT] = {
    [COMP_NONE]     = NULL,
    [COMP_HYPRLAND] = "hyprctl monitors -j | jq -r '.[] | select(.focused) | .name'",
    [COMP_SWAY]     = "swaymsg -t get_outputs | jq -r '.[] | select(.focused)' | jq -r '.name'",
};

// Commands to get the region of the active window
const char *comp_active_window_cmds[COMP_COUNT] = {
    [COMP_NONE] = NULL,
    [COMP_HYPRLAND] =
        "hyprctl activewindow -j | jq -r '\"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'",
    [COMP_SWAY] =
        "swaymsg -t get_tree"
        " | jq -r 'recurse(.nodes[]?, .floating_nodes[]?) | select(.focused)'"
        " | jq -r '.rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'",
};

// Commands to get the region of all visible windows
const char *comp_windows_cmds[COMP_COUNT] = {
    [COMP_NONE] = NULL,
    // FIXME: this includes windows hidden by a fullscreen window
    [COMP_HYPRLAND] =
        /**/
    // lists windows in Hyprland
    "hyprctl clients -j"
    // filters window outside of the current workspace
    " | jq -r --argjson workspaces \"$(hyprctl monitors -j | jq -r 'map(.activeWorkspace.id)')\""
    "   'map(select([.workspace.id] | inside($workspaces)))'"
    // gets the position and size of each window and turn it into format slurp can read
    " | jq -r '.[] | \"\\(.at[0]),\\(.at[1]) \\(.size[0])x\\(.size[1])\"'"
    // and those informations will be used by slurp to automatically snap selection of
    // region to the windows
    ,
    [COMP_SWAY] =
        "swaymsg -t get_tree"
        " | jq -r '.. | select(.pid? and .visible?) | .rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'",
};

void _comp_print_support(Compositor compositor, FILE *stream) {
    fprintf(stream, "Your compositor is ");
    fprintf(stream, comp_supported(compositor) ? "supported.\n" : "not supported.\n");
    fprintf(stream, "    Mode `active-window` is unavailable for unsupported compositor and\n");
    fprintf(stream, "    mode `region` does not have snap to window.\n");
    fprintf(stream, "    Mode `full` is also unable to pick current output automatically.\n");
    fprintf(stream, "    You must specify it yourself with `-o` or it will capture all outputs\n");
    fprintf(stream, "    which is the behaviour of `--all`.\n");
}
