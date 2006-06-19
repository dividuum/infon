LUADIR=lua-5.0.2
SDLDIR=$(shell sdl-config --prefix)

CFLAGS   = -I$(LUADIR)/include/ -I$(SDLDIR)/include/SDL -std=gnu99 -Wall 
# CFLAGS  += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
CFLAGS  += -ggdb

LDFLAGS  = -L$(LUADIR)/lib -L$(SDLDIR)/lib -levent -llua -llualib -lm -lSDL -lSDL_image -lSGE -lSDL_gfx

all: infond

infond: infond.o server.o listener.o player.o map.o path.o sprite.o misc.o world.o path.o map.o video.o creature.o packet.o
	$(MAKE) -C $(LUADIR)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	$(MAKE) -C $(LUADIR) clean
	-rm -f *.o infon tags
