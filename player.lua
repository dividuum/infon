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

-- save in registry for usage within 'out of cycles' handler
save_in_registry('traceback', debug.traceback)
save_in_registry = nil

-- save traceback function
_TRACEBACK = debug.traceback

-- new dofile function uses defined PREFIX
do
    local PREFIX, orig_dofile = PREFIX, dofile
    function dofile(file)
        return orig_dofile(PREFIX .. file .. ".lua")
    end
end

-- limit strings accepted by loadstring to 16k
do
    local orig_loadstring, error = loadstring, error
    function loadstring(code, name)
        if #code > 16384 then
            error("code too large")
        else
            return orig_loadstring(code, name)
        end
    end
end

-- provide thread tracing function
do
    local sethook = debug.sethook
    local getinfo = debug.getinfo
    local type, error, print = type, error, print
    thread_trace = function(thread, text)
        if type(thread) ~= "thread" then
            error("arg #1 is not a thread")
        end
        local dumper = type(text) == "function" and text or function(info)
            print(text .. ":" .. info.source .. ":" .. info.currentline)
        end
        local hook = function(what, where)
            dumper(getinfo(2, "nlS"))
        end
        sethook(thread, hook, "l")
    end
    thread_untrace = function(thread)
        if type(thread) ~= "thread" then
            error("arg #1 is not a thread")
        end
        sethook(thread)
    end
end

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
        print(creature .. ":")
        print("------------------------------")
        if creature.message then
            print("current message: " .. creature.message)
        end
        if type(creature.thread) == 'thread' then
            print(_TRACEBACK(creature.thread, 
                             "thread status  : " .. coroutine.status(creature.thread), 
                             coroutine.status(creature.thread) == "dead" and 0 or 1))
        end
        print()
    end
end

------------------------------------------------------------------------
-- compatibility
------------------------------------------------------------------------

function nearest_enemy(...)
    print(_TRACEBACK("calling 'nearest_enemy' is deprecated. use 'get_nearest_enemy' instead.", 2))
    nearest_enemy = get_nearest_enemy
    return get_nearest_enemy(...)
end

------------------------------------------------------------------------
-- Load Highlevel API
------------------------------------------------------------------------

dofile(...)
dofile = function(what) 
    print("cannot 'dofile(\"" .. what .. "\")'. upload it using the batch (b) command.")
end

