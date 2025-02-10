
bin_PROGRAMS = varsel

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
        -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
        -DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(VTE_CFLAGS) \
	$(ARCHIVE_CFLAGS) \
	$(LIBGIT2_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(GLIBMM_CFLAGS) \
	$(SRCVIEW_CFLAGS) \
	$(GTKMM_CFLAGS)

varsel_CFLAGS = -Wall
varsel_CXXFLAGS = -Wall \
	-Wextra \
	-Wpedantic \
	-Wno-unused-parameter

varsel_LDFLAGS = @EXTRA_LDFLAGS@

varsel_LDADD =  \
	$(VTE_LIBS) \
	$(ARCHIVE_LIBS) \
	$(LIBGIT2_LIBS) \
	$(GENERICIMG_LIBS) \
	$(GLIBMM_LIBS) \
	$(SRCVIEW_LIBS) \
	$(GTKMM_LIBS)

varsel_SOURCES = resources.c \
	VarselApp.cpp \
	VarselApp.hpp \
	VarselWin.cpp \
	VarselWin.hpp \
	FileDataSource.cpp \
	FileDataSource.hpp \
	Archiv.cpp \
	Archiv.hpp \
	ThreadWorker.cpp \
	ThreadWorker.hpp \
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
	PrefDialog.cpp \
	PrefDialog.hpp \
	SourceView.cpp \
	SourceView.hpp \
	EventBus.cpp \
	EventBus.hpp \
	DataSource.cpp \
	DataSource.hpp

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

EXTRA_DIST = m4/ChangeLog