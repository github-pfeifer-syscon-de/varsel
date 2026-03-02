
va_lib_src = files(
      'EventBus.cpp'
    , 'Archiv.cpp'
    , 'VarselConfig.cpp'
    , 'ListFactory.cpp'
    , 'ExecFactory.cpp'
    , 'SourceFactory.cpp'
    )

va_lib_deps= [
      thread_deps
    , libarchive_deps
    , genericimg_deps
    ]

varsel_lib = static_library('varselLib'
    , va_lib_src
    , include_directories: incDir
    , dependencies: va_lib_deps)