function bot()
    function onSpawned(parent)
        if parent then
            print("Creature " .. id .. " spawned by " .. parent)
        else
            print("Creature " .. id .. " spawned")
        end
    end

    function onAttacked(attacker)
    end

    function onRestart()
    end

    function onKilled(killer)
        if killer == id then
            print("Creature " .. id .. " suicided")
        elseif killer then 
            print("Creature " .. id .. " killed by Creature " .. killer)
        else
            print("Creature " .. id .. " died")
        end
    end

    function onIdle()
    end
end
