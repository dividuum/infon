# Expects that the editor-server source code folder resides in the contrib directory
# Change it if required
INFONDIR = ../..

ifdef WINDOWS
	EDITOR_EXECUTABLE = editor-server.exe
else
	EDITOR_EXECUTABLE = editor-server
endif

objects = editor-server.o packet.o

$(EDITOR_EXECUTABLE): $(objects)
	cc -o $@ $(objects)

editor-server.o: $(INFONDIR)/packet.h $(INFONDIR)/common_world.h $(INFONDIR)/common_player.h $(INFONDIR)/common_creature.h

packet.o: $(INFONDIR)/packet.c $(INFONDIR)/packet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	-rm -f $(EDITOR_EXECUTABLE) $(objects)
