
varsel_src = files(
      'VarselApp.cpp'
    , 'VarselWin.cpp'
    , 'PrefDialog.cpp'
    )

varsel_src += varsel_resources

varsel_deps = [
      thread_deps
    , vte_deps
    , genericimg_deps
    ]

varsel_target = executable(meson.project_name()
    , varsel_src
    , dependencies: varsel_deps
    , include_directories: incDir
    , link_with: varsel_lib
    , install: true
    , win_subsystem: 'windows'  # disable this to see console output for windows
    )
