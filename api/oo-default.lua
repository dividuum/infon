--------------------------------------------------------------------------
-- Default Code
--------------------------------------------------------------------------

-- Called after the Creature was created. You cannot 
-- call long-running methods (like moveto) here.
function Creature:onSpawned(parent)
    if parent then
        print("Creature " .. self.id .. " spawned by " .. parent)
    else
        print("Creature " .. self.id .. " spawned")
    end
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


-- Called after being killed. Since the creature is already 
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


-- Your Creature Logic here :-)
function Creature:main()

end
