# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2025-01-14

### Added

- `--format`: Customise output file name.

### Fixed

- Fix unable to pass name to `-d` and `-f` with spaces.
- Improve argument parsing and error reporting.
- Fix program not exiting correctly when passing path with subdirectories to `-f`.

## [1.1.0] - 2025-01-05

### Added

- Detect image format from file extension with `-f` flag.

### Fixed

- Fix program not exiting correctly if the given output (`-o` flag) is unknown.
- Fix program trying to screenshot in `active-window` mode if there's no active window.
- Wait (`-w` flag) directly before executing grim.

## [1.0.2] - 2025-01-01

### Fixed

- Print screenshot output path when successful.
- Warn user if output path already exists.
- Fix not checking if cache directory exists before caching last region.
- Fix unable to fail correctly when a needed directory path already exists but not as a directory.

## [1.0.1] - 2024-07-04

### Fixed

- Fix Nix package.

## [1.0.0] - 2024-06-03

### Added

- Initial release.

[Unreleased]: https://github.com/spitulax/gripper/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/spitulax/gripper/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/spitulax/gripper/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/spitulax/gripper/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/spitulax/gripper/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/spitulax/gripper/releases/tag/v1.0.0
