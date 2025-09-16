# Installation Guide for `victus-control` on Arch Linux / CachyOS

This guide provides detailed, step-by-step instructions to securely install and configure `victus-control` on Arch-based Linux distributions.

## 1. Install Dependencies

First, install the necessary packages to build and run the application from the official repositories.

```bash
sudo pacman -S meson ninja gtk4 systemd git
```

## 2. Create Users and Groups

For security, the backend service runs as a dedicated, non-privileged user (`victus-backend`). You must create this user and two groups (`victus-backend` and `victus`) to ensure secure communication between the backend service and the frontend application.

1.  **Create the backend user and group:**
    This user will run the backend service. The `-r` flag creates a system user, which cannot be used to log in.

    ```bash
    sudo useradd -r -s /usr/bin/nologin -g victus-backend victus-backend
    ```

2.  **Create the user group:**
    This group is for users who are allowed to control the application.

    ```bash
    sudo groupadd victus
    ```

3.  **Add your user to the `victus` group:**
    Replace `$USER` with your actual username if it's not set correctly in your shell.

    ```bash
    sudo usermod -aG victus $USER
    ```

4.  **Apply Group Changes:**
    For the new group membership to take effect, you must **log out and log back in**.

## 3. Install the HP-WMI Kernel Module

This application requires a patched `hp-wmi` kernel module. You must install it for the fan and keyboard controls to work.

```bash
# Clone the repository for the kernel module
git clone https://github.com/Vilez0/hp-wmi-fan-and-backlight-control.git
cd hp-wmi-fan-and-backlight-control

# Install kernel headers for your current kernel
sudo pacman -S linux-headers

# Build and install the module using DKMS
sudo dkms add .
sudo dkms install hp-wmi/1.0

# Load the new module
sudo rmmod hp_wmi
sudo modprobe hp_wmi
```

## 4. Build and Install `victus-control`

Now, you can clone and install the `victus-control` application itself.

```bash
# Go back to your home or source directory
cd ..

# Clone the application repository
git clone https://github.com/Vilez0/victus-control
cd victus-control

# Build the project
meson setup build --prefix=/usr
ninja -C build

# Install the application and system files
sudo ninja -C build install
```

## 5. Enable and Start the Backend Service

The installer placed the necessary `systemd` service, `udev` rule, and `tmpfiles.d` config in the correct locations.

1.  **Reload systemd and udev:**
    This makes the system aware of the new service and rules.

    ```bash
    sudo systemctl daemon-reload
    sudo udevadm control --reload-rules && sudo udevadm trigger
    ```

2.  **Enable and start the service:**
    This will start the backend service now and ensure it starts automatically on boot.

    ```bash
    sudo systemctl enable --now victus-backend.service
    ```

## 6. Launch the Application

You can now launch the frontend application. It will appear in your desktop environment's application menu as "Victus Control", or you can run it from the terminal:

```bash
victus-control
```

The application should now be running securely and reliably.
