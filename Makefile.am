#-----------------------------------------------------------------------------
# Project Makefile for wdb's feltLoad
#
# This file is processed by automake to generate the non-recursive
# project Makefile
#-----------------------------------------------------------------------------


# Scanned by Autoreconf; must be static (no expansion)
ACLOCAL_AMFLAGS = 		-I config
				
# Share directory organization
wdbdatadir = 			$(datadir)/wdb
sqldir = 				$(wdbdatadir)/sql
wdblibdir = $(libdir)/wdb

# Setting Targets
wdblib_PROGRAMS = feltLoad

# This is to be replaced by a "find wdb" macro:
AM_CXXFLAGS = $(wdb_CFLAGS)
	
feltLoad_CPPFLAGS =  
feltLoad_LDADD = $(wdb_LIBS) \
				-lwdbLoaderBase -lwdbConfiguration -lwdbProjection -lwdbException -lwdbLogHandler -lwdbMath


SED_SUBSTITUTION = 		sed s/__WDB_VERSION__/$(WDB_VERSION)/g $<\
						| sed s¤__WDB_LIB_PATH__¤"$(LD_LIBRARY_PATH)"¤g \
						| sed s:__WDB_LOGDIR__:@abs_builddir@/var/logs:g \
						| sed s:__WDB_BUILDDIR__:@abs_builddir@:g \
						| sed s:__WDB_SRCDIR__:@srcdir@:g \
			 			| sed s:__WDB_BINDIR__:@bindir@:g \
			 			| sed s:__WDB_LIBDIR__:@libdir@:g \
			 			| sed s:__WDB_PKGLIBDIR__:$(libdir)/wdb:g \
			 			| sed s:__WDB_PKGDATADIR__:$(pkgdatadir):g \
			 			| sed s:__WDB_DATADIR__:$(wdbdatadir):g \
			 			| sed s:__WDB_SYSCONFDIR__:@sysconfdir@:g \
			 			| sed s:__WDB_LOCALSTATEDIR__:@localstatedir@:g \
						| sed s:__POSTGIS_DIR__:@postgis_SQL@:g \
						| sed s:__WDB_DBNAME__:$(DATABASE_NAME):g \
						| sed s:__WDB_SCHEMA__:wdb_$(DATABASE_SCHEMA):g \
						| sed s:__WCI_SCHEMA__:wci_$(DATABASE_SCHEMA):g \
						| sed s:__WCI_LIB__:wci-$(WDB_VERSION).so:g \
			 			> $@

EXTRA_DIST =

include src/feltLoad.mk
include test/feltLoad.mk
###
# Debian package related stuff
###

PKG_DIR = $(PACKAGE)-$(VERSION)
DEBIAN_DIR = $(PKG_DIR)/debian
DEBIAN_PACKAGE = $(PACKAGE)_$(VERSION)
DEBIAN_VERSION = 1
ARCH = `dpkg-architecture -qDEB_HOST_ARCH_CPU`
DEBIAN_PACKAGE_NAME = $(DEBIAN_PACKAGE)-$(DEBIAN_VERSION)_$(ARCH).deb

debian:	dist clean-debian 
	tar xvzf $(PKG_DIR).tar.gz
	mkdir -p $(DEBIAN_DIR)
	rm -rf $(DEBIAN_DIR)/*
	cp -r $(top_srcdir)/debian_files/* $(DEBIAN_DIR)
	cp $(PKG_DIR).tar.gz $(DEBIAN_PACKAGE).orig.tar.gz
	cd $(PKG_DIR) && dpkg-buildpackage -rfakeroot -us -uc
	lintian $(DEBIAN_PACKAGE_NAME)
	linda $(DEBIAN_PACKAGE_NAME)

update-debian:
	rm -rf $(DEBIAN_DIR)/*
	cp -r $(top_srcdir)/debian_files/* $(DEBIAN_DIR)
	cd $(PKG_DIR) && dpkg-buildpackage -rfakeroot -us -uc -nc
	lintian $(DEBIAN_PACKAGE_NAME)
	linda $(DEBIAN_PACKAGE_NAME)


clean-debian:
	debclean
	rm -rf $(PKG_DIR) $(DEBIAN_PACKAGE)*
