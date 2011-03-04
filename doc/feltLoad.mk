.xml.html:
	$(DOCBOOK) html-nochunks $< -o `dirname $@`

DOCUMENTATION_SRC = \
	doc/developer/feltload_system-design-specification.xml \
	doc/developer/feltload_system-test-specification.xml

DOCUMENTATION_OUTPUT = $(DOCUMENTATION_SRC:.xml=.html)
	
html_DATA = $(DOCUMENTATION_OUTPUT)

documentation:	$(DOCUMENTATION_OUTPUT)

UNUSED_DOC = \
	doc/user/feltLoad.man.xml

USED_DOCS = \
	doc/docbook/wdb_entities.ent \
	doc/docbook/xinclude.mod
	
EXTRA_DIST += $(DOCUMENTATION_SRC) $(USED_DOCS)

CLEANFILES += $(DOCUMENTATION_OUTPUT)

include doc/doxygen/feltLoad.mk
include doc/man/feltLoad.mk
