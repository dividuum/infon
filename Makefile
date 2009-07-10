all: targets

ifndef REVISION
-include REVISION
endif

# EVENT_NAME = Computer Night 2006
# EVENT_HOST = 172.30.100.1

CFLAGS += -pedantic -std=gnu99 -Wall -DREVISION="\"$(REVISION)\""
LUA     = lua-5.1.2

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
	CFLAGS += -O3 -DNDEBUG
else
	CFLAGS += -ggdb
endif

ifdef WINDOWS
	PREFIX           ?= .\\\\
	MINGW             = $(HOME)/progs/mingw32/
	SDLDIR            = $(MINGW)
	CC                = i586-mingw32msvc-gcc
	CFLAGS           += -I$(MINGW)/include
	WINDRES           = i586-mingw32msvc-windres
	STRIP             = i586-mingw32msvc-strip
	LUAPLAT           = mingw
	INFON_EXECUTABLE  = infon.exe
	INFOND_EXECUTABLE = infond.exe

	SDL_RENDERER      = sdl_gui.dll
	GL_RENDERER       = gl_gui.dll
	NULL_RENDERER     = null_gui.dll

	RENDERER          = $(SDL_RENDERER) $(GL_RENDERER)
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

	RENDERER          = $(SDL_RENDERER) $(NULL_RENDERER) $(GL_RENDERER)
endif

CFLAGS += -DPREFIX=\"$(PREFIX)\"

############################################################
# Target specific Configuration
############################################################

ifdef WINDOWS
$(INFON_EXECUTABLE) : LDFLAGS  += $(MINGW)/lib/libevent.a $(MINGW)/lib/libz.a \
                                  -lmingw32 $(MINGW)/lib/libSDLmain.a -lwsock32 -mwindows -Wl,-s
$(INFON_EXECUTABLE) : infon.res

$(INFOND_EXECUTABLE): CFLAGS   += -I$(LUA)/src/ -DNO_CONSOLE_CLIENT
$(INFOND_EXECUTABLE): LDFLAGS  += $(MINGW)/lib/libevent.a $(MINGW)/lib/libz.a \
                                  -lmingw32 -lwsock32 -mconsole -Wl,-s
$(INFOND_EXECUTABLE): infond.res luacore.o

$(SDL_RENDERER)     : CFLAGS   += -I$(SDLDIR)/include/SDL 
$(SDL_RENDERER)     : LDFLAGS  += $(MINGW)/lib/libSGE.a $(MINGW)/lib/libevent.a $(MINGW)/lib/libSDL_image.a \
                                  $(MINGW)/lib/libpng.a $(MINGW)/lib/libz.a     $(MINGW)/lib/libSDL_gfx.a $(MINGW)/lib/libSDL.a \
                                  -lmingw32 -lstdc++ -lwsock32 -lwinmm -mwindows -Wl,-s
$(SDL_RENDERER)     : infon.res

$(GL_RENDERER)      : CFLAGS   += -I$(SDLDIR)/include/SDL 
$(GL_RENDERER)      : LDFLAGS  += $(MINGW)/lib/libSGE.a $(MINGW)/lib/libevent.a \
                                  $(MINGW)/lib/libz.a   $(MINGW)/lib/libSDL.a \
                                  -lmingw32 -lopengl32 -lglu32 -lstdc++ -lwsock32 -lwinmm -mwindows -Wl,-s
$(GL_RENDERER)      : infon.res
else
$(INFON_EXECUTABLE) : LDFLAGS  += -levent -lz -lm 

# Example for embedding a renderer
ifdef NULL_INFON
$(INFON_EXECUTABLE) : CFLAGS   += -DNO_EXTERNAL_RENDERER -DBUILTIN_RENDERER=null_gui
$(INFON_EXECUTABLE) : null_gui.o
else
ifdef SDL_INFON
$(INFON_EXECUTABLE) : CFLAGS   += -DNO_EXTERNAL_RENDERER -DBUILTIN_RENDERER=sdl_gui -I$(SDLDIR)/include/SDL 
$(INFON_EXECUTABLE) : LDFLAGS  += -lSDL -lSDL_image -lSGE -lSDL_gfx 
$(INFON_EXECUTABLE) : sdl_video.o sdl_sprite.o sdl_gui.o
else
$(INFON_EXECUTABLE) : LDFLAGS  += -ldl
misc.o              : CFLAGS   += -fPIC
endif
endif

$(INFOND_EXECUTABLE): CFLAGS   += -I$(LUA)/src/ # -DCHEATS
$(INFOND_EXECUTABLE): LDFLAGS  += -levent -lz -lm

# Experimental usage of 'all of lua in one file' as seen in lua-5.1.2/etc/all.c
ifdef OPTIMIZE
$(INFOND_EXECUTABLE): luacore.o
luacore.o           : CFLAGS   += -DLUA_USE_POSIX
else
$(INFOND_EXECUTABLE): $(LUA)/src/liblua.a
endif

