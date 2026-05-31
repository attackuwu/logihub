project(
  'logihub',
  'c',
  version: '1.0.0',
  default_options: ['warning_level=2', 'c_std=c17'],
)

cc      = meson.get_compiler('c')
gtk_dep = dependency('gtk4', version: '>=4.10')
m_dep   = cc.find_library('m', required: false)

executable(
  'logihub',
  ['src/main.c', 'src/device.c', 'src/udev_install.c'],
  dependencies: [gtk_dep, m_dep],
  install: true,
)
