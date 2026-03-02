
va_edit_src = files(
      'EditApp.cpp'
    , 'LspConf.cpp'
    , 'LspServer.cpp'
    , 'PrefSourceView.cpp'
    , 'RpcLaunch.cpp'
    , 'SourceFile.cpp'
    , 'SourceView.cpp'
    )

va_edit_src += va_edit_resources

va_edit_deps = [
      thread_deps
    , srcview_deps
    , genericimg_deps
    ]

va_edit_target = executable('va_edit'
    , va_edit_src
    , dependencies: va_edit_deps
    , include_directories: incDir
    , link_with: varsel_lib
    , install: true
    , win_subsystem: 'windows'  # disable this to see console output for windows
    )
