include ${BASEDIR}/Makefile.inc

CFLAGS=$(PRJCFLAGS) -fpic
LDFLAGS=-shared

SOURCES=mod_base.c
OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))

mod_base.so: objects
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

objects: $(OBJECTS)
$(OBJECTS):

