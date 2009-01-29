
#-----------------------------------------------------------------------------
# WDB FeltLoad Component
#-----------------------------------------------------------------------------

feltLoad_SOURCES =  \
	src/main.cpp \
	src/feltConstants.h \
	src/feltTypeConversion.h \
	src/feltTypeConversion.cpp \
	src/FeltFile.cpp \
	src/FeltFile.h \
	src/FeltField.cpp \
	src/FeltField.h \
	src/FeltLoader.cpp \
	src/FeltLoader.h \
	src/FeltGridDefinition.h \
	src/FeltGridDefinition.cpp \
	src/FeltDatabaseInterface.h \
	src/FeltDatabaseConnection.cpp \
	src/FeltDatabaseConnection.h \
	src/transactors/ReadDataProvider.h \
	src/transactors/ReadValueParameter.h \
	src/transactors/ReadAdditionalLevels.h \
	src/transactors/ReadLevelParameter.h \
	src/transactors/ReadValidDuration.h
	
EXTRA_DIST +=				src/feltLoad.mk
