-- This bot uses a role-priority neural network to make decisions about
-- its actions each turn.  The weights labelled zero_xxxxx, one_xxxxx and
-- two_xxxxx represent the role bias for each type of creature (type0, type1
-- and type2).  Changing those weights will change the behaviour of the
-- creature and is a really simple way of making different bots.  There are
-- a total of 2903040 different combinations for this network, so experiment
-- a try to find a useful bot for yourself!  For example, for a type0 creature,
-- setting the weights to:
--		zero_convert1 		= 1
--		zero_convert2   	= 2
--		zero_heal  	 		= 3
--		zero_distribute 	= 4
--		zero_runaway		= 5
--		zero_koth  	 		= 6
--		zero_fight 			= 7
-- means that the creature would fight before becoming king, become king before running,
-- run before distributing, distribute before healing, etc.  So if the fight node is 'on'
-- then the creature will ALWAYS fight.

-- Once a decision is made, the appropriate role is called.  These roles are
-- by no means optimized, and we are still working on them.  Gather, for example
-- is still fairly primitive and we will (hopefully!) be updating it soon.

-- If you are interested, a short paper on the results of this bot is available
-- at www.uoguelph.ca/~mmcmanus

-- If you have any question regarding the code or the algorithms used feel free
-- to contact us at:

-- mike mcmanus -> mmcmanus@uoguelph.ca
-- nick irvine  -> nirvine@uoguelph.ca

--WEIGHTS

w_health_type0 	= 80 		-- 0 - 100
w_health_type1	= 35		-- 0 - 100
w_health_type2	= 50		-- 0 - 100

w_distribute_amount = 5000 	-- 0 - 5000

w_fed_type0	= 5000 		-- 0 - 10000
w_fed_type1 = 10000 	-- 0 - 20000
w_fed_type2 = 4000		-- 0 - 5000

w_type1_total 	= .25	-- 0 - 1
w_type2_total 	= .10	-- 0 - 1

w_need_amount	= 5000 -- 0 - 20000

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


-- CREATURE DEFAULT VALUES
TYPE0_HEALTH 	= 100
TYPE0_STORAGE 	= 10000

-- ENERGY COSTS
CONVERT_FOOD_COST = 8000
SPAWN_FOOD_COST = 8000
COVERT_HEALTH_COST = 20
SPAWN_HEALTH_COST = 20

-- ROLE STATES
CREATURE_BREEDER 		= 0
CREATURE_FIGHTER 		= 1
CREATURE_PATIENT 		= 2
CREATURE_DISTRIBUTOR 	= 3
CREATURE_KING			= 4
CREATURE_RUNNER			= 5
CREATURE_CONVERTER		= 6
CREATURE_NOROLE 		= 100

RUNNER_DISTANCE			= 30

--SPECIAL STATES -- see self.special_state
CREATURE_WAIT 		= 10
CREATURE_GO			= 13

GATHER_FOOD_THRESHOLD = 1
BOUNTIFUL = 1
UNEXPLORED = 0
BARREN = -1

-- DEBUGGING FLAGS
ANN_DECISION_TYPE0 = false
ANN_DECISION_TYPE1 = false
ANN_DECISION_TYPE2 = true

cell_width = 0
cell_height = 0
gather_circuit = {}

grid = {}

distribution = {}
need = {}
dlen = 0

creaturelist = {}
rlen = 0

init = 0

winning = false

-------------------------------------------------------------
-- INITIALIZING
-------------------------------------------------------------
function game_init( self )
	distribution_init( self )
	food_init( self )
end

function food_init( self )
	gather_circuit = {
		{ ["viable"] = UNEXPLORED, ["i"] = 0, ["j"] = 0},
		{ ["viable"] = UNEXPLORED, ["i"] = 1, ["j"] = 0},
		{ ["viable"] = UNEXPLORED, ["i"] = 2, ["j"] = 0},
		{ ["viable"] = UNEXPLORED, ["i"] = 3, ["j"] = 0},
		{ ["viable"] = UNEXPLORED, ["i"] = 3, ["j"] = 1},
		{ ["viable"] = UNEXPLORED, ["i"] = 2, ["j"] = 1},
		{ ["viable"] = UNEXPLORED, ["i"] = 1, ["j"] = 1},
		{ ["viable"] = UNEXPLORED, ["i"] = 1, ["j"] = 2},
		{ ["viable"] = UNEXPLORED, ["i"] = 2, ["j"] = 2},
		{ ["viable"] = UNEXPLORED, ["i"] = 3, ["j"] = 2},
		{ ["viable"] = UNEXPLORED, ["i"] = 3, ["j"] = 3},
		{ ["viable"] = UNEXPLORED, ["i"] = 2, ["j"] = 3},
		{ ["viable"] = UNEXPLORED, ["i"] = 1, ["j"] = 3},
		{ ["viable"] = UNEXPLORED, ["i"] = 0, ["j"] = 3},
		{ ["viable"] = UNEXPLORED, ["i"] = 0, ["j"] = 2},
		{ ["viable"] = UNEXPLORED, ["i"] = 0, ["j"] = 1},
	}
	
	x1,y1,x2,y2 = world_size()	
	cell_width = math.floor((x2-x1)/4)
	cell_height = math.floor((y2-y1)/4)
end

function grid_init( self )
	local x1, y1, x2, y2 = world_size()
	
	truex = (x2-x1)/xweight
	truey = (y2-y1)/yweight
	
	truex = math.floor(truex)
	truey = math.floor(truey)
	
    for i=0,truex do
		grid[i] = {}
		for j=0,truey do
			if self:set_path(math.floor(i*xweight), math.floor(j*yweight)) then
				grid[i][j] = -1
			else
				grid[i][j] = nil
			end
		end
	end
end

function distribution_init( self )
	for i=0,1024 do
		distribution[i] = nil
		need[i] = nil
	end
end

function roles_init( self )
	for i=0,1024 do
		creaturelist[i] = -1
	end
end

function creature_init( self )
	if not self.init then
		self.find = 0
		self.init = true
		self.special_state = CREATURE_GO
		add_creature( self )
		self.giving = false
		self.giveto = -1
		self.giveamount = -1
		self.givestart = -1
		self.role = CREATURE_NOROLE
		pick_gather_dest( self )
		print("creature" .. self.id .. " initialized" )
	end
end

-------------------------------------------------------------
-- LIBRARY
-------------------------------------------------------------
--converts the coordinates to indices in the grid
function convert_to_index( x, y )
	return math.floor(x/xweight), math.floor(y/yweight)
end	

--converts the indices of the grid to coordinates on the map
function convert_to_coords( x, y)
	return math.floor(x*xweight), math.floor(y*yweight)
end

--adds a creature to the global creature list
function add_creature( self )
	creaturelist[rlen] = self
	rlen = rlen + 1
end

--returns the amount of a given creature type 
function type_amount( type )
	amount = 0
	
	for i=0, rlen-1 do
		if get_type( creaturelist[i].id ) == type then
			amount = amount + 1
		end
	end
	
	return amount
end

--returns the amount of creatures that are a given role
function role_amount( creature_role )
	amount = 0
	
	for i=0, rlen-1 do
		if creaturelist[i].role == creature_role then
			amount = amount + 1
		end
	end
	
	return amount
end

--sets the special state of the creature
function set_special_state( self, state )
	self.special_state = state
end

function get_total_food( )
	food = 0
	
	for i=0, dlen-1 do 
		food = food + get_food( distribution[i] )
	end
	
	return food
end

function get_total_need( )
	total_need = 0
	
	for i=0, dlen-1 do 
		total_need = total_need + need[i]
	end
	
	return total_need
end
--checks to see if any creatures have died since the last turn
function creature_check( )
	tempd = {}
	tempa = {}
	tlen = 0
	
	for i=0,dlen-1 do
		if creature_exists( distribution[i] ) then
			tempd[tlen] = distribution[i]
			tempa[tlen] = need[i]
			tlen = tlen+1
		end
	end
	
	for i=0,tlen-1 do
		distribution[i] = tempd[i]
		need[i] = tempa[i]
	end
	
	dlen = tlen
	
	tempc = {}
	tlen = 0
	
	for i=0,rlen-1 do
		if creature_exists( creaturelist[i].id ) then
			tempc[tlen] = creaturelist[i]
			tlen = tlen+1
		else
			print("A CREATURE DIED")
		end
	end
	
	for i=0,tlen-1 do
		tempc[i] = creaturelist[i]
	end
	
	rlen = tlen
end

function set_special_state( id, state )
	for i=0,rlen-1 do
		if creaturelist[i].id == id then
			creaturelist[i].special_state = state
		end
	end
end

function enemy_info( self )
	enemy_creature_id, x, y, playernum, dist = nearest_enemy( self.id )

	if enemy_creature_id ~= nil then
		closest_enemy_health 		= get_health( enemy_creature_id )
		closest_enemy_food 			= get_food( enemy_creature_id )
	else
		closest_enemy_health 		= 0
		closest_enemy_food 			= 0
	end	
	
	return closest_enemy_health, enemy_creature_id
end

function decision_max( decision, len )
	best_choice = decision[0]
	best_role = 0	
	
	for i=1,len do
		if decision[i] > best_choice then
			best_choice = decision[i]
			best_role = i
		end
	end
	
	return best_role
end

function cell_to_bounds( self, c )
	i,j = gather_circuit[c].i, gather_circuit[c].j

	x1,y1,x2,y2 = world_size()	
	return i * cell_width+x1, j * cell_height+y1,
		(i+1) * cell_width + x1, (j+1) * cell_height + y1
end


function coords_to_cell( self, x, y )
	x1,y1,x2,y2 = world_size()	
	i,j = math.floor((x-x1)*1.0/cell_width), math.floor((y-x1)*1.0/cell_height)
	
	for c = 1, 16 do
		if gather_circuit[c].i == i and gather_circuit[c].j == j then
			return c
		end
	end
end

function say( self , msg)
	print(self.id..": "..msg)
end
-------------------------------------------------------------
-- DISTRIBUTION
-------------------------------------------------------------
function request_food( id, amount )
	--dont let a creature add itself twice!
	for i=0,dlen-1 do
		if distribution[i] == id then
			need[i] = amount
			return
		end
	end
	
	print("added " .. id .. " to distro")
	distribution[dlen] = id
	need[dlen] = amount
	dlen = dlen + 1
end
	
--decides who needs food the most!
function pop( )
	if dlen <= 0 then
		dlen = 0
		return -1, -1	
	end

	creature = distribution[dlen-1]
	amount = need[dlen-1]
	dlen = dlen - 1
	return creature, amount
end

function request_remove( id )
	for i=0,dlen-1 do
		if distribution[i] == id then
			distribution[i] = -1
			need[i] = -1
		end
	end
end

-------------------------------------------------------------
-- GATHERING 
-------------------------------------------------------------
function pick_gather_dest( self )
	x,y = get_pos( self.id )
	mycell = coords_to_cell(self, x,y )

	dest_cell =  math.fmod(mycell + 1 ,16)+1
	v = gather_circuit[dest_cell].viable
	while v == BARREN do
		dest_cell = math.fmod(dest_cell , 16)+1
		v = gather_circuit[dest_cell].viable
	end
	
	x1,y1,x2,y2 = cell_to_bounds( self, dest_cell)
	
	x = math.floor(math.random()*(x2-x1)+x1)
	y = math.floor(math.random()*(y2-y1)+y1)
	
	-- should have something in here in case all the points in a cell are
	--  unreachable (water, etc.)  Meh
	
	while not set_path(self.id, x, y)  do
			x = math.floor(math.random()*(x2-x1)+x1)
			y = math.floor(math.random()*(y2-y1)+y1)
	end
	
	self.gather_dest = {["x"]= x, ["y"]= y}
end


function gather( self )
	x, y = get_pos( self.id )
	c = coords_to_cell( self, x, y )
	
	if food_here( self  ) then
		update_gather_circuit(self, c, BOUNTIFUL )
		set_state( self.id, CREATURE_EAT )
	else
		if x == self.gather_dest.x and y == self.gather_dest.y then
			pick_gather_dest( self )
		else
			set_path( self.id, self.gather_dest.x, self.gather_dest.y )
			set_state( self.id, CREATURE_WALK )
		end
	end
end

function food_here( self )
	return get_tile_food( self.id ) > GATHER_FOOD_THRESHOLD
end


function update_gather_circuit(self, cell, viable )
	gather_circuit[cell].viable = viable
end

