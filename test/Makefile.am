

TESTS = git_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GLIBMM_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(LIBGIT2_CFLAGS) \
	-I ../src

AM_CFLAGS= \
	-Wall \
	-Wpedantic

AM_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

AM_LDFLAGS =

git_test_LDADD =  \
	../src/varsel-GitRepository.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(LIBGIT2_LIBS)

check_PROGRAMS = git_test

git_test_SOURCES = common.c args.c Git_test.cpp
