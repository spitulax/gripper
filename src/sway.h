#ifndef SWAY_H
#define SWAY_H

static const char *sway_active_monitor =
    "swaymsg -t get_outputs | jq -r '.[] | select(.focused)' | jq -r '.name'";

static const char *sway_active_window =
    "swaymsg -t get_tree"
    " | jq -r 'recurse(.nodes[]?, .floating_nodes[]?) | select(.focused)'"
    " | jq -r '.rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'";

static const char *sway_windows =
    "swaymsg -t get_tree"
    " | jq -r '.. | select(.pid? and .visible?) | .rect | \"\\(.x),\\(.y) \\(.width)x\\(.height)\"'";

#endif /* ifndef SWAY_H */
