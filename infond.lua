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

require 'config.lua'

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
    return true
end

function Client:readln()
    return coroutine.yield(self.thread)
end

function Client:on_new_client(addr) 
    self.addr = addr
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
    if self.fd ~= 0 then
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

function Client:execute(code)
    client_execute(self.fd, code)
end

function Client:writeln(line)
    if line then 
        self:write(line .. "\n")
    else
        self:write("\n")
    end
end

function Client:check_repeat(name, time)
    if self[name] and self[name] + time > game_time() then
        return false
    else
       self[name] = game_time()
       return true
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
    if msglen > 30 then
        print('welcome message too long')
        msg = "Press <enter>"
        msglen = string.len(msg)
    end
    self:write("  \n" .. msg .. string.rep(" ", 30 - msglen) .. "\n")
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

function server_tick()
    server_tick = coroutine.wrap(ServerMain)
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

function p(x) 
    if type(x) == "table" then
        print("+--- Table: " .. tostring(x))
        for key, val in x do
            print("| " .. tostring(key) .. " " .. tostring(val))
        end
        print("+-----------------------")
    else
        print(type(x) .. " - " .. tostring(x))
    end
end

function isnumber(var)
    return tostring(tonumber(var)) == var
end

function clientlist(adminfd)
    table.foreach(clients, function (fd, obj)
                               client_write(adminfd, fd .. " - " .. obj.addr .. "\n")
                           end)
end

function kick(fd, msg)
    clients[fd]:disconnect(msg)
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
    table.foreach(clients, function (fd, obj)
                               clients[fd]:disconnect("kicked")
                           end)
end

-----------------------------------------------------------
-- Clienthandler laden
-----------------------------------------------------------

require "server.lua"
