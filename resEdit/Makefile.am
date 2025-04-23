## build resources
##   the autoreconf complains about the $(shell) construct, fix see configure.ac  -Wno-portability.
##   But this seems the best way to keep this portable

PKGCONFIG = pkg-config
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)
GLIB_COMPILE_SCHEMAS = $(shell $(PKGCONFIG) --variable=glib_compile_schemas gio-2.0)

VARSEL_RESOURCE_XML=varsel.gresources.xml
RESOURCE_C=../srcEdit/resources.c
VARSEL_RESOURCES=$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies $(VARSEL_RESOURCE_XML))

all:$(RESOURCE_C)

$(RESOURCE_C): $(VARSEL_RESOURCE_XML) $(VARSEL_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --target=$(RESOURCE_C) --sourcedir=. --generate-source $(VARSEL_RESOURCE_XML)

# The desktop files
desktopdir = $(datadir)/applications
desktop_DATA=va_edit.desktop

# application icon
icon_basename=va_edit
pixmapdir=$(datadir)/icons/hicolor/64x64/apps
pixmap_DATA=$(icon_basename).png
appicondir=$(datadir)/icons/hicolor/scalable/apps
appicon_DATA=$(icon_basename).svg

# include source for build
EXTRA_DIST = $(VARSEL_RESOURCE_XML) $(VARSEL_RESOURCES) $(desktop_DATA) $(pixmap_DATA) $(appicon_DATA)

clean-local:
	-rm -rf $(RESOURCE_C)
