--[[
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
]]
-- author g²
-- version 0.7


--some defines
ATTACK_RANGE = 512
FLIGHT_RANGE = 2 * ATTACK_RANGE
groupWeHaveAKing = false
groupWeHaveAFlyer = false
kothIsWalkable = false

function Creature:onSpawned(parent)
	if parent then
	        print("Creature " .. self.id .. " spawned by " .. parent)
	else
 	        print("Creature " .. self.id .. " spawned")
	end

	-- test if koth is walkable
	local kingx, kingy = get_koth_pos()
	if set_path(self.id, kingx, kingy) then
		self.iCanWalkToKoth = true
		if not kothIsWalkable then 
			kothIsWalkable = self.id
		end
		-- set a new path 
		local x1, y1, x2, y2 = world_size()
		while not set_path(self.id,math.random(x1, x2),math.random(y1, y2)) do end
	end
end

function Creature:eat ()
	set_state(self.id,CREATURE_EAT)
	while get_food(self.id) < get_max_food(self.id) and get_tile_food(self.id) > 0 
	      and (not self:enemyInRange()) and get_health(self.id) > 2 do
		self:wait_for_next_round()
	end
end

function Creature:detstinationReached ()	
	self.currentx, self.currenty = get_pos(self.id)
	-- test if the x and y postition is near the destination
	if self.destx == self.currentx and self.desty == self.currenty then
		return true
	else return false 
	end
end

function Creature:go ()
-- 	test if already going
	if get_state(self.id) ~= CREATURE_WALK then
		-- if no valid path is set or reached destination
		if (not self.validPath) or self:detstinationReached() then
			local x1, y1, x2, y2 = world_size()

			self.destx = math.random(x1, x2)
			self.desty = math.random(y1, y2)

			while not set_path(self.id, self.destx, self.desty) do
				self.destx = math.random(x1, x2)
				self.desty = math.random(y1, y2)
			end
			-- the path has set and have to be gone
			self.validPath=true		
		end
		if not set_state(self.id, CREATURE_WALK) then 
			self:screen_message("stateErr")
			print("cannot set sate CREATURE_WALK in Creature:go() ID "..self.id)
		end
	end
end

-- returns the type of an enemy
function Creature:enemyInRange ()
	local enemy_id, ex, ey, eplayernum, edist = get_nearest_enemy(self.id)		
	if get_type(self.id) == 1 then 
		if edist and edist <= ATTACK_RANGE then      
			return get_type(enemy_id)	
		else 	return false
		end
	else 
		if edist and edist <= FLIGHT_RANGE then      
			return get_type(enemy_id)	
		else 	return false	
		end
	end
end

function attackNearestEnemy ()
	local creature_id, x, y, playernum, dist = get_nearest_enemy(self.id)
	if (not creature_id) or dist > ATTACK_RANGE then 
		return false
	else 
		set_target(self.id, creature_id)
		set_state(self.id, CREATURE_ATTACK)
	end
end

function Creature:becomeKing ()
	-- set lock, there can be only one king ;-)
	groupWeHaveAKing = self.id
	self:screen_message("King!")

	local kingx, kingy = get_koth_pos()
	if set_path(self.id, kingx, kingy) then
		set_state(self.id, CREATURE_WALK)
		while get_state(self.id) == CREATURE_WALK do
			self:wait_for_next_round()
		end
		set_state(self.id, CREATURE_IDLE)

		while self:enemyInRange() ~= 1 and get_health(self.id) > 30 do
			if get_health(self.id) < 70 and get_food(self.id) > 0 then
				set_state(self.id, CREATURE_HEAL)
				while get_health(self.id) < 100 and get_food(self.id) > 0 and not (self:enemyInRange() == 1)  do
					self:wait_for_next_round()
				end
				set_state(self.id, CREATURE_IDLE)
			end
			self:wait_for_next_round()
		end

	end
	self:screen_message("ID "..self.id)
	self.validPath = false
	--release lock
	groupWeHaveAKing = false
end

