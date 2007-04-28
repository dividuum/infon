-- This source creates menu for the sissy-bot. 
-- To use it, append it to sissy-bot.lua source 
-- and type '0' in the prompt.

dofile("menu")

menu "Sissybot"
  .sub "Health Goals"
    .sub "Type 0"
      .add("30%", function () w_health_type0 = 30 end)
      .add("50%", function () w_health_type0 = 50 end)
      .add("80%", function () w_health_type0 = 80 end)
      .super
    .sub "Type 1"
      .add("30%", function () w_health_type1 = 30 end)
      .add("50%", function () w_health_type1 = 50 end)
      .add("80%", function () w_health_type1 = 80 end)
      .super
    .sub "Type 2"
      .add("30%", function () w_health_type2 = 30 end)
      .add("50%", function () w_health_type2 = 50 end)
      .add("80%", function () w_health_type2 = 80 end)
      .super
    .super
  .sub "Food Goals"
    .sub "Type 0"
      .add(" 2000", function () w_fed_type0 =  2000 end)
      .add(" 5000", function () w_fed_type0 =  5000 end)
      .add("10000", function () w_fed_type0 = 10000 end)
      .super
    .sub "Type 1"
      .add(" 5000", function () w_fed_type1 =  5000 end)
      .add("10000", function () w_fed_type1 = 10000 end)
      .add("20000", function () w_fed_type1 = 20000 end)
      .super
    .sub "Type 2"
      .add(" 1000", function () w_fed_type2 =  1000 end)
      .add(" 2000", function () w_fed_type2 =  2000 end)
      .add(" 5000", function () w_fed_type2 =  5000 end)
      .super
    .super
  .sub "Type Distribution"        
    .add("65%, 25%, 10%", function () w_type1_total, w_type2_total = .25, .10 end)
    .add("34%, 33%, 33%", function () w_type1_total, w_type2_total = .33, .33 end)
    .add("20%, 30%, 50%", function () w_type1_total, w_type2_total = .30, .50 end)
    .super
  .sub "Strategies"
    .add("Aggressive", function ()
            zero_convert1 	= 7
            zero_convert2   = 6
            zero_heal  	 	= 5
            zero_distribute = 2
            zero_runaway	= 4
            zero_koth  	 	= 3
            zero_fight 		= 1

            one_fight 		= 4
            one_heal 		= 2
            one_breed 		= 3
            one_king 		= 1

            two_heal		= 2
            two_distribute	= 1
            two_runaway		= 4
            two_koth		= 3
         end)
    .add("Default", function ()
            zero_convert1 	= 4
            zero_convert2   = 3
            zero_heal  	 	= 6
            zero_distribute = 2
            zero_runaway	= 7
            zero_koth  	 	= 5
            zero_fight 		= 1

            one_fight 		= 1
            one_heal 		= 4
            one_breed 		= 2
            one_king 		= 3

            two_heal		= 3
            two_distribute	= 1
            two_runaway		= 4
            two_koth		= 2
         end)
    .super
  .sep
  .sub "Debugging"
    .add("Show Decisions", function ()
          ANN_DECISION_TYPE0 = true
          ANN_DECISION_TYPE1 = true
          ANN_DECISION_TYPE2 = true
        end)
    .add("Hide Decisions", function ()
          ANN_DECISION_TYPE0 = false
          ANN_DECISION_TYPE1 = false
          ANN_DECISION_TYPE2 = false
        end)
    .super        
  .sep    
  .add("About", function () 
         print("role-priority neural network based bot by mike mcmanus and nick irvine")
       end)
