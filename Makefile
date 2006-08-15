LUADIR=lua-5.0.2

REVISION=$(shell svnversion .)

COMMON_CFLAGS  = -std=gnu99 -Wall -DREVISION="$(REVISION)" -I$(LUADIR)/include/ # -DCHEATS
ifdef WINDOWS
	COMMON_CFLAGS += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
else
	COMMON_CFLAGS += -ggdb
endif

ifdef WINDOWS
	MINGW  = $(HOME)/progs/mingw32/
	SDLDIR = $(MINGW)
	CC     = /opt/xmingw/bin/i386-mingw32msvc-gcc
	CFLAGS = $(COMMON_CFLAGS) -I$(MINGW)/include
	WINDRES= /opt/xmingw/bin/i386-mingw32msvc-windres
else
	SDLDIR = $(shell sdl-config --prefix)
	CFLAGS = $(COMMON_CFLAGS)
endif

CFLAGS     += -I$(SDLDIR)/include/SDL 
LDFLAGS 	= -L$(LUADIR)/lib -levent -llua -llualib -lm -lz

ifdef WINDOWS
	MINGW=/home/dividuum/progs/mingw32/
	GUI_LDFLAGS += $(MINGW)/lib/libSGE.a $(MINGW)/lib/libevent.a $(MINGW)/lib/libSDL_image.a \
				   $(MINGW)/lib/libpng.a $(MINGW)/lib/libz.a     $(MINGW)/lib/libSDL_gfx.a   \
				   $(MINGW)/lib/libSDL.a \
				   -lmingw32 $(MINGW)/lib/libSDLmain.a \
	               -lstdc++ -lwsock32 -lwinmm -mwindows -Wl,-s
	RES=infon.res				   
	GUI_EXECUTABLE=infon.exe
else
	GUI_LDFLAGS = -L$(SDLDIR)/lib -levent -lSDL -lSDL_image -lSGE -lSDL_gfx -lz -lm
	GUI_EXECUTABLE=infon
endif

all: infond $(GUI_EXECUTABLE)

dist:
	$(MAKE) source-dist
	$(MAKE) clean
	WINDOWS=1 $(MAKE) win32-client-dist
	$(MAKE) clean
	$(MAKE) linux-client-dist linux-server-dist

source-dist: distclean
	tar cvz -C.. --exclude ".svn" --exclude "infon-source*" --file infon-source-r$(REVISION).tgz infon

win32-client-dist: $(GUI_EXECUTABLE)
	/opt/xmingw/bin/i386-mingw32msvc-strip $(GUI_EXECUTABLE)
	zip      infon-win32-r$(REVISION).zip README $(GUI_EXECUTABLE) gfx/*.fnt gfx/*.png example.demo

linux-client-dist: $(GUI_EXECUTABLE)
	strip $(GUI_EXECUTABLE)
	tar cfvz infon-linux-r$(REVISION).tgz README $(GUI_EXECUTABLE) gfx/*.fnt gfx/*.png

linux-server-dist: infond
	strip infond infond-static
	tar cfvz infond-linux-r$(REVISION).tgz README infond infond-static *.lua level/*.lua rules/*.lua

infond: lua-5.0.2/lib/liblua.a  infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o game.o
	$(CC) $^ $(LDFLAGS) -o $@
	$(CC) $^ $(LDFLAGS) -static -o $@-static

$(GUI_EXECUTABLE): infon.o client.o packet.o misc.o gui_player.o gui_world.o gui_creature.o gui_scroller.o gui_game.o video.o sprite.o $(RES)
	$(CC) $^ $(GUI_LDFLAGS) -o $@ 

infon.res: infon.rc
	$(WINDRES) -i $^ --input-format=rc -o $@ -O coff

lua-5.0.2/lib/liblua.a:
	$(MAKE) -C $(LUADIR)

clean:
	-rm -f *.o infond infond-static infon infon.exe infon.res tags 

distclean: clean
	$(MAKE) -C $(LUADIR) clean
	-rm -f infon*.zip infon*.tgz *.orig *.rej
