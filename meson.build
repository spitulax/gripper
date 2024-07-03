project(
  'gripper',
  'c',
  version : '1.0.0',
  license: 'MIT',
  default_options : [
    'c_std=c11',
    'warning_level=3',
  ])

cc = meson.get_compiler('c')

if get_option('buildtype').startswith('debug')
  add_project_arguments(['-Og', '-DDEBUG'], language: 'c')
endif

add_project_arguments('-DPROG_NAME="' + meson.project_name() + '"', language: 'c')
add_project_arguments('-DPROG_VERSION="' + meson.project_version() + '"', language: 'c')
add_project_arguments(['-Wconversion', '-Wsign-conversion', '-Wpedantic'], language: 'c')

subdir('src')

executable(
  'gripper',
  src,
  include_directories : 'src',
  install : true)
