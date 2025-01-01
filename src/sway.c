#include "sway.h"

const char *sway_active_monitor =
    "swaymsg -t get_outputs | jq -r '.[] | select(.focused)' | jq -r '.name'";

const char *sway_active_window =
    "swaymsg -t get_tree"
    " | jq -r 'recurse(.nodes[]?, .floating_nodes[]?) | select(.focused)'"
    " | jq -r '.rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'";

const char *sway_windows =
    "swaymsg -t get_tree"
    " | jq -r '.. | select(.pid? and .visible?) | .rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'";
