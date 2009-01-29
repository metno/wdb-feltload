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


# Setting Targets
pkglib_PROGRAMS = feltLoad

# This is to be replaced by a "find wdb" macro:
AM_CXXFLAGS = $(wdb_CFLAGS)
	
feltLoad_CPPFLAGS =  
feltLoad_LDADD = $(wdb_LIBS) \
				-lwdbLoaderBase -lwdbConfiguration -lwdbProjection -lwdbException -lwdbLogHandler


SED_SUBSTITUTION = 		sed s/__WDB_VERSION__/$(WDB_VERSION)/g $<\
						| sed s�__WDB_LIB_PATH__�"$(LD_LIBRARY_PATH)"�g \
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