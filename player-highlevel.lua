--[[

    Copyright (c) 2006 Florian Wesch <fw@dividuum.de>. All Rights Reserved.
    
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

]]--

------------------------------------------------------------------------
-- Creature Klasse
------------------------------------------------------------------------

Creature = {}

function Creature:moveto(tx, ty)
    local x, y = self:pos()
    if x == tx and y == ty then
        return true
    end
    
    if not self:set_path(tx, ty) then
        return false
    end

    if not self:begin_walk_path() then
        return false
    end

    local review = game_time()
    while self:is_walking() do
        self:wait_for_next_round()
        if game_time() > review + 500 then
            self:set_path(tx, ty)     
            review = game_time()
        end
    end
    
    x, y = self:pos()
    return x == tx and y == ty
end

function Creature:heal() 
    if not self:begin_healing() then
        return false
    end
    while self:is_healing() do
        self:wait_for_next_round()
    end
    return true
end

function Creature:eat()
    if not self:begin_eating() then
        return false
    end
    while self:is_eating() do
        self:wait_for_next_round()
    end
    return true
end

function Creature:feed(target)
    if not self:set_target(target) then
        return false
    end
    if not self:begin_feeding() then
        return false
    end
    while self:is_feeding() do
        self:wait_for_next_round()
    end
    return true
end

function Creature:attack(target) 
    if not self:set_target(target) then
        return false
    end
    if not self:begin_attacking() then
        return false
    end
    while self:is_attacking() do
        self:wait_for_next_round()
    end
    return true
end

function Creature:convert(to_type)
    if not self:set_conversion(to_type) then
        return false
    end
    if not self:begin_converting() then 
        return false
    end
    while self:is_converting() do
        self:wait_for_next_round()
    end
    return self:type() == to_type
end

function Creature:suicide()
    suicide(self.id)
end

function Creature:nearest_enemy()
    return get_nearest_enemy(self.id)
end

function Creature:set_path(x, y)
    return set_path(self.id, x, y)
end

function Creature:set_target(c)
    return set_target(self.id, c)
end

function Creature:set_conversion(t)
    return set_convert(self.id, t)
end

function Creature:pos()
    return get_pos(self.id)
end

function Creature:speed()
    return get_speed(self.id)
end

function Creature:health()
    return get_health(self.id)
end

function Creature:food()
    return get_food(self.id)
end

function Creature:max_food()
    return get_max_food(self.id)
end

function Creature:tile_food()
    return get_tile_food(self.id)
end

function Creature:tile_type()
    return get_tile_type(self.id)
end

function Creature:type()
    return get_type(self.id)
end

function Creature:screen_message(msg)
    return set_message(self.id, msg)
end

function Creature:begin_idling()
    return set_state(self.id, CREATURE_IDLE)
end

function Creature:begin_walk_path()
    return set_state(self.id, CREATURE_WALK)
end

function Creature:begin_healing()
    return set_state(self.id, CREATURE_HEAL)
end

function Creature:begin_eating()
    return set_state(self.id, CREATURE_EAT)
end

function Creature:begin_attacking()
    return set_state(self.id, CREATURE_ATTACK)
end

function Creature:begin_converting()
    return set_state(self.id, CREATURE_CONVERT)
end

function Creature:begin_spawning()
    return set_state(self.id, CREATURE_SPAWN)
end

function Creature:begin_feeding()
    return set_state(self.id, CREATURE_FEED)
end

function Creature:is_idle()
    return get_state(self.id) == CREATURE_IDLE
end

function Creature:is_healing()
    return get_state(self.id) == CREATURE_HEAL
end

function Creature:is_walking()
    return get_state(self.id) == CREATURE_WALK
end

function Creature:is_eating()
    return get_state(self.id) == CREATURE_EAT
end

function Creature:is_attacking()
    return get_state(self.id) == CREATURE_ATTACK
end

function Creature:is_converting()
    return get_state(self.id) == CREATURE_CONVERT
end

function Creature:is_spawning()
    return get_state(self.id) == CREATURE_SPAWN
end

function Creature:is_feeding()
    return get_state(self.id) == CREATURE_FEED
end

function Creature:sleep(msec)
    local time = game_time()
    while time + msec > game_time() do
        self:wait_for_next_round()
    end
end

function Creature:main_restarter() 
    while true do
        self:main()
        self:wait_for_next_round()
    end
end

function Creature:traceback()
    print(_TRACEBACK(self.thread))
end

function Creature:restart()
    set_state(self.id, CREATURE_IDLE)
    self.thread = coroutine.create(self.main_restarter)
 -- self.thread = coroutine.create(function(self) self:main_restarter() end)
    if self.onRestart then
        local ok, msg = pcall(self.onRestart, self)
        if not ok then 
            print("restarting main failed: " .. msg)
        end
    else
        print("onRestart method deleted")
    end
