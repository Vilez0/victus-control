#!/bin/bash
# This script applies the necessary permissions fix for victus-control.
set -e

echo "--- Applying permissions fix ---"

# 1. Copy the udev rule to the correct system directory
echo "--> Copying udev rule..."
cp /home/batuhan4/victus-control/99-hp-wmi-permissions.rules /etc/udev/rules.d/

# 2. Reload udev rules to apply the changes
echo "--> Reloading udev rules..."
udevadm control --reload-rules && udevadm trigger

# 3. Restart the backend service
echo "--> Restarting backend service..."
systemctl restart victus-backend.service

echo ""
echo "--- Fix applied successfully! ---"
echo "The fan control should now work correctly."
