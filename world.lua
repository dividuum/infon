function world_main()
    level_init()
    while true do
        level_tick()
        coroutine.yield()
    end
end

function world_init()
    require 'level.lua'
    local w,  h  = level_size()
    local kx, ky = level_koth_pos()
    world_tick = coroutine.wrap(world_main)
    return w, h, kx, ky
end
