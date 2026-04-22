# gnu_jcom

`gnu_jcom` is a Qt 6 / QML serial console aimed at the Arch Linux + Hyprland MVP described in `doc/PRD.md`.

## Features

- Serial port enumeration and open/close control
- Configurable baud rate, data bits, stop bits, parity, and flow control
- ASCII / HEX send modes with selectable line ending
- Live receive log with ASCII or HEX display toggle
- Periodic send for the current composer payload
- Saved send list with manual replay
- Custom packet definition editor in the UI
- Optional JSON import/export for packet definitions
- Packet field editor with HEX frame preview and direct packet send
- Packet receive parsing with live field display
- Port hotplug detection and same-port auto reconnect
- Plain text log export
- JSON workspace save/load for session settings, send templates, and packet editor state
- Permission error hints tailored for Arch Linux
- Demo device for virtual serial port testing

## Arch Dependencies

Install the required packages from the official repository:

```bash
sudo pacman -S --needed qt6-base qt6-declarative qt6-wayland qt6-serialport qt6-charts cmake ninja gcc pkgconf
```

Optional development tools:

```bash
sudo pacman -S --needed qt6-tools gdb ccache
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

Run the demo device:

```bash
./build/gnu_jcom_demo_device /dev/tnt1 115200 examples/linear_demo_schema.json
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Virtual Serial Demo

If you use `tty0tty` on Arch Linux:

```bash
sudo modprobe tty0tty
ls -l /dev/tnt*
```

Typical loopback pairs are:

- `/dev/tnt0 <-> /dev/tnt1`
- `/dev/tnt2 <-> /dev/tnt3`

Recommended parser test flow:

1. Start the demo device on one side of the pair:

```bash
./build/gnu_jcom_demo_device /dev/tnt1 115200 examples/linear_demo_schema.json
```

2. Start `gnu_jcom` in another window:

```bash
./build/gnu_jcom
```

3. In `gnu_jcom`:

- Open `/dev/tnt0`
- Expand the right-side panel
- Go to `解析`
- Keep the default `linear_demo` definition, or import `examples/linear_demo_schema.json`
- Click `应用解析`
- Verify the receive values continuously update as `x / x2 / x3`

The bundled demo schema uses:

- Header: `AA 55`
- Fields: `x`, `x2`, `x3`
- Checksum: `sum8`

## Notes

- Workspace files default to the Qt app config directory as `workspace.json`.
- Session log export defaults to `~/Documents/gnu_jcom-session.log`.
- The default packet schema path resolves to the bundled `examples/linear_demo_schema.json` when available.
- On Arch Linux, serial permissions often require the user to be in `uucp` and `lock`.