function search_for_food( self, amount )
	request_food( self.id, amount )
	gather( self )
end
	

-------------------------------------------------------------
-- BREED
-------------------------------------------------------------
function breed( self )		
	if get_food( self.id ) >= SPAWN_FOOD_COST then
		set_state( self.id, CREATURE_SPAWN )
	else
		search_for_food( self, SPAWN_FOOD_COST - get_food( self.id ) )
	end
end

-------------------------------------------------------------
-- CONVERT
-------------------------------------------------------------
function convert( self, type )
	if get_food( self.id ) >= CONVERT_FOOD_COST and set_convert( self.id, type )then
		set_state( self.id, CREATURE_CONVERT )
	else		
		search_for_food( self, CONVERT_FOOD_COST - get_food( self.id ) )
	end
end

-------------------------------------------------------------
-- HEAL
-------------------------------------------------------------
function heal( self )	
	if get_food( self.id ) >= w_need_amount then
		set_state( self.id, CREATURE_HEAL )
	else
		search_for_food( self, w_need_amount )
	end
end

-------------------------------------------------------------
-- FIGHT
-------------------------------------------------------------
function fight( self )
	enemy_id, e_x, e_y, playernum, dist = nearest_enemy( self.id )
	
	if enemy_id ~= nil then	
		set_target( self.id, enemy_id )
		
		if get_distance( self.id, enemy_id ) < 256 then
			set_state( self.id, CREATURE_ATTACK)		
		elseif set_path( self.id, e_x, e_y ) then
			set_state( self.id, CREATURE_WALK )
		end
	end
end

function fight_king( self )
	x, y = get_pos( self.id )
	kx, ky = get_koth_pos( )
	
	if x ~= kx and y ~= ky then
		if math.abs(x - kx) < 128 and math.abs(y - ky) < 128 then
			enemy_id, e_x, e_y, playernum, dist = nearest_enemy( self.id )
			set_target( self.id, enemy_id )
			set_state( self.id, CREATURE_ATTACK)
		elseif set_path( self.id, kx, ky ) then
			set_state( self.id, CREATURE_WALK )
		end
	end
end	

function check_enemy_type( enemy_type, self_type )
	
	if self_type == 0 and enemy_type ~= 2 then
		return false
	elseif self_type == 2 then
		return false
	else
		return true
	end
end

-------------------------------------------------------------
-- RUNNER!
-------------------------------------------------------------
function run( self )
       bx1,by1,bx2,by2 = world_size()

       x = math.floor(math.random()*(bx2-bx1)+bx1)
       y = math.floor(math.random()*(by2-by1)+by1)
		
		if get_state( self.id ) ~= CREATURE_WALK then
	       	while not set_path(self.id, x, y) do
				x = math.floor(math.random()*(bx2-bx1)+bx1)
				y = math.floor(math.random()*(by2-by1)+by1)
       		end
		end
		set_state( self.id, CREATURE_WALK )
end

-------------------------------------------------------------
-- DISTRIBUTE
-------------------------------------------------------------
function distribute( self )
	request_remove( self.id )
	if self.giving == true then	
		print("giving food to" .. self.giveto)	
		if get_distance( self.id, self.giveto ) < 256 then
			set_target( self.id, self.giveto )
			set_state( self.id, CREATURE_FEED )		
		elseif set_path( self.id, get_pos( self.giveto ) ) then
			set_special_state( self.giveto, CREATURE_WAIT )
			set_state( self.id, CREATURE_WALK )
		end
		
		disfood = get_food( self.id )
		if disfood == 0 or self.givestart - disfood > self.giveamount then
			set_special_state( self.giveto, CREATURE_GO )
			self.giving = false
			self.giveto = -1
			self.giveamount = -1
			self.givestart = -1
		end
	elseif dlen == 0 or get_food( self.id ) < self.giveamount and get_food( self.id ) ~= get_max_food( self.id ) then
		print("gathering")
		gather( self )		
	else
		self.giving = true
		self.giveto, self.giveamount= pop( )
		self.givestart = get_food( self.id )
		print("id:" .. self.id .." got " .. self.giveto)

		for i=0,4 do
			if self.giveto == -1 or self.giveto == self.id then
				self.giving = true
				self.giveto, self.giveamount= pop( )
				self.givestart = get_food( self.id )self.giving = false
				break
			end
		end
						
		if self.giveto == self.id or self.giveto == -1 then
			self.giving = false
			self.giveto, self.giveamount= -1, -1
			self.givestart = -1
		end				
	end
