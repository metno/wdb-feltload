
#-----------------------------------------------------------------------------
# WCI Types Unit Test Framework
#
# See: <top>/test/unit
#-----------------------------------------------------------------------------
							
if HAS_CPPUNIT

TESTS =   			feltLoadTest
check_PROGRAMS =	feltLoadTest


feltLoadToTest = src/FeltGridDefinition.cpp
							
feltLoadTest_SOURCES = \
	test/wdbUnitTest.cpp \
	$(feltLoadToTest) \
	test/FeltGridDefinitionTest.cpp \
	test/FeltGridDefinitionTest.h
	
wdbUnitTest_CPPFLAGS = 		$(cppunit_CFLAGS) -I$(srcdir)/test/utility/configuration
	
	
feltLoadTest_CPPFLAGS =		-I$(top_srcdir)/src $(cppunit_CFLAGS)
feltLoadTest_LDFLAGS  =   	-lwdbTest $(feltLoad_LDADD) $(cppunit_LIBS)

EXTRA_DIST += 				test/FakeDatabaseConnection.h
endif
    						
EXTRA_DIST +=				test/feltLoad.mk

