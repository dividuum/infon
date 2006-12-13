ifndef REVISION
REVISION := $(shell svnversion . || echo 'exported')
endif

# EVENT_NAME = Computer Night 2006
# EVENT_HOST = 172.30.100.1

CFLAGS += -pedantic -std=gnu99 -Wall -DREVISION="\"$(REVISION)\""

ifdef EVENT_NAME
	CFLAGS += -DEVENT_NAME="\"$(EVENT_NAME)\""
endif

ifdef EVENT_HOST
	CFLAGS += -DEVENT_HOST="\"$(EVENT_HOST)\""
endif

ifdef WINDOWS 
	OPTIMIZE=1
endif

ifdef OPTIMIZE
	CFLAGS += -O3 -fexpensive-optimizations -finline-functions -fomit-frame-pointer -DNDEBUG
else
	CFLAGS += -ggdb
endif

ifdef WINDOWS
	PREFIX           ?= .\\
	MINGW             = $(HOME)/progs/mingw32/
	SDLDIR            = $(MINGW)
	CC                = /opt/xmingw/bin/i386-mingw32msvc-gcc
	CFLAGS           += -I$(MINGW)/include
	WINDRES           = /opt/xmingw/bin/i386-mingw32msvc-windres
	STRIP             = /opt/xmingw/bin/i386-mingw32msvc-strip
	LUAPLAT            = mingw
	INFON_EXECUTABLE  = infon.exe

	SDL_RENDERER      = sdl_gui.dll
	NULL_RENDERER     = null_gui.dll

	RENDERER          = $(SDL_RENDERER)
else
	PREFIX           ?= ./
	SDLDIR           := $(shell sdl-config --prefix)
ifdef OPTIMIZE
	LUAPLAT           = optimize
else
	LUAPLAT           = debug
endif

	INFON_EXECUTABLE  = infon
	INFOND_EXECUTABLE = infond

	NULL_RENDERER     = null_gui.so
	SDL_RENDERER      = sdl_gui.so
	GL_RENDERER       = gl_gui.so 
	AA_RENDERER       = aa_gui.so
	LILITH_RENDERER   = lilith_gui.so 

	RENDERER          = $(SDL_RENDERER) $(NULL_RENDERER) 
endif

CFLAGS += -DPREFIX=\"$(PREFIX)\"

############################################################
# Target specific Configuration
############################################################

ifdef WINDOWS
$(INFON_EXECUTABLE) : LDFLAGS  += $(MINGW)/lib/libevent.a $(MINGW)/lib/libz.a \
                                  -lmingw32 $(MINGW)/lib/libSDLmain.a -lwsock32 -mwindows -Wl,-s
$(INFON_EXECUTABLE) : infon.res

$(SDL_RENDERER)     : CFLAGS   += -I$(SDLDIR)/include/SDL 
$(SDL_RENDERER)     : LDFLAGS  += $(MINGW)/lib/libSGE.a $(MINGW)/lib/libevent.a $(MINGW)/lib/libSDL_image.a \
                                  $(MINGW)/lib/libpng.a $(MINGW)/lib/libz.a     $(MINGW)/lib/libSDL_gfx.a $(MINGW)/lib/libSDL.a \
                                  -lmingw32 -lstdc++ -lwsock32 -lwinmm -mwindows -Wl,-s
else
$(INFON_EXECUTABLE) : LDFLAGS  += -levent -lz -lm 

# Example for embedding a renderer
ifdef NULL_INFON
$(INFON_EXECUTABLE) : CFLAGS   += -DNO_EXTERNAL_RENDERER -DBUILTIN_RENDERER=null_gui
$(INFON_EXECUTABLE) : null_gui.o
else
$(INFON_EXECUTABLE) : LDFLAGS  += -ldl
misc.o              : CFLAGS   += -fPIC
endif

$(INFOND_EXECUTABLE): CFLAGS   += -Ilua-5.1.1/src/ # -DCHEATS
$(INFOND_EXECUTABLE): LDFLAGS  += -levent -lz -lm

$(SDL_RENDERER)     : CFLAGS   += -fPIC -I$(SDLDIR)/include/SDL 
$(SDL_RENDERER)     : LDFLAGS  += -lSDL -lSDL_image -lSGE -lSDL_gfx 

$(GL_RENDERER)      : CFLAGS   += -fPIC
$(GL_RENDERER)      : LDFLAGS  += -lSDL -lGL -lGLU

$(AA_RENDERER)      : CFLAGS   += -fPIC
$(AA_RENDERER)      : LDFLAGS  += -laa

$(LILITH_RENDERER)  : CPPFLAGS += -fPIC
$(LILITH_RENDERER)  : LDFLAGS  += -lSDL -lGL -lGLU -lstdc++ 

$(NULL_RENDERER)    : CFLAGS   += -fPIC
endif

############################################################
# Go Go Go!
############################################################

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

win32-client-dist: $(INFON_EXECUTABLE) $(SDL_RENDERER)
	$(STRIP) $^
	upx -9 --all-methods $(INFON_EXECUTABLE)
	upx -9 --all-methods $(SDL_RENDERER)
	zip infon-win32-r$(REVISION).zip README $^ gfx/*.fnt gfx/*.png

linux-client-dist: $(INFON_EXECUTABLE) $(SDL_RENDERER) $(NULL_RENDERER)
	strip $^
	tar cfvz infon-linux-i386-r$(REVISION).tgz README $^ gfx/*.fnt gfx/*.png

linux-server-dist: $(INFOND_EXECUTABLE)
	tar cfvz infond-linux-i386-r$(REVISION).tgz README $(INFOND_EXECUTABLE) $(INFOND_EXECUTABLE)-static *.lua level/*.lua rules/*.lua

$(INFOND_EXECUTABLE): infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o game.o lua-5.1.1/src/liblua.a  
	$(CC) $^ $(LDFLAGS) -o $@
	$(CC) $^ $(LDFLAGS) -static -o $@-static

$(INFON_EXECUTABLE): infon.o client.o packet.o misc.o client_player.o client_world.o client_creature.o client_game.o renderer.o
	$(CC) $^ $(LDFLAGS) -o $@ 

$(NULL_RENDERER) : null_gui.o
	$(CC) $^ -shared -o $@

$(AA_RENDERER): aa_gui.o
	$(CC) $^ $(LDFLAGS) -shared -o $@

$(SDL_RENDERER): sdl_video.o sdl_sprite.o sdl_gui.o misc.o
	$(CC) $^ $(LDFLAGS) -shared -o $@

$(GL_RENDERER): gl_video.o gl_gui.o gl_mdl.o misc.o
	$(CC) $^ $(LDFLAGS) -shared -o $@

$(LILITH_RENDERER): lilith_gui.o misc.o lilith/lilith/liblilith.a
	$(CC) $^ $(LDFLAGS) -shared -o $@

infon.res: infon.rc
	$(WINDRES) -i $^ -DREVISION="\\\"$(REVISION)\\\"" --input-format=rc -o $@ -O coff

lua-5.1.1/src/liblua.a:
	$(MAKE) -C lua-5.1.1 $(LUAPLAT)

%.o:%.c
	$(CC) $(CFLAGS) $^ -c -o $@

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $^ -c -o $@

clean:
	-rm -f *.o *.so *.dll infond infond-static infon infon.exe infon.res tags 

distclean: clean
	$(MAKE) -C lua-5.1.1 clean
	-rm -f infon*.zip infon*.tgz *.orig *.rej infond-*.demo
