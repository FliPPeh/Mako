include ${BASEDIR}/Makefile.inc

CFLAGS=$(PRJCFLAGS) -fpic -std=c99 -pedantic
LDFLAGS=-shared

SOURCES=mod_base.c
OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))

mod_base.so: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJECTS):

