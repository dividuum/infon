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

-----------------------------------------------------------
-- Konfiguration laden
-----------------------------------------------------------

require 'config'

stats  = {
    num_clients     = 0;
    num_refused     = 0;
    num_players     = 0;
    num_maps        = 0;
    num_exec        = 0;
    start_time      = os.time();
}

-----------------------------------------------------------
-- Klasse fuer Clientverbindung
-----------------------------------------------------------

clients= {}

Client = {}

function Client.create(fd, addr)
    stats.num_clients = stats.num_clients + 1

    local obj = {}
    setmetatable(obj, {
        __index = function(self, what)
            -- Nach Klassenmethode suchen
            if Client[what] then
                return Client[what]
            end
            -- Nix? Dann per default in Objekttabelle suchen
        end
    })
    clients[fd] = obj
    obj.fd      = fd
    obj:on_new_client(addr)
end

function Client.check_accept(addr) 
    for n, rule in ipairs(acl) do
        if rule.time and rule.time < os.time() then
            table.remove(acl, n)
        elseif string.match(addr, rule.pattern) then
            if rule.deny then
                stats.num_refused = stats.num_refused + 1
                return false, "refusing connection " .. addr .. ": " .. rule.deny .. "\r\n"
            else
                return true
            end
        end
    end
    return true
end

function Client:readln()
    return coroutine.yield(self.thread)
end

paste = 0

function Client:nextpaste()
    paste = paste + 1
    return paste
end

function Client:paste_name(num) 
    return num
end

function Client:paste_full_name(num) 
    return "paste " .. self:paste_name(num) .. " from client " .. self.fd
end

function Client:kick_ban(reason, time)
    local addr = self.addr:match("^([^:]+:[^:]+:?)")
    local time = time and os.time() + time or nil
    table.insert(acl, 1, {
        pattern = "^" .. addr .. ".*$",
        deny    = reason,
        time    = time
    })
    self:disconnect(reason)
end

function Client:on_new_client(addr) 
    self.addr            = addr
    self.local_output    = true
    self.prompt          = "> "
    self.failed_shell    = 0
    self.forward_unknown = false
    self.highlevel       = highlevel[1]
    self.last_action     = {}
    print(self.addr .. " accepted")
    scroller_add(self.addr .. " joined")
    self:start_thread()
end

function Client:start_thread() 
    self.thread = coroutine.create(self.handler)
    local ok, msg = coroutine.resume(self.thread, self)
    if not ok then
        self:disconnect(msg)
    end
end

function Client:on_destroy(reason)
    print(self.addr .. " closed: " .. reason)
    scroller_add(self.addr .. " disconnected: " .. reason)
end

function Client:on_input(line)
    local ok, msg = coroutine.resume(self.thread, line)
    if not ok then
        self:writeln(msg)
        self:writeln("restarting mainloop...")
        self:start_thread()
    end
end

function Client:disconnect(reason)
    client_disconnect(self.fd, reason)
end

function Client:write(data) 
    client_write(self.fd, data)
end

function Client:turn_into_guiclient()
    client_make_guiclient(self.fd)
end

function Client:is_guiclient()
    return client_is_gui_client(self.fd)
end

function Client:attach_to_player(playerno, pass)
    local ok, ret = pcall(client_attach_to_player, self.fd, playerno, pass)
    if not ok then
        return ret
    elseif not ret then
        return "attach failed. password wrong?"
    else
        return nil
    end
end

function Client:detach()
    local playerno = self:get_player()
    if playerno then
        client_detach_from_player(self.fd, playerno)
    else
        self:writeln("you cannot detach since you are not attached")
    end
end

function Client:get_player()
    return client_player_number(self.fd)
end

function Client:set_name(name)
    local playerno = self:get_player()
    if playerno and player_get_name(playerno) ~= name and name ~= '' then
        local oldname = player_get_name(playerno)
        player_set_name(playerno, name)
        local newname = player_get_name(playerno)
        scroller_add(oldname .. " renamed to " .. newname)
        return true
    else
        return false
    end
end

function Client:set_color(color)
    local playerno = self:get_player()
    if playerno then
        player_set_color(playerno, color)
    else
        self:writeln("you have no player!")
    end
end

function Client:kill()
    if self:get_player() then
        player_kill(self:get_player())
    else
        self:writeln("no need to kill. you have no player")
    end
end

function Client:execute(code, name)
    if self:get_player() then
        stats.num_exec = stats.num_exec + 1
        client_execute(self.fd, code, self.local_output, name)
    else
        self:writeln("no player to execute the code. sorry")
    end
end

function Client:writeln(line)
    if line then 
        self:write(line .. "\r\n")
    else
        self:write("\r\n")
    end
end

function Client:rate_limit(action, every)
    local time, last = game_time(), self.last_action[action]
    if not last or time < last or time > last + every then 
        self.last_action[action] = game_time()
        return true
    else
        self:writeln(string.format("%s too fast. please wait %.1fs...", 
                                   action, (every - (time - last)) / 1000))
        return false
    end
end

function Client:centerln(line)
    self:writeln(string.rep(" ", (60 - string.len(line)) / 2) .. line)
end