end

function Creature:wait_for_next_round()
    if can_yield then
        coroutine.yield()
    else
        print("-----------------------------------------------------------")
        print("Error: A called function wanted to wait_for_next_round().")
        print("-----------------------------------------------------------")
        error("cannot continue")
    end
end

------------------------------------------------------------------------
-- Callbacks von C
------------------------------------------------------------------------
function this_function_call_fails_if_cpu_limit_exceeded() end

-- Globales Array alle Kreaturen
creatures = creatures or {}

function player_think(events)
    can_yield = false

    -- Events abarbeiten
    for n, event in ipairs(events) do 
        if event.type == CREATURE_SPAWNED then
            local id     = event.id
            local parent = event.parent ~= -1 and event.parent or nil
            local creature = {}
            setmetatable(creature, {
                __index    = Creature, 
                __tostring = function(self)
                    local x, y  = get_pos(self.id)
                    local states = { [0]="idle",   [1]="walk",    [2]="heal",  [3]="eat",
                                     [4]="attack", [5]="convert", [6]="spawn", [7]="feed"}
                    return "<creature " .. self.id .." [" .. x .. "," .. y .."] " .. 
                            "type " .. get_type(self.id) ..", health " .. get_health(self.id) .. ", " ..
                            "food " .. get_food(self.id) .. ", state " .. states[get_state(self.id)]  .. ">"
                end,
                __concat = function (op1, op2)
                    return tostring(op1) .. tostring(op2)
                end
            })
            creatures[id] = creature
            creature.id = id
            if creature.onSpawned then 
                local ok, msg = pcall(creature.onSpawned, creature, parent)
                if not ok then
                    print("onSpawned failed: " .. msg)
                end
            else
                print("onSpawned method deleted")
            end
            creature:restart()
        elseif event.type == CREATURE_KILLED then
            local id     = event.id
            local killer = event.killer ~= -1 and event.killer or nil
            assert(creatures[id])
            if creatures[id].onKilled then 
                local ok, msg = pcall(creatures[id].onKilled, creatures[id], killer)
                if not ok then 
                    print("cannot call onKilled: " .. msg)
                end
            else
                print("onKilled method deleted")
            end
            creatures[id] = nil
        elseif event.type == CREATURE_ATTACKED then
            local id       = event.id
            local attacker = event.attacker
            if creatures[id].onAttacked then 
                local ok, msg = pcall(creatures[id].onAttacked, creatures[id], attacker)
                if not ok then 
                    print("onAttacked failed: " .. msg)
                end
            else
                print("onAttacked method deleted")
            end
        elseif event.type == PLAYER_CREATED then
            if onGameStart then
                local ok, msg = pcall(onGameStart)
                if not ok then
                    print("onGameStart failed: " .. msg)
                end
            end
        else
            error("invalid event " .. event.type)
        end
    end

    -- Vor jeder Runde eventuell vorhandene Funktion onRoundStart aufrufen
    if onRoundStart then
        local ok, msg = pcall(onRoundStart)
        if not ok then
            print("onRoundEnd failed: " .. msg)
        end
    end
    
    can_yield = true

    -- Vorhandene Kreaturen durchlaufen
    for id, creature in pairs(creatures) do
        if type(creature.thread) ~= 'thread' then
            creature.message = 'uuh. self.thread is not a coroutine.'
        elseif coroutine.status(creature.thread) == 'dead' then
            if not creature.message then 
                creature.message = 'main() terminated (maybe it was killed for using too much cpu/memory?)'
            end
        else
            if creature.debug then
                thread_trace(creature.thread, creature.id)
            else
                thread_untrace(creature.thread)
            end
            local ok, msg = coroutine.resume(creature.thread, creature)
            if not ok then
                creature.message = msg
                -- Falls die Coroutine abgebrochen wurde, weil zuviel
                -- CPU benutzt wurde, so triggert folgender Funktions-
                -- aufruf den Abbruch von player_think. Um zu ermitteln,
                -- wo zuviel CPU gebraucht wurde, kann der Traceback
                -- in creature.message mittels 'i' angezeigt werden.
                this_function_call_fails_if_cpu_limit_exceeded()
            end
        end
    end

    can_yield = false

    -- Nach jeder Runde eventuell vorhandene Funktion onRoundEnd aufrufen
    if onRoundEnd then
        local ok, msg = pcall(onRoundEnd)
        if not ok then
            print("onRoundEnd failed: " .. msg)
        end
    end
end

------------------------------------------------------------------------
-- Load default BotCode
------------------------------------------------------------------------
dofile('player-default')
