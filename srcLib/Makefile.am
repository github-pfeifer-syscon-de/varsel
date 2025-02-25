## Process this file with automake to produce Makefile.in
## for now a static lib is sufficent
## a shared lib woud reqire a separate build and install

noinst_LIBRARIES = libcommon.a

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir). "\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(ARCHIVE_CFLAGS) \
	$(GENERICIMG_CFLAGS)

libcommon_a_CFLAGS = -Wall
libcommon_a_CXXFLAGS = -Wall

libcommon_a_SOURCES = \
	EventBus.cpp \
	EventBus.hpp \
	Archiv.cpp \
	Archiv.hpp \
	ThreadWorker.cpp \
	ThreadWorker.hpp \
	VarselConfig.cpp \
	VarselConfig.hpp \
	ListFactory.cpp \
	ListFactory.hpp \
	SourceFactory.cpp \
	SourceFactory.hpp


## include_HEADERS =


