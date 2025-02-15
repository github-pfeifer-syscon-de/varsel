

TESTS = git_test worker_test archiv_test

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

check_PROGRAMS = git_test worker_test archiv_test

git_test_LDADD =  \
	../src/varsel-GitRepository.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(LIBGIT2_LIBS)

git_test_SOURCES =  Git_test.cpp

worker_test_LDADD = \
	../src/varsel-ThreadWorker.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS)

worker_test_SOURCES = WorkerTest.cpp

archiv_test_LDADD = \
	../src/varsel-Archiv.o \
	$(GLIBMM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(ARCHIVE_LIBS)

archiv_test_SOURCES = ArchivTest.cpp
