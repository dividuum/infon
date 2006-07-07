LUADIR=lua-5.0.2

REVISION=$(shell svn info |grep Revision| cut -b 11-)

COMMON_CFLAGS  = -std=gnu99 -Wall -DREVISION="$(REVISION)"
# COMMON_CFLAGS += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
COMMON_CFLAGS += -ggdb -I$(LUADIR)/include/ 

ifdef WINDOWS
	MINGW  = $(HOME)/progs/mingw32/
	SDLDIR = $(MINGW)
	CC     = /opt/xmingw/bin/i386-mingw32msvc-gcc
	CFLAGS = $(COMMON_CFLAGS) -I$(MINGW)/include
else
	SDLDIR = $(shell sdl-config --prefix)
	CFLAGS = $(COMMON_CFLAGS)
endif

CFLAGS     += -I$(SDLDIR)/include/SDL 

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
