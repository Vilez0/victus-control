# victus-control

**victus-control** is a software for HP Victus and Omen laptops that provides fan control and RGB keyboard settings.

## Features

- **Fan Control**: Monitor and control the fan speeds (auto and max).
- **RGB Keyboard Settings**: Change keyboard backlighting colors, brightness and toggle on/off.
- **Service Support**: Backend runs as a systemd service.

## Requirements

- **Meson**: Required for building the project.
- **Ninja**: Used for the build process.
- **GTK4**: The frontend is developed using GTK4.
- **Systemd**: Backend runs as a systemd service.

## Recent Fixes

- **Major Permissions Overhaul**: The application's permissions model has been completely reworked to be secure and reliable. The backend now runs as a dedicated, non-privileged user (`victus-backend`), and a custom `udev` rule automatically handles permissions for the fan control hardware. This resolves a critical issue where fan control would fail for most users.
- **Fan Mode Persistence**: Implemented a workaround for an issue where HP firmware reverts custom fan modes to "AUTO" after about two minutes. The backend service now periodically reapplies the selected fan mode to ensure it persists.

## Installation (Arch Linux)

The installation process has been automated. Simply clone the repository and run the installer script.

```bash
git clone https://github.com/Vilez0/victus-control
cd victus-control
sudo ./install.sh
```

The script will handle all necessary steps:
- Installing dependencies
- Creating the required users and groups
- Installing the patched `hp-wmi` kernel module via DKMS
- Building and installing the application
- Setting up and starting the backend `systemd` service

After the script completes, you **must log out and log back in** for the group changes to take effect. You can then launch the application from your desktop menu or by running `victus-control`.

## Usage
With victus-control, you can:

- Monitor and control fan speeds (MAX and AUTO only!!)
- Change keyboard backlighting colors and toggle on/off.

## Contributing
If you'd like to contribute to the project, please follow these steps:

Fork the repository, make your changes, submit a pull request.

Any contribution is welcome!!

## License
This project is licensed under the GPL3 License. See the LICENSE file for more details.