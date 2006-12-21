function onNewGame()
    round_end_text = nil
    round_end_time = nil

    king_player = nil
    king_time   = 0
end

function onRound()
    local time_limit = get_time_limit()
    if not round_end_text and time_limit and game_time() > time_limit then
        local maxscore   = -1000000
        local winner     = nil
        local num_winner = 0
        for n in each_player() do 
            local score = player_score(n)
            if score > maxscore then 
                winner     = n
                maxscore   = score
                num_winner = 1
            elseif score == maxscore then
                num_winner = num_winner + 1
            end
        end
        if not winner then
            round_end_text = "Timelimit hit"
        elseif num_winner == 1 then
            round_end_text = "Timelimit hit. " .. player_get_name(winner) .. " wins the game!"
        else
            round_end_text = "Timelimit hit. The game is a draw."
        end
    end
    
    if round_end_text then 
        if not round_end_time then
            round_end_time = game_time()
            scroller_add(round_end_text)
            set_intermission(round_end_text)
        elseif round_end_time + 10000 < game_time() then
            set_intermission("")
            world_rotate_map()
            reset()
        end
    end
end

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
    local x, y = world_get_spawn_point(player)
    if x and y then creature_spawn(player, nil, x, y, CREATURE_SMALL) end
    local x, y = world_get_spawn_point(player)
    if x and y then creature_spawn(player, nil, x, y, CREATURE_SMALL) end
end

function onPlayerAllCreaturesDead(player, time)
    if time < 2000 then return end
    local x, y = world_get_spawn_point(player)
    if x and y then creature_spawn(player, nil, x, y, CREATURE_SMALL) end
    local x, y = world_get_spawn_point(player)
    if x and y then creature_spawn(player, nil, x, y, CREATURE_SMALL) end
end

function onPlayerScoreChange(player, score, reason)
    local score_limit = get_score_limit()
    if not round_end_text and score_limit and score >= score_limit then
        round_end_text = "player " .. player_get_name(player) .. " wins the game!"
    end 
end

function onKingPlayer(player, delta)
    if player ~= king_player then
        king_player = player
        king_time   = 0
    end
    king_time = king_time + delta
    if king_time > 10000 then
        player_change_score(player, 30, "King of the Hill!")
        king_time = king_time - 10000
    end
end

function onNoKing()
    king_player = nil
    king_time   = 0
end
