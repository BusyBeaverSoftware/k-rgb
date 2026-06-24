# k-rgb

A native **KDE / Qt 6** application (plus a CLI and a Python reference tool) for
controlling the per-key RGB lighting of the **Alienware AW410K** mechanical
keyboard on Linux — no vendor software, no root daemon, no cloud.

It talks directly to the keyboard's vendor HID interface over `hidraw`, so it's
small, fast, and dependency-light.

> Status: fully working — solid colours, a visual per-key editor, named
> profiles, per-key static rainbow, all the hardware effects, brightness, a
> system-tray quick-switcher, and restore-on-login.

## Features

- **Solid colour** across the whole keyboard
- **Per-key custom editor** — a graphical keyboard you paint directly: click,
  box-select, or Ctrl-click keys, pick a colour, and assign it per key
- **Named profiles** — save any number of lighting setups and switch between
  them from the window or the tray; the active one is restored at login
- **Per-key static rainbow** (all 107 keys individually addressed)
- **Hardware effects** that run on the keyboard itself: Breathing,
  Pulse, Spectrum, Single Wave, Rainbow Wave, Scanner
- **Speed** and **direction** controls (per effect)
- **Brightness** (software intensity scaling) with live preview
- **System-tray icon** (`KStatusNotifierItem`) with quick controls — profile
  switcher, Off, Rainbow, Spectrum, colour presets, show/hide, quit
- **Remembers your setup** and can **restore it at login**
- Runs **without root** via a udev rule

## Supported hardware

| Device | USB ID | Interface |
| --- | --- | --- |
| Alienware AW410K RGB Mechanical Keyboard | `04f2:1968` | 2 (vendor HID, `0xFF00`) |

The protocol is the AW410K/AW510K family; adding the closely-related **AW510K**
(`04f2:1830`) would be a small change. Contributions for other Alienware
devices are welcome.

## Requirements

- Linux with a recent KDE Plasma / Qt 6 desktop
- Qt 6, KDE Frameworks 6
- A C++17 compiler and CMake ≥ 3.16

### Build dependencies (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake extra-cmake-modules \
  qt6-base-dev qt6-base-dev-tools \
  libkf6coreaddons-dev libkf6i18n-dev libkf6config-dev libkf6configwidgets-dev \
  libkf6widgetsaddons-dev libkf6xmlgui-dev libkf6dbusaddons-dev \
  libkf6statusnotifieritem-dev
```

## Build

```bash
git clone https://github.com/BusyBeaverSoftware/k-rgb.git
cd k-rgb
cmake -S . -B build -DBUILD_GUI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces:

- `build/bin/krgb` — the KDE GUI
- `build/krgb-cli` — a command-line tool

The core engine and CLI have **no Qt/KF6 dependency** — omit `-DBUILD_GUI=ON`
to build just those.

## Install

```bash
sudo cmake --install build
```

Installs `krgb` and `krgb-cli` to `/usr/local/bin`, the desktop entry, icon,
AppStream metainfo, and the udev rule (to `/lib/udev/rules.d`). Pass
`-DCMAKE_INSTALL_PREFIX=/usr` at configure time to install under `/usr` instead.
After installing, **k-rgb** appears in your application menu; reload udev (below)
once so the keyboard is accessible without root.

## Permissions (run without root)

Install the udev rule so your desktop user can access the keyboard's lighting
interface:

```bash
sudo cp packaging/udev/60-alienware-aw410k.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=hidraw
```

(You may need to replug the keyboard once.) Your user must be in the `plugdev`
group. Until this is installed you can still run the tools with `sudo`.

## Usage

### GUI

```bash
./build/bin/krgb
```

Pick a mode/colour and hit **Apply**, or right-click the tray icon for quick
presets. Closing the window hides it to the tray; quit from the tray menu.

Two independent login options live at the bottom of the window:

- **Restore lighting at login** — re-applies the active profile's lighting at
  login (runs `krgb --apply` headlessly and exits; no window, no tray icon).
- **Start k-rgb in the system tray at login** — launches k-rgb hidden to the
  tray (`krgb --tray`) so the quick-switcher is always available. Tick both if
  you want the tray icon *and* your lighting restored.

**Per-key editor:** click the **Per-Key Editor…** button next to the mode
selector (or choose **Per-key (custom)** as the mode) to reveal a graphical
keyboard. Click a key to select it, drag a box to select many, or Ctrl-click to
add/remove. Pick a colour, then **Paint Selected** (or **Fill All**); **Off
Selected** turns keys dark. Changes apply live. The **Presets** row offers
ready-made **USA Flag** and **Ukraine Flag** layouts as a starting point.

**Profiles:** use the **Profile** bar at the top to create, rename, or delete
named setups. Switching profiles (in the window or from the tray's **Profiles**
submenu) applies it immediately; the active profile is the one restored at
login.

### CLI

```bash
krgb-cli solid 0 255 255      # whole keyboard cyan
krgb-cli rainbow              # per-key static rainbow
krgb-cli spectrum             # animated hardware rainbow
krgb-cli breathing 0 128 255
krgb-cli key ESC 255 0 0      # light a single key (see keymap)
krgb-cli off
```

### Python reference tool

`tools/aw410k.py` is a zero-dependency implementation of the full protocol,
useful for experimentation and as living documentation:

```bash
python3 tools/aw410k.py --dry-run solid 0 255 255   # print packets, touch nothing
python3 tools/aw410k.py rainbow
```

## How it works

The AW410K exposes a vendor-defined HID interface (usage page `0xFF00`,
interface 2). Lighting is set by writing 65-byte reports to its `hidraw` node:

- a feature-report "prelude" puts the keyboard into software control,
- solid/per-key frames stream 4 keys per packet,
- hardware effects are selected with a single mode packet.

The packet format was learned from the open-source
[OpenRGB](https://openrgb.org/) project's AW410K driver (the protocol facts);
this is an original, independent implementation.

## Project layout

```
src/core/     AW410KDevice — the hidraw protocol engine (no Qt/KF6)
src/cli/      krgb-cli — command-line front-end
src/gui/      krgb — KDE/Qt 6 GUI (KStatusNotifierItem, KConfig, KColorButton)
tools/        aw410k.py — Python reference driver
packaging/    udev rule for rootless access
```

## Roadmap

- Per-key colours in the CLI (`krgb-cli` currently does single keys / rainbow)
- Import/export profiles to a file
- Additional Alienware devices (AW510K, mouse, headset, chassis AlienFX)

## Credits

- [OpenRGB](https://openrgb.org/) — open-source reverse engineering of the
  AW410K lighting protocol
- [AKBL](https://github.com/rsm-gh/akbl) — Alienware chassis lighting on Linux

## License

[GPL-2.0-or-later](LICENSE). © 2026 Randy Yates / BusyBeaverSoftware.
