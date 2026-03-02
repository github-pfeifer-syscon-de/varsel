
va_list_src = files(
      'ListApp.cpp'
    , 'FileDataSource.cpp'
    , 'ArchiveDataSource.cpp'
    , 'GitDataSource.cpp'
    , 'VarselList.cpp'
    , 'GitRepository.cpp'
    , 'ListColumns.cpp'
    , 'ExtractDialog.cpp'
    , 'CopyDialog.cpp'
    , 'LookupEntry.cpp'
    , 'DataSource.cpp'
    )

va_list_src  += va_list_resources

va_list_deps = [
      thread_deps
    , libarchive_deps
    , libgit_deps 
    , genericimg_deps
    ]

va_list_target = executable('va_list'
    , va_list_src
    , dependencies: va_list_deps
    , include_directories: incDir
    , link_with: varsel_lib
    , install: true
    , win_subsystem: 'windows'  # disable this to see console output for windows
    )
