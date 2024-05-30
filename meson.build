project(
  'waysnip',
  'c',
  version : '0.1.0',
  license: 'MIT',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ])

cc = meson.get_compiler('c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-Og', language: 'c')
endif

subdir('src')

executable('waysnip',
           src,
           include_directories : 'src',
           install : true)
