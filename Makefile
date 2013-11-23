include Makefile.inc

CFLAGS=$(PRJCFLAGS) -std=c99 -pedantic
LDFLAGS=-ldl -Wl,-rpath,lib/libutil/ -Wl,-export-dynamic

SOURCES=bot/bot.c          \
		bot/module.c       \
	   	bot/handlers.c     \
		bot/reguser.c      \
	   	bot/helpers.c      \
		irc/irc.c          \
		irc/session.c      \
	   	irc/util.c         \
	   	irc/channel.c      \
		irc/net/socket.c   \
		util/tokenbucket.c \
	   	util/log.c         \
		util/util.c

# Same as $(SOURCES) but with each entry having a .o extension
OBJECTS=$(addprefix src/, $(addsuffix .o, $(basename $(SOURCES))))
MODULES=mod_base mod_lua
MODULE_LIBS=$(addsuffix .so, $(MODULES))

# Subdirectory within src/modules/ where each mod resides
export MODSRCPATH=$(PWD)/src/modules

bot: $(OBJECTS) libutil
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) lib/libutil/libutil.so.1.0

$(OBJECTS):

# libutil does not get recompiled unless there is no .so file for it
libutil:
	@test ! -f "lib/libutil/libutil.so.1.0" && make -C lib/libutil/ || true

modules: $(MODULES)

$(MODULES): $(addsuffix .so, $@) force
	make -C "$(MODSRCPATH)/$@" BASEDIR=$(PWD)
	ln -sf "$(MODSRCPATH)/$@/$@.so" .
	echo

clean:
	@find -name '*.o'  -print -delete | sed -e 's/^/Delete /'
	@find -name '*.so' -print -delete | sed -e 's/^/Delete /'

	@find -type f -perm +111 -print -delete | sed -e 's/^/Delete /'

	$(foreach module, $(MODULES), @make -C $(addprefix $(MODSRCPATH)/, \
		$(module)) clean)

force:
	@true
