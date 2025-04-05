
bin_PROGRAMS = va_edit

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
        -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
        -DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I"$(srcdir)"/../srcLib \
	$(GENERICIMG_CFLAGS) \
	$(GLIBMM_CFLAGS) \
	$(SRCVIEW_CFLAGS) \
	$(ARCHIVE_CFLAGS) \
	$(GTKMM_CFLAGS)


va_edit_CFLAGS = -Wall
va_edit_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

va_edit_LDFLAGS = @EXTRA_LDFLAGS@

va_edit_LDADD =  \
 	../srcLib/libcommon.a \
	$(GENERICIMG_LIBS) \
	$(GLIBMM_LIBS) \
	$(SRCVIEW_LIBS) \
	$(ARCHIVE_LIBS) \
	$(GTKMM_LIBS)

va_edit_SOURCES = resources.c \
	PrefSourceView.cpp \
	PrefSourceView.hpp \
	EditApp.cpp \
	EditApp.hpp \
	RpcLaunch.cpp \
	RpcLaunch.hpp \
	LspServer.cpp \
	LspServer.hpp \
	Language.cpp \
	Language.hpp \
	SourceFile.cpp \
	SourceFile.hpp \
	SourceView.cpp \
	SourceView.hpp




# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

EXTRA_DIST = m4/ChangeLog