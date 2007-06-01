-- Example for the state API. 
needs_api "state"

function bot()
    function onKilled()
        print(id .. " died")
    end

    function onIdle()
        return and_start_state "find_food"
    end

    function find_food()
        random_move()
    end

    function onTileFood()
        if can_eat() then 
            return and_be_in_state "eat_food"
        end
    end

    function eat_food()
        eat()
        return and_start_state "find_food"
    end

    function onLowHealth()
        return and_be_in_state "healing"
    end

    function healing()
        heal()
    end
end
