
SUBDIRS = res src test po

dist_doc_DATA = \
	NEWS \
	ChangeLog \
	COPYING \
	AUTHORS \
	INSTALL

ACLOCAL_AMFLAGS = --install -I m4

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(docdir)
