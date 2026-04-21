# gnu_jcom

`gnu_jcom` is a Qt 6 / QML serial console aimed at the Arch Linux + Hyprland MVP described in `doc/PRD.md`.

## Features

- Serial port enumeration and open/close control
- Configurable baud rate, data bits, stop bits, parity, and flow control
- ASCII / HEX send modes with selectable line ending
- Live receive log with ASCII or HEX display toggle
- Periodic send for the current composer payload
- Saved send list with manual replay
- Port hotplug detection and same-port auto reconnect
- Plain text log export
- JSON workspace save/load for session settings and send templates
- Permission error hints tailored for Arch Linux

## Arch Dependencies

Install the required packages from the official repository:

```bash
sudo pacman -S --needed qt6-base qt6-declarative qt6-wayland qt6-serialport cmake ninja gcc pkgconf
```

Optional development tools:

```bash
sudo pacman -S --needed qt6-tools gdb ccache
```

If you later add charting support, install:

```bash
sudo pacman -S --needed qt6-charts
```

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

`build/compile_commands.json` is generated automatically, so the existing clangd workspace setting `--compile-commands-dir=build` works after configure/build.

Run the app:

```bash
./build/gnu_jcom
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Notes

- Workspace files default to the Qt app config directory as `workspace.json`.
- Session log export defaults to `~/Documents/gnu_jcom-session.log`.
- On Arch Linux, serial permissions often require the user to be in `uucp` and `lock`.
