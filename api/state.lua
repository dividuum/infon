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

global = {}

function setupCreature()
    -- Internal Stuff -----------------

    function _set_state(state, ...)
        _GG.assert(_G[state], "cannot change from state '" .. _state_name .. "' to undefined state '" .. state .. "'")
        if _debug_state_change then
            print("creature " .. id .. " is changing from '" .. _state_name .. "' to '" .. state .. "'")
        end
        _state_name = state
        _state_args = {...}
    end

    function _state_loop() 
        while true do
            local function handle_state_change(state, ...) 
                if not state then 
                    -- change to idle if not already there
                    if _state_name ~= "idle" then
                       _set_state("idle")
                    end
                elseif state == true then
                    -- keep state (restart state handler)
                else
                    -- change to new state
                    _set_state(state, ...)
                end
            end
            if not _G[_state_name] then
                print("state '" .. _state_name .. "' does not exist. changing to 'idle'")
                _set_state("idle")
            end
            handle_state_change(_G[_state_name](_GG.unpack(_state_args)))
            wait_for_next_round()
        end
    end

    function _restart_thread()
        thread = _GG.coroutine.create(_state_loop)
    end

    -- Utility functions ----------------

    -- Enforces a switch to state 'state' and a restart 
    -- of the corresponding state handler.
    and_start_state   = function(state, ...)
        _GG.assert(_G[state], "state '" .. state .. "' not defined")
        return state, ...
    end
    
    -- Enforces being in state 'state'. If the creature 
    -- already is in state 'state', nothing happens. If
    -- the creature is in any other state, the creature
    -- switches to 'state'.
    and_be_in_state   = function(state, ...)
        if _state_name == state then
            return true
        else
            _GG.assert(_G[state], "state '" .. state .. "' not defined")
            return state, ...
        end
    end
    
    -- The creature stays in its current state
    and_keep_state    = false

    -- The creature restarts its current state
    and_restart_state = true

    function reload()
        if _GG.bot then
            local ok, msg = _GG.epcall(_GG._TRACEBACK, _GG.setfenv(_GG.bot, _G))
            if not ok then
                _GG.print("cannot restart creature " .. id .. ": " .. msg)
            end
        end
    end

    function restart()
        message = nil
        reload()
        _set_state("restarting")
        _restart_thread()
    end

    function wait_for_next_round()
        if _GG.can_yield then
            _GG.coroutine.yield()
        else
            _GG.error("cannot wait")
        end
    end

    function execute_event(event, ...)
        if not _G[event] then return end
        local function handle_state_change(ok, state, ...) 
            if not ok then
                _GG.print("calling event '" .. event .. "' failed: " .. state)
            else
                if not state then 
                    -- no change
                elseif state == true then
                    -- keep state (restart state handler)
                    _restart_thread()
                else
                    -- change to new state
                    _set_state(state, ...)
                    _restart_thread()
                end
            end
        end
        return handle_state_change(_GG.epcall(_GG._TRACEBACK, _G[event], ...))
    end

    -- States ---------------------------

    function idle()
        _GG.set_state(id, _GG.CREATURE_IDLE)
        if onIdle then return onIdle() end
    end

    function restarting()
        if onRestart then return onRestart() end
    end

    -- Functions ------------------------
    
    time       = _GG.game_time
    koth_pos   = _GG.get_koth_pos
    world_size = _GG.world_size

    function suicide()
        _GG.suicide(id)
    end

    function nearest_enemy()
        return _GG.get_nearest_enemy(id)
    end

    function set_path(x, y)
        return _GG.set_path(id, x, y)
    end

    function set_target(c)
        return _GG.set_target(id, c)
    end

    function set_conversion(t)
        return _GG.set_convert(id, t)
    end

    function pos()
        return _GG.get_pos(id)
    end

    function speed()
        return _GG.get_speed(id)
    end

    function health()
        return _GG.get_health(id)
    end

    function food()
        return _GG.get_food(id)
    end

    function max_food()
        return _GG.get_max_food(id)
    end

    function tile_food()
        return _GG.get_tile_food(id)
    end

    function tile_type()
        return _GG.get_tile_type(id)
    end

    function type()
        return _GG.get_type(id)
    end

    function say(msg)
        return _GG.set_message(id, msg)
    end

    function begin_idling()
        return _GG.set_state(id, _GG.CREATURE_IDLE)
    end

    function begin_walk_path()
        return _GG.set_state(id, _GG.CREATURE_WALK)
    end

    function begin_healing()
        return _GG.set_state(id, _GG.CREATURE_HEAL)
    end

    function begin_eating()
        return _GG.set_state(id, _GG.CREATURE_EAT)
    end

    function begin_attacking()
        return _GG.set_state(id, _GG.CREATURE_ATTACK)
    end

    function begin_converting()
        return _GG.set_state(id, _GG.CREATURE_CONVERT)
    end

    function begin_spawning()
        return _GG.set_state(id, _GG.CREATURE_SPAWN)
    end

    function begin_feeding()
        return _GG.set_state(id, _GG.CREATURE_FEED)
    end

    function is_idle()
        return _GG.get_state(id) == _GG.CREATURE_IDLE
    end

    function is_healing()
        return _GG.get_state(id) == _GG.CREATURE_HEAL
    end

    function is_walking()
        return _GG.get_state(id) == _GG.CREATURE_WALK
    end

    function is_eating()
        return _GG.get_state(id) == _GG.CREATURE_EAT
    end

    function is_attacking()
        return _GG.get_state(id) == _GG.CREATURE_ATTACK
    end

    function is_converting()
        return _GG.get_state(id) == _GG.CREATURE_CONVERT
    end

    function is_spawning()
        return _GG.get_state(id) == _GG.CREATURE_SPAWN
    end

    function is_feeding()
        return _GG.get_state(id) == _GG.CREATURE_FEED
    end

    function random_path()
        local x1, y1, x2, y2 = world_size()
        local i = 0
        while true do 
            for i = 1, 300 do 
                local x, y = math.random(x1, x2), math.random(y1, y2)
                if set_path(x, y) then
                    return  x, y
                end
            end
            wait_for_next_round()
        end
    end

    function in_state(state)
        if _GG.type(state) == "function" then
            return _G[_state_name] == state
        elseif _GG.type(state) == "string" then
            return _state_name == state
        else
            return false
        end
    end

    -- Blocking Actions -----------------
    
    function sleep(msec)
        local now = time()
        while now + msec < time() do
            wait_for_next_round()
        end
    end
    
    function random_move()
        return move_to(random_path())
    end

    function move_to(tx, ty)
        local x, y = pos()
        if x == tx and y == ty then
            return true
        end
        
        if not _GG.set_path(id, tx, ty) then
            return false
        end

        return move_path(tx, ty)
    end

    function move_path(tx, ty)
        if not _GG.set_state(id, _GG.CREATURE_WALK) then
            return false
        end

        while _GG.get_state(id) == _GG.CREATURE_WALK do
            wait_for_next_round()
        end
        
        local x, y = pos()
        return x == tx and y == ty
    end

    function can_eat()
        return food() < max_food()
    end

    function heal() 
        if not begin_healing() then
            return false
        end
        while is_healing() do
            wait_for_next_round()
        end
        return true
    end

    function eat()
        if not begin_eating() then
            return false
        end
        while is_eating() do
            wait_for_next_round()
        end
        return true
    end

    function feed(target)
        if not set_target(target) then
            return false
        end
        if not begin_feeding() then
            return false
        end
        while is_feeding() do
            wait_for_next_round()
        end
        return true
    end

    function attack(target) 
        if not set_target(target) then
            return false
        end
        if not begin_attacking() then
            return false
        end
        while is_attacking() do
            wait_for_next_round()
        end
        return true
    end

    function convert(to_type)
        if not set_conversion(to_type) then
            return false
        end
        if not begin_converting() then 
            return false
        end
        while is_converting() do
            wait_for_next_round()
        end
        return type() == to_type
    end

    function spawn()
        if not begin_spawning() then
            return false
        end
        while is_spawning() do
            wait_for_next_round()
        end
        return true
    end
