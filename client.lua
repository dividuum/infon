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

require "clientlib.lua"

-----------------------------------------------------------
-- Clientlogik
-----------------------------------------------------------

function Client:joinmenu()
    if self.last_join and self.last_join + 1000 > game_time() then
        self:writeln("joining too fast. please wait...")
        return
    end
    self:writeln("------------------------------")
    local numplayers = 0
    for n = 0, MAXPLAYERS - 1 do 
        ok, name = pcall(get_player_name, n)
        if ok then
            numplayers = numplayers + 1
            self:writeln(string.format("%3d - %s", n, name))
        end
    end
    if numplayers == 0 then
        self:writeln("no one here")
    end
    self:writeln("------------------------------")

    local playerno
    local password = nil
    if numplayers == 0 then 
        playerno = ""
    else
        self:write("enter <playernum> or press enter for new player: ")
        playerno = self:readln()
    end
    if playerno == "" then
        self:write("password for new player: ")
        password = self:readln()
        if password == "" then
            self:writeln("password empty? aborting")
            return
        end
        playerno = create_player(password)
        if playerno == nil then 
            self:writeln("could not join: full?")
            return
        end
    else
        self:write("password for player " .. playerno .. ": ")
        password = self:readln()
    end
        
    local errormsg = self:attach_to_player(playerno, password)
    if errormsg then 
        self:writeln("could not attach to player " .. playerno .. ": " .. errormsg)
    else
        self:writeln("joined. player " .. playerno .. " has now " .. player_num_clients(playerno) .. " client(s)")
        self.last_join = game_time()
    end
end

function Client:partmenu() 
    if self:get_player() then
        self:detach()
        self:writeln("detached")
    else
        self:writeln("not attached")
    end
end

function Client:namemenu() 
    self:write("Player Name: ")
    if not self:set_player_name(self:readln()) then
        self:writeln("cannot set name")
    end
end
 
function Client:luamenu() 
    if not self:get_player() then
        self:writeln("must join first")
        return
    end
    while true do 
        self:write("lua> ")
        local line = self:readln()
        if line == "" then 
            break
        else
            self:execute(line)
        end
    end
end

function Client:batchmenu()
    if not self:get_player() then
        self:writeln("must join first")
        return
    end
    self:writeln("enter your lua code. '.' ends input")
    local code = ""
    while true do 
        local input = self:readln()
        if input == "." then
            break
        else
            code = code .. input .. "\n"
        end
    end
    self:execute(code)
end

 
function Client:shell()
    if debugpass == "" then
        self:writeln("password must be set in config.lua")
        return
    end
    local ok = false
    if self.authorized then 
        ok = true
    else
        self:write("password: ")
        ok = self:readln() == debugpass 
    end
        
    if ok then 
        self.authorized = true
        while true do
            self:write("debug("..self.fd..")> ")
            local code = self:readln() 
            if code == "" then 
                break
            end
            local ok, msg = xpcall(loadstring(code), _TRACEBACK)
            if not ok then 
                self:writeln(msg)
            end
        end
    else
        self:writeln("go away!")
    end
end

function Client:mainmenu()
    local prompt = "> "
    while true do 
        self:write(prompt)
        local input = self:readln()
        if input == "q" then
            self:disconnect() 
            break
        elseif input == "j" then
            self:joinmenu() 
        elseif input == "p" then
            self:partmenu() 
        elseif input == "l" then
            self:luamenu() 
        elseif input == "b" then
            self:batchmenu() 
        elseif input == "n" then
            self:namemenu() 
        elseif input == "r" then
            self:execute("restart()")
        elseif input == "i" then
            self:execute("info()")
     -- elseif input == "x" then
     --     self:attach_to_player(create_player("bla"), "bla")
        elseif input == "k" then
            self:write("kill all your creatures? [y/N] ")
            if self:readln() == "y" then
                self:kill()
            end
        elseif input == "?" then
            self:writeln("-------------------------------------------------")
            self:writeln(GAME_NAME)
            self:writeln("visit http://infon.dividuum.de/ for documentation")
            self:writeln("-------------------------------------------------")
            if not self:get_player() then
                self:writeln("j - oin game")
            else
                self:writeln("n - ame")
                self:writeln("p - art game")
                self:writeln("l - ua shell")
                self:writeln("b - atch. enter bunch of lua code")
                self:writeln("r - estart all your creatures")
                self:writeln("i - nformation on your creatures")
                self:writeln("k - ill me")
            end
            self:writeln("q - uit")
            self:writeln("-------------------------------------------------")
        elseif input == "prompt" then
            self:write("new prompt: ")
            prompt = self:readln()
        elseif input == "shell" then
            self:shell()
        elseif input == "" then
            -- nix
        else
            self:writeln("huh? use '?' for help")
        end
    end
end

function Client:handler()
    self:writeln("Welcome to " .. GAME_NAME .. ". Press <enter>")
    binary = self:readln()
    if binary == "guiclient" then
        self:turn_into_guiclient()
        return
    end
    self:writeln("")
    self:writeln("")
    self:writeln("                      Hello " .. self.addr .. "!")
    self:writeln("")
    --                  von http://www.asciiworld.com/animals_birds.html
    self:writeln("                                     \\")
    self:writeln("                     .___             `.")
    self:writeln("        ___              `~\\           |               \\")
    self:writeln("      o~   `.               |         /'                |")
    self:writeln("    .----._ `|             ,'       /'              _./'")
    self:writeln("    `o     `\\|___       __,|----'~~~~T-----,__  _,'~")
    self:writeln("          /~~o   `~>-/|~ '   ' ,   '      '   ~~\\_")
    self:writeln("         |_      <~   |   ' ,   ' '   '  ' , '     \\")
    self:writeln("           `-...-'~\./' '     '     '   '   '  , '  >")
    self:writeln("                     `-, __'  ,  '  '  , ' ,   '_,'-'")
    self:writeln("                       /'   `~~~~~~~|`--------~~\\")
    self:writeln("                     /'            ,'            `.")
    self:writeln("              ~~`---'             /               |")
    self:writeln("                               ,-'              _/'")
    self:writeln("")
    self:writeln("")
    self:writeln("enter '?' for help")
    self:mainmenu()
end


function ServerMain()
    require 'level.lua'
    add_to_scroller("Welcome!")
    add_to_scroller(join_info)
    food_spawner = {}
    for s = 0, 10 do
        local dx, dy = find_digged()
        food_spawner[s] = { x = dx,
                            y = dy,
                            r = math.random(3),
                            a = math.random(100) + 30,
                            i = math.random(1000) + 1000,
                            n = game_info() }
        world_add_food(food_spawner[s].x, 
                       food_spawner[s].y, 
                       10000)
    end
                        

    info_time = game_info()
    while true do
        coroutine.yield()

        if game_info() > info_time + 10000 then
           info_time = game_info() 
           if join_info ~= "" then
               add_to_scroller(join_info)
           end
        end

        for n, spawner in food_spawner do
            if game_info() > spawner.n then 
                world_add_food(spawner.x + math.random(spawner.r * 2 + 1) - spawner.r,
                               spawner.y + math.random(spawner.r * 2 + 1) - spawner.r,
                               spawner.a)
                spawner.n = spawner.n + spawner.i                               
            end
        end
    end
end