$(SDL_RENDERER)     : CFLAGS   += -fPIC -I$(SDLDIR)/include/SDL 
$(SDL_RENDERER)     : LDFLAGS  += -lSDL -lSDL_image -lSGE -lSDL_gfx 

$(GL_RENDERER)      : CFLAGS   += -fPIC -I$(SDLDIR)/include/SDL 
$(GL_RENDERER)      : LDFLAGS  += -lSDL -lGL -lGLU

$(AA_RENDERER)      : CFLAGS   += -fPIC
$(AA_RENDERER)      : LDFLAGS  += -laa

$(LILITH_RENDERER)  : CPPFLAGS += -fPIC
$(LILITH_RENDERER)  : LDFLAGS  += -lSDL -lGL -lGLU -lstdc++ 

$(NULL_RENDERER)    : CFLAGS   += -fPIC
endif

gl_gui.o			: CFLAGS   += -Wno-unused

ifdef DEFAULT_RENDERER
$(INFON_EXECUTABLE) : CFLAGS   += -DDEFAULT_RENDERER=$(DEFAULT_RENDERER)
endif

############################################################
# Go Go Go!
############################################################

targets: infond $(INFON_EXECUTABLE) $(RENDERER)

dist:
	$(MAKE) distclean
	$(MAKE) source-dist
	$(MAKE) clean
	WINDOWS=1  $(MAKE) win32-client-dist
	$(MAKE) clean
	WINDOWS=1  $(MAKE) win32-server-dist
	$(MAKE) clean
	OPTIMIZE=1 $(MAKE) linux-client-dist
	$(MAKE) clean
	$(MAKE) linux-server-dist

source-dist: REVISION creature_config.h
	tar cvzh -C.. --exclude ".svn" --exclude "infon-source*" --file infon-source-r$(REVISION).tgz infon

win32-client-dist: $(INFON_EXECUTABLE) $(SDL_RENDERER) $(GL_RENDERER)
	$(STRIP) $^
	upx -9 --all-methods $(INFON_EXECUTABLE)
	upx -9 --all-methods $(SDL_RENDERER)
	upx -9 --all-methods $(GL_RENDERER)
	zip infon-win32-r$(REVISION).zip \
		README.txt $^ gfx/*.fnt gfx/*.png gfx/*.bmp gfx/*.mdl \
		contrib/bots/*.lua contrib/bots/*.txt

win32-server-dist: $(INFOND_EXECUTABLE)
	$(STRIP) $^
	upx -9 --all-methods $(INFOND_EXECUTABLE)
	zip infond-win32-r$(REVISION).zip \
		README.txt $(INFOND_EXECUTABLE) \
		*.lua level/*.lua rules/*.lua api/*.lua libs/*.lua \
		contrib/bots/*.lua contrib/bots/*.txt

linux-client-dist: $(INFON_EXECUTABLE) $(SDL_RENDERER) $(NULL_RENDERER) $(GL_RENDERER)
	strip $^
	tar cfvz infon-linux-i386-r$(REVISION).tgz \
		README.txt $^ gfx/*.fnt gfx/*.png gfx/*.bmp gfx/*.mdl \
		contrib/bots/*.lua contrib/bots/*.txt

linux-server-dist: $(INFOND_EXECUTABLE) infond-wrapper
	tar cfvz infond-linux-i386-r$(REVISION).tgz \
		README.txt $(INFOND_EXECUTABLE) infond-wrapper $(INFOND_EXECUTABLE)-static \
		*.lua level/*.lua rules/*.lua api/*.lua libs/*.lua \
		contrib/bots/*.lua contrib/bots/*.txt

$(INFOND_EXECUTABLE): infond.o server.o listener.o map.o path.o misc.o packet.o player.o world.o creature.o scroller.o game.o pinger.o
	$(CC) $^ $(LDFLAGS) -o $@
	$(CC) $^ $(LDFLAGS) -lrt -static -o $@-static

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

creature.c: creature_config.h

creature_config.h: creature_config.gperf
	gperf --output-file=$@ -c -C -t  $^ 

%.res: %.rc
	$(WINDRES) -i $^ -DREVISION="\\\"$(REVISION)\\\"" --input-format=rc -o $@ -O coff

$(LUA)/src/liblua.a:
	$(MAKE) -C $(LUA) $(LUAPLAT)

%.o:%.c
	$(CC) $(CFLAGS) $^ -c -o $@

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $^ -c -o $@

REVISION:
	echo "REVISION=`svnversion .`" > $@

clean: 
	-rm -f *.o *.so *.dll infond infond-static infon infon.exe infon*.res infond.exe infond.exe-static tags

distclean: clean
	$(MAKE) -C $(LUA) clean 
	-rm -f infon*.zip infon*.tgz *.orig *.rej infond-*.demo infond-wrapper REVISION creature_config.h
