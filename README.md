# victus-control

**victus-control** restores manual thermal and keyboard lighting controls to HP Victus and Omen laptops on Linux. HP firmware hides the necessary ACPI hooks, so stock kernels expose only AUTO/MAX fan modes. This project combines a patched `hp-wmi` driver, a privileged backend watchdog, and a GTK4 desktop app to unlock true manual control.

> [!WARNING]
> Tested primarily on **HP Victus 16-s00xxxx** (CachyOS/Arch). Other Victus or Omen models may work but are unverified—use at your own risk and monitor temperatures.

## Architecture at a Glance
- **Patched hp-wmi driver** (`dkms`): sourced from [Vilez0/hp-wmi-fan-and-backlight-control](https://github.com/Vilez0/hp-wmi-fan-and-backlight-control). Enables writable `fan*_target` nodes and single-zone RGB that HP disables upstream.
- **Backend service** (`victus-backend`): C++17 daemon running as the `victus-backend` user. Talks to the driver through sysfs, exposes a UNIX socket at `/run/victus-control/victus_backend.sock`, and reapplies manual speeds every 90 seconds to survive firmware resets. Provides a “Better Auto” control loop that watches CPU/GPU temps + utilisation (sampling every ~2 s) and continuously retunes RPMs in the background while clamping to each fan’s hardware maximum.
- **Frontend** (`victus-control`): GTK4 UI for selecting AUTO/MANUAL/MAX, setting manual RPM steps, and adjusting keyboard color/brightness. Communicates with the backend over the socket.

## Requirements
- 64-bit Linux with `systemd` (validated on Arch-based distributions).
- Packages: `meson`, `ninja`, `gtk4`, `git`, `dkms`, `linux-headers` (installer pulls them via pacman).
- Root access to install the kernel module, systemd unit, sudoers entries, and build artifacts.

## Quick Start Installation
```bash
git clone https://github.com/Batuhan4/victus-control.git
cd victus-control
sudo ./install.sh
```
`install.sh` performs the full setup:
1. Installs build/runtime dependencies.
2. Creates the `victus` group, the sandboxed `victus-backend` user, and places `/usr/bin/set-fan-speed.sh` with the required sudoers entry.
3. Clones and registers the patched `hp-wmi` DKMS module, rebuilding it when kernels change.
4. Builds and installs the backend daemon, GTK frontend, `systemd-tmpfiles`, and service units.
5. Enables and starts `victus-backend.service`, which hosts the control socket in `/run/victus-control/`.

Log out and back in after installation so your user picks up the new `victus` group membership.

## Using victus-control
- Launch the GUI from your application menu or run `victus-control` in a terminal.
- Switch modes with the dropdown: `AUTO`, `Better Auto`, `MANUAL`, or `MAX`. In MANUAL, the slider maps eight steps between ~2000 RPM and ~5700 RPM. “Better Auto” keeps the hardware in manual mode while a background loop samples CPU/GPU temps and utilisation and reasserts fan targets every 90 s (with the required 10 s offset between fans), fixing the stock firmware behaviour where AUTO leaves both fans idling near 2000 RPM regardless of temperature.
- Keyboard tab lets you pick RGB colors (single-zone) and brightness. Fn+P performance hotkey remains functional.
- Check service status with `systemctl status victus-backend.service`; logs are available via `journalctl -u victus-backend`.

## Development & Manual Builds
```bash
meson setup build --prefix=/usr
meson compile -C build
meson install -C build    # sudo required
gtk4-builder-tool    # optional GTK debug helpers
```
- Run the backend smoke test (requires service running): `python test_backend.py`.
- Installer intentionally fetches the hp-wmi DKMS source during setup; the directory is ignored in Git to keep the repo lean.

## Troubleshooting
- **No manual control:** Ensure `dkms status | grep hp-wmi-fan-and-backlight-control` shows the module installed, then `sudo modprobe hp_wmi`.
- **Permission denied:** Verify `groups $USER` lists `victus`. Re-run the installer or add yourself manually: `sudo usermod -aG victus $USER`.
- **Socket missing:** `sudo systemd-tmpfiles --create` then `sudo systemctl restart victus-backend.service`.
- **Uninstalling:** Disable the service (`sudo systemctl disable --now victus-backend`) and remove the DKMS module (`sudo dkms remove hp-wmi-fan-and-backlight-control/0.0.2 --all`).

## Contributing
See `AGENTS.md` for contributor guidelines, coding style, and testing expectations. Manual fan features must be validated on real hardware—note any gaps in your pull request.

## License
GPLv3. Refer to `LICENSE` for full details.
