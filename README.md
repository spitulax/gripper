<h1 align="center">Waysnip</h1>

<h4 align="center">Simple and easy to use screenshot program for Wayland for just works out-of-the box experience</h4>

Waysnip runs [grim](https://sr.ht/~emersion/grim/) under the hood, but makes it easier to use
by providing "alias" of common operations in grim that are often compositor-specific.
Also copies screenshot to clipboard and sends notification on completion.

## Modes

Mode is a common screenshooting operation that is bundled into single subcommand.
Some modes are compositor-specific such as `active-window`, but Waysnip will run the correct command intended for each compositor respectively.

These are the modes that are implemented, these can be run as `waysnip <mode>`:
- `full`: Fullscreen (TODO: select monitor)
- `region`: Select a region using slurp. The region selection is free if you hold and drag, but if you use supported compositors it also has window snapping which highlights the window your cursor is currently in and automatically select the region the window occupies by clicking on it.
- `active-window`: Screenshot the window currently focused (needs supported compositor).
- `last-region`: The region selected by previous execution of `region` or `active-window` mode.

Supported compositor means that some compositor-specific operations are implemented for that compositor.
Supported compositors currently:
- Hyprland

## Compositors

Waysnip should run on compositors that [grim](https://sr.ht/~emersion/grim/) and [slurp](https://github.com/emersion/slurp) support.

Some modes such as `active-window` requires it to be implemented for specific compositor.

## Prerequisites

Make sure the following commands are available.

_NOTE: I might implement my own screenshot functionality instead of using grim._

- `grim`
- `slurp`
- `wl-copy`
- `notify-send` (optional, for notification)

You can check them with the program by running `waysnip --check`.

## Building

Waysnip is written in pure C without any external libraries.
Simply install a C compiler such as `gcc` or `clang` and run the Makefile.

```
$ make
$ ./build/waysnip --help
```

### Nix (with Flakes)

Simply run the following commands.

```
$ nix build
$ ./result/bin/waysnip --help
```

All the required programs should be available to the program but check if your notification daemon supports `notify-send`.
