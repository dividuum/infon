/*

    Copyright (c) 2007 Moritz Kroll <Moritz.Kroll@gmx.de>. All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#ifdef _WIN32
    #include <winsock.h>
    #include <io.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include "../../common_world.h"
#include "../../common_player.h"
#include "../../common_creature.h"
#include "../../packet.h"


#ifdef _WIN32
    #define SOCKTYPE SOCKET
    #define CLOSESOCKET(a) closesocket(a)
#else
    #define SOCKTYPE int
    #define CLOSESOCKET(a) close(a)
#endif

#define VERSIONSTR "Infon Editor-Server v0.1 (2007-06-10)"

#define PORT 1234                           // Portnumber
#define RCVBUFSIZE 1024                     // Receive buffer


#define ABS(x) (((x) < 0) ? (-(x)) : (x))
#define LEVEL(x, y) level[(y) * levelwidth + (x)]

int levelwidth = 40, levelheight = 30;
unsigned char *level = NULL;
char *outputname = "testmap.lua";
int kothx = -1, kothy = -1;

SOCKTYPE client;
packet_t packet;

typedef struct
{
    int last_x, last_y;
} editCreature_t;

enum
{
    EDITCREATURE_PEN,
    EDITCREATURE_SOLID,
    EDITCREATURE_WALKABLE,
    EDITCREATURE_FOOD1,
    EDITCREATURE_FOOD2,
    MAX_EDITCREATURES
};

editCreature_t creatures[MAX_EDITCREATURES];

#define NUM_SOLIDPENS 7
int solidPens[NUM_SOLIDPENS] = { TILE_GFX_NONE, TILE_GFX_SOLID, TILE_GFX_BORDER,
    TILE_GFX_SNOW_SOLID, TILE_GFX_SNOW_BORDER, TILE_GFX_WATER, TILE_GFX_LAVA };

#define NUM_WALKABLEPENS 4
int walkablePens[NUM_WALKABLEPENS] = { TILE_GFX_PLAIN, TILE_GFX_SNOW_PLAIN,
    TILE_GFX_DESERT, TILE_GFX_KOTH };

#define NUM_FOODPENS 11

#define SOLID_STARTY 1
#define SOLID_ENDY (SOLID_STARTY + ((NUM_SOLIDPENS - 1) >> 1))
#define WALKABLE_STARTY 7
#define WALKABLE_ENDY (WALKABLE_STARTY + ((NUM_WALKABLEPENS - 1) >> 1))
#define FOOD_STARTY 11
#define FOOD_ENDY (FOOD_STARTY + ((NUM_FOODPENS - 1) >> 1))

typedef enum
{
    PENMODE_TILE,
    PENMODE_FOOD,
    PENMODE_FILLTILE
} penmode_t;

penmode_t penMode = PENMODE_TILE;
int penVal = TILE_GFX_PLAIN;
int lastPenX = -1, lastPenY = -1;

int fillSrcTile;

char tilenames[16] = { 'S', 'P', 'B', 'T', 'U', 'V', 'W', 'L',
                       'N', 'K', 'D', '0', '1', '2', '3', '4' };

// Reports an error and exits
void error_exit(SOCKTYPE sock, char *error_message) {
#ifdef _WIN32
    fprintf(stderr,"%s: %d\n", error_message, WSAGetLastError());
#else
    fprintf(stderr, "%s: %s\n", error_message, strerror(errno));
#endif

    if(sock)
        CLOSESOCKET(sock);
    if(level != NULL)
        free(level);
    exit(EXIT_FAILURE);
}

SOCKTYPE initConnection()
{
    struct sockaddr_in server, client;
    SOCKTYPE sock, fd;
    int len;

#ifdef _WIN32
    // Initialize TCP for Windows ("winsock")
    WORD wVersionRequested = MAKEWORD (1, 1);
    WSADATA wsaData;
    if (WSAStartup (wVersionRequested, &wsaData) != 0)
        error_exit(0, "Error initializing Winsock");
    else
        printf("Winsock initialized\n");
#endif

    // Create socket
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        error_exit(0, "Cannot create socket!");

    // Create socket address for the server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // Create binding to the server address
    if(bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
        error_exit(0, "Cannot bind created socket!");

    // Tell the socket that we want to accept one connection from a client
    if(listen(sock, 1) == -1)
        error_exit(0, "listen() returned an error!");

    printf("Server ready - waiting for connection ...\n");
    len = sizeof(client);
    fd = accept(sock, (struct sockaddr *) &client, &len);
    if (fd < 0)
        error_exit(0, "accept() returned an error!");
    printf("Accepted connection from: %s\n", inet_ntoa(client.sin_addr));

    return fd;
}

// Parses an decimal integer at the string position specified by ptr
// and moves ptr to the position after the last parsed character
int parseInt(char **ptr)
{
    return strtol(*ptr, ptr, 10);
}

void usage()
{
    printf(VERSIONSTR "\n"
        "Usage: editor-server [options]\n"
        "Options:\n"
        "  -w <width>        Sets the width of the map (default: 40)\n"
        "  -h <height>       Sets the height of the map (default: 30)\n"
        "  -o <outputfile>   Sets the name of the output file (default: \"testmap.lua\")\n");
}

int parseCmdLine(int argc, char **argv)
{
    int i;
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] != '-' || argv[i][1] == 0 || argv[i][2] != 0)
        {
            printf("Illegal option: \"%s\"\n", argv[i]);
            usage();
            return 0;
        }
        switch(argv[i][1])
        {
            case 'w':
                i++;
                if(i >= argc)
                {
                    printf("No width specified after \"-w\"!");
                    usage();
                    return 0;
                }
                levelwidth = atoi(argv[i]);
                if(levelwidth <= 0 || levelwidth > 256)
                {
                    printf("Illegal width specified! Must be >0 and <= 256!");
                    return 0;
                }
                break;

            case 'h':
                i++;
                if(i >= argc)
                {
                    printf("No height specified after \"-h\"!");
                    usage();
                    return 0;
                }
                levelheight = atoi(argv[i]);
                if(levelheight <= 0 || levelheight > 256)
                {
                    printf("Illegal height specified! Must be >0 and <= 256!");
                    return 0;
                }
                break;

            case 'o':
                i++;
                if(i >= argc)
                {
                    printf("No output file specified after \"-o\"!");
                    usage();
                    return 0;
                }
                outputname = argv[i];
                break;

            default:
                printf("Illegal option: '%c'\n", argv[i][1]);
                usage();
                return 0;
        }
    }

    printf("Level size: %ix%i\nOutput file name: %s\n", levelwidth, levelheight,
        outputname);
    return 1;
}

// Copies the contents of the file specified by filename into destfile
int copyFileInto(const char *filename, FILE *destfile)
{
    FILE *srcfile = fopen(filename, "rb");
    if(!srcfile)
    {
        printf("Unable to open file \"%s\"!", filename);
        return 0;
    }

    char buf[1024];
    int readsize;
    while((readsize = fread(buf, 1, 1024, srcfile)) > 0)
        fwrite(buf, 1, readsize, destfile);

    fclose(srcfile);
    return 1;
}

// Writes the current map into a lua file specified by outputname
int writeLevel()
{
    int x, y;
    FILE *file = fopen(outputname, "wb");
    if(!file)
    {
        printf("Unable to create file \"testmap.lua\"!");
        return 0;
    }

    if(!copyFileInto("front.tmpl", file)) return 0;

    for(y = 0; y < levelheight; y++)
    {
        fprintf(file, "       m[%2d] = \"", y + 1);
        for(x = 0; x < levelwidth; x++)
            fputc(tilenames[LEVEL(x, y) & 0x0f], file);
        fprintf(file, "\";\n");
    }

    if(!copyFileInto("middle.tmpl", file)) return 0;

    int numspawners = 0;
    for(y = 0; y < levelheight; y++)
    {
        for(x = 0; x < levelwidth; x++)
        {
            int food = LEVEL(x,y) >> 4;
            if(food)
            {
                fprintf(file,
                    "       food_spawner[%3i] = { x = %i,\n"
                    "                             y = %i,\n"
                    "                             rx = 10,\n"
                    "                             ry = 10,\n"
                    "                             a = %i,\n"
                    "                             i = 200,\n"
                    "                             n = game_time() }\n",
                    numspawners++, x, y, food * 10);
            }
        }
    }

    if(!copyFileInto("end.tmpl", file)) return 0;

    fclose(file);
    return 1;
}

void sendPacket()
{
    packet.len = packet.offset;
    send(client, (void *) &packet, PACKET_HEADER_SIZE + packet.len, 0);
}

void sendWorldUpdate(int x, int y, int food, int gfx)
{
    char field[] = { 4, 1, x, y, food, gfx };
    send(client, field, sizeof(field), 0);
}

void sendScrollMessage(const char *msg)
{
    packet_init(&packet, PACKET_SCROLLER_MSG);
    packet_writeXX(&packet, msg, strlen(msg));

    sendPacket();
}

void initPlayer(int playerID, const char *name, int color)
{
    packet_init(&packet, PACKET_PLAYER_UPDATE);

    packet_write08(&packet, playerID);
    packet_write08(&packet, PLAYER_DIRTY_ALIVE | (name ? PLAYER_DIRTY_NAME : 0)
        | PLAYER_DIRTY_COLOR | PLAYER_DIRTY_SCORE);

    // ALIVE part
    packet_write08(&packet, 1);

    if(name != NULL)
    {
        // NAME part
        int len = strlen(name);
        packet_write08(&packet, len);
        packet_writeXX(&packet, name, len);
    }

    // COLOR part
    packet_write08(&packet, color);

    // SCORE part
    packet_write16(&packet, 500 + playerID);

    sendPacket();
}

void sendPlayerName(int playerID, const char *name, int highlight)
{
    packet_init(&packet, PACKET_PLAYER_UPDATE);

    packet_write08(&packet, playerID);
    packet_write08(&packet, PLAYER_DIRTY_NAME | PLAYER_DIRTY_CPU);

    // NAME part
    int len = strlen(name);
    packet_write08(&packet, len);
    packet_writeXX(&packet, name, len);

    // CPU part
    packet_write08(&packet, highlight ? 100 : 0);

    sendPacket();
}

void printWithCreature(int worldx, int worldy, int playerID,
                       unsigned int creatureID, const char *msg)
{
    assert(creatureID < MAX_EDITCREATURES);
    editCreature_t *creature = creatures + creatureID;

    // If creature is already used, kill it

    if(creature->last_x != -1)
    {
        packet_init(&packet, PACKET_CREATURE_UPDATE);
        packet_write16(&packet, creatureID);
        packet_write08(&packet, CREATURE_DIRTY_ALIVE);
        packet_write08(&packet, 0xff);
        packet.len = packet.offset;
        send(client, (void *) &packet, PACKET_HEADER_SIZE + packet.len, 0);
    }

    packet_init(&packet, PACKET_CREATURE_UPDATE);
    packet_write16(&packet, creatureID);
    packet_write08(&packet, CREATURE_DIRTY_ALIVE | CREATURE_DIRTY_MESSAGE);

    // ALIVE part

    int newx = worldx / CREATURE_POS_RESOLUTION;
    int newy = worldy / CREATURE_POS_RESOLUTION;
    packet_write08(&packet, playerID);
    packet_write16(&packet, creatureID);
    packet_write16(&packet, newx);
    packet_write16(&packet, newy);
    creature->last_x = newx;
    creature->last_y = newy;

    // MESSAGE part

    int msglen = strlen(msg);
    packet_write08(&packet, msglen);
    packet_writeXX(&packet, msg, msglen);

    sendPacket();
}

// Clears the whole map (not the control panel) with TILE_GFX_SOLID
void clearMap()
{
    int x, y;
    memset(level, 0, levelwidth * levelheight * sizeof(char));
    for(y = 0; y < levelheight; y++)
        for(x = 0; x < levelwidth; x++)
            sendWorldUpdate(x + 4, y, 0, TILE_GFX_SOLID);
}

void initGuiClient()
{
    int i, j;

    // Initialize guiclient
    char header[] = { 4, 6, 4 + levelwidth, levelheight, 5, 5 };
    if(FOOD_STARTY + 5 >= levelheight) header[3] = FOOD_STARTY + 6;
    send(client, "                                  ", 34, 0);
    send(client, header, sizeof(header), 0);

    sendScrollMessage("Please press F11 to enable editing!");

    //
    // Build control panel on the left side of the map
    //

    // Clear three columns on the left
    for(i = 0; i < levelheight; i++)
        for(j = 0; j < 4; j++)
            sendWorldUpdate(j, i, 0, TILE_GFX_NONE);

    // Place solid tile selection fields
    for(i = 0; i < NUM_SOLIDPENS; i++)
        sendWorldUpdate(1 + (i & 1), SOLID_STARTY + (i >> 1), 0, solidPens[i]);

    // Place walkable tile selection fields
    for(i = 0; i < NUM_WALKABLEPENS; i++)
        sendWorldUpdate(1 + (i & 1), WALKABLE_STARTY + (i >> 1), 0, walkablePens[i]);


    // Place food pen selection fields
    for(i = 0; i < 11; i++)
        sendWorldUpdate(1 + (i & 1), FOOD_STARTY + (i >> 1), i, TILE_GFX_NONE);

    // Init two dummy players for creatures
    initPlayer(0, "Tile mode", 0x77);
    initPlayer(1, NULL, 0x88);

    printWithCreature(512, (SOLID_STARTY - 1) * 256, 0, EDITCREATURE_SOLID, "Solid");
    printWithCreature(512, (WALKABLE_STARTY - 1) * 256, 0, EDITCREATURE_WALKABLE, "Walkable");
    printWithCreature(256, (FOOD_STARTY - 1) * 256, 0, EDITCREATURE_FOOD1, "Food");
    printWithCreature(768, (FOOD_STARTY - 1) * 256, 0, EDITCREATURE_FOOD2, "Amount");
    printWithCreature(128, WALKABLE_STARTY * 256 + 128, 1, EDITCREATURE_PEN, "Pen");
}

// 0 < x < 3 holds
void handleControlPanel(int x, int y)
{
    x--;            // => x == 0 || x == 1

    if(y >= SOLID_STARTY && y <= SOLID_ENDY)
    {
        int newval = x + (y - SOLID_STARTY) * 2;
        if(newval < NUM_SOLIDPENS)
        {
            if(penMode == PENMODE_FOOD)
            {
                sendPlayerName(0, "Tile mode", 0);
                penMode = PENMODE_TILE;
            }
            penVal = solidPens[newval];

            printWithCreature(x * 3 * 256 + 128, y * 256 + 128, 1, EDITCREATURE_PEN, "Pen");
        }
    }
    if(y >= WALKABLE_STARTY && y <= WALKABLE_ENDY)
    {
        int newval = x + (y - WALKABLE_STARTY) * 2;
        if(newval < NUM_WALKABLEPENS)
        {
            if(penMode == PENMODE_FOOD)
            {
                sendPlayerName(0, "Tile mode", 0);
                penMode = PENMODE_TILE;
            }
            penVal = walkablePens[newval];

            printWithCreature(x * 3 * 256 + 128, y * 256 + 128, 1, EDITCREATURE_PEN, "Pen");
        }
    }
    else if(y >= FOOD_STARTY && y <= FOOD_ENDY)
    {
        int newfood = x + (y - FOOD_STARTY) * 2;
        if(newfood <= 10)
        {
            if(penMode != PENMODE_FOOD)
            {
                sendPlayerName(0, "Food mode", 0);
                penMode = PENMODE_FOOD;
            }
            penVal = newfood;

            printWithCreature(x * 3 * 256 + 128, y * 256 + 128, 1, EDITCREATURE_PEN, "Pen");
        }
    }
}

void drawOnMap(int mapx, int mapy)
{
    int curTile, curFood;
    if(penMode == PENMODE_FOOD)
    {
        curTile = LEVEL(mapx, mapy) & 0x0f;
        curFood = penVal;
    }
    else
    {
        curTile = penVal;
        curFood = LEVEL(mapx, mapy) >> 4;
    }
    sendWorldUpdate(mapx + 4, mapy, curFood, curTile);
    LEVEL(mapx, mapy) = curTile | (curFood << 4);
}

void recursiveFloodFill(int mapx, int mapy)
{
    int lx = mapx, rx = mapx + 1;
    for(; lx >= 0; lx--)
    {
        if((LEVEL(lx, mapy) & 0x0f) != fillSrcTile) break;
        drawOnMap(lx, mapy);
    }
    for(; rx < levelwidth; rx++)
    {
        if((LEVEL(rx, mapy) & 0x0f) != fillSrcTile) break;
        drawOnMap(rx, mapy);
    }
    rx--;

    int x;
    if(mapy > 0)
    {
        for(x = rx; x > lx; x--)
        {
            if((LEVEL(x, mapy - 1) & 0x0f) == fillSrcTile)
                recursiveFloodFill(x, mapy - 1);
        }
    }
    if(mapy < levelheight - 1)
    {
        for(x = rx; x > lx; x--)
        {
            if((LEVEL(x, mapy + 1) & 0x0f) == fillSrcTile)
                recursiveFloodFill(x, mapy + 1);
        }
    }
}

void floodFill(int mapx, int mapy)
{
    fillSrcTile = LEVEL(mapx, mapy) & 0x0f;
    if(fillSrcTile != penVal)
    {
        recursiveFloodFill(mapx, mapy);

        if((LEVEL(kothx, kothy) & 0x0f) != TILE_GFX_KOTH)   // has koth been removed?
        {
            kothx = -1;
            kothy = -1;
        }
    }
}

void handleDrawOnMap(int x, int y, int dragged)
{
    int i, j;

    if(penMode == PENMODE_FILLTILE)
    {
        floodFill(x - 4, y);
    }
    else if(dragged && (penMode != PENMODE_TILE || penVal != TILE_GFX_KOTH) && lastPenX != - 1)
    {
        int dx = x - lastPenX;
        int dy = y - lastPenY;

        int adx = ABS(dx), ady = ABS(dy);


        if(adx > ady)       // implies adx > 0
        {
            int i;
            for(i = 0; i <= adx; i++)
            {
                int curx = lastPenX + i * dx / adx;
                int cury = lastPenY + i * dy / adx;
                drawOnMap(curx - 4, cury);
            }
        }
        else if(ady > 0)
        {
            int i;
            for(i = 0; i <= ady; i++)
            {
                int curx = lastPenX + i * dx / ady;
                int cury = lastPenY + i * dy / ady;
                drawOnMap(curx - 4, cury);
            }
        }
        else                // adx == 0 && ady == 0
        {
            drawOnMap(x - 4, y);
        }
    }
    else
    {
        drawOnMap(x - 4, y);

        // set/reset koth?
        if(penMode == PENMODE_TILE && penVal == TILE_GFX_KOTH)
        {
            if(kothx == x - 4 && kothy == y) return;
            if(kothx >= 0)              // koth already set?
            {
                // Replace old koth by the most often surrounding tile

                int surTileCount[16];
                unsigned char replaceVal = 0;
                int bestCount = 0;

                memset(surTileCount, 0, 16);

                for(i = -1; i <= 1; i++)
                {
                    if(kothy + i < 0 || kothy + i >= levelheight) continue;

                    for(j = -1; j <= 1; j++)
                    {
                        if(kothx + j < 0 || kothx + j >= levelwidth
                            || i == 0 && j == 0) continue;
                        int curtile = LEVEL(kothx + j, kothy + i) & 0x0f;
                        if(curtile == TILE_GFX_KOTH)
                            continue;
                        surTileCount[curtile]++;
                        if(surTileCount[curtile] > bestCount)
                        {
                            bestCount = surTileCount[curtile];
                            replaceVal = curtile;
                        }
                    }
                }

                sendWorldUpdate(kothx + 4, kothy, 0, replaceVal);
                LEVEL(kothx, kothy) = replaceVal;
            }
            kothx = x - 4;
            kothy = y;
        }
        else if(kothx == x - 4 && kothy == y)        // koth has been removed
        {
            kothx = -1;
            kothy = -1;
        }
    }
    lastPenX = x;
    lastPenY = y;
}

int main(int argc, char **argv)
{
    if(!parseCmdLine(argc, argv)) return 1;

    client = initConnection();

    memset(creatures, -1, sizeof(creatures));
    initGuiClient();

    level = (char *) malloc(levelwidth * levelheight * sizeof(char));
    memset(level, 0, levelwidth * levelheight * sizeof(char));

    char buffer[RCVBUFSIZE + 1];
    int awaitingClearConfirm = 0;
    while(1)
    {
        int recv_size;
        if((recv_size = recv(client, buffer, RCVBUFSIZE, 0)) <= 0)
        {
            if(recv_size == 0)
            {
                printf("Client has closed the connection!\n");
                break;
            }
            error_exit(client, "recv() returned an error!");
        }
        buffer[recv_size] = '\0';

        char *ptr = buffer;

        // Left or right mouse button pressed down, or right mouse button dragged?
        if((buffer[0] == 'M' && (buffer[1] == '1' || buffer[1] == '3')
            || buffer[0] == 'D' && buffer[1] == '4') && buffer[2] == ',')
        {
            ptr += 3;
            int x = parseInt(&ptr) / 256;
            ptr++;
            int y = parseInt(&ptr) / 256;

            if(y >= 0 && y < levelheight)
            {
                if(buffer[0] == 'M' && x > 0 && x < 3)  // no dragging over control panel
                {
                    handleControlPanel(x, y);
                }
                else if((buffer[1] == '3' || buffer[1] == '4') &&
                    x >= 4 && x < levelwidth + 4)
                {
                    handleDrawOnMap(x, y, buffer[0] == 'D');
                }
            }
        }
        // Key pressed?
        else if(buffer[0] == 'K')
        {
            ptr++;
            int input = parseInt(&ptr);

            if(awaitingClearConfirm)
            {
                if(input == 'y')
                {
                    clearMap();
                    sendScrollMessage("Map cleared!");
                }
                else
                {
                    sendScrollMessage("Clearing map cancelled!");
                }
                awaitingClearConfirm = 0;
            }
            else
            {
                switch(input)
                {
                    case 'c':
                        sendScrollMessage("Please confirm clearing the map with 'y'!");
                        awaitingClearConfirm = 1;
                        break;

                    case 's':
                        if(writeLevel())
                            sendScrollMessage("Level saved successfully!");
                        break;

                    case 'f':
                        if(penMode == PENMODE_FILLTILE)
                        {
                            penMode = PENMODE_TILE;
                            sendPlayerName(0, "Tile mode", 0);
                        }
                        else
                        {
                            if(penMode == PENMODE_FOOD)
                            {
                                sendScrollMessage("Please select a tile first!");
                                break;
                            }
                            penMode = PENMODE_FILLTILE;
                            sendPlayerName(0, "Fill mode", 1);
                        }
                        break;

                    // Solid tile selection keys

                    case 'r':
                        handleControlPanel(1, SOLID_STARTY);
                        break;
                    case 't':
                        handleControlPanel(2, SOLID_STARTY);
                        break;
                    case 'y':
                        handleControlPanel(1, SOLID_STARTY + 1);
                        break;
                    case 'u':
                        handleControlPanel(2, SOLID_STARTY + 1);
                        break;
                    case 'i':
                        handleControlPanel(1, SOLID_STARTY + 2);
                        break;
                    case 'o':
                        handleControlPanel(2, SOLID_STARTY + 2);
                        break;
                    case 'p':
                        handleControlPanel(1, SOLID_STARTY + 3);
                        break;

                    // Walkable tile selection keys

                    case 'g':
                        handleControlPanel(1, WALKABLE_STARTY);
                        break;
                    case 'h':
                        handleControlPanel(2, WALKABLE_STARTY);
                        break;
                    case 'j':
                        handleControlPanel(1, WALKABLE_STARTY + 1);
                        break;
                    case 'k':
                        handleControlPanel(2, WALKABLE_STARTY + 1);
                        break;

                    // Input forwarding enabled key (F11)

                    case 292:
                        sendScrollMessage("Keys: 's'=save, 'c'=clear map, "
                            "'f'=toggle fill, 'r'<->'p'=solid tiles, "
                            "'g'<->'k'=walkable tiles");
                        break;
                }
            }
        }
    }

    free(level);

    /* Schlieﬂe die Verbindung */
    CLOSESOCKET(client);

    return EXIT_SUCCESS;
}
