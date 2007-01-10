dofile("menu")

menu "Menu"
 .sub "Bot Functions"
     .add("Print Information", info)
     .add("Restart",           restart)
     .add("Creature Tables",   function ()
                                    for id, creature in pairs(creatures) do 
                                        print(creature)
                                        p(creature)
                                    end
                                end)
     .add("Suicide",           function ()
                                    for id, creature in pairs(creatures) do 
                                        suicide(id)
                                    end
                                end)
     .super
 .sep
 .add("Error", function () error("Example error") end)
 .add("About", function () print("Example menu!") end)
