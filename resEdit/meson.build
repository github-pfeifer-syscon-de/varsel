
#
# see https://mesonbuild.com/Gnome-module.html
gnome = import('gnome')

va_edit_resources = gnome.compile_resources(
    'va_edit_res', 'va_edit.gresources.xml'
    , source_dir: meson.current_source_dir()
)

# icons + desktop
dataDir = join_paths(get_option('prefix'), get_option('datadir'))
iconDir = join_paths(dataDir, 'icons/hicolor')
install_data(
      'va_edit.png'
    , install_dir: join_paths(iconDir, '64x64/apps'))
install_data(
      'va_edit.svg'
    , install_dir: join_paths(iconDir, 'scalable/apps'))
install_data(
      'va_edit.desktop'
    , install_dir: join_paths(dataDir, 'applications'))