function Creature:mainWorker ()
	self:screen_message("ID "..self.id)

	if get_health(self.id) > 80 and kothIsWalkable and not groupWeHaveAKing and self:enemyInRange() ~= 1 then
		self:becomeKing()
	end

	if get_food(self.id) > 9000 and get_health(self.id) > 90 and not (self:enemyInRange() == 1) then
		if not groupWeHaveAFlyer and not kothIsWalkable then
			set_convert(self.id,2)
			groupWeHaveAFlyer = self.id
		else    set_convert(self.id,1)
		end

		set_state(self.id, CREATURE_CONVERT)
		while get_state(self.id) == CREATURE_CONVERT do
			self:wait_for_next_round()
		end  
	end

	if get_health(self.id) < 91 and get_food(self.id) > 0 and not (self:enemyInRange() == 1) then
		if not set_state(self.id, CREATURE_HEAL) then print("setStateErr CREATURE_HEAL") end
		while get_health(self.id) < 100 and get_food(self.id) > 0 and self:enemyInRange() ~= 1 do
			self:wait_for_next_round()
		end
	end

	if get_tile_food(self.id) > 0  and get_food(self.id) < get_max_food(self.id) and not (self:enemyInRange() == 1)  then 
		self:eat()
	elseif get_tile_food(self.id) == 0 or self:enemyInRange() == 1 then
		self:go()
	end
end

function Creature:mainBreeder ()
	if get_health(self.id) > 90 and kothIsWalkable and  not groupWeHaveAKing then
		self:becomeKing()
	end

	self:screen_message("ID "..self.id)
	
	local enemy_id, ex, ey, eplayernum, edist = get_nearest_enemy(self.id)	
	
	if edist and edist <= ATTACK_RANGE then
		set_target(self.id, enemy_id)
		set_state(self.id, CREATURE_ATTACK)
	else

		if get_food(self.id) > 14000 and get_health(self.id) > 80 then
		
			set_convert(self.id,1); set_state(self.id, CREATURE_SPAWN)
			while get_state(self.id) == CREATURE_SPAWN and (not self:enemyInRange()) do
				self:wait_for_next_round()
			end
		end
	
		if get_health(self.id) < 85 and get_food(self.id) > 0 then
			set_state(self.id, CREATURE_HEAL)
			while get_health(self.id) < 100 and get_food(self.id) > 0 and (not self:enemyInRange()) do
				self:wait_for_next_round()
			end
		end
	
		if get_tile_food(self.id) > 0 and get_food(self.id) < get_max_food(self.id) then 
			self:eat()
		elseif get_tile_food(self.id) == 0 then
			self:go()
		end	
	end
--	self:screen_message(self.id.."_"..get_cpu_usage())
end

-------------------------------------------------------------------------------
function Creature:mainFlyer ()
	self:screen_message("ID "..self.id)

	if get_health(self.id) > 80 and not groupWeHaveAKing then
		self:becomeKing()
	end
	
	if get_health(self.id) < 85 and get_food(self.id) > 0 and not (self:enemyInRange() == 1) then
		if not set_state(self.id, CREATURE_HEAL) then print("setStateErr CREATURE_HEAL") end
		while get_health(self.id) < 100 and get_food(self.id) > 0 and not (self:enemyInRange() == 1)  do
			self:wait_for_next_round()
		end
	end
	
-- 	choose a new destination befor go() -- good luck ;-)
	if self:enemyInRange() == 1 and get_state(self.id) ~= CREATURE_WALK then 
		self.validPath=false
	end


	if get_tile_food(self.id) > 0  and get_food(self.id) < get_max_food(self.id) and not (self:enemyInRange() == 1)  then 
		self:eat()
	elseif get_tile_food(self.id) == 0 then
		self:go()
	end
end

function Creature:main ()
-- check for invalid kothIsWalkable lock
	if kothIsWalkable and not creature_exists(kothIsWalkable) then
		kothIsWalkable = false
	end
-- maybe kothIsWalkable is false but another creature can walk to koth	
	if not kothIsWalkable and self.iCanWalkToKoth then
		local kingx, kingy = get_koth_pos()
		if set_path(self.id, kingx, kingy) then
			kothIsWalkable = self.id
		else
			self.iCanWalkToKoth = false
		end
		-- set a new path 
		local x1, y1, x2, y2 = world_size()
		while not set_path(self.id,math.random(x1, x2),math.random(y1, y2)) do end
	end



-- check for invalid kinglock (maybe creature was killed)
	if groupWeHaveAKing then
		if not creature_exists(groupWeHaveAKing) then 
			groupWeHaveAKing = false 
		end
	end

	if groupWeHaveAKing == self.id then 
		groupWeHaveAKing = false 
	end

-- check for invalid flyerlock
	if groupWeHaveAFlyer then
		if not creature_exists(groupWeHaveAFlyer) then 
			groupWeHaveAFlyer = false 
		end
	end

	if groupWeHaveAFlyer == self.id and get_type(self.id) ~= 2 then 
		groupWeHaveAFlyer = false 
	end

-- jump to type specific code
	if get_type(self.id) == 0 then 
		self:mainWorker()
	elseif get_type(self.id) == 1 then
		self:mainBreeder()
	elseif get_type(self.id) == 2 then
		self:mainFlyer()
	end
end

