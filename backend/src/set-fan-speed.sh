#!/bin/bash

# This script sets the fan speed for a given fan number.
# It is intended to be called by the victus-control backend service with sudo.

# Exit immediately if a command exits with a non-zero status.
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <fan_number> <speed>"
    exit 1
fi

FAN_NUM=$1
SPEED=$2

# Find the correct hwmon directory path
HWMON_BASE="/sys/devices/platform/hp-wmi/hwmon"
HWMON_PATH=$(find "$HWMON_BASE" -mindepth 1 -type d -name "hwmon*" | head -n 1)
echo "Debug: Found hwmon path: $HWMON_PATH" >&2

if [ -z "$HWMON_PATH" ]; then
    echo "Error: Hwmon directory not found."
    exit 1
fi

TARGET_FILE="$HWMON_PATH/fan${FAN_NUM}_target"

if [ ! -w "$TARGET_FILE" ]; then
    # Even though we run with sudo, this check is good for debugging.
    echo "Error: Target file does not exist or is not writable: $TARGET_FILE"
    # Attempting to write anyway, sudo will handle permissions.
fi

# CRITICAL: Ensure manual mode is enabled right before setting speed.
# This is the most likely fix for the Fan 1 race condition.
echo "1" > "$HWMON_PATH/pwm1_enable"

# This is the command that is known to work.
echo "$SPEED" | tee "$TARGET_FILE" > /dev/null
