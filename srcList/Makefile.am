
bin_PROGRAMS = va_list

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
        -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
        -DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I"$(srcdir)"/../srcLib \
	$(ARCHIVE_CFLAGS) \
	$(LIBGIT2_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(GLIBMM_CFLAGS) \
	$(GTKMM_CFLAGS)

va_list_CFLAGS = -Wall
va_list_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

va_list_LDFLAGS = @EXTRA_LDFLAGS@

va_list_LDADD =  \
 	../srcLib/libcommon.a \
	$(ARCHIVE_LIBS) \
	$(LIBGIT2_LIBS) \
	$(GENERICIMG_LIBS) \
	$(GLIBMM_LIBS) \
	$(GTKMM_LIBS)

va_list_SOURCES = resources.c \
	ListApp.cpp \
	ListApp.hpp \
	FileDataSource.cpp \
	FileDataSource.hpp \
	ArchiveDataSource.cpp \
	ArchiveDataSource.hpp \
	GitDataSource.cpp \
	GitDataSource.hpp \
	VarselList.cpp \
	VarselList.hpp \
	DataModel.cpp \
	DataModel.hpp \
	GitRepository.cpp \
	GitRepository.hpp \
	DataSource.cpp \
	DataSource.hpp

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

EXTRA_DIST = m4/ChangeLog