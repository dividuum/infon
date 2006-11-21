PREFIX  ?= ./
LUADIR   = lua-5.1.1
REVISION = $(shell svnversion . || echo 'exported')

# EVENT_NAME = Computer Night 2006
# EVENT_HOST = 172.30.100.1

COMMON_CFLAGS  = -std=gnu99 -Wall -DREVISION="\"$(REVISION)\"" -I$(LUADIR)/src/ # -DCHEATS

ifdef EVENT_NAME
	COMMON_CFLAGS += -DEVENT_NAME="\"$(EVENT_NAME)\""
endif
ifdef EVENT_HOST
	COMMON_CFLAGS += -DEVENT_HOST="\"$(EVENT_HOST)\""
endif

ifdef WINDOWS 
	OPTIMIZE=1
endif

ifdef OPTIMIZE
	COMMON_CFLAGS += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
else
	COMMON_CFLAGS += -ggdb
endif

ifdef WINDOWS
	MINGW    = $(HOME)/progs/mingw32/
	SDLDIR   = $(MINGW)
	CC       = /opt/xmingw/bin/i386-mingw32msvc-gcc
	CFLAGS  += $(COMMON_CFLAGS) -I$(MINGW)/include
	WINDRES  = /opt/xmingw/bin/i386-mingw32msvc-windres
	LUAPLAT  = mingw
else
	SDLDIR   = $(shell sdl-config --prefix)
	CFLAGS  += $(COMMON_CFLAGS)
	LUAPLAT  = debug #linux
	LDFLAGS += -ldl
endif

CFLAGS  += -I$(SDLDIR)/include/SDL -DPREFIX=\"$(PREFIX)\"
LDFLAGS += -levent -lm -lz

ifdef WINDOWS
	MINGW=/home/dividuum/progs/mingw32/
	GUI_LDFLAGS += $(MINGW)/lib/libevent.a $(MINGW)/lib/libz.a $(MINGW)/lib/libSDL_gfx.a   \
				   -lmingw32 $(MINGW)/lib/libSDLmain.a \
	               -lwsock32 -mwindows -Wl,-s
	RES=infon.res				   
	GUI_EXECUTABLE=infon.exe

	SDL_RENDERER_LDFLAGS+= $(MINGW)/lib/libSGE.a $(MINGW)/lib/libevent.a $(MINGW)/lib/libSDL_image.a \
				   $(MINGW)/lib/libpng.a $(MINGW)/lib/libz.a     $(MINGW)/lib/libSDL_gfx.a   \
				   $(MINGW)/lib/libSDL.a \
				   -lmingw32 $(MINGW)/lib/libSDLmain.a \
	               -lstdc++ -lwsock32 -lwinmm -mwindows -Wl,-s
	SDL_RENDERER =sdl_gui.dll
	NULL_RENDERER=null_gui.dll

	RENDERER=$(SDL_RENDERER)
else
	GUI_LDFLAGS = -L$(SDLDIR)/lib -levent -lz -lm -ldl
	GUI_EXECUTABLE=infon

	NULL_RENDERER=null_gui.so

	SDL_RENDERER_LDFLAGS =-lSDL -lSDL_image -lSGE -lSDL_gfx 
	SDL_RENDERER=sdl_gui.so

	GL_RENDERER_LDFLAGS =-lSDL -lGL -lGLU
	GL_RENDERER=gl_gui.so 

	AA_RENDERER_LDFLAGS =-laa
	AA_RENDERER=aa_gui.so

	LILITH_RENDERER_LDFLAGS =-lSDL -lGL -lGLU -lstdc++
	LILITH_RENDERER=lilith_gui.so 
	CPPFLAGS=$(CFLAGS) -fPIC

	RENDERER=$(SDL_RENDERER) $(NULL_RENDERER) $(AA_RENDERER) $(LILITH_RENDERER) $(GL_RENDERER) 
endif

all: infond $(GUI_EXECUTABLE) $(RENDERER)

dist:
	$(MAKE) source-dist
	$(MAKE) clean
	WINDOWS=1  $(MAKE) win32-client-dist
	$(MAKE) clean
	OPTIMIZE=1 $(MAKE) linux-client-dist
	$(MAKE) clean
	$(MAKE) linux-server-dist

source-dist: distclean
	tar cvzh -C.. --exclude ".svn" --exclude "infon-source*" --file infon-source-r$(REVISION).tgz infon

win32-client-dist: $(GUI_EXECUTABLE)
	/opt/xmingw/bin/i386-mingw32msvc-strip $(GUI_EXECUTABLE)
	upx -9 --all-methods $(GUI_EXECUTABLE)
	zip infon-win32-r$(REVISION).zip README $(GUI_EXECUTABLE) gfx/*.fnt gfx/*.png example.demo

linux-client-dist: $(GUI_EXECUTABLE)
	strip $(GUI_EXECUTABLE)
	# upx $(GUI_EXECUTABLE)
	tar cfvz infon-linux-i386-r$(REVISION).tgz README $(GUI_EXECUTABLE) gfx/*.fnt gfx/*.png

linux-server-dist: infond
	# strip infond infond-static
	# upx infond infond-static
	tar cfvz infond-linux-i386-r$(REVISION).tgz README infond infond-static *.lua level/*.lua rules/*.lua

infond: infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o game.o $(LUADIR)/src/liblua.a  
	$(CC) $^ $(LDFLAGS) -o $@
	$(CC) $^ $(LDFLAGS) -static -o $@-static

$(GUI_EXECUTABLE): infon.o client.o packet.o misc.o client_player.o client_world.o client_creature.o client_game.o renderer.o $(RES)
	$(CC) $^ $(GUI_LDFLAGS) -o $@ 

$(NULL_RENDERER): null_gui.o
	$(CC) $^ -shared -o $@

$(AA_RENDERER): aa_gui.o
	$(CC) $^ $(AA_RENDERER_LDFLAGS)  -shared -o $@

$(SDL_RENDERER): sdl_video.o sdl_sprite.o sdl_gui.o misc.o
	$(CC) $^ $(SDL_RENDERER_LDFLAGS) -shared -o $@

$(GL_RENDERER): gl_video.o gl_gui.o gl_mdl.o misc.o
	$(CC) $^ $(GL_RENDERER_LDFLAGS) -shared -o $@

$(LILITH_RENDERER): lilith_gui.o misc.o lilith/lilith/liblilith.a
	$(CC) $^ $(LILITH_RENDERER_LDFLAGS) -shared -o $@

infon.res: infon.rc
	$(WINDRES) -i $^ -DREVISION="\\\"$(REVISION)\\\"" --input-format=rc -o $@ -O coff

$(LUADIR)/src/liblua.a:
	$(MAKE) -C $(LUADIR) $(LUAPLAT)

clean:
	-rm -f *.o *.so *.dll infond infond-static infon infon.exe infon.res tags 

distclean: clean
	$(MAKE) -C $(LUADIR) clean
	-rm -f infon*.zip infon*.tgz *.orig *.rej infond-*.demo