end

-------------------------------------------------------------
-- KING OF THE HILL!
-------------------------------------------------------------
function king( self )
	if get_pos( self.id ) ~= get_koth_pos( ) then
		set_path( self.id, get_koth_pos( ) )
		set_state( self.id, CREATURE_WALK )
	else
		set_state( self.id, CREATURE_IDLE )
	end
end


-------------------------------------------------------------
-- DECIDER
-------------------------------------------------------------
function decide( self )
	decision = 0
	
	if get_type( self.id ) == 0 then
		decision = type0_task( self )
	elseif get_type( self.id ) == 1 then
		decision = type1_task( self )
	elseif get_type( self.id ) == 2 then
		decision = type2_task( self )
	end

	if decision == CREATURE_BREEDER then
		self.role = decision
		set_message( self.id, "BREED")
		breed( self )
	elseif decision == CREATURE_GATHERER then
		self.role = decision
		set_message( self.id, "GATHER")
		gather( self )
	elseif decision == CREATURE_FIGHTER then
		self.role = decision
		set_message( self.id, "FIGHT")
		fight( self )		
	elseif decision == CREATURE_PATIENT then
		self.role = decision
		set_message( self.id, "HEAL")
		heal( self )
	elseif decision == CREATURE_DISTRIBUTOR then
		self.role = decision
		set_message( self.id, "DISTRO")
		distribute( self )
	elseif decision == CREATURE_KING then
		self.role = decision
		set_message( self.id, "KING")
		if get_type( self.id ) == 1 then
			fight_king( self )
		else
			king( self )
		end	
	elseif decision == CREATURE_RUNNER then
		self.role = decision
		set_message( self.id, "RUN")
		run( self )
	elseif decision == CREATURE_CONVERTER then
		self.role = decision
		set_message( self.id, "CONVERT")
		convert( self, self.convert )
	end
end

function type0_task( self )
	total_type1 		= type_amount( 1 )
	total_type2 		= type_amount( 2 )
 	
 	creature_food 	= get_food( self.id )
	creature_health	= get_health( self.id )
	creature_type	= get_type( self.id )
	
	total_need = get_total_need( )
	total_food = get_total_food( )
	total_creatures = rlen
	
	closest_enemy_health, enemy_id = enemy_info( self )
	
	well_fed_node	= well_fed( creature_type, creature_food )	
	
	type1_overpop_node	= type1_overpopulated( total_creatures, total_type1 )
	type2_overpop_node	= type2_overpopulated( total_creatures, total_type2 )
	
	starved_node = population_starved( total_need, total_food )
		
	decision = {}
	for i=0,6 do
		decision[i] = 0
	end
	
	decision[0]	= zero_convert1 * type0_convert_type1( type1_overpop_node )
	decision[1] 	= zero_convert2 * type0_convert_type2( type2_overpop_node )
	decision[2] 	= zero_heal * not_healthy( creature_type, creature_health )		
	decision[3]	= zero_distribute * distribute_node( starved_node, well_fed_node )
	decision[4] 	= zero_runaway * type0_runaway( self, enemy_id )
	decision[5] 	= zero_koth * type0_king_of_the_hill( self )	
	decision[6]	= zero_fight * type0_fight( enemy_id )
	choice = decision_max( decision, 6 )
	
	if ANN_DECISION_TYPE0 then
		print("convert1? " .. decision[0])
		print("convert2? " .. decision[1])
		print("patient? " .. decision[2])
		print("distri? " .. decision[3])
		print("run? " .. decision[4])
		print("koth? " .. decision[5])
		print("fight? " .. decision[6])
	end	
	
	if choice == 0 then
		self.convert = 1
		return CREATURE_CONVERTER
	elseif choice == 1 then
		self.convert = 2
		return CREATURE_CONVERTER
	elseif choice == 2 then
		return CREATURE_PATIENT
	elseif choice == 3 then
		return CREATURE_DISTRIBUTOR
	elseif choice == 4 then
		return CREATURE_RUNNER
	elseif choice == 5 then
		return CREATURE_KING
	else
		return CREATURE_FIGHTER
	end
end

function type1_task( self )	
	total_type1 		= type_amount( 1 )
	total_type2 		= type_amount( 2 )
 	
 	creature_food 	= get_food( self.id )
	creature_health	= get_health( self.id )
	creature_type	= get_type( self.id )
	
	total_need = get_total_need( )
	total_food = get_total_food( )
	total_creatures = rlen
	
	closest_enemy_health, enemy_id = enemy_info( self )
	
	well_fed_node	= well_fed( creature_type, creature_food )	
	
	type1_overpop_node	= type1_overpopulated( total_creatures, total_type1 )
	type2_overpop_node	= type2_overpopulated( total_creatures, total_type2 )
	
	starved_node = population_starved( total_need, total_food )
	decision = {}
	for i=0,3 do
		decision[i] = 0
	end
	
	health_node = not_healthy( creature_type, creature_health )
	if health_node == 0 then
		health_node = 1
	else
		health_node = 0
	end
	decision[0]	= one_fight * type1_fight( enemy_id )
	decision[1] = one_heal * not_healthy( creature_type, creature_health )
	decision[2] = one_breed * health_node		
	decision[3]	= one_king * type1_king_of_the_hill( self )
	choice = decision_max( decision, 3 )
	
	if ANN_DECISION_TYPE1 then
		print("fight? " .. decision[0])
		print("heal? " .. decision[1])
		print("breed? " .. decision[2])
		print("koth? " .. decision[3])
	end
	
	
	if choice == 0 then
		return CREATURE_FIGHTER
	elseif choice == 1 then
		return CREATURE_PATIENT
	elseif choice == 2 then
		return CREATURE_BREEDER
	else
		return CREATURE_KING
	end
end

function type2_task( self )	 	
 	creature_food 	= get_food( self.id )
	creature_health	= get_health( self.id )
	creature_type	= get_type( self.id )
	
	total_need = get_total_need( )
	total_food = get_total_food( )
	
	closest_enemy_health, enemy_id = enemy_info( self )
	
	well_fed_node	= well_fed( creature_type, creature_food )	
	
	starved_node = population_starved( total_need, total_food )	
	
	decision = {}
	for i=0,3 do
		decision[i] = 0
	end
	
	health_node = not_healthy( creature_type, creature_health )
	if health_node == 0 then
		health_node = 1
	else
		health_node = 0
	end

	decision[0]	= two_heal * not_healthy( creature_type, creature_health )
	decision[1] = two_distribute * distribute_node( starved_node, well_fed_node )
	decision[2] = two_runaway * type2_runaway( self, enemy_id )
	decision[3]	= two_koth * type2_king_of_the_hill( self )
	choice = decision_max( decision, 3 )
	
	if ANN_DECISION_TYPE2 then
		print("heal? " .. decision[0])
		print("distro? " .. decision[1])
		print("run? " .. decision[2])
		print("koth? " .. decision[3])
	end
	
	
	if choice == 0 then
		return CREATURE_PATIENT
	elseif choice == 1 then
		return CREATURE_DISTRIBUTOR
	elseif choice == 2 then
		return CREATURE_RUNNER
	else
		return CREATURE_KING
	end
end

-------------------------------------------------------------
-- ARTIFICIAL NEURAL NETWORK
-------------------------------------------------------------
function type1_fight( enemy_id )
	if enemy_id == nil then
		return 0
	else
		return 1
	end 
end

function type0_fight( enemy_id )
	if enemy_id == nil then
		return 0
	elseif get_type( enemy_id ) == 2 then
		return 1
	else
		return 0
	end
end

function type2_runaway( self, enemy_id )
	if enemy_id == nil then
		return 0
	elseif get_type( enemy_id ) == 1 or get_type( enemy_id ) == 2 and get_distance( self.id, enemy_id ) < 1024 then
		return 1
	else
		return 0
	end
end

function type0_runaway( self, enemy_id )
	if enemy_id == nil then
		return 0
	elseif get_type( enemy_id ) == 1 and get_distance( self.id, enemy_id ) < 1024 then
		return 1
	else
		return 0
	end
end

function distribute_node( starved, well_fed )
	if starved == 1 or well_fed == 1 then
		return 1
	else
		return 0
	end
