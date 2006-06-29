LUADIR=lua-5.0.2
SDLDIR=$(shell sdl-config --prefix)

CFLAGS   = -I$(LUADIR)/include/ -I$(SDLDIR)/include/SDL -std=gnu99 -Wall 
# CFLAGS  += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
CFLAGS  += -ggdb

LDFLAGS 	= -L$(LUADIR)/lib -levent -lSDL -llua -llualib -lm
GUI_LDFLAGS = -L$(SDLDIR)/lib -levent -lSDL -lSDL_image -lSGE -lSDL_gfx

all: infond infon

infond: lua-5.0.2/lib/liblua.a  infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o 
	$(CC) $^ $(LDFLAGS) -o $@

infon: lua-5.0.2/lib/liblua.a infon.o client.o packet.o misc.o gui_player.o gui_world.o gui_creature.o gui_scroller.o video.o sprite.o
	$(CC) $^ $(GUI_LDFLAGS) -o $@ 

lua-5.0.2/lib/liblua.a:
	$(MAKE) -C $(LUADIR)

clean:
	$(MAKE) -C $(LUADIR) clean
	-rm -f *.o infond infon tags
