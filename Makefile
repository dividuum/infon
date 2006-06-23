#SERVER_GUI=1

LUADIR=lua-5.0.2
SDLDIR=$(shell sdl-config --prefix)

CFLAGS   = -I$(LUADIR)/include/ -I$(SDLDIR)/include/SDL -std=gnu99 -Wall 
# CFLAGS  += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
CFLAGS  += -ggdb

ifdef SERVER_GUI
	LDFLAGS = -L$(LUADIR)/lib -L$(SDLDIR)/lib -levent -llua -llualib -lm -lSDL -lSDL_image -lSGE -lSDL_gfx
	GUIFILES=sprite.o video.o 
	CFLAGS+=-DSERVER_GUI=1
else
	LDFLAGS = -L$(LUADIR)/lib -L$(SDLDIR)/lib -levent -llua -llualib -lm -lSDL 
endif

all: infond

infond: infond.o server.o listener.o player.o map.o path.o misc.o world.o path.o map.o creature.o packet.o scroller.o $(GUIFILES)
	$(MAKE) -C $(LUADIR)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	$(MAKE) -C $(LUADIR) clean
	-rm -f *.o infond tags
