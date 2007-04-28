-- Mapname: castle
-- Author:  g², copied lot of code from Dunedan
-- Version: 0.1

function maplayout()

       tile = {}
       tile["S"] = TILE_GFX_SOLID;
       tile["P"] = TILE_GFX_PLAIN;
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
       m[ 1] = "SSSSSSWWWWWWWWWWWWWWWWWWWWWWWSSSSSWWWWWWWWWWWWWWWWWWWWWWWSSSSSS";
       m[ 2] = "SPPPPSWSSSSSWSSSSSWSSSSSWSSSSSPPPSSSSSWSSSSSWSSSSSWSSSSSWSPPPPS";
       m[ 3] = "SPPPPSSSPPPSSSPPPSSSPPPSSSPPPPPPPPPPPSSSPPPSSSPPPSSSPPPSSSPPPPS";
       m[ 4] = "SPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPS";
       m[ 5] = "SPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPS";
       m[ 6] = "SSSPPSSPPPPPPPPPPPPPPPPPPPPSSPPPPPSSPPPPPPPPPPPPPPPPPPPPSSPPSSS";
       m[ 7] = "WWSPPSSSSPPPPSPPSSSPPPPPPSSSSSPPPSSSSSPPPPPPSSSPPSPPPPSSSSPPSWW";
       m[ 8] = "WSSPPPSSSSSPPSSPPSSSSPPSSSSSSSPPPSSSSSSSPPSSSSPPSSPPSSSSSPPPSSW";
       m[ 9] = "WSPPPPSSSSSSPPSSPPSSSSSSSSSSSPPPPPSSSSSSSSSSSPPSSPPSSSSSSPPPPSW";
       m[10] = "WSPPPPPSSSSSSSSSSPPSSSSSSSSSPPPSPPPSSSSSSSSSPPSSSSSSSSSSPPPPPSW";
       m[11] = "WSPPPPPSSSSSSSSSSSPPSSSSSSSPPPSSSPPPSSSSSSSPPSSSSSSSSSSSPPPPPSW";
       m[12] = "WSSPPPPPSSSSSSSSSSSPPSSSSSPPPSSSSSPPPSSSSSPPSSSSSSSSSSSPPPPPSSW";
       m[13] = "WWSPPPPPSSSSSSSSSSSSPPSPSPPPPSSSSSPPPPSPSPPSSSSSSSSSSSSPPPPPSWW";
       m[14] = "WSSPPPPPPSPPPSPPPSPPPPPPPPPPPPSSSPPPPPPPPPPPPSPPPSPPPSPPPPPPSSW";
       m[15] = "WSPPPPPPPPPSPPPSPPPSPPPPPPSSSPPSPPSSSPPPPPPSPPPSPPPSPPPPPPPPPSW";
       m[16] = "WSPPPPPPSSSSSSSSSSSSSSPPPSSSSSPPPSSSSSPPPSSSSSSSSSSSSSSPPPPPPSW";
       m[17] = "WSPPPPPPSSSSSSSSSSSSSPPPSSSSSSSPSSSSSSSPPPSSSSSSSSSSSSSPPPPPPSW";
       m[18] = "WSSPPPPSSSSSSSSSSSSSPPPSSSSSSSPPPSSSSSSSPPPSSSSSSSSSSSSSPPPPSSW";
       m[19] = "WWSPPPPSSSSSSSSSSSSPPPSSSSSSSPPPPPSSSSSSSPPPSSSSSSSSSSSSPPPPSWW";
       m[20] = "WSSPPPSSSSSSSSSSSSPPPSSSSSSSPPPSPPPSSSSSSSPPPSSSSSSSSSSSSPPPSSW";
       m[21] = "WSPPPPSSSSSSSSSSSPPPSSSSSSSSPSSSSSPSSSSSSSSPPPSSSSSSSSSSSPPPPSW";
       m[22] = "SSPPPSSPPSSPPSSPPPPPPSSPPSSPPSPPPSPPSSPPSSPPPPPPSSPPSSPPSSPPPSS";
       m[23] = "SPPPPPPPPPPPPPPPPPPPPPPPPPPPSSPKPSSPPPPPPPPPPPPPPPPPPPPPPPPPPPS";
       m[24] = "SSPPPSSPPSSPPSSPPPPPPSSPPSSPPSPPPSPPSSPPSSPPPPPPSSPPSSPPSSPPPSS";
       m[25] = "WSPPPPSSSSSSSSSSSPPPSSSSSSSSPSSSSSPSSSSSSSSPPPSSSSSSSSSSSPPPPSW";
       m[26] = "WSSPPPSSSSSSSSSSSSPPPSSSSSSSPPPSPPPSSSSSSSPPPSSSSSSSSSSSSPPPSSW";
       m[27] = "WWSPPPPSSSSSSSSSSSSPPPSSSSSSSPPPPPSSSSSSSPPPSSSSSSSSSSSSPPPPSWW";
       m[28] = "WSSPPPPSSSSSSSSSSSSSPPPSSSSSSSPPPSSSSSSSPPPSSSSSSSSSSSSSPPPPSSW";
       m[29] = "WSPPPPPPSSSSSSSSSSSSSPPPSSSSSSSPSSSSSSSPPPSSSSSSSSSSSSSPPPPPPSW";
       m[30] = "WSPPPPPPSSSSSSSSSSSSSSPPPSSSSSPPPSSSSSPPPSSSSSSSSSSSSSSPPPPPPSW";
       m[31] = "WSPPPPPPPPPSPPPSPPPSPPPPPPSSSPPSPPSSSPPPPPPSPPPSPPPSPPPPPPPPPSW";
       m[32] = "WSSPPPPPPSPPPSPPPSPPPPPPPPPPPPSSSPPPPPPPPPPPPSPPPSPPPSPPPPPPSSW";
       m[33] = "WWSPPPPPSSSSSSSSSSSSPPSPSPPPPSSSSSPPPPSPSPPSSSSSSSSSSSSPPPPPSWW";
       m[34] = "WSSPPPPPSSSSSSSSSSSPPSSSSSPPPSSSSSPPPSSSSSPPSSSSSSSSSSSPPPPPSSW";
       m[35] = "WSPPPPPSSSSSSSSSSSPPSSSSSSSPPPSSSPPPSSSSSSSPPSSSSSSSSSSSPPPPPSW";
       m[36] = "WSPPPPPSSSSSSSSSSPPSSSSSSSSSPPPSPPPSSSSSSSSSPPSSSSSSSSSSPPPPPSW";
       m[37] = "WSPPPPSSSSSSPPSSPPSSSSSSSSSSSPPPPPSSSSSSSSSSSPPSSPPSSSSSSPPPPSW";
       m[38] = "WSSPPPSSSSSPPSSPPSSSSPPSSSSSSSPPPSSSSSSSPPSSSSPPSSPPSSSSSPPPSSW";
       m[39] = "WWSPPSSSSPPPPSPPSSSPPPPPPSSSSSPPPSSSSSPPPPPPSSSPPSPPPPSSSSPPSWW";
       m[40] = "SSSPPSSPPPPPPPPPPPPPPPPPPPPSSPPPPPSSPPPPPPPPPPPPPPPPPPPPSSPPSSS";
       m[41] = "SPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPS";
       m[42] = "SPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPS";
       m[43] = "SPPPPSSSPPPSSSPPPSSSPPPSSSPPPPPPPPPPPSSSPPPSSSPPPSSSPPPSSSPPPPS";
       m[44] = "SPPPPSWSSSSSWSSSSSWSSSSSWSSSSSPPPSSSSSWSSSSSWSSSSSWSSSSSWSPPPPS";
       m[45] = "SSSSSSWWWWWWWWWWWWWWWWWWWWWWWSSSSSWWWWWWWWWWWWWWWWWWWWWWWSSSSSS";
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

