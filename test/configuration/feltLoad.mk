#-----------------------------------------------------------------------------
# libwdbtest
#-----------------------------------------------------------------------------

# Library
#-----------------------------------------------------------------------------

noinst_LTLIBRARIES = 	libwdbTest.la

libwdbTest_la_SOURCES = test/configuration/testConfiguration.cpp

pkginclude_HEADERS = 	test/configuration/testConfiguration.h

libwdbTest_la_CPPFLAGS = \
						$(AM_CPPFLAGS) -fPIC

EXTRA_DIST +=			test/configuration/feltLoad.mk \
						test/configuration/Makefile.am \
						test/configuration/Makefile.in

DISTCLEANFILES =		test/configuration/Makefile


# Local Makefile Targets
#-----------------------------------------------------------------------------

test/configuration/all: libwdbTest.la

test/configuration/clean: clean
