#!/bin/bash
set -euo pipefail

module_name="hp-wmi-fan-and-backlight-control"
module_version="0.0.2"
log_prefix="[victus-healthcheck]"

if ! command -v dkms >/dev/null 2>&1; then
    echo "$log_prefix dkms command not found; skipping kernel module verification" >&2
    exit 0
fi

current_kernel="$(uname -r)"

status_output="$(dkms status -m "${module_name}" -v "${module_version}" || true)"
if [[ ! "$status_output" =~ ${current_kernel}.*installed ]]; then
    echo "$log_prefix module not built for ${current_kernel}; attempting dkms autoinstall" >&2
    if ! dkms autoinstall "${module_name}/${module_version}" >/dev/null 2>&1; then
        echo "$log_prefix dkms autoinstall failed; retrying targeted install" >&2
        dkms install "${module_name}/${module_version}" -k "${current_kernel}" >/dev/null 2>&1 || true
    fi
fi

if ! lsmod | grep -q '^hp_wmi'; then
    echo "$log_prefix loading hp_wmi kernel module" >&2
    modprobe hp_wmi || echo "$log_prefix warning: failed to load hp_wmi module" >&2
fi

exit 0
