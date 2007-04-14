needs_api("oo")

function debug_trace_hook(creature, source, line)
    local lsource = creature.debug_last_source
    local lline   = creature.debug_last_line
    creature.debug_last_source = source
    creature.debug_last_line   = line

    if lsource == source and lline == line then
        return false
    end

    if creature.break_points[line] then
        print("breakpoint hit creature " .. creature.id .. " at " ..  _TRACEBACK(nil, 3))
        creature.stopped = true
        return true -- yield please
    end

    if creature.stop_next then
        print("stopped creature " .. creature.id .. " at " ..  _TRACEBACK(nil, 3))
        creature.stopped = true
        return true -- yield please
    else
        return false
    end
end

function debug_hook(creature)
    if creature.hooked and not creature.installed_hook then
        linehook(creature.thread, function (source, line)
            return debug_trace_hook(creature, source, line)
        end)
        creature.installed_hook = true
    elseif not creature.hooked and creature.installed_hook then
        linehook(creature.thread)
        creature.installed_hook = nil
    end
    return not creature.stopped
end

function debug_stop(creature)
    print("stopping creature " .. creature.id)
    creature.stopped = true
end

function debug_next(creature)
    -- print("tracing creature " .. creature.id)
    creature.stop_next = true
    creature.stopped   = false
end

function debug_continue(creature)
    -- print("restarting creature" .. creature.id)
    creature.stop_next = false
    creature.stopped   = false
end

function debug_break(creature, line)
    if not line then
        print("breakpoints for " .. creature.id .. ":")
        for line, _ in pairs(creature.break_points) do
            print(string.format("%4", line))
        end
    else
        if creature.break_points[tonumber(line)] then
           creature.break_points[tonumber(line)] = nil
           print("removed breakpoint @" .. line)
        else
           creature.break_points[tonumber(line)] = true
           print("added breakpoint @" .. line)
        end
    end
end

function debug_info(creature)
    print("info for " .. creature)
    print(_TRACEBACK(creature.thread))
end

function debug_init(arg)
    local creature = creatures[tonumber(arg)]
    if not creature then
        print("creature " .. arg .. " does not exist")
        return
    end
    debug_cid = creature.id
    print("debugging creature " .. creature.id)
    creature.hooked       = true
    creature.stop_next    = false
    creature.break_points = {}
    creature.stopped      = false 
end

function onCommand(cmd)
    local cmd, arg = cmd:match("^d(.?)(.*)$")
    if not cmd then
        print("huh?")
    elseif cmd == "" then
        print("debugger usage")
        print("-----------------------")
        print("d[command][arg]")
    elseif cmd == "d" then
        debug_init(arg)
    elseif not debug_cid then 
        print("no debug creature set")
    elseif not creatures[debug_cid] then
        print("creature " .. debug_cid .. " no longer exists")
        debug_cid = nil
    elseif cmd == "s" then
        debug_stop(creatures[debug_cid])
    elseif cmd == "n" then
        debug_next(creatures[debug_cid])
    elseif cmd == "c" then
        debug_continue(creatures[debug_cid])
    elseif cmd == "b" then
        debug_break(creatures[debug_cid], arg)
    elseif cmd == "i" then
        debug_info(creatures[debug_cid])
    else
        print("unknown debug command?")
    end
end

print("Debugger loaded. Be sure to enable command forwarding (type 'fwd')")
