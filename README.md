<h1 align="center">Gripper</h1>

<h4 align="center">Simple and easy to use screenshot utility for Wayland</h4>

Gripper is an alternative to [grimblast](https://github.com/hyprwm/contrib/blob/main/grimblast) and
[grimshot](https://github.com/OctopusET/sway-contrib/blob/master/grimshot) that works for both
Hyprland and Sway.

Gripper runs [grim](https://sr.ht/~emersion/grim/) under the hood, but makes it easier to use by
providing "alias" of common operations that are often compositor-specific. Also copies captured
image to clipboard and sends notification on completion by default.

## Modes

Mode is a common screenshotting operation that is bundled into a single subcommand. Some modes are
compositor-specific such as `active-window`, but Gripper will run the correct command intended for
each compositor respectively.

These are the modes that are implemented, these can be run as `gripper <mode>`:

- `full`: Fullscreen (focused/selected monitor).
- `region`: Select a region using slurp. The region selection is free if you hold and drag, but if
  you use supported compositors it also has window snapping which highlights the window your cursor
  is currently in and automatically select the region occupied by the window by just clicking on it.
- `active-window`: Screenshot the window currently focused (needs supported compositor).
- `last-region`: The region selected by previous execution of `region`, `active-window` and `custom`
  mode.
- `custom`: Specify the region to capture yourself.

## Compositors

Gripper should run on compositors that [grim](https://sr.ht/~emersion/grim/) and
[slurp](https://github.com/emersion/slurp) support.

Some modes such as `active-window` require them to be implemented for specific compositor.

Supported compositor means that some compositor-specific operations are implemented for that
compositor.

Supported compositors for now:

- Hyprland
- Sway

## Prerequisites

Make sure the following commands are available. If you install Gripper with Nix, these are
automatically installed.

- `grim`
- `slurp`
- `jq`
- `wl-copy` (optional, for copying image to clipboard)
- `notify-send` (optional, for notification)

You can check them with the program by running `gripper --check`.

## Building

Gripper is written in pure C without any external libraries. Install `meson` then run these
commands:

```
$ meson setup build
$ meson compile -C build
$ ./build/gripper --help
```

### Nix (with Flake)

Simply run the following commands.

```
$ nix build
$ ./result/bin/gripper --help
```

All the required programs should be available to the program but check if your notification daemon
supports sending notification through `notify-send`.
