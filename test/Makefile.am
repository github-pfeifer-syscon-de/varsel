

TESTS = git_test archiv_test lsp_test file_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GLIBMM_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(LIBGIT2_CFLAGS) \
	-I ../srcList \
	-I ../srcEdit \
	-I ../srcLib

AM_CFLAGS= \
	-Wall \
	-Wpedantic

AM_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

AM_LDFLAGS =

check_PROGRAMS = git_test archiv_test lsp_test file_test

git_test_LDADD =  \
	../srcList/va_list-GitRepository.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(LIBGIT2_LIBS)

git_test_SOURCES = Git_test.cpp

archiv_test_LDADD = \
	../srcLib/libcommon.a \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(ARCHIVE_LIBS)

archiv_test_SOURCES = ArchivTest.cpp

lsp_test_LDADD = \
	../srcLib/libcommon.a \
	../srcEdit/va_edit-RpcLaunch.o \
	../srcEdit/va_edit-LspServer.o \
	../srcEdit/va_edit-LspConf.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS)

lsp_test_SOURCES = LspTest.cpp

file_test_LDADD = \
	../srcLib/libcommon.a \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS)

file_test_SOURCES = FileTest.cpp

