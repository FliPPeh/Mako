CFLAGS=-Iinclude/ -Wall -g
LDFLAGS=-ldl -Wl,-export-dynamic
SOURCES=bot/bot.c bot/module.c bot/handlers.c 				\
		irc/irc.c irc/session.c irc/util.c irc/channel.c  	\
		irc/net/socket.c					  				\
		util/list.c util/log.c util/util.c
CC=clang -Wextra #-std=gnu11


OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))
MODULES=mod_ctcp mod_lua
MODULE_LIBS=$(addsuffix .so, $(MODULES))

# Subdirectory within src/modules/ where each mod resides
export MODSRCPATH=$(PWD)/src/modules

bot: objects
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

objects: $(OBJECTS)
$(OBJECTS):

modules: $(MODULES)

$(MODULES): $(addsuffix .so, $@)
	make -C "$(MODSRCPATH)/$@" INC=$(PWD)/include/ SRC=$(PWD)/src/
	ln -sf "$(MODSRCPATH)/$@/$@.so" .
	echo

clean:
	@find -name '*.o' -print -delete | sed -e 's/^/Delete /'
	@find -name '*.so' -print -delete | sed -e 's/^/Delete /'

	$(forach module, $(MODULES), @make -C $(addprefix $(MODSRCPATH)/, \
		$(module)) clean)

	@find -type f -perm +111 -print -delete | sed -e 's/^/Delete /'
