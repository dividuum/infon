function onCreatureSpawned(creature, parent)
    if parent then
        local parent_player = creature_get_player(parent)
        player_change_score(parent_player, 10, "Spawned a creature")
        creature_set_food(creature, 1000)
    end
end

function onCreatureKilled(victim, killer) 
    local victim_food        = creature_get_food(victim)
    local victim_x, victim_y = creature_get_pos(victim)

    -- Suiciding returns less food
    if not killer then
        victim_food = victim_food / 3
    end

    world_add_food_by_worldcoord(victim_x, victim_y, victim_food)

    local victim_player = creature_get_player(victim)

    if killer == victim then
        player_change_score(victim_player, -40, "Creature suicides")
    elseif not killer then
        player_change_score(victim_player, -3, "Creature died")
    else
        local victim_type = creature_get_type(victim)
        local killer_type = creature_get_type(killer)
        local killer_player = creature_get_player(killer)
        if victim_type == 0 and killer_type == 1 then
            player_change_score(victim_player, -3, "Runner was killed")
            player_change_score(killer_player, 10, "Killed a runner")
        elseif victim_type == 1 and killer_type == 1 then
            player_change_score(victim_player, -8, "Fatty was killed")
            player_change_score(killer_player, 15, "Crushed a fatty")
        elseif victim_type == 2 and killer_type == 0 then
            player_change_score(victim_player, -4, "Flyer was killed")
            player_change_score(killer_player, 12, "Smashed a flyer")
        elseif victim_type == 2 and killer_type == 1 then
            player_change_score(victim_player, -4, "Flyer was killed")
            player_change_score(killer_player, 12, "Smashed a flyer")
        else
            print("BUG: Impossible killer/victim combo")
            print("victim = " .. victim_type .. " killer = " .. killer_type)
        end
    end
end

function onPlayerCreated(player)
    local x, y
    x, y = world_find_digged_worldcoord()
    creature_spawn(player, nil, x, y, CREATURE_BIG)
    x, y = world_find_digged_worldcoord()
    creature_spawn(player, nil, x, y, CREATURE_SMALL)
end

function onPlayerAllCreaturesDied(player)
    local x, y
    x, y = world_find_digged_worldcoord()
    creature_spawn(player, nil, x, y, CREATURE_SMALL)
    x, y = world_find_digged_worldcoord()
    creature_spawn(player, nil, x, y, CREATURE_SMALL)
end
