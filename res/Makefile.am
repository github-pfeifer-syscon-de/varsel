## build resources
##   the autoreconf complains about the $(shell) construct, fix see configure.ac  -Wno-portability.
##   But this seems the best way to keep this portable

PKGCONFIG = pkg-config
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)
GLIB_COMPILE_SCHEMAS = $(shell $(PKGCONFIG) --variable=glib_compile_schemas gio-2.0)

CALCPP_RESOURCE_XML=varsel.gresources.xml
RESOURCE_C=../src/resources.c
CALCPP_RESOURCES=$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies $(CALCPP_RESOURCE_XML))

all:$(RESOURCE_C) 

$(RESOURCE_C): $(CALCPP_RESOURCE_XML) $(CALCPP_RESOURCES) 
	$(GLIB_COMPILE_RESOURCES) --target=$(RESOURCE_C) --sourcedir=. --generate-source $(CALCPP_RESOURCE_XML)

# The desktop files
desktopdir = $(datadir)/applications
desktop_in_files=varsel.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)

# application icon
appicondir = $(datadir)/icons/hicolor/64x64/apps
appicon_DATA = varsel.png
appdir_icon=$(appicondir)/$(appicon_DATA)

# include source for build
EXTRA_DIST = $(CALCPP_RESOURCE_XML) $(CALCPP_RESOURCES) $(desktop_in_files) $(appicon_DATA)

clean-local:
	-rm -rf $(RESOURCE_C)
	-rm -rf $(GSCHEMA_COMPILED)
	-rm -rf $(desktop_DATA)

$(desktop_DATA): $(desktop_in_files)
	@regex_cmd@ -e "s,\@APP_ICON\@,$(appdir_icon),g" $? > $@
