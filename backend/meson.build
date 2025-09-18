executable('victus-backend',
  sources: ['src/fan.cpp', 'src/fan.hpp', 'src/keyboard.cpp','src/keyboard.hpp', 'src/main.cpp', 'src/util.cpp', 'src/util.hpp'],
  dependencies: [dependency('threads')],
  install: true,
  install_dir: get_option('bindir'))

install_data(
	'victus-backend.service',
	install_dir: '/etc/systemd/system'
)

install_data(
    'victus-healthcheck.service',
    install_dir: '/etc/systemd/system'
)

install_data(
    'victus-healthcheck.sh',
    install_dir: '/usr/lib/victus-control',
    install_mode: 'rwxr-xr-x'
)

install_data(
    'victus-control.rules',
    install_dir: '/etc/udev/rules.d'
)

install_data(
    'victus-control.tmpfiles',
    install_dir: '/etc/tmpfiles.d',
    rename: 'victus-control.conf'
)
