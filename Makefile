LUADIR=lua-5.0.2
SDLDIR=$(shell sdl-config --prefix)

CFLAGS   = -I$(LUADIR)/include/ -I$(SDLDIR)/include/SDL -std=gnu99 -Wall 
# CFLAGS  += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
CFLAGS  += -ggdb

LDFLAGS = -L$(LUADIR)/lib -L$(SDLDIR)/lib -levent -llua -llualib -lm -lSDL 
GUI_LDFLAGS = $(LDFLAGS) -lSDL_image -lSGE -lSDL_gfx

all: infond infon

infond: infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o 
	$(MAKE) -C $(LUADIR)
	$(CC) $^ $(LDFLAGS) -o $@

infon: infon.o client.o packet.o gui_player.o gui_world.o gui_creature.o gui_scroller.o 
	$(MAKE) -C $(LUADIR)
	$(CC) $^ $(GUI_LDFLAGS) -o $@ 

clean:
	$(MAKE) -C $(LUADIR) clean
	-rm -f *.o infond infon tags
