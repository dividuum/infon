-- Mapname: Owl
-- Author: Dunedan
-- Version: 0.1

function level_size()
    return 31, 31
end

function level_koth_pos()
    return 15, 15
end

function level_init()

	tile = {}
	tile["S"] = TILE_SOLID;
	tile["W"] = TILE_WATER;
	tile["P"] = TILE_PLAIN;
	tile["L"] = TILE_LAVA;

	m = {}
	m[1] = 	"PPPWWWWWWWWWWPPPWWWWWWWWWWPPP";
	m[2] = 	"PPPWWWWWWWWWWPPPWWWWWWWWWWPPP";
	m[3] = 	"PPPPWWWWWWWWPPPPPWWWWWWWWPPPP";
	m[4] = 	"WWWPPPPPPPPPPPPPPPPPPPPPPPWWW";
 	m[5] = 	"WWWWPPWWWWWWWWWWWWWWWWWPPWWWW";
	m[6] = 	"WWWWWPPWWWWWWWWWWWWWWWPPWWWWW";
	m[7] = 	"WWWWWPPPWWWWWWWWWWWWWPPPWWWWW";
 	m[8] = 	"WWWWPPWPPWWWWWWWWWWWPPWPPWWWW";
	m[9] = 	"WWWPPWWWPPWWWWWWWWWPPWWWPPWWW";
	m[10] = "WWPPWWWWPPPWWWWWWWPPPWWWWPPWW";
	m[11] = "WPPWWWWWPWPPWWWWWPPWPWWWWWPPW";
	m[12] = "PPWWWWWWPwWPPWWWPPWWPWWWWWWPP";
	m[13] = "PPWWWWWWPWWWPPPPPWWWPWWWWWWPP";
	m[14] = "PPWWWWWWPWWWWPPPWWWWPWWWWWWPP";
	m[15] = "PPWWWWWWPWWWWPPPWWWWPWWWWWWPP";
	m[16] = "PPWWWWWWPWWWWPPPWWWWPWWWWWWPP";
	m[17] = "PPWWWWWWPWWWPPPPPWWWPWWWWWWPP";
	m[18] = "PPWWWWWWPWWPPWWWPPWWPWWWWWWPP";
 	m[19] = "WPPWWWWWPWPPWWWWWPPWPWWWWWPPW";
	m[20] = "WWPPWWWWPPPWWWWWWWPPPWWWWPPWW";
	m[21] = "WWWPPWWWPPWWWWWWWWWPPWWWPPWWW";
	m[22] = "WWWWPPWPPWWWWWWWWWWWPPWPPWWWW";
	m[23] = "WWWWWPPPWWWWWWWWWWWWWPPPWWWWW";
	m[24] = "WWWWWPPWWWWWWWWWWWWWWWPPWWWWW";
	m[25] = "WWWWPPWWWWWWWWWWWWWWWWWPPWWWW";
	m[26] = "WWWPPPPPPPPPPPPPPPPPPPPPPPWWW";
	m[27] = "PPPPWWWWWWWWPPPPPWWWWWWWWPPPP";
	m[28] = "PPPWWWWWWWWWWPPPWWWWWWWWWWPPP";
	m[29] = "PPPWWWWWWWWWWPPPWWWWWWWWWWPPP";


	arraySize = table.getn(m);
	for i=1, arraySize, 1 do
		for j=1, string.len(m[i]), 1 do
			k = string.upper(string.sub(m[i],j,j))
--			print (k);
			world_dig(j,i, tile[k])
		end
	end

    world_make_border(TILE_GFX_WATER)

	food_spawner = {}
	for s = 0, 11 do
		local dx, dy = world_find_digged()
		food_spawner[s] = {	x = dx,
					y = dy,
					r = math.random(2),
					a = math.random(100) + 30,
					i = math.random(1000) + 1000,
					n = game_time() }
		world_add_food(food_spawner[s].x, food_spawner[s].y, 10000)
	end
	last_food = game_time()
end

function level_tick()
	if game_time() > last_food + 10000 then
		for n, spawner in pairs(food_spawner) do
			if game_time() > spawner.n then 
				world_add_food(spawner.x + math.random(spawner.r * 2 + 1) - spawner.r,
				spawner.y + math.random(spawner.r * 2 + 1) - spawner.r,
				spawner.a)
			spawner.n = spawner.n + spawner.i
			end
		end
	end
end
