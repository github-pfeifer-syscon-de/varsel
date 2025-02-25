
bin_PROGRAMS = varsel

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
        -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
        -DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I"$(srcdir)"/../srcLib \
	$(ARCHIVE_CFLAGS) \
	$(VTE_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(GLIBMM_CFLAGS) \
	$(GTKMM_CFLAGS)

varsel_CFLAGS = -Wall
varsel_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

varsel_LDFLAGS = @EXTRA_LDFLAGS@

varsel_LDADD =  \
 	../srcLib/libcommon.a \
	$(ARCHIVE_LIBS) \
	$(VTE_LIBS) \
	$(GENERICIMG_LIBS) \
	$(GLIBMM_LIBS) \
	$(GTKMM_LIBS)

varsel_SOURCES = resources.c \
	VarselApp.cpp \
	VarselApp.hpp \
	VarselWin.cpp \
	VarselWin.hpp \
	PrefDialog.cpp \
	PrefDialog.hpp
# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

EXTRA_DIST = m4/ChangeLog