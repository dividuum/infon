--------------------------------------------------------------------------
-- Default Logik 
--
-- Fuer jedes gespawnte Vieh wird eine eigene Creature Klasse instanziiert
--------------------------------------------------------------------------

function Creature:onSpawned()
    print("Creature " .. self.id .. " spawned")
end

function Creature:onAttacked(attacker)
    print("Help! Creature " .. self.id .. " is attacked by Creature " .. attacker)
    
                                        -- Ich bin noch zu klein zum kmpfen
    if (get_type(self.id) == 0) or (self:health() > 20) then
        local x,y = self:pos()
                                         -- weglaufen
        self:set_path(x-16000+math.random(8000),y-1600+math.random(8000)) 
        self:begin_walk_path()
    else                                 -- Smack my Bit up!
        self:set_target(attacker)
        self:begin_attacking()      
    end
end

function Creature:onKilled(killer)
    if killer == self.id then
        print("Creature " .. self.id .. " suicided")
    elseif killer then 
        print("Creature " .. self.id .. " killed by Creature " .. killer)
    else
        print("Creature " .. self.id .. " died")
    end
end

function Creature:Wait()
    while (get_state(self.id) ~= CREATURE_IDLE) do
        self:wait_for_next_round()  
    end;
end

function Creature:Fressen()
    if get_tile_food(self.id) > 0 then  
        set_state(self.id, CREATURE_EAT)
        self:Wait()
    else
        local x, y = self:pos()
        if not self:moveto(x+self.dx,y+self.dy) then

            self.dx = math.random(256)-128
            self.dy = math.random(256)-128
            
            
        end
    end
    
    if self:health() < 90 then
        set_state(self.id, CREATURE_HEAL)
        self:Wait()
    end
end

function Creature:Angreifen()
    -- Wo ist der nchste Feind?
    local feind_id, feind_x, feind_y, feind_playernum, feind_dist = nearest_enemy(self.id)  
    
    -- Wenn er nah genug ist und wir gesund sind -> angreifen
    
    if (feind_id ~= nil) and (feind_dist < 500) and (self:health() > 60) then
        print("Starte Angriff auf " .. feind_id .. " Abstand: " .. feind_dist)

        while (exists(feind_id)) do
            local x,y = get_pos(feind_id)                  -- Wo ist der angegriffene?
            self:moveto(x,y)                               -- hinlaufen
            self:attack(feind_id)                          -- angreifen!
            self:Wait()
        end
    end
    
end

function Creature:main()
    if self.dx == nil then self.dx = 256 end
    if self.dy == nil then self.dy = 0 end
    if victim  == nil then victim  = -1 end
    
    if get_type(self.id) == 0 then
        print("Creature " .. self.id .. " soll fressen")
        while get_food(self.id) < 8000 do
            self:Fressen()
        end
        print("Creature " .. self.id .. " soll sich verwandeln")
        set_convert(self.id, 1)
        set_state(self.id,CREATURE_CONVERT)
        self:Wait()
    else
        print("Creature " .. self.id .. " soll fressen")
        while get_food(self.id) < 8000 do
            self:Fressen()
            self:Angreifen()
        end
        print("Creature " .. self.id .. " vermehrt sich")
        set_state(self.id,CREATURE_SPAWN)
        self:Wait()
        --print("Creature " .. self.id .. " soll zu King of the Hill")
        
        --self:moveto(get_koth_pos())
        --self:Wait()
        --print("Creature " .. self.id .. " ist King of the Hill")
        --while self:health() > 30 do
        --  local feind_id, feind_x, feind_y, feind_playernum, feind_dist = nearest_enemy(self.id)          
        --  self:attack(feind_id) 
        --end
    end
end