end

function type0_convert_type2( type2_population )
	if type2_population == 1 then
		return 0
	else
		return 1
	end		
end

function type0_convert_type1( type1_population )
	if type1_population == 1 then
		return 0
	else
		return 1
	end		
end

function not_healthy( creature_type, creature_health )
	if creature_type == 0 and creature_health >= w_health_type0 then
		return 0
	elseif creature_type == 1 and creature_health >= w_health_type1 then
		return 0
	elseif creature_type == 2 and creature_health >= w_health_type2 then
		return 0
	else
		return 1
	end
end

function well_fed( creature_type, creature_food )
	if creature_type == 0 and creature_food >= w_fed_type0 then
		return 1
	elseif creature_type == 1 and creature_food >= w_fed_type1 then
		return 1
	elseif creature_type == 2 and creature_food >= w_fed_type2 then
		return 1
	else
		return 0
	end
end

function type1_overpopulated( total_creatures, total_breeders  )	
	if total_breeders / total_creatures >= w_type1_total then
		return 1
	else
		return 0
	end		
end

function type2_overpopulated( total_creatures, total_flyers  )	
	if total_flyers / total_creatures >= w_type2_total then
		return 1
	else
		return 0
	end		
end

function type0_king_of_the_hill( self )	
	k = king_player( )
	if k == nil or (k == player_number and get_pos( self.id ) == get_koth_pos( )) then
		return 1
	else
		return 0
	end
end

function type1_king_of_the_hill( self )	
	k = king_player( )
	if k ~= nil and k ~= player_number then
		return 1
	else
		return 0
	end
end

function type2_king_of_the_hill( self )	
	k = king_player( )
	if k == nil or (k == player_number and get_pos( self.id ) == get_koth_pos( )) then
		return 1
	else
		return 0
	end
end

function gatherer_overpopulated( total_creatures, total_gatherers )
	percentage = total_gatherers / total_creatures
	
	if percentage >= w_gather_total then
		return 1
	else
		return 0
	end		
end

function fighter_overpopulated( total_creatures, total_fighters )
	percentage = total_fighters / total_creatures
	
	if percentage >= w_fight_total then
		return 1
	else
		return 0
	end		
end

function population_starved( total_need, total_food )
	if total_need < total_food  then
		return 1
	else
		return 0
	end
end

function potential_target( enemy_id, enemy_food, enemy_health )
	if enemy_id == nil then
		return 0
	elseif get_type( enemy_id ) == 0 or get_type( enemy_id ) == 2 then
		return 1
	elseif enemy_health < w_enemy_health then
		return 1
	else
		return 0
	end
end

function enemy_doing_well( enemy_score, my_score )
	if enemy_score > my_score then
		return 1
	else
		return 0
	end
end

-------------------------------------------------------------
-- GENERAL TASK CHECKER!
-------------------------------------------------------------
-- checks to see if someone is coming to give the agent food.
function creature_is_busy( self )
	if not non_critical_state( self ) then
		return true	
	elseif self.special_state == CREATURE_WAIT and non_critical_state( self ) then
		set_state( self.id, CREATURE_IDLE )
		return true		
	elseif self.special_state == CREATURE_WAIT then
		return true
	else		
		return false
	end
end

function non_critical_state( self )
	state = get_state( self.id )
	
	if state == CREATURE_SPAWN or state == CREATURE_FEED or state == CREATURE_HEAL or state == CREATURE_CONVERT or state == CREATURE_FIGHT then
		return false
	else
		return true
	end
end

function distro_check( self )
	if (self.role ~= CREATURE_DISTRIBUTOR and self.giving == true) or (not creature_exists( self.giveto )) then
		set_special_state( self.giveto, CREATURE_GO )
		self.giving = false
		self.giveto = -1
		self.giveamount = -1
		self.givestart = -1
	end
end

-------------------------------------------------------------
-- MAIN
-------------------------------------------------------------	
function Creature:main ()
	if init == 0 then
		print("game initialized")
		game_init( self )
		init = 1		
	end

	creature_init( self )
	
	creature_check( )
	distro_check( self )
	
	if not creature_is_busy( self ) then
		decide( self )
	end
	
	self:wait_for_next_round()
end

