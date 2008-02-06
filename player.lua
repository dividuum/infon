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
-- load config.lua
------------------------------------------------------------------------

local config = {}
setfenv(assert(loadfile(os.getenv("INFOND_CONFIG") or (PREFIX .. "config.lua"))), config)()

------------------------------------------------------------------------
-- save traceback in registry for usage within 'out of cycles' handler
------------------------------------------------------------------------

save_in_registry('traceback', debug.traceback)
save_in_registry = nil

-- save traceback function
_TRACEBACK = debug.traceback

------------------------------------------------------------------------
-- Prevent high cpu usage by limiting some functions
------------------------------------------------------------------------

do
    local assert, type = assert, type

    -- new dofile function uses defined PREFIX
    local PREFIX, orig_dofile = PREFIX, dofile
    function dofile(file)
        return orig_dofile(PREFIX .. file .. ".lua")
    end

    -- limit strings accepted by loadstring to 16k
    local orig_loadstring = loadstring
    function loadstring(code, name)
        assert(#code <= 16384, "code too large")
        return orig_loadstring(code, name)
    end

    -- limit string.rep
    local orig_string_rep = string.rep
    function string.rep(s, n)
        assert(n <= 10000, "string.rep's n is limited to 10000")
        return orig_string_rep(s, n)
    end

    -- provide thread tracing function
    local sethook, getinfo = debug.sethook, debug.getinfo
    function thread_trace(thread, text)
        assert(type(thread) == "thread", "arg #1 is not a thread")
        text = text or tostring(thread)
        local dumper = type(text) == "function" and text or function(info)
            print(text .. ":" .. info.source .. ":" .. info.currentline)
        end
        local hook = function(what, where)
            dumper(getinfo(2, "nlS"))
        end
        sethook(thread, hook, "l")
    end
    
    function thread_untrace(thread)
        assert(type(thread) == "thread", "arg #1 is not a thread")
        sethook(thread)
    end
end

------------------------------------------------------------------------
-- Load debugger if requested
------------------------------------------------------------------------

package.path = PREFIX .. "libs/?.lua"
if config.debugger then
    ldb = require("ldb")
end

------------------------------------------------------------------------
-- Disable dangerous functions
------------------------------------------------------------------------

PREFIX          = nil   -- no need to know
debug           = nil   -- no debugging functions
load            = nil   -- not needed
require         = nil   -- no file loading
loadfile        = nil   -- no file loading
_VERSION        = nil   -- who cares
newproxy        = nil   -- huh?
gcinfo          = nil   -- no access to the garbage collector
os              = nil   -- no os access
package         = nil   -- package support is not needed
io              = nil   -- disable io
module          = nil   -- module support not needed
collectgarbage  = nil   -- no access to the garbage collector

------------------------------------------------------------------------
-- Disable Linehook unless permitted
------------------------------------------------------------------------

if not config.linehook then 
    linehook = nil 
end

------------------------------------------------------------------------
-- 'pretty'-print function
------------------------------------------------------------------------

function p(...)
    dofile("pp", true)
    if pp then
        p = pp
    else
        p = function (x) 
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
    end
    return p(...)
end

------------------------------------------------------------------------
-- Functions called by the 'r' and 'i' commands
------------------------------------------------------------------------

function restart()
    for id, creature in pairs(creatures) do
        creature:restart()
    end
end

function info()
    for id, creature in pairs(creatures) do
        print(creature .. ":")
        print("------------------------------")
        if creature.message then
            print("current message: " .. creature.message)
        end
        if type(creature.thread) == 'thread' then
            local status = coroutine.status(creature.thread)
            if status == "suspended" then
                status = "alive"
            end
            if creature.status then 
                status = creature.status
            end
            print(_TRACEBACK(creature.thread, 
                             "thread status  : " .. status, 
                             coroutine.status(creature.thread) == "dead" and 0 or 1))
        end
        print()
    end
end

------------------------------------------------------------------------
-- Compatibility
------------------------------------------------------------------------

function nearest_enemy(...)
    print(_TRACEBACK("calling 'nearest_enemy' is deprecated. use 'get_nearest_enemy' instead.", 2))
    nearest_enemy = get_nearest_enemy
    return get_nearest_enemy(...)
end

function exists(...)
    print(_TRACEBACK("calling 'exists' is deprecated. use 'creature_exists' instead.", 2))
    exists = creature_exists
    return creature_exists(...)
end

function get_hitpoints(...)
    error("'get_hitpoints' is no longer available. use the values of 'creature_config' instead.", 2)
end

function get_attack_distance(...)
    error("'get_attack_distance' is no longer available. use the values of 'creature_config' instead.", 2)
end

------------------------------------------------------------------------
-- Creature Config Access
------------------------------------------------------------------------

creature_config = setmetatable({}, {
    -- See the file creature_config.gperf for the 
    -- list of available values within this table.
    __index = function (t, val) 
        return creature_get_config(val)
    end
})

------------------------------------------------------------------------
-- Install default onCommand
------------------------------------------------------------------------

function onCommand(cmd)
    print("huh? use '?' for help")
end

------------------------------------------------------------------------
-- Load Highlevel API
------------------------------------------------------------------------

do
    local api = (...)

    -- Load API
    dofile("api/" .. api)

    -- Load Default Code for API
    dofile("api/" .. api .. "-default")

    function needs_api(needed)
        assert(needed == api, "This Code needs the API '" .. needed .. "' but '" .. api .. "' is loaded")
    end
end

------------------------------------------------------------------------
-- Modify dofile to only load allowed files once.
------------------------------------------------------------------------

do
    local orig_dofile, assert, type, pairs = dofile, assert, type, pairs

    local loaded = {}

    function dofile(file, silent) 
        assert(type(file) == "string", "filename must be string")

        -- already loaded into this vm?
        if loaded[file] then
            return
        end

        -- not allowed to load this file?
        if not config.dofile_allowed or not config.dofile_allowed[file] then
            if not silent then
                print(_TRACEBACK("dofile('" .. file .. "') denied. " ..
                      "upload it using the batch (b) command"))
            end
            return
        end

        loaded[file] = true
        return orig_dofile("libs/" .. file)
    end

    function dofile_list()
        print("You can load the following files:")
        print("-----------------------------------------------")
        for name, help in pairs(config.dofile_allowed) do
            print(string.format("%-10s - %s", name, help))
        end
        print("-----------------------------------------------")
    end
end

------------------------------------------------------------------------
-- Switch print
------------------------------------------------------------------------
print = client_print
