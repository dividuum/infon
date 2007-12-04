-- Mapname: pacman 
-- code based on: castle
-- Author:  queaker, plaisthos
-- Version: 0.1

function maplayout()
       tile = {}
       tile["@"] = TILE_GFX_SOLID;
       tile[" "] = TILE_GFX_PLAIN;
       tile["."] = TILE_GFX_PLAIN;
       tile["B"] = TILE_GFX_BORDER;
       tile["T"] = TILE_GFX_SNOW_SOLID;
       tile["U"] = TILE_GFX_SNOW_PLAIN;
       tile["V"] = TILE_GFX_SNOW_BORDER;
       tile["W"] = TILE_GFX_WATER;
       tile["L"] = TILE_GFX_LAVA;
       tile["N"] = TILE_GFX_NONE;
       tile["K"] = TILE_GFX_KOTH;
       tile["D"] = TILE_GFX_DESERT;

       m = {}
       m[ 1] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
       m[ 2] = "@ . . . . . . . . @@@ . . . . . . . . @"
       m[ 3] = "@                 @@@                 @"
       m[ 4] = "@ .@@@@@. @@@@@@. @@@ .@@@@@@ .@@@@@. @"
       m[ 5] = "@  @@@@@  @@@@@@  @@@  @@@@@@  @@@@@  @"
       m[ 6] = "@ .@@@@@. @@@@@@. @@@ .@@@@@@ .@@@@@. @"
       m[ 7] = "@  @@@@@  @@@@@@  @@@  @@@@@@  @@@@@  @"
       m[ 8] = "@ . . . . . . . . . . . . . . . . . . @"
       m[ 9] = "@                                     @"
       m[10] = "@ .@@@@@  @@. @@@@@@@@@@@ .@@ .@@@@@. @"
       m[11] = "@  @@@@@  @@  @@@@@@@@@@@  @@  @@@@@  @"
       m[12] = "@ . . . . @@. . . @@@ . . .@@ . . . . @"
       m[13] = "@         @@      @@@      @@         @"
       m[14] = "@@@@@@@@. @@@@@@ .@@@ .@@@@@@ .@@@@@@@@"
       m[15] = "@@@@@@@@  @@@@@@  @@@  @@@@@@  @@@@@@@@"
       m[16] = "@@@@@@@@. @@               @@ .@@@@@@@@"
       m[17] = "@@@@@@@@  @@               @@  @@@@@@@@"
       m[18] = "@@@@@@@@. @@  @@@@DDD@@@@  @@ .@@@@@@@@"
       m[19] = "@@@@@@@@  @@  @DDDDDDDDD@  @@  @@@@@@@@"
       m[20] = "              @DDDDDDDDD@              "
       m[21] = "        .     @DDDDKDDDD@     .        "
       m[22] = "              @DDDDDDDDD@              "
       m[23] = "@@@@@@@@  @@  @DDDDDDDDD@  @@  @@@@@@@@"
       m[24] = "@@@@@@@@. @@  @@@@@@@@@@@  @@ .@@@@@@@@"
       m[25] = "@@@@@@@@  @@               @@  @@@@@@@@"
       m[26] = "@@@@@@@@. @@               @@ .@@@@@@@@"
       m[27] = "@@@@@@@@  @@  @@@@@@@@@@@  @@  @@@@@@@@"
       m[28] = "@@@@@@@@. @@. @@@@@@@@@@@ .@@ .@@@@@@@@"
       m[29] = "@                 @@@                 @"
       m[30] = "@ . . . . . . . . @@@ . . . . . . . . @"
       m[31] = "@  @@@@@  @@@@@@  @@@  @@@@@@  @@@@@  @"
       m[32] = "@ .@@@@@. @@@@@@. @@@ .@@@@@@ .@@@@@. @"
       m[33] = "@     @@                       @@     @"
       m[34] = "@ . . @@ . . . . . .  . . . . .@@ . . @"
       m[35] = "@@@@  @@  @@  @@@@@@@@@@@  @@  @@  @@@@"
       m[36] = "@@@@. @@ .@@ .@@@@@@@@@@@ .@@ .@@ .@@@@"
       m[37] = "@         @@      @@@      @@         @"
       m[38] = "@ . . . . @@ .  . @@@ . . .@@ . . . . @"
       m[39] = "@  @@@@@@@@@@@@@  @@@  @@@@@@@@@@@@@  @"
       m[40] = "@ .@@@@@@@@@@@@@. @@@ .@@@@@@@@@@@@@. @"
       m[41] = "@                                     @"
       m[42] = "@ . . . . . . . . . . . . . . . . . . @"
       m[43] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
end

function level_size()
    local mapsizeX = 1
    local mapsizeY = 1
    maplayout()
    arraySize = table.getn(m);
    for i=1, arraySize, 1 do
        if string.len(m[i]) > mapsizeX then
            mapsizeX = string.len(m[i])
        end
    end
    mapsizeY = arraySize
    return mapsizeX+2, mapsizeY+2
end

function level_koth_pos()
    local kothX = 1
    local kothY = 1
    maplayout()
    arraySize = table.getn(m);
    for i=1, arraySize, 1 do
        for j=1, string.len(m[i]), 1 do
            k = string.upper(string.sub(m[i],j,j))
            if k == "K" then
                kothX = j
                kothY = i
            end
        end
    end
    return kothX, kothY
end

function level_init()
    maplayout()
    arraySize = table.getn(m);
    for i=1, arraySize, 1 do
        for j=1, string.len(m[i]), 1 do
            k = string.upper(string.sub(m[i],j,j))
            if k == " " or k=="." or k == "U" or k == "D" or k == "K" then
                world_set_type(j,i, TILE_PLAIN)
            end
            world_set_gfx(j,i, tile[k])
        end
    end
    world_make_border(TILE_GFX_WATER)
    last_food = game_time()-10000
end

-- wird, wie der Name schon sagt, jede Runde ausgefuehrt
function level_tick()
    if game_time() > last_food + 10000 then
        arraySize = table.getn(m)
        for i=1, arraySize, 1 do
            for j=1, string.len(m[i]), 1 do
                if string.sub(m[i],j,j)=="." then
                    --              print (i .. " " .. j )
                    world_add_food(j, i, 100)
                end
            end
        end
        last_food = game_time()
    end
end

