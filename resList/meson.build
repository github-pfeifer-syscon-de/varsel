
#
# see https://mesonbuild.com/Gnome-module.html
gnome = import('gnome')

va_list_resources = gnome.compile_resources(
    'va_list_res', 'va_list.gresources.xml'
    , source_dir: meson.current_source_dir()
)

# icons + desktop
dataDir = join_paths(get_option('prefix'), get_option('datadir'))
iconDir = join_paths(dataDir, 'icons/hicolor')
install_data(
      'va_list.png'
    , install_dir: join_paths(iconDir, '64x64/apps'))
install_data(
      'va_list.svg'
    , install_dir: join_paths(iconDir, 'scalable/apps'))
install_data(
      'va_list.desktop'
    , install_dir: join_paths(dataDir, 'applications'))

