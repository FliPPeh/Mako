CFLAGS=-Isrc/ -Wall -g -Wextra -Wno-unused-parameter -Wunused -std=c11 -O
LDFLAGS=-ldl -Wl,-export-dynamic
SOURCES=bot/bot.c bot/module.c bot/handlers.c bot/reguser.c bot/helpers.c \
		irc/irc.c irc/session.c irc/util.c irc/channel.c  	\
		irc/net/socket.c					  				\
		util/list.c util/log.c util/util.c util/hashtable.c util/tokenbucket.c
CC=clang


OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))
MODULES=mod_base mod_lua
MODULE_LIBS=$(addsuffix .so, $(MODULES))

# Subdirectory within src/modules/ where each mod resides
export MODSRCPATH=$(PWD)/src/modules

bot: objects
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

objects: $(OBJECTS)
$(OBJECTS):

modules: $(MODULES)

$(MODULES): $(addsuffix .so, $@)
	make -C "$(MODSRCPATH)/$@" INC=$(PWD)/src/ SRC=$(PWD)/src/
	ln -sf "$(MODSRCPATH)/$@/$@.so" .
	echo

clean:
	@find -name '*.o' -print -delete | sed -e 's/^/Delete /'
	@find -name '*.so' -print -delete | sed -e 's/^/Delete /'

	$(forach module, $(MODULES), @make -C $(addprefix $(MODSRCPATH)/, \
		$(module)) clean)

	@find -type f -perm +111 -print -delete | sed -e 's/^/Delete /'
