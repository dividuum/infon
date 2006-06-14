--------------------------------------------------------------------------
-- Default Logik 
--
-- Fuer jedes gespawnte Vieh wird eine eigene Creature Klasse instanziiert
--------------------------------------------------------------------------

function Creature:onSpawned()
    -- Hier keine langlaufenden Aktionen ausfuehren!
    print("Creature " .. self.id .. " spawned")
end

function Creature:onAttacked(attacker)
    -- Hier keine langlaufenden Aktionen ausfuehren!
    -- Wird pro Angreifer einmal aufgerufen.
    print("Help! Creature " .. self.id .. " is attacked by Creature " .. attacker)
end

function Creature:onRestart()
    -- Methode wird nach Eingabe von 'r' ausgefuehrt
    -- Hier keine langlaufenden Aktionen ausfuehren!
end

function Creature:onKilled(killer)
    -- Die getoetete Kreatur existiert hier bereits nicht mehr.
    -- => die in self.id stehende Kreatur ist nicht mehr vorhanden,
    --    bzw gehoert einem anderen Spieler.
    if killer == self.id then
        print("Creature " .. self.id .. " suicided")
    elseif killer then 
        print("Creature " .. self.id .. " killed by Creature " .. killer)
    else
        print("Creature " .. self.id .. " died")
    end
end

function Creature:main()
    -- Dein Programm hier
end

