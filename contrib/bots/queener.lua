--------------------------------------------------------------------------
-- Default Code
--------------------------------------------------------------------------

-- Called after the Creature was created. You cannot 
-- call long-running methods (like moveto) here.
function Creature:onSpawned()
    print("Creature " .. self.id .. " spawned")
end


-- Called each round for every attacker on this
-- creature. No long-running methods here!
function Creature:onAttacked(attacker)
    -- print("Help! Creature " .. self.id .. " is attacked by Creature " .. attacker)
end


-- Called by typing 'r' in the console, after creation (after 
-- onSpawned) or by calling self:restart(). No long-running 
-- methods calls here!
function Creature:onRestart()
    
end


-- Called after beeing killed. Since the creature is already 
-- dead, self.id cannot be used to call the Lowlevel API.
function Creature:onKilled(killer)
    if killer == self.id then
        print("Creature " .. self.id .. " suicided")
    elseif killer then 
        print("Creature " .. self.id .. " killed by Creature " .. killer)
    else
        print("Creature " .. self.id .. " died")
    end
end

function Creature:start(fun)
    if self.fun ~= self[fun] then
        self:screen_message(fun)
        self.fun = self[fun]
        self.logic = coroutine.create(self[fun])
    end
end

function Creature:handle()
    if self.logic then
        local ok, msg = coroutine.resume(self.logic, self)
        if not ok then 
            print("creature logic " .. self.id .. " aborted: " .. msg)
            self.logic = nil
        end
    end
end

function Creature:queen_idle()
    if self:health() < 70 and self:food() > 0 then
        self:begin_healing()
        while self:is_healing() do 
            self:wait_for_next_round()
        end
        queen_stop = false
    end
    if self:health() > 30 and self:food() > 8000 then
        self:begin_spawning()
        while self:is_spawning() do
            self:wait_for_next_round()
        end
        queen_stop = false
    end
end

function Creature:be_queen()
    queen_id = self.id
    if self:type() == 0 then
        while true do 
            local x1, y1, x2, y2 = world_size()
            while not self:set_path(math.random(x2 - x1) + x1,
                                    math.random(y2 - y1) + y1) do
            end                            
            self:begin_walk_path()
            while self:is_walking() and self:food() < 8000 do 
                if self:tile_food() > 0 then 
                    self:begin_eating()
                    while self:is_eating() and self:food() < 8000 do
                        self:wait_for_next_round()
                    end
                    self:begin_walk_path()
                end
                self:wait_for_next_round()
            end
            if self:food() >= 8000 then
                self:convert(1)
                break
            end
        end
    end
    self:screen_message("Queen")
    while true do
        if queen_stop then
            self:begin_idling()
            self:queen_idle()
            self:wait_for_next_round()
        else
            local x1, y1, x2, y2 = world_size()
            while not self:set_path(math.random(x2 - x1) + x1,
                                    math.random(y2 - y1) + y1) do
            end                            
            self:begin_walk_path()
            local time = game_time()
            while self:is_walking() do
                if numcreatures < 4 then
                    if self:tile_food() > 0 then 
                        self:begin_eating()
                        while self:is_eating() and self:food() < 15000 do
                            self:wait_for_next_round()
                        end
                        self:begin_walk_path()
                    end
                end
                self:queen_idle()
                self:begin_walk_path()
                self:wait_for_next_round()
                if queen_stop then 
                    break
                end
            end
        end
    end
end

function dist(x1, y1, x2, y2)
    return math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2))
end

function Creature:dist_to_queen()
    if not queen_id then
        return 1000000
    else
        local kx, ky = get_pos(queen_id)
        local cx, cy = self:pos()
        return dist(kx, ky, cx, cy)
    end
end

queen_stop = false

function Creature:bring_queen()
    while true do 
        if self:food() == 0 then
            return
        end
        if self:dist_to_queen() < 500 then
            queen_stop = true
        end
        if self:dist_to_queen() > 100 then
            if not queen_id then
                return
            end
            local kx, ky = get_pos(queen_id)
            local time = game_time()
            self:set_path(kx, ky)
            self:begin_walk_path()
            while self:is_walking() do 
                if self:health() < 70 then
                    return
                end
                self:wait_for_next_round()
                if time + 500 < game_time() then
                    kx, ky = get_pos(queen_id)
                    self:set_path(kx, ky)
                end
                self:wait_for_next_round()
            end
        else
            self:set_target(queen_id)
            self:begin_feeding()
            while self:is_feeding() do
                self:wait_for_next_round()
            end
        end
        self:wait_for_next_round()
    end
end

function Creature:be_slave()
    if self:type() == 0 then
        while true do 
            local x1, y1, x2, y2 = world_size()
            while not self:set_path(math.random(x2 - x1) + x1,
                                    math.random(y2 - y1) + y1) do
            end                            
            self:begin_walk_path()
            while self:is_walking() and self:food() < 5000 do 
                if self:tile_food() > 0 then 
                    self:begin_eating()
                    while self:is_eating() and self:food() < 5000 do
                        self:wait_for_next_round()
                    end
                    self:begin_walk_path()
                end
                self:wait_for_next_round()
            end
            if self:food() >= 5000 then
                self:convert(2)
                break
            end
        end
    end
    local lastx, lasty
    self.last_food = game_time()
    while true do
        local x1, y1, x2, y2
        if lastx and lasty then
            self:screen_message("old")
            x1, y1, x2, y2 = lastx - 512, lasty - 512, lastx + 512, lasty + 512
            lastx, lasty = nil, nil
        elseif queen_id and numcreatures < 5 then
            self:screen_message("area")
            local kx, ky = get_pos(queen_id)
            x1 = kx - 2048
            x2 = kx + 2048
            y1 = ky - 2048
            y2 = ky + 2048
        else
            self:screen_message("world")
            x1, y1, x2, y2 = world_size()
        end
        while not self:set_path(math.random(x2 - x1) + x1,
                                math.random(y2 - y1) + y1) do
        end                            
        self:begin_walk_path()
        while self:is_walking() do
            local x, y = self:pos()
            if last_food_x and 
               last_food_y and 
               dist(x, y, last_food_x, last_food_y) < 3000 and
               self.last_food + 10000 < game_time()
            then
                self:screen_message("called")
                self:set_path(last_food_x, last_food_y)
            end
            if self:tile_food() > 0 then
                self.last_food = game_time()
                last_food_x, last_food_y = self:pos()
                self:begin_eating()
                while self:is_eating() do
                    self:wait_for_next_round()
                end
                self:begin_healing()
                while self:is_healing() do 
                    self:wait_for_next_round()
                end
                lastx, lasty = self:pos()
            end
            if self:type() == 1 then
                if self:food() > 9600 and self:health() > 70 then
                    self:screen_message("bring")
                    self:bring_queen()
                end
            else
                if self:food() > 4600 and self:health() > 30 then
                    self:screen_message("bring")
                    self:bring_queen()
                end
            end
            self:wait_for_next_round()
        end
    end
end
    
function onRoundStart()
    numcreatures = 0
    for n, creature in pairs(creatures) do
        if numcreatures == 0 then
            creature:start('be_queen')
        else
            creature:start('be_slave')
        end
        numcreatures = numcreatures + 1
    end
end


-- Your Creature Logic here :-)
function Creature:main()
    self:handle()
end

