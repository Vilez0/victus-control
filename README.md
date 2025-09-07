# victus-control

> [!NOTE]
> This is an initial UI based on https://github.com/betelqeyza/victus-control, expect missing features and major frontend UI changes (it's planned, not the main focus tho)

**victus-control** is a software for HP Victus and Omen laptops with fan control and RGB keyboard settings.

**IMPORTANT:** You need to use [my hp-wmi fork](https://github.com/Vilez0/hp-wmi-fan-and-backlight-control) to make this work.

## Features

- **Fan Control**: Monitor and control the fan speeds (auto and max).
- **RGB Keyboard Settings**: Change keyboard backlighting colors, brightness and toggle on/off.
- **Service Support**: Backend runs as a systemd service.

## Requirements

- **Meson**: Required for building the project.
- **Ninja**: Used for the build process.
- **GTK4**: The frontend is developed using GTK4.
- **Systemd**: Backend runs as a systemd service.

## Installation

### 1. Install Dependencies

Install the necessary dependencies:

For Arch-based systems:

```bash
sudo pacman -S meson ninja gtk4 systemd
```

For Debian/Ubuntu-based systems:

```bash
sudo apt-get install meson ninja-build libgtk-4-dev systemd
```

### 2. Clone the Repository

```bash
git clone https://github.com/Vilez0/victus-control
cd victus-control
```

### 3. Build 'n' install

```bash
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```

### 4. Enable 'n' Start the Backend Service

```bash
sudo systemctl daemon-reload
sudo systemctl enable victus-backend.service
sudo systemctl start victus-backend.service
```

### 5. Launch the Application

To launch the frontend application, search for "Victus Control" in your DE or run it directly from the terminal:

```bash
victus-control
```

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