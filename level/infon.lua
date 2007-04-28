-- Mapname: infon
-- Author:  g², copied a lot of code from Dunedan
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
       m[ 1] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 2] = "PPPPPSPPPPPPPPPPPPPPPPPPPPPPPSSSSSSSPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 3] = "PPPPSSSPPPPPPPPPPPPPPPPPPPPPSSSPPSSSPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 4] = "PPPPSSSPPPPPPPPPPPPPPPPPPPPSSSPPPSSSPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 5] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPSSSPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 6] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPSSSPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 7] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPSSSPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[ 8] = "PSSSSSSPPPSSSSPPSSSSSPPPPPSSSSSSSPPPSSSSSSPPPPSSSSSPSSSSSSPPPP";
       m[ 9] = "PPPPSSSPPPPSSSSSPPPSSSPPPPPSSSPPPPPSSWWWWSSSPPPPSSSSPPPSSSSPPP";
       m[10] = "PPPPSSSPPPPSSSSPPPPPSSPPPPPSSSPPPPSSSWWWWWSSPPPPSSSSPPPPSSSPPP";
       m[11] = "PPPPSSSPPPPSSSSPPPPPSSSPPPPSSSPPPPSSSWWWWWSSSPPPSSSPPPPPSSSPPP";
       m[12] = "PPPPSSSPPPPSSSPPPPPPSSSPPPPSSSPPPPSSWWWWWWSSSPPPSSSPPPPPSSSPPP";
       m[13] = "PPPPSSSPPPPSSSPPPPPPSSSPPPPSSSPPPPSSWWWKWWSSSPPPSSSPPPPPSSSPPP";
       m[14] = "PPPPSSSPPPPSSSPPPPPPSSSPPPPSSSPPPPSSWWWWWWSSSPPPSSSPPPPPSSSPPP";
       m[15] = "PPPPSSSPPPPSSSPPPPPPSSSPPPPSSSPPPPSSSWWWWWSSPPPPSSSPPPPPSSSPPP";
       m[16] = "PPPPSSSPPPPSSSPPPPPPSSSPPPPSSSPPPPPSSWWWWWSSPPPPSSSPPPPPSSSPPP";
       m[17] = "PPPPSSSPPPPSSSSPPPPPSSSPPPPSSSPPPPPSSSWWWSSPPPPPSSSPPPPPSSSPPP";
       m[18] = "PPSSSSSSSPSSSSSSSPSSSSSSSPSSSSSSSPPPPSSSSSPPPPSSSSSSSPSSSSSSSP";
       m[19] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
       m[20] = "PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP";
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
--      world_make_border(TILE_GFX_WATER)

       food_spawner = {}
       food_spawner[0] = {
                               x = 17,
                               y = 12,
                               r = 4,
                               a = 300,
                               i = 200,
                               n = 0,
                       }
       food_spawner[1] = {
                               x = 54,
                               y = 12,
                               r = 4,
                               a = 300,
                               i = 200,
                               n = 0,
                       }
       for s = 2, 12 do
               local dx, dy = world_find_digged()
               food_spawner[s] = {     x = dx,
                                       y = dy,
                                       r = math.random(2),
                                       a = math.random(100) + 30,
                                       i = math.random(1000) + 1000,
                                       n = game_time(),
                               }
               world_add_food(food_spawner[s].x, food_spawner[s].y, 9000)
       end
       last_food = game_time()


end

-- wird, wie der Name schon sagt, jede Runde ausgefuehrt
function level_tick()
       if game_time() > last_food + 10000 then
               for n, spawner in pairs(food_spawner) do
                       if game_time() > spawner.n then
                               world_add_food(spawner.x + math.random(spawner.r * 2 + 1 ) - spawner.r,
                               spawner.y + math.random(spawner.r * 2 + 1 ) - spawner.r,
                               spawner.a)
                       spawner.n = spawner.n + spawner.i
                       end
               end
       end
end