-- wird aufgerufen wenn ein Bot joint
-- player ist, uh wie erstaunlich, die Spielernummer
--function level_spawn_point(player)
--      availSpawnpoints = {{2,2},{15,15}}
--      return world_tile_center(15, 15)
--end


function level_init()

       maplayout()
       arraySize = table.getn(m);
       for i=1, arraySize, 1 do
               for j=1, string.len(m[i]), 1 do
                       k = string.upper(string.sub(m[i],j,j))
                       if k == "P" or k == "U" or k == "D" or k == "K" then
                               world_set_type(j,i, TILE_PLAIN)
                       end
                       world_set_gfx(j,i, tile[k])
               end
       end
       world_make_border(TILE_GFX_WATER)

       food_spawner = {}
       food_spawner[0] = {     x = 1,
                               y = 1,
                               rx = 61,
                               ry = 2,
                               a = 200,
                               i = 200,
                               n = game_time()
                         }
       food_spawner[1] = {     x = 1,
                               y = 42,
                               rx = 61,
                               ry = 2,
                               a = 200,
                               i = 200,
                               n = game_time()
                         }
       food_spawner[2] = {     x = 1,
                               y = 3,
                               rx = 2,
                               ry = 39,
                               a = 200,
                               i = 200,
                               n = game_time()
                         }
       food_spawner[3] = {     x = 60,
                               y = 3,
                               rx = 2,
                               ry = 39,
                               a = 200,
                               i = 200,
                               n = game_time()
                         }
       for s = 4, 20 do
               local dx, dy = world_find_digged()
               food_spawner[s] = {     x = dx,
                                       y = dy,
                                       rx = math.random(5),
                                       ry = math.random(5),
                                       a = math.random(100) + 30,
                                       i = math.random(1000) + 1000,
                                       n = game_time() }
               world_add_food(food_spawner[s].x, food_spawner[s].y, 10000)
       end
       last_food = game_time()
end

-- wird, wie der Name schon sagt, jede Runde ausgefuehrt
function level_tick()
       if game_time() > last_food + 10000 then
               for n, spawner in pairs(food_spawner) do
                       if game_time() > spawner.n then
                               world_add_food(spawner.x + math.random(spawner.rx) ,
                               spawner.y + math.random(spawner.ry) ,
                               spawner.a)
                       spawner.n = spawner.n + spawner.i
                       end
               end
       end
end
