CFLAGS=-I$(INC) -Wall -g -fpic
LDFLAGS=-shared
CC=clang -Wextra -std=gnu11

SOURCES=mod_base.c
OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))

mod_base.so: objects
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

objects: $(OBJECTS)

$(OBJECTS):