-- Speziell formatiert, so dass es von einem GUI Client
-- wie andere in packet.h definierte Packete gelesen
-- gelesen werden kann (len = 32 (space), type = 32 (space).
function Client:welcome(msg)
    local msglen = string.len(msg)
    if msglen > 28 then
        print('welcome message too long')
        msg = "Press <enter>"
        msglen = string.len(msg)
    end
    self:write("  \r\n" .. msg .. string.rep(" ", 28 - msglen) .. "\r\n")
end

function Client.writeAll(line)
    table.foreach(clients, function (fd, obj)
                               obj:write(line)
                           end)
end

function Client.writelnAll(line)
    table.foreach(clients, function (fd, obj)
                               obj:writeln(line)
                           end)
end

wall = Client.writelnAll

-----------------------------------------------------------
-- Server C Callbacks
-----------------------------------------------------------

function on_new_client(addr) 
    return Client.check_accept(addr)
end

function on_client_accepted(fd, addr) 
    Client.create(fd, addr)
end


function on_client_input(fd, line)
    clients[fd]:on_input(line)
end

function on_client_close(fd, reason)
    clients[fd]:on_destroy(reason)
    clients[fd] = nil
end

function server_new_game()
    server_tick = coroutine.wrap(ServerMain)
end

function server_tick()
end

-----------------------------------------------------------
-- Game C Callbacks
-----------------------------------------------------------

function onNewGameStarted()
    if type(demo) == "string" then
        server_start_demo(string.format("%s-%08X.demo", demo, os.time()));
    elseif type(demo) == "function" then
        server_start_demo(demo())
    end
end

-----------------------------------------------------------
-- World Funktionen
-----------------------------------------------------------

current_map = 1
map         = maps[current_map]

function world_rotate_map()
    current_map = current_map + 1
    if current_map > #maps then
       current_map = 1
    end
    map = maps[current_map]
end

function world_load(map)
    local world_code = assert(loadfile("level/" .. map .. ".lua"))

    -- prepare safe world environment
    world = {
        -- lua functions
        print             = print;
        pairs             = pairs;
        math              = { random = math.random };
        string            = { upper  = string.upper;
                              sub    = string.sub;
                              len    = string.len; };
        table             = { getn   = table.getn  };                                
        
        -- game functions
        world_dig         = world_dig;
        world_find_digged = world_find_digged;
        world_add_food    = world_add_food;
        game_info         = game_info;
  
        -- game constants                              
        TILE_SOLID        = TILE_SOLID;
        TILE_PLAIN        = TILE_PLAIN;
        TILE_WATER        = TILE_WATER;
    }

    -- activate environment for world code and load world
    setfenv(world_code, world)()

    local w,  h  = world.level_size()
    local kx, ky = world.level_koth_pos()
    return w, h, kx, ky
end

function world_init()
    stats.num_maps = stats.num_maps + 1
    local ok, w, h, kx, ky = pcall(world_load, map)
    if not ok then
        print("cannot load world '" .. map .. "': " .. w .. ". using 3x3 dummy world")
        world_tick = coroutine.wrap(function () while true do coroutine.yield() end end)
        return 3, 3, 1, 1
    else
        world_tick = coroutine.wrap(world_main)
        return w, h, kx, ky
    end
end

function world_main()
    local ok, ret = pcall(world.level_init)
    if not ok then
        print("initializing world failed: " .. ret)
    end
    while true do
        ok, ret = pcall(world.level_tick)
        if not ok then
            print("calling level_tick failed: " .. ret)
        end
        coroutine.yield()
    end
end

function world_add_food_by_worldcoord(x, y, amount)
    return world_add_food(x / TILE_WIDTH, y / TILE_HEIGHT, amount)
end

function world_find_digged_worldcoord()
    local x, y = world_find_digged()
    return x * TILE_WIDTH  + TILE_WIDTH  / 2, 
           y * TILE_HEIGHT + TILE_HEIGHT / 2
end

-----------------------------------------------------------
-- Rules Funktionen
-----------------------------------------------------------

function rules_init()
    dofile("rules/" .. rules .. ".lua")
end

-----------------------------------------------------------
-- Praktisches?
-----------------------------------------------------------

function isnumber(var)
    return tostring(tonumber(var)) == var
end

function kick(fd, msg)
    msg = msg or "kicked"
    clients[fd]:disconnect(msg)
end

function kickban(fd, msg)
    msg = msg or "kickbanned"
    clients[fd]:kick_ban(msg)
end

function killall()
    for n = 0, MAXPLAYERS - 1 do
        pcall(player_kill_all_creatures, n)
    end
end

function reset()
    killall()
    for n = 0, MAXPLAYERS - 1 do
        pcall(player_set_score, n, 0)
    end
    scroller_add("About to restart on map " .. map)
    wall("About to restart on map " .. map)
    game_end()
end

function kickall()
    table.foreach(clients, function (fd, client)
        clients[fd]:disconnect("kicked")
    end)
end

function p(x) 
    if type(x) == "table" then
        cprint("+--- Table: " .. tostring(x))
        for key, val in pairs(x) do
            cprint("| " .. tostring(key) .. " " .. tostring(val))
        end
        cprint("+-----------------------")
    else
        cprint(type(x) .. " - " .. tostring(x))
    end
end

function clientlist()
    table.foreach(clients, function (fd, client)
        cprint(string.format("%4d - %s", fd, client.addr))
    end)
end


function acllist()
    for n, rule in ipairs(acl) do
        cprint(string.format("%2d: %-30s - %9d - %s", n, rule.pattern, rule.time and rule.time - os.time() or -1, rule.deny or "accept"))
    end
end

-----------------------------------------------------------
-- Clienthandler laden
-----------------------------------------------------------

require "server"
