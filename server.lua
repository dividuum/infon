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
    if not self:rate_limit("joining", 3000) then return end

    self:writeln("------------------------------")
    local numplayers = 0
    for pno in each_player() do 
        numplayers = numplayers + 1
        self:writeln(string.format("%3d - %s", pno, player_get_name(pno)))
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
            self:writeln("you cannot join with an empty password.")
            return
        end

        if config.disable_joining then
            self:writeln("joining is currently disabled: " .. config.disable_joining)
            return
        end
    
        playerno = player_create(nil, password, self.highlevel)
        if playerno == nil then 
            self:writeln("cannot join: full?")
            return
        end

        stats.num_players = stats.num_players + 1
    else
        if not isnumber(playerno) then
            self:writeln("the player number must be numeric!")
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
    end
end

function Client:partmenu() 
    local playerno = self:get_player()
    local disconnect_time = player_get_no_client_kick_time(playerno)
    local warning  = player_num_clients(playerno) == 1 and disconnect_time ~= 0 and 
                         string.format(" reattach within %d seconds or your player will be killed.", 
                                       disconnect_time / 1000) or ""
    self:detach()
    self:writeln("detached from player " .. playerno .. "." .. warning)
end

function Client:namemenu() 
    if not self:rate_limit("changing name", 1000) then return end

    self:write("Player Name: ")
    if not self:set_name(self:readln()) then
        self:writeln("cannot set name")
    end
end

function Client:colormenu() 
    if not self:rate_limit("changing color", 1000) then return end

    self:write("color (0 - 255): ")
    local color = self:readln()
    if not isnumber(color) then
        self:writeln("not numeric")
    else
        self:set_color(color)
    end
end
 
function Client:luamenu() 
    while true do 
        local paste = self:nextpaste()
        self:write("lua(" .. self:paste_name(paste) .. ")> ")
        local line = self:readln()
        if line == "" then 
            break
        else
            self:execute(line, self:paste_full_name(paste))
        end
    end
end

function Client:batchmenu()
    local paste = self:nextpaste()
    self:writeln("enter your lua code for paste " .. self:paste_name(paste) .. ". '.' ends input.")
    local code = ""
    while true do 
        local input = self:readln()
        if input == "." then
            break
        else
            code = code .. input .. "\n"
        end
        if #code > 262144 then
            self:writeln("your code is too large.")
            return
        end
    end
    self:execute(code, self:paste_full_name(paste))
end

function Client:hexbatchmenu()
    local paste = self:nextpaste()
    self:writeln("enter your hex-encoded lua code for paste " .. self:paste_name(paste) .. ". '.' ends input")
    local code = ""
    while true do 
        local input = self:readln()
        if input == "." then
            break
        else
            code = code .. input
        end
        if #code > 262144 then
            self:writeln("your code is too large.")
            return
        end
    end
    self:execute(hex_decode(code), self:paste_full_name(paste))
end

function Client:killmenu() 
    self:write("kill all your creatures? [y/N] ")
    if self:readln() == "y" then
        self:kill()
    end
end

function Client:highmenu() 
    self:writeln("------------------------------")
    for n, file in ipairs(config.highlevel) do
        self:writeln(string.format("%3d - %-s", n, file))
    end
    self:writeln("------------------------------")
    self:write("choose your api: ")
    local num = self:readln()
    if not isnumber(num) then
        self:writeln("not numeric")
        return
    end
    local api = config.highlevel[tonumber(num)]
    if not api then
        self:writeln("no highlevel api " .. num)
    else
        self.highlevel = api
        self:writeln("highlevel api set to '" .. api .. "'")
    end
end

function Client:shell()
    local ok = false
    if self.authorized then 
        ok = true
    else
        if not config.debugpass or config.debugpass == "" then
            self:writeln("password must be set in config.lua")
            return
        end
        if not self:rate_limit("entering the shell", 5000) then return end
        self:write("password: ")
        ok = self:readln() == config.debugpass 
    end
        
    if ok then 
        self.authorized = true
        while true do
            self:write("admin("..self.fd..")> ")
            local code = self:readln() 
            if code == "" then 
                break
            elseif code == "?" then
                admin_help()
            else
                local chunk, msg = loadstring(code, "input from client '" .. self.addr .. "'")
                if not chunk then
                    self:writeln(msg .. ". use '?' for help")
                else
                    local ok, msg = xpcall(chunk, debug.traceback)
                    if not ok then 
                        self:writeln(msg)
                    end
                end
            end
        end
    else
        self.failed_shell = self.failed_shell + 1
        if     self.failed_shell  > 5 then 
            self:kick_ban("you tried to hack the shell. banning 5 minutes.", 5 * 60)
        elseif self.failed_shell == 5 then
            self:writeln("don't try again or you will be banned!")
        else
            self:writeln("go away!")
        end
    end
end

function Client:showscores()
    local players = {}
    for pno in each_player() do 
        table.insert(players, {
            num         = pno,
            clients     = player_num_clients(pno),
            name        = player_get_name(pno),
            score       = player_score(pno),
            creatures   = player_num_creatures(pno),
            age         = (game_time() - player_spawntime(pno)) / 1000 / 60,
            mem         = player_get_used_mem(pno),
            cpu         = player_get_used_cpu(pno)
        })
    end
    table.sort(players, function (a,b) 
        return a.score > b.score
    end)
    self:writeln("Scores | Creatures | Time |     Mem | CPU | #Cl | No | Name")
    self:writeln("-------+-----------+------+---------+-----+-----+----+-------------")
    for i,player in ipairs(players) do 
        self:writeln(string.format("%s%5d | %9d | %4dm| %7d | %3d%%| %3d | %2d | %s",
                                   self:get_player() == player.num and "*" or " ",
                                   player.score,
                                   player.creatures,
                                   player.age,
                                   player.mem,
                                   player.cpu,
                                   player.clients,
                                   player.num,
                                   player.name))
    end
    self:writeln("-------+-----------+------+---------+-----+-----+----+-------------")
end

function Client:info()
    self:writeln("-------------------------------------------------")
    self:writeln("Server Information")
    self:writeln("-------------------+-----------------------------")
    self:writeln("version            | " .. GAME_NAME)
    self:writeln("uptime             | " .. string.format("%ds", real_time() / 1000))
    self:writeln("cpu usage          | " .. os.clock() .. "s")
    self:writeln("memory             | " .. string.format("%d", collectgarbage("count")) .. "kb")
    self:writeln("traffic            | " .. server_get_traffic())
    self:writeln("-------------------+------------------------------")
    self:writeln("accepted clients   | " .. stats.num_clients)
    self:writeln("refused clients    | " .. stats.num_refused)
    self:writeln("joined players     | " .. stats.num_players)
    self:writeln("played maps        | " .. stats.num_maps)
    self:writeln("code executions    | " .. stats.num_exec)
    self:writeln("-------------------+------------------------------")
    local w, h = world.level_size()
    self:writeln("rules              | " .. config.rules)
    self:writeln("map                | " .. map .. " (" .. w .. "x" .. h .. ")")
    self:writeln("game time          | " .. string.format("%ds", game_time() / 1000))
    self:writeln("time limit         | " .. (config.time_limit and string.format("%ds", config.time_limit / 1000) or "none"))
    self:writeln("score limit        | " .. (config.score_limit or "none"))
    self:writeln("-------------------------------------------------")
end

function Client:menu_header()
    self:writeln("-------------------------------------------------")
    self:writeln(GAME_NAME)
    self:writeln("visit http://infon.dividuum.de/ for documentation")
    self:writeln("-------------------------------------------------")
end

function Client:menu_footer()
    self:writeln("-------------------------------------------------")
end

function Client:mainmenu()
    while true do 
        self:write(self.prompt)
        local input = self:readln()

        if input == "q" then
            self:disconnect("quitting") 
            break
        elseif input == "s" then
            self:showscores() 
        elseif input == "prompt" then
            self:write("new prompt: ")
            self.prompt = self:readln()
        elseif input == "shell" then
            self:shell()
        elseif input == "info" then
            self:info()
        elseif input == "" then
            -- nix
        elseif self:get_player() then
            if     input == "p" then
                self:partmenu() 
            elseif input == "l" then
                self:luamenu() 
            elseif input == "c" then
                self:colormenu() 
            elseif input == "b" then
                self:batchmenu() 
            elseif input == "bb" then
                self:hexbatchmenu() 
            elseif input == "n" then
                self:namemenu() 
            elseif input == "r" then
                self:execute("restart()", "restart")
            elseif input == "i" then
                self:execute("info()",    "info")
            elseif input:match("^[0-9]$") then
                self:execute("onInput" .. input .. "()", "calling onInput" .. input)
            elseif input == "k" then
                self:killmenu()
            elseif input == "lio" then
                self:write("limit interactive output to local connection? [Y/n] ")
                self.local_output = self:readln() ~= "n"
            elseif input == "lbo" then
                self:write("limit botcode output to this connection? [y/N] ")
                local bot_output_client = self:readln() == "y" and self.fd or nil
                player_set_output_client(self:get_player(), bot_output_client)
            elseif input == "fwd" then
                self:write("forward unknown commands to onCommand function? [y/N] ")
                self.forward_unknown = self:readln() == "y"
            elseif input == "?" then
                self:menu_header()
                self:writeln("n - ame")
                self:writeln("c - olor")
                self:writeln("p - art game")
                self:writeln("l - ua shell")
                self:writeln("b - atch. enter bunch of lua code")
                self:writeln("r - estart all your creatures")
                self:writeln("i - nformation on your creatures")
                self:writeln("k - ill me")
                self:writeln("s - how scores")
                self:writeln("q - uit                    use '??' for more help")
                self:menu_footer()
            elseif input == "??" then
                self:menu_header()
                self:writeln("lbo    - limit bot output")
                self:writeln("lio    - limit interactive output")
                self:writeln("fwd    - set unknown command forward")
                self:writeln("prompt - change prompt")
                self:writeln("bb     - hex batch (load precompiled code)")
                self:writeln("0 - 9  - execute onInputX()")
                self:writeln("info   - server information")
                self:menu_footer()
            elseif self.forward_unknown then
                self:execute("onCommand(" .. string.format("%q", input) .. ")", "onCommand")
            else
                self:writeln("huh? use '?' for help")
            end
        else
            if     input == "j" then
                self:joinmenu() 
            elseif input == "hl" then
                self:highmenu()
            elseif input == "?" then
                self:menu_header()
                if config.disable_joining then
                    self:writeln("j - oin game (currently disabled)")
                else
                    self:writeln("j - oin game")
                end
                self:writeln("s - how scores")
                self:writeln("q - uit                    use '??' for more help")
                self:menu_footer()
            elseif input == "??" then
                self:menu_header()
                self:writeln("hl     - choose highlevel api")
                self:writeln("info   - server information")
                self:menu_footer()
            else
                self:writeln("huh? use '?' for help")
            end
        end
    end
end

function Client:gui_mode()
    self:turn_into_guiclient()
    while true do 
        self:readln()
    end
end

function Client:telnet_mode()
    self:centerln("Hello " .. self.addr .. "!")
    self:centerln("Welcome to " .. GAME_NAME)
    self:writeln("")
    if config.banner then
        self:write(config.banner:match("\r\n") and config.banner or config.banner:gsub("\n", "\r\n"))
        self:writeln("")
    end
    self:writeln("enter '?' for help")
    self:mainmenu()
end

function Client:www_mode()
    --[[
    self:writeln()
    self:writeln("This is an infon game server, not a webserver.")
    self:writeln("You'll need a telnet client to access this game.")
    self:writeln("See http://infon.dividuum.de for more information")
    self:writeln()
    self:info()
    self:writeln()
    self:writeln()
    self:showscores() 
    self:writeln()
    self:writeln()
    ]]--
    self:disconnect("please use telnet")
end

function Client:handler()
    if self.addr == "special:console" then 
        self.authorized = true
        self:writeln("")
        self:telnet_mode()
    else
        self:welcome("Press <enter>")
        local mode = self:readln()
        if mode == "guiclient" then
            self:gui_mode()
        elseif mode:match("^GET") then
            self:www_mode()
        else
            self:telnet_mode()
        end
    end
end

function ServerMain()
    scroller_add("Welcome to " .. GAME_NAME .. "!")
                        
    local info_time = game_time()
    while true do
        if game_time() > info_time + 10000 then
           info_time = game_time() 
           if config.join_info and config.join_info ~= "" then
               scroller_add(config.join_info)
           end
        end
        coroutine.yield()
    end
end
