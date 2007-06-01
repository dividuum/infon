--------------------------------------------------------------------------
-- Blueloop V00 
--
-- Fuer jedes gespawnte Vieh wird eine eigene Creature Klasse instanziiert
--------------------------------------------------------------------------

function Creature:onSpawned()
    print("Creature " .. self.id .. " spawned")
end

function Creature:onAttacked(attacker)
    print("Help! Creature " .. self.id .. " is attacked by Creature " .. attacker)
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

function Wait()
    while (get_state(self.id) ~= CREATURE_IDLE) do
        self:wait_for_next_round()  
    end;
end

function Creature:main()
    if self.dx == nil then self.dx = 256 end
    if self.dy == nil then self.dy = 0 end
    if victim  == nil then victim  = -1 end
    
    ---------------------------------------------- Wenn ein globaler Angriff aktiv ist:
    if (victim ~= -1) and (get_type(self.id) == 1) then
        if exists(victim) then
        
            local x,y = get_pos(victim)                    -- Wo ist der angegriffene?
            self:moveto(x,y)                               -- hinlaufen
            self:attack(victim)                            -- angreifen!
        else
            victim = -1                                    -- Tot -> Angriff beenden
            print"Angriff beendet!"
        end
    end
    

    ---------------------------------------------- Warten bis Creatur IDLE
    if get_state(self.id) ~= CREATURE_IDLE  then
        self:wait_for_next_round()
        return
    end
    
    ---------------------------------------------- Wo ist der nchste Feind?
    local feind_id, feind_x, feind_y, feind_playernum, feind_dist = nearest_enemy(self.id)  
    
    ---------------------------------------------- Wenn er nah genug ist und wir gesund sind -> angreifen
    
    if (feind_id ~= nil) and (victim == -1) then
        if (feind_dist < 8000) and (self:health() > 50) and (victim == -1) then
            print("Starte Angriff auf " .. feind_id )
            victim = feind_id
        end
    end

    ---------------------------------------------- Fressen!
    
    if get_tile_food(self.id) > 0 then  
        set_state(self.id, CREATURE_EAT)
    else
        local x, y = self:pos()
        if not self:moveto(x+self.dx,y+self.dy) then
            self.dx = math.random(512)-256
            self.dy = math.random(512)-256
        end
    end
    
    
    if self:health() < 90 then 
        set_state(self.id, CREATURE_HEAL)
    end;
        
    if get_food(self.id) > 8000 then
        if get_type(self.id) == 0 then
            set_convert(self.id, 1)
            set_state(self.id,CREATURE_CONVERT)
        else
            set_state(self.id,CREATURE_SPAWN)
        end
    end
    
    if get_state(self.id) == CREATURE_IDLE then
        self:moveto(get_koth_pos())
    end
    
    self:wait_for_next_round()
end
---------------------------------------------------------------------------------
