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

-----------------------------------------------------------
-- Klasse fuer Clientverbindung
-----------------------------------------------------------

clients= {}

Client = {}

function Client.create(fd, addr)
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
    obj.fd   = fd
    obj:on_new_client(addr)
end

function Client.check_accept(addr) 
    for n, rule in ipairs(acl) do
        if rule.time and rule.time < os.time() then
            table.remove(acl, n)
        elseif string.match(addr, rule.pattern) then
            if rule.deny then
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

function Client:pastename(num) 
    return "paste " .. num .. " from client " .. self.fd
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
    print(self.addr .. " accepted")
    scroller_add(self.addr .. " joined")
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
        self.thread = coroutine.create(self.handler)
        local ok, msg = coroutine.resume(self.thread, self)
        if not ok then
            self:disconnect(msg)
        end
    end
end

function Client:disconnect(reason)
    if not string.match(self.addr, "^special:") then 
        client_disconnect(self.fd, reason)
    end
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
    player_set_color(playerno, color)
end

function Client:kill()
    if self:get_player() then
        player_kill(self:get_player())
        return true
    else
        return false
    end
end

function Client:execute(code, name)
    client_execute(self.fd, code, self.local_output, name)
end

function Client:writeln(line)
    if line then 
        self:write(line .. "\r\n")
    else
        self:write("\r\n")
    end
end

function Client:check_repeat(name, time)
    if not self[name] or game_time() < self[name] or game_time() > self[name] + time then
        self[name] = game_time()
        return true
    else
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

function world_main()
    level_init()
    while true do
        level_tick()
        coroutine.yield()
    end
end

current_map = 1
map         = maps[current_map]

function world_rotate_map()
    current_map = current_map + 1
    if current_map > #maps then
       current_map = 1
    end
    map = maps[current_map]
end

function world_init()
    dofile("level/" .. map .. ".lua")
    local w,  h  = level_size()
    local kx, ky = level_koth_pos()
    world_tick = coroutine.wrap(world_main)
    return w, h, kx, ky
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
