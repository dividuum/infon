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
-- Clientlogik
-----------------------------------------------------------

function Client:joinmenu()
    if not self:check_repeat("last_join", 3000) then
        self:writeln("joining too fast. please wait...")
        return
    end
    self:writeln("------------------------------")
    local numplayers = 0
    for n = 0, MAXPLAYERS - 1 do 
        local ok, name = pcall(player_get_name, n)
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
        playerno = player_create(password)
        if playerno == nil then 
            self:writeln("could not join: full?")
            return
        end
    else
        if not isnumber(playerno) then
            self:writeln("playernum not numeric.")
            return
        end
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
    if not self:get_player() then
        self:writeln("must join first")
        return
    end

    if not self:check_repeat("last_name", 1000) then
        self:writeln("changing too fast. please wait")
        return
    end

    self:write("Player Name: ")
    if not self:set_name(self:readln()) then
        self:writeln("cannot set name")
    end
end

function Client:colormenu() 
    if not self:get_player() then
        self:writeln("must join first")
        return
    end

    if not self:check_repeat("last_color", 1000) then
        self:writeln("changing too fast. please wait")
        return
    end

    self:write("color (0 - 255): ")
    local color = self:readln()
    if not isnumber(color) then
        self:writeln("not numeric")
    else
        self:set_color(color)
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

function Client:killmenu() 
    if not self:get_player() then
        self:writeln("must join first")
        return
    end
    self:write("kill all your creatures? [y/N] ")
    if self:readln() == "y" then
        self:kill()
    end
end
 
function Client:shell()
    local ok = false
    if self.authorized then 
        ok = true
    else
        if debugpass == "" then
            self:writeln("password must be set in config.lua")
            return
        end
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
            local ok, msg = xpcall(loadstring(code), debug.traceback)
            if not ok then 
                self:writeln(msg)
            end
        end
    else
        self:writeln("go away!")
    end
end

function Client:showscores()
    local players = {}
    for n = 0, MAXPLAYERS - 1 do 
        if player_exists(n) then
            table.insert(players, {
                name        = player_get_name(n),
                score       = player_score(n),
                creatures   = player_num_creatures(n),
                age         = (game_time() - player_spawntime(n)) / 1000 / 60,
                mem         = player_get_used_mem(n),
                cpu         = player_get_used_cpu(n)
            })
        end
    end
    table.sort(players, function (a,b) 
        return a.score > b.score
    end)
    self:writeln("Scores | Creatures | Time |     Mem | CPU | Name")
    self:writeln("-------+-----------+------+---------+-----+-------------")
    for i,player in ipairs(players) do 
        self:writeln(string.format("%6d | %9d | %4dm| %7d | %3d%%| %s",
                                   player.score,
                                   player.creatures,
                                   player.age,
                                   player.mem,
                                   player.cpu,
                                   player.name))
    end
    self:writeln("-------+-----------+------+---------+-----+-------------")
end

function Client:mainmenu()
    local prompt = "> "
    while true do 
        self:write(prompt)
        local input = self:readln()
        if input == "q" then
            self:disconnect("quitting") 
            break
        elseif input == "j" then
            self:joinmenu() 
        elseif input == "p" then
            self:partmenu() 
        elseif input == "l" then
            self:luamenu() 
        elseif input == "c" then
            self:colormenu() 
        elseif input == "b" then
            self:batchmenu() 
        elseif input == "n" then
            self:namemenu() 
        elseif input == "s" then
            self:showscores() 
        elseif input == "r" then
            self:execute("restart()")
        elseif input == "i" then
            self:execute("info()")
        elseif input == "k" then
            self:killmenu()
        elseif input == "?" then
            self:writeln("-------------------------------------------------")
            self:writeln(GAME_NAME)
            self:writeln("visit http://infon.dividuum.de/ for documentation")
            self:writeln("-------------------------------------------------")
            if not self:get_player() then
                self:writeln("j - oin game")
            else
                self:writeln("n - ame")
                self:writeln("c - olor")
                self:writeln("p - art game")
                self:writeln("l - ua shell")
                self:writeln("b - atch. enter bunch of lua code")
                self:writeln("r - estart all your creatures")
                self:writeln("i - nformation on your creatures")
                self:writeln("k - ill me")
            end
            self:writeln("s - how scores")
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

function Client:gui_client_menu()
    while true do 
        self:readln()
    end
end

function Client:handler()
    if self.fd == 0 then -- XXX: HACK, stdin client
        self.authorized = true
        self:writeln("")
    else
        self:welcome("Press <enter>")
        if self:readln() == "guiclient" then
            self:turn_into_guiclient()
            self:gui_client_menu()
        end
    end
    self:centerln("Hello " .. self.addr .. "!")
    self:centerln("Welcome to " .. GAME_NAME)
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
    scroller_add("Welcome to " .. GAME_NAME .. "!")
    scroller_add(join_info)
                        
    local info_time = game_info()
    while true do
        if game_info() > info_time + 10000 then
           info_time = game_info() 
           if join_info ~= "" then
               scroller_add(join_info)
           end
        end
        coroutine.yield()
    end
end
