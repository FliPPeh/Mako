CFLAGS=-I$(INC) -Iinclude/ -Wall -g -fpic
LDFLAGS=-shared -llua -lm
CC=clang

SOURCES=mod_lua.c lua_util.c modules/log.c modules/core.c
OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))

mod_lua.so: objects
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

objects: $(OBJECTS)

$(OBJECTS):

clean:
	@find -name '*.o' -print -delete | sed -e 's/^/Delete /'
