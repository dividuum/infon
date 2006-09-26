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
-- Unsicheres Zeugs weg
------------------------------------------------------------------------

_TRACEBACK = debug.traceback

save_in_registry('traceback', debug.traceback)
save_in_registry = nil

dofile          = nil
getfenv         = nil
setfenv         = nil
loadfile        = nil
_VERSION        = nil
newproxy        = nil
debug           = nil
gcinfo          = nil
os              = nil
package         = nil
io              = nil

collectgarbage()
collectgarbage  = nil

------------------------------------------------------------------------
-- Creature Klasse
------------------------------------------------------------------------

Creature = {}

function Creature:moveto(tx, ty)
    local x, y = self:pos()
    if x == tx and y == ty then
        -- Um Endlosschleifen mit einer einzelnen 
        -- moveto Anweisung zu verhindern.
        self:wait_for_next_round() 
        return true
    end
    
    if not self:set_path(tx, ty) then
        self:wait_for_next_round() 
        return false
    end

    self:begin_walk_path()
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
    self:begin_healing()
    while self:is_healing() do
        self:wait_for_next_round()
    end
end

function Creature:eat()
    self:begin_eating()
    while self:is_eating() do
        self:wait_for_next_round()
    end
end

function Creature:attack(target) 
    if not self:set_target(target) then
        return
    end
    self:begin_attacking()
    while self:is_attacking() do
        self:wait_for_next_round()
    end
end

function Creature:convert(to_type)
    if not self:set_conversion(to_type) then
        return false
    end
    self:begin_converting()
    while self:is_converting() do
        self:wait_for_next_round()
    end
    return self:type() == to_type
end

function Creature:suicide()
    suicide(self.id)
end

function Creature:nearest_enemy()
    return nearest_enemy(self.id)
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
    while time + msec >= game_time() do
        self:wait_for_next_round()
    end
end

function Creature:main_restarter() 
    while true do
        self:main()
        coroutine.yield()
    end
end

function Creature:restart()
    set_state(self.id, CREATURE_IDLE)
    self.thread = coroutine.create(self.main_restarter)
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
    --[[
    local ok, msg = xpcall(coroutine.yield, _TRACEBACK)
    if not ok then 
        print(msg)
        error("wait_for_next_round cannot be called in interactive mode")
    end
    ]]--
end

------------------------------------------------------------------------
-- praktisches
------------------------------------------------------------------------

function p(x) 
    if type(x) == "table" then
        print("+--- Table: " .. tostring(x))
        for key, val in pairs(x) do
            print("| " .. tostring(key) .. " " .. tostring(val))
        end
        print("+-----------------------")
    else
        print(type(x) .. " - " .. tostring(x))
    end
end

------------------------------------------------------------------------
-- Von Telnet ueber r/i aufrufbare Funktionen
------------------------------------------------------------------------

function restart()
    for id, creature in pairs(creatures) do
        creature.message = nil
        creature:restart()
    end
end

function info()
    for id, creature in pairs(creatures) do
        print("creature " .. id .. ": " .. (creature.message or "-"))
    end
end

------------------------------------------------------------------------
-- Callbacks von C
------------------------------------------------------------------------
function this_function_call_fails_if_cpu_limit_exceeded() end

_spawned_creatures  = {}
_killed_creatures   = {}
_attacked_creatures = {}

-- Globales Array alle Kreaturen
creatures = {}

function player_think()
    can_yield = false

    -- Gesetzt nach Rundenstart und/oder Joinen
    if _player_created and onGameStart then
        local ok, msg = pcall(onGameStart)
        if not ok then
            print("onGameStart failed: " .. msg)
        end
        _player_created = nil
    end
    
    -- Getoetete Kreaturen handlen
    for victim, killer in pairs(_killed_creatures) do
        if killer == -1 then 
           killer = nil
        end
        assert(creatures[victim])
        if creatures[victim].onKilled then 
            local ok, msg = pcall(creatures[victim].onKilled, creatures[victim], killer)
            if not ok then 
                print("cannot call onKilled: " .. msg)
            end
        else
            print("onKilled method deleted")
        end
        creatures[victim] = nil
    end
    
    -- Angegriffene Kreaturen
    for attacker, victim in pairs(_attacked_creatures) do
        if creatures[victim] then -- Getoetete Viecher ueberspringen
            if creatures[victim].onAttacked then 
                local ok, msg = pcall(creatures[victim].onAttacked, creatures[victim], attacker)
                if not ok then 
                    print("onAttacked failed: " .. msg)
                end
            else
                print("onAttacked method deleted")
            end
        end
    end

    -- Erzeugte Kreaturen
    for id, parent in pairs(_spawned_creatures) do
        if parent == -1 then
           parent = nil
        end
        local creature = {}
        setmetatable(creature, {
            __index = function(self, what)
                if Creature[what] then
                    return Creature[what]
                end
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
    end

    -- Transfertabellen leeren
    _spawned_creatures  = {}
    _killed_creatures   = {}
    _attacked_creatures = {}

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
                creature.message = 'main() terminated (maybe it was killed for using too much cpu?)'
            end
        else
            local ok, msg = coroutine.resume(creature.thread, creature)
            if not ok then
                creature.message = msg
                -- Falls die Coroutine abgebrochen wurde, weil zuviel
                -- CPU benutzt wurde, so triggert folgeder Funktions
                -- Aufruf den Abbruch von player_think. Um zu ermitteln,
                -- wo zuviel CPU gebraucht wurde, kann der Traceback
                -- in creature.message mittels 'i' angezeigt werden.
                this_function_call_fails_if_cpu_limit_exceeded()
            end
        end
    end

    can_yield = false

    if onRoundEnd then
        local ok, msg = pcall(onRoundEnd)
        if not ok then
            print("onRoundEnd failed: " .. msg)
        end
    end
    
end

------------------------------------------------------------------------
-- Default Laden
------------------------------------------------------------------------

require 'player-default'
require = nil
