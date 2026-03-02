project('varsel', 'c', 'cpp'
    , default_options: ['c_std=c11'
                       , 'cpp_std=c++20'
                       , 'buildtype=debugoptimized'
                       , 'warning_level=3']
    , license : 'GPL-3.0'
    , meson_version: '>=1.3.0'
    , version: '0.1')

fs = import('fs')

pkgdatadir = join_paths(get_option('prefix'), get_option('datadir'), meson.project_name())
pkglocaledir = join_paths(get_option('prefix'), get_option('datadir'), 'locale')
pkgsrcdir = fs.relative_to( meson.global_source_root(), meson.current_build_dir()) + '/src'
add_project_arguments('-DPACKAGE_DATA_DIR="' + pkgdatadir + '"', language: ['cpp','c'])
add_project_arguments('-DPACKAGE="' + meson.project_name() + '"', language: ['cpp','c'])
# this is to recreate the conventions from autotools
add_project_arguments('-DPACKAGE_SRC_DIR="' + pkgsrcdir + '"', language: ['cpp','c'])
add_project_arguments('-DGETTEXT_PACKAGE="' + meson.project_name() + '"', language: ['cpp','c'])
add_project_arguments('-DPACKAGE_LOCALE_DIR="' + pkglocaledir + '"', language: ['cpp','c'])

compiler_add_flags = [  # limit the compiler noisiness
      '-Wno-unused-parameter'
     ,'-Wno-overlength-strings'  # this matters only if you are pedantic
    ]
cc = meson.get_compiler('c')
foreach ccflag : compiler_add_flags
    if cc.has_argument(ccflag)
        add_project_arguments(ccflag, language: ['c', 'cpp'])
    endif
endforeach

conf = configuration_data()
conf.set_quoted('VARSEL_VERSION', meson.project_version())     # Surround the version in quotes to make it a C string
conf.set_quoted('TOPSRCDIR', meson.global_source_root())       # use for git test
config_file = configure_file(output : meson.project_name() + '_config.h'
               , configuration : conf)
incDir = include_directories('srcLib'
                            ,fs.parent(config_file))

genericimg_deps = dependency('genericimg', version : '>= 0.4.8')
vte_deps = dependency('vte-2.91', version : '>= 0.78.0')
srcview_deps = dependency('gtksourceview-4', version : '>= 4.8')
libarchive_deps = dependency('libarchive', version : '>= 3.7.0')
libgit_deps = dependency('libgit2', version : '>= 1.8.0')
thread_deps = dependency('threads')
subdir('srcLib')
subdir('res')
subdir('src')
subdir('resList')
subdir('srcList')
subdir('resEdit')
subdir('srcEdit')
subdir('test')
subdir('po')
