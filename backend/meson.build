executable('victus-backend',
  sources: ['src/main.cpp', 'src/util.cpp', 'src/util.hpp'],
  install: true,
  install_dir: get_option('bindir'))

install_data(
	'victus-backend.service',
	install_dir: '/etc/systemd/system'
)