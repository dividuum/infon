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

argv   = {...}

config = setmetatable({}, {__index = _G})
setfenv(assert(loadfile(os.getenv("INFOND_CONFIG") or (PREFIX .. "config.lua"))), config)()

-- update some configuration values
config.servername = config.servername or 'Unnamed Server'
if config.nokickpass == "" then config.nokickpass = nil end
if config.debugpass  == "" then config.debugpass  = nil end

stats  = {
    num_clients     = 0;
    num_refused     = 0;
    num_players     = 0;
    num_maps        = 0;
    num_exec        = 0;
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
    for n, rule in ipairs(config.acl) do
        if rule.time and rule.time < os.time() then
            table.remove(config.acl, n)
        elseif string.match(addr, rule.pattern) then
            if rule.deny then
                stats.num_refused = stats.num_refused + 1
                return false, "infon refused your connection from " .. addr .. ": " .. rule.deny .. "\r\n"
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
    table.insert(config.acl, 1, {
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
    self.forward_unknown = true
    self.pre_string      = ""
    self.post_string     = ""
    self.highlevel       = config.highlevel[1]
    self.last_action     = {}
 -- print(self.addr .. " accepted")
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
 -- print(self.addr .. " closed: " .. reason)
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
    client_write(self.fd, self.pre_string .. data .. self.post_string)
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
        self:writeln("you cannot detach since you are not attached.")
    end
end

function Client:get_player()
    return client_player_number(self.fd)
end

function Client:check_name(name)
    if config.check_name_password and config.check_name_password(name, nil) then 
        self:write("Name '" .. name .. "' requires a password: ")
        return config.check_name_password(name, self:readln())
    else
        return true
    end
end

function Client:set_name(name)
    local playerno = self:get_player()
    if playerno and player_get_name(playerno) ~= name and name ~= '' then
        local oldname = player_get_name(playerno)
        if not self:check_name(name) then return false end
        player_set_name(playerno, name)
        local newname = player_get_name(playerno)
        scroller_add(oldname .. " renamed to " .. newname .. ".")
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
    local playerno = self:get_player()
    if playerno then
        player_kill(playerno, player_get_name(playerno) .. " removed from the game.")
    else
        self:writeln("no need to kill. you have no player!")
    end
end

function Client:execute(code, name)
    local playerno = self:get_player()
    if playerno then
        stats.num_exec = stats.num_exec + 1
        return player_execute(playerno, self.local_output and self.fd or nil, code, name)
    else
        self:writeln("you do not have a player. cannot execute your code.")
        return false
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
    local time, last = real_time(), self.last_action[action]
    if not last or last + every <= time then 
        self.last_action[action] = real_time()
        return true
    else
        self:writeln(string.format("%s too fast. please wait %d seconds...", 
                                   action, (every - (time - last))))
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

-----------------------------------------------------------
-- Game C Callbacks
-----------------------------------------------------------

function on_game_started()
    -- Demos starten?
    if type(config.demo) == "string" then
        server_start_writer(string.format("%s-%08X.demo", config.demo, os.time()), true, true);
    elseif type(config.demo) == "function" then
        server_start_writer(config.demo(), true, true)
    end

    -- Message
    scroller_add("Started map " .. map)
    -- wall("Started map " .. map)

    -- server_tick anlegen
    server_tick = coroutine.wrap(ServerMain)

    -- Handler fuer Dinge, welche nur beim ersten Spiel aufgerufen werden sollen
    if not first_game_init then
        first_game_init = true

        -- competition?
        if config.competition then
            set_realtime(false)
            competition_rounds     = #config.maps
            config.disable_joining = "competition mode"
            for _, bot in pairs(config.competition.bots) do
                appendline(config.competition.log, "joining '%s' as %d", bot.source, start_bot(bot))
            end
        end

        -- Nach dem Starten des ersten Spiels einmalig Funktion autoexec aufrufen
        if config.autoexec then 
            local ok, msg = epcall(debug.traceback, config.autoexec) 
            if not ok then 
                print(msg)
            end
        end
    end

    -- Mapchange
    if config.competition then
        appendline(config.competition.log, "%d starting on map '%s'", os.time(), map)
    end
end

function on_game_ended()
    -- competition?
    if config.competition then
        for pno in each_player() do 
            appendline(config.competition.log, "%d: score %d, creatures %d", pno, player_score(pno), player_num_creatures(pno))
        end
        appendline(config.competition.log, "%d completed", os.time())
        competition_rounds = competition_rounds - 1
        if competition_rounds == 0 then 
            shutdown()
        end
    end

    -- kill all creatures
    killall()

    -- reset score
    for pno in each_player() do 
        player_set_score(pno, 0)
    end
end

------------------------------------------------------------------------
-- Creature Config Access
------------------------------------------------------------------------

creature_config = setmetatable({}, {
    __index = function (t, val) 
        return creature_get_config(val)
    end,
    __newindex = function (t, name, val)
        return creature_set_config(name, val)
    end
})

-----------------------------------------------------------
-- World Funktionen
-----------------------------------------------------------

current_map = 1
map         = config.maps[current_map]

function world_rotate_map()
    current_map = current_map + 1
    if current_map > #config.maps then
       current_map = 1
    end
    map = config.maps[current_map]
end

function world_get_dummy()
    return {
        level_size        = function () return 3,3 end;
        level_koth_pos    = function () return 1,1 end;
        level_init        = function () end;
        level_tick        = function () end;
        level_spawn_point = function () end;
    }
end

function world_load(map)
    local world_code = assert(loadfile(PREFIX .. "level/" .. map .. ".lua"))

    -- prepare safe world environment
    world = {
        -- lua functions
        print             = print;
        pairs             = pairs;
        ipairs            = ipairs;
        unpack            = unpack;
        math              = { random = math.random;
                              sqrt   = math.sqrt;
                              floor  = math.floor  };
        string            = { upper  = string.upper;
                              sub    = string.sub;
                              len    = string.len; };
        table             = { getn   = table.getn  };                                
        
        -- game functions
        world_dig         = world_dig;
        world_set_type    = world_set_type;
        world_set_gfx     = world_set_gfx;
        world_get_type    = world_get_type;
        world_get_gfx     = world_get_gfx;
        world_find_digged = world_find_digged;
        world_add_food    = world_add_food;
        world_make_border = world_make_border;
        world_fill_all    = world_fill_all;
        world_tile_center = world_tile_center;
        game_time         = game_time;
        server_info       = server_info;

        level_spawn_point = world_find_digged_worldcoord;

        -- game constants                              
        TILE_WIDTH                 = TILE_WIDTH;
        TILE_HEIGHT                = TILE_HEIGHT;
        
        TILE_SOLID                 = TILE_SOLID;
        TILE_PLAIN                 = TILE_PLAIN;
        TILE_WATER                 = 2; -- compatibility

        TILE_GFX_SOLID             = TILE_GFX_SOLID;
        TILE_GFX_PLAIN             = TILE_GFX_PLAIN;
        TILE_GFX_BORDER            = TILE_GFX_BORDER;
        TILE_GFX_SNOW_SOLID        = TILE_GFX_SNOW_SOLID;
        TILE_GFX_SNOW_PLAIN        = TILE_GFX_SNOW_PLAIN;
        TILE_GFX_SNOW_BORDER       = TILE_GFX_SNOW_BORDER;
        TILE_GFX_WATER             = TILE_GFX_WATER;
        TILE_GFX_LAVA              = TILE_GFX_LAVA;
        TILE_GFX_NONE              = TILE_GFX_NONE;
        TILE_GFX_KOTH              = TILE_GFX_KOTH;
        TILE_GFX_DESERT            = TILE_GFX_DESERT;
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
        print("cannot load world '" .. map .. "': " .. w .. ". using dummy world")
        world  = world_get_dummy()
        w,  h  = world.level_size()
        kx, ky = world.level_koth_pos()
    end
    world_tick = coroutine.wrap(world_main)
    return w, h, kx, ky
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

function world_tile_center(x, y)
    return (x + 0.5) * TILE_WIDTH, (y + 0.5) * TILE_HEIGHT
end

function world_find_digged_worldcoord()
    local x, y = world_find_digged()
    if not x then return end
    return world_tile_center(x, y)
end

function world_get_spawn_point(player)
    return world.level_spawn_point(player)
end

-- compatibility function. do not use for new maps.
function world_dig(x, y, typ, gfx)
    gfx = gfx or typ
    if typ == world.TILE_WATER then -- water
        world_set_gfx(x, y, TILE_GFX_WATER)
        return
    end
    local w,  h  = world.level_size()
    local kx, ky = world.level_koth_pos()
    if x == kx and y == ky then
        gfx = TILE_GFX_KOTH
    end
    return world_set_type(x, y, typ) and world_set_gfx(x, y, gfx)
end

function world_fill_all(gfx)
    local w, h = world.level_size()
    for x = 0, w - 1 do
        for y = 0, h - 1 do
            world_set_gfx(x, y, gfx)
        end
    end
end

function world_make_border(gfx)
    local w, h = world.level_size()
    for x = 0, w-1 do
        world_set_gfx(x,     0, gfx)
        world_set_gfx(x, h - 1, gfx)
    end
    for y = 0, h-1 do
        world_set_gfx(0    , y, gfx)
        world_set_gfx(w - 1, y, gfx)
    end
end

-----------------------------------------------------------
-- Rules Funktionen
-----------------------------------------------------------

function rules_init()
    local rules_code = assert(loadfile(PREFIX .. "rules/" .. config.rules .. ".lua"))

    -- prepare safe rules environment
    rules = {
        -- lua functions
        print                       = print;
        
        -- game functions
        game_time                   = game_time;
        each_player                 = each_player;

        player_score                = player_score;
        player_get_name             = player_get_name;
        player_change_score         = player_change_score;
        
        creature_spawn              = creature_spawn;
        creature_set_food           = creature_set_food;
        creature_get_food           = creature_get_food;
        creature_get_pos            = creature_get_pos;
        creature_get_player         = creature_get_player;
        creature_get_type           = creature_get_type;

        world_get_spawn_point       = world_get_spawn_point;
        world_add_food_by_worldcoord= world_add_food_by_worldcoord;

        get_score_limit             = function () return config.score_limit end;
        get_time_limit              = function () return config.time_limit  end;
        scroller_add                = scroller_add;
        set_intermission            = set_intermission;
        world_rotate_map            = world_rotate_map;
        reset                       = reset;

        -- game constants                              
        CREATURE_SMALL              = CREATURE_SMALL;
        CREATURE_BIG                = CREATURE_BIG;
        CREATURE_FLYER              = CREATURE_FLYER;
    }

    -- activate environment for rules code and load rules
    setfenv(rules_code, rules)()
end

function rules_call(name, ...)
    assert(type(rules) == "table", "'rules' is not a table. cannot call rule handler '" .. name .. "'")
    assert(rules[name], "rule handler '" .. name .. "' not defined")
    rules[name](...)
end

-----------------------------------------------------------
-- Praktisches?
-----------------------------------------------------------

function isnumber(var)
    return tostring(tonumber(var)) == var
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

function admin_help()
    cprint("-------------------------------------------------")
    cprint("Admin functions")
    cprint("-------------------------------------------------")
    cprint("acllist()                 - lists acl")
    cprint("aclrm(num)                - removes acl rule")
    cprint("batch()                   - starts admin batch")
    cprint("changemap(map)            - changes to new map")
    cprint("clientlist()              - lists all clients")
    cprint("kick(cno, [msg])          - kicks client cno")
    cprint("kick_player(pno, [msg])   - kicks a player")
    cprint("kickall()                 - kicks all clients")
    cprint("kickban(cno, [msg], time) - kicks and bans")
    cprint("killall()                 - kills all creatures")
    cprint("reset()                   - restarts current map")
    cprint("shutdown()                - shutdown the server")
    cprint("wall(msg)                 - writes to all clients")
    cprint("-------------------------------------------------")
end

function appendline(filename, fmt, ...)
    if not filename then return end
    local file = assert(io.open(filename, "a+"))
    file:write(string.format(fmt .. "\n", ...))
    file:close()
end

function kick(fd, msg)
    clients[fd]:disconnect(msg or "kicked")
end

function kickban(fd, msg, time)
    clients[fd]:kick_ban(msg or "kickbanned", time)
end

function killall()
    for pno in each_player() do 
        player_kill_all_creatures(pno)
    end
end

function kick_player(pno, msg)
    player_kill(pno, msg or "kicked")
end

function reset()
    game_end()
end

function changemap(next_map)
    map = next_map
    reset()
end

function kickall(msg)
    for fd, client in pairs(clients) do 
        client:disconnect(msg or "kicked")
    end
end

function clientlist()
    for fd, client in pairs(clients) do 
        cprint(string.format("%4d %s - %s", fd, client_is_gui_client(fd) and "*" or " ", client.addr))
    end
end

function wall(msg)
    for fd, client in pairs(clients) do 
        client:writeln()
        client:writeln("-[ Message from Admin ]-------")
        client:writeln(msg)
        client:writeln("------------------------------")
    end
end

function batch()
    cprint("enter lua code. '.' ends input.")
    local code = ""
    while true do 
        local input = coroutine.yield() -- read next line
        if input == "." then
            break
        else
            code = code .. input .. "\n"
        end
    end
    assert(loadstring(code))()
end

function acllist()
    for n, rule in ipairs(config.acl) do
        cprint(string.format("%2d: %-30s - %9d - %s", n, rule.pattern, rule.time and rule.time - os.time() or -1, rule.deny or "accept"))
    end
end

function aclrm(num)
    assert(config.acl[num], "rule " .. num .. " does not exist")
    table.remove(config.acl, num)
end

function start_listener() 
    assert(setup_listener(config.listenaddr, config.listenport), 
           string.format("cannot setup listener on %s:%d", config.listenaddr, config.listenport))
end

function stop_listener()
    setup_listener("", 0)
end

-- Options:
-- source   - bot source code
-- log      - log file
-- api      - highlevel api
-- name     - bot name
-- password - password

function start_bot(options)
    assert(type(options) == "table", "start_bot needs an options table")
    local highlevel = options.api or config.highlevel[1]
    local password  = options.password or tostring(math.random(100000, 999999))
    local botfile   = assert(io.open(options.source, "rb"))
    local botsource = botfile:read("*a")
    botfile:close()
    local name = options.name or select(3, options.source:find("([^/\\]+)\.lua")) or options.source
    local playerno = player_create(name, password, highlevel)
    assert(playerno, "cannot create new player")
    cprint(string.format("player %d - %s (%s) joined with password '%s'", playerno, name, options.source, password))
    player_set_no_client_kick_time(playerno, 0)
    if options.log then
        local ok, logclient = pcall(server_start_writer, options.log, false, false)
        if ok then 
            client_attach_to_player(logclient, playerno, password)
        else
            cprint("cannot start log writer: " .. logclient)
        end
    end
    assert(player_execute(playerno, nil, botsource, options.source), "cannot load bot code " .. options.source)
    return playerno
end

-----------------------------------------------------------
-- Load hint file
-----------------------------------------------------------

hints = dofile(PREFIX .. "hints.lua") or {}

-----------------------------------------------------------
-- Clienthandler laden
-----------------------------------------------------------

dofile(PREFIX .. "server.lua")

-- setup listen socket
if config.listenaddr and config.listenport then start_listener() end 

if PLATFORM == "win32" then
    print("\n")
    print(config.banner:match("\r\n") and config.banner or config.banner:gsub("\n", "\r\n"))
    print [[

          The Infon Server is now running. Have fun!

           To terminate it, just close this window.
]]
end