end

function createCreature(id, parent)
    local creature = {
        id          = id;
        parent      = parent;
        global      = global;

        print       = print;
        error       = error;
        math        = math;
        table       = table;
        string      = string;

        _GG         = _G; -- Reference to the global environment
        _state_name = 'none'
    }

    creature._G = creature

    setmetatable(creature, {
        __tostring = function(self)
            local x, y  = get_pos(self.id)
            local states = { [0]="idle",   [1]="walk",    [2]="heal",  [3]="eat",
                             [4]="attack", [5]="convert", [6]="spawn", [7]="feed"}
            return "<creature " .. self.id .." [" .. x .. "," .. y .."] " .. 
                    "type " .. get_type(self.id) ..", health " .. get_health(self.id) .. ", " ..
                    "food " .. get_food(self.id) .. ", state " .. states[get_state(self.id)]  .. ", handler " .. self._state_name .. ">"
        end, 
        __concat = function (op1, op2)
            return tostring(op1) .. tostring(op2)
        end
    })

    setfenv(setupCreature, creature)()
    return creature
end

------------------------------------------------------------------------
-- Callbacks von C
------------------------------------------------------------------------

function this_function_call_fails_if_cpu_limit_exceeded() end

-- global table containing all your creatures
creatures = creatures or {}

function player_think(events)
    can_yield = false

    -- Handle the events since the last round.
    for n, event in ipairs(events) do 
        if event.type == CREATURE_SPAWNED then
            local id     = event.id
            local parent = event.parent ~= -1 and event.parent or nil
            local creature = createCreature(id, parent)
            creatures[id]  = creature
            creature.restart()
            creatures[id].execute_event("onSpawned", parent)
        elseif event.type == CREATURE_KILLED then
            local id     = event.id
            local killer = event.killer ~= -1 and event.killer or nil
            assert(creatures[id])
            creatures[id].execute_event("onKilled", killer)
            creatures[id] = nil
        elseif event.type == CREATURE_ATTACKED then
            local id       = event.id
            local attacker = event.attacker
            creatures[id].execute_event("onAttacked", attacker)
        elseif event.type == PLAYER_CREATED then
            -- TODO
        else
            error("invalid event " .. event.type)
        end
    end

    can_yield = true

    -- Iterate all creatures and execute their code by resuming the creatures coroutine
    for id, creature in pairs(creatures) do
        if type(creature.thread) ~= 'thread' then
            creature.message = 'uuh. self.thread is not a coroutine.'
        elseif coroutine.status(creature.thread) == 'dead' then
            if not creature.message then 
                creature.message = 'main() terminated (maybe it was killed for using too much cpu/memory?)'
            end
        else
            creature.execute_event("onTick")
            if get_tile_food(creature.id) > 0 then 
                creature.execute_event("onTileFood")
            end
            if get_health(creature.id) < 5 then
                creature.execute_event("onLowHealth")
            end
            local ok, msg = coroutine.resume(creature.thread)
            if not ok then
                creature.message = msg
                -- If the coroutine was interrupted for using too much
                -- CPU, the following function call will abort the
                -- player_think function (since no more function calls
                -- are possible at this point). The code that caused 
                -- too much CPU can be seen by inspecting the traceback 
                -- that was saved in creature.message. Use 'i' in the client.
                this_function_call_fails_if_cpu_limit_exceeded()
            end
        end
    end

    can_yield = false
end
