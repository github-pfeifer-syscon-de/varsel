
incSrcLibTest = include_directories(
                    '../src'
                  , '../srcLib'
                  , '../srcEdit'
                  , '../srcList'
                  , '..')   # for ...config.h

git_test = executable('git_test'
    , ['Git_test.cpp'
      , '../srcList/GitRepository.cpp']
    , dependencies: va_list_deps
    , include_directories : incSrcLibTest)
test('git_test', git_test)

archiv_test = executable('archiv_test'
    , ['ArchivTest.cpp']
    , dependencies: va_list_deps
    , include_directories : incSrcLibTest
    , link_with: [varsel_lib])
test('archiv_test', archiv_test)

lsp_test = executable('lsp_test'
    , ['LspTest.cpp'
      ,'../srcEdit/RpcLaunch.cpp'
      ,'../srcEdit/LspServer.cpp'
      ,'../srcEdit/LspConf.cpp']
    , dependencies: va_edit_deps
    , include_directories : incSrcLibTest
    , link_with: [varsel_lib])
test('lsp_test', lsp_test)

file_test = executable('file_test'
    , ['FileTest.cpp']
    , dependencies: varsel_deps
    , include_directories : incSrcLibTest
    , link_with: [varsel_lib])
test('file_test', file_test)