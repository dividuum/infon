-- By Chromix
-- !!! unfinished !!!


-- max tilefood = 9999
--local gtMap;
--gtMap = nil;
gtCreatures = gtCreatures or {};
unexploredfields = unexploredfields or 0;
local currTick = 0;



local function asserttype( _var, _type )
	if type( _var ) ~= _type then
		assert( false, "type " .. type( _var ) .. " matches " .. _type );
	end
end

--[[
local function heap_pop( _heap )
	local retNode = table.remove( _heap, 1 );
	if _heap[1] then
		table.insert( _heap, 1, table.remove(_heap, table.getn(_heap)) );
	
		local oldPosition = 1;
		local currValue = _heap[oldPosition];
		while true do
			local newPosition = oldPosition * 2;
			local node = _heap[newPosition];
			if node and node.totalCosts < currValue.totalCosts then
				_heap[oldPosition] = node;
				_heap[newPosition] = currValue;
				oldPosition = newPosition;
			else
				newPosition = newPosition + 1;
				node = _heap[newPosition];
				if node and node.totalCosts < currValue.totalCosts then
					_heap[oldPosition] = node;
					_heap[newPosition] = currValue;
				else
					break;
				end
			end
		end
	end
	
	asserttype( retNode, "table" );
	return retNode;
end

--	Node:
--		totalCosts

function heap_push( _heap, _what )
	asserttype(_heap, "table");
	asserttype(_what, "table");

	local newSize = table.getn( _heap ) + 1;
	table.insert( _heap, _what );
	if newSize == 1 then
		return;
	end

	local oldSize = newSize;
	newSize = math.floor( oldSize / 2 );
	local nextNode = _heap[newSize];
	while newSize >= 1 and nextNode.totalCosts > _what.totalCosts do
		_heap[oldSize] = nextNode;
		_heap[newSize] = _what;
		oldSize, newSize = newSize, math.floor( newSize / 2 );
		nextNode = _heap[newSize];
	end
end
]]

function scanMap( _creature )
	local tMap = {};
	local minx, miny, maxx, maxy = world_size();
	local crX, crY = _creature:pos();
	local totalfields = 0;
	for x = math.floor(minx/256), math.floor(maxx/256) do
		tMap[x] = {};
		for y = math.floor(miny/256), math.floor(maxy/256) do
			local walkable = ( _creature:set_path( x * 256 + 128, y * 256 + 128 ) or ( x == crX and y == crY ) ) and { food = 0, seenfood = false, lastvisit = 0, nearunexplored = 0, nearfood = 0 } or nil;
			if walkable then
				if tMap[x][y-1] then
					tMap[x][y-1].nearunexplored = tMap[x][y-1].nearunexplored + 1;
					walkable.nearunexplored = walkable.nearunexplored + 1;
				end
				if tMap[x-1] and tMap[x-1][y] then
					tMap[x-1][y].nearunexplored = tMap[x-1][y].nearunexplored + 1;
					walkable.nearunexplored = walkable.nearunexplored + 1;
				end
				tMap[x][y] = walkable
				totalfields = totalfields + 1;
			end
			
			if get_cpu_usage() > 80 then
				_creature:wait_for_next_round();
			end
		end
	end
	unexploredfields = totalfields;
	return tMap;
end

function onGameStart()
	gtMap = nil;
	gtCreatures = {};
end

Creature.ROLE_EXPLORER = 1;
Creature.ROLE_ASPIRING_GUARDIAN = Creature.ROLE_EXPLORER + 1;
Creature.ROLE_GUARDIAN = Creature.ROLE_ASPIRING_GUARDIAN + 1;

function Creature:hasNoRole()
	return self.currRole == nil;
end

function Creature:hasRoleExplorer()
	return self.currRole == self.ROLE_EXPLORER;
end

function Creature:takeRoleExplorer()
	self.currRole = self.ROLE_EXPLORER;
end

function Creature:hasRoleAspiringGuardian()
	return self.currRole == self.ROLE_ASPIRING_GUARDIAN;
end

function Creature:takeRoleAspiringGuardian()
	self.currRole = self.ROLE_ASPIRING_GUARDIAN;
end

function Creature:hasRoleGuardian()
	return self.currRole == self.ROLE_GUARDIAN;
end

function Creature:takeRoleGuardian()
	self.currRole = self.ROLE_GUARDIAN;
end

function Creature:hasGuardField()
	return self.guardPatch ~= nil;
end


function Creature:estimateNearbyFood()
	local myX, myY = self.currX, self.currY;
	local estimate = 0;
	for x = myX - 2, myX + 2 do
		local xRow = gtMap[x];
		if xRow then
			for y = myY - 2, myY + 2 do
				local tile = xRow[y];
				if tile and tile.seenfood then
					estimate = estimate + tile.food + (currTick - tile.lastvisit) * 0.001
				end
			end
		end
	end
	
	return math.floor(estimate);
end

function Creature:findNearbyFood()
	local myX, myY = self.currX, self.currY;
	local maxfood = 0;
	local foodX, foodY;
	for x = myX - 2, myX + 2 do
		local xRow = gtMap[x];
		if xRow then
			for y = myY - 2, myY + 2 do
				local tile = gtMap[x][y];
				if tile and tile.seenfood then
					local estimate = tile.food + (currTick - tile.lastvisit) * 0.001
					if maxfood < estimate then
						foodX, foodY = x, y;
						maxfood = estimate;
					end
				end
			end
		end
	end
	
	return foodX, foodY;
end

function Creature:checkForNewField()
	local myX, myY = self.currX, self.currY;
	assert( gtMap[myX][myY].nearunexplored >= 0 );
	assert( gtMap[myX][myY].nearfood >= 0 );
	if gtMap[myX][myY].lastvisit == 0 then
		unexploredfields = unexploredfields - 1;
		if gtMap[myX][myY-1] then
			gtMap[myX][myY-1].nearunexplored = gtMap[myX][myY-1].nearunexplored - 1;
		end
		if gtMap[myX][myY+1] then
			gtMap[myX][myY+1].nearunexplored = gtMap[myX][myY+1].nearunexplored - 1;
		end
		if gtMap[myX-1] and gtMap[myX-1][myY] then
			gtMap[myX-1][myY].nearunexplored = gtMap[myX-1][myY].nearunexplored - 1;
		end
		if gtMap[myX+1] and gtMap[myX+1][myY] then
			gtMap[myX+1][myY].nearunexplored = gtMap[myX+1][myY].nearunexplored - 1;
		end
		return true;
	else
		return false;
	end
end

function Creature:checkForFoodOnField()
	local myX, myY = self.currX, self.currY;
	local food = self:tile_food();
	gtMap[myX][myY].food = food;
	gtMap[myX][myY].lastvisit = currTick;
	if food > 0 and not gtMap[myX][myY].seenfood then
		gtMap[myX][myY].seenfood = true;
		
		if gtMap[myX][myY-1] then
			gtMap[myX][myY-1].nearfood = gtMap[myX][myY-1].nearfood + 1;
		end
		if gtMap[myX][myY+1] then
			gtMap[myX][myY+1].nearfood = gtMap[myX][myY+1].nearfood + 1;
		end
		if gtMap[myX-1] and gtMap[myX-1][myY] then
			gtMap[myX-1][myY].nearfood = gtMap[myX-1][myY].nearfood + 1;
		end
		if gtMap[myX+1] and gtMap[myX+1][myY] then
			gtMap[myX+1][myY].nearfood = gtMap[myX+1][myY].nearfood + 1;
		end
		
		return food, true;
	else
		return food, false;
	end
end

-- Called after the Creature was created. You cannot 
-- call long-running methods (like moveto) here.
function Creature:onSpawned(parent)
    if parent then
        print("Creature " .. self.id .. " spawned by " .. parent)
    else
        print("Creature " .. self.id .. " spawned")
    end
	
	gtCreatures[self] = true;
--	self:takeRoleExplorer();
end

function Creature:findInterestingUnexploredField()
	local tInteresting = {};
	local myX, myY = self.currX, self.currY;
	local bestscore;
	for x, yEntry in pairs( gtMap ) do
		for y, entry in pairs( yEntry ) do
			local score = 0;

			-- unexplored fields are interesting
			if entry.lastvisit == 0 and x ~= myX and y ~= myY then
				-- they're even more interesting, if there are more unexplored fields next to them
				-- fields that contain food might have other food fields next to them
				score = 1 + entry.nearunexplored + (6 * entry.nearfood);
				
				score = score - math.sqrt( math.pow( myX - x, 2 ) + math.pow( myY - y, 2 ) ) / 10;
				if not bestscore or score > bestscore then
					bestscore = score;
					table.insert( tInteresting, { score = score, x = x, y = y } );
				end
			end
		end
	end
	table.sort( tInteresting, function( _e1, _e2 ) return _e1.score > _e2.score end );
	local bestentry = tInteresting[1];
	if bestentry then
		return bestentry.x, bestentry.y;
	else
		return false, false;
	end
end

function Creature:findNextFoodField()
	local myX, myY = self.currX, self.currY;
	local bestDist, foodX, foodY;
	for x, yEntry in pairs( gtMap ) do
		for y, entry in pairs( yEntry ) do
			local dist = math.sqrt( math.exp( myX - x, 2 ) + math.exp( myY - y, 2 ) );
			if not bestDist or dist < bestDist then
				bestDist = dist;
				foodX, foodY = x, y;
			end
		end
	end
	
	return foodX, foodY;
end

function Creature:goTo( _x, _y )
	if self:set_path( _x * 256 + 128, _y * 256 + 128 ) then
		self.targetX = _x;
		self.targetY = _y;
		self:begin_walk_path();
		return true;
	else
		return false;
	end
end

function Creature:stopMoving()
	if self:is_walking() then
		self:begin_idling();
		self.targetX = self.currX;
		self.targetY = self.currY;
	end
end

function Creature:checkForArrival()
--	if self:is_walking() and self.currX == self.targetX and self.currY == self.targetY then
--		self:begin_idling();
--		self:screen_message( "Arrived" );
--		return true;
--	else
--		return false;
--	end
end


function Creature:createGuardField()
	local tTiles = {};
	local tStack = {};
	local tKnown = {};
	local myX, myY = self.currX, self.currY;
	local totalX, totalY, tilecount = 0, 0, 0;
	local currTile = gtMap[myX][myY];
	assert( currTile.seenfood or self.guardPatch );
	if not currTile.seenfood and self.guardPatch then
		currTile = next( self.guardPatch.tTiles );	-- recreate
	else
		currTile.X, currTile.Y = myX, myY;	-- create new
	end
	
	tKnown[currTile] = true;
	while currTile do
		tTiles[currTile] = true;
		tilecount = tilecount + 1;
		for _, nextOffset in pairs( { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } } ) do
			local nextTile = gtMap[currTile.X + nextOffset[1]] and gtMap[currTile.X + nextOffset[1]][currTile.Y + nextOffset[2]];
			if nextTile and nextTile.seenfood and not tKnown[nextTile] then
				tKnown[nextTile] = true;
				nextTile.X = currTile.X + nextOffset[1];
				nextTile.Y = currTile.Y + nextOffset[2];
				table.insert( tStack, nextTile );
				totalX = totalX + nextTile.X;
				totalY = totalY + nextTile.Y;
			end
		end
		
		currTile = table.remove( tStack );
	end
	
	assert( next( tTiles ) );
	
	local centerX, centerY = math.floor( totalX / tilecount ), math.floor( totalY / tilecount );
	while not gtMap[centerX] or not gtMap[centerX][centerY] do
		centerX = centerX - 1 + math.random( 2 );
		centerY = centerY - 1 + math.random( 2 );
		-- TODO: improve this
	end
	
	self.guardPatch = { centerX = centerX, centerY = centerY, tTiles = tTiles, lastscan = 0, scanInProgress = false };
end

function Creature:needsToCheckGuardField()
	return self.guardPatch and not self.guardPatch.scanInProgress and self.guardPatch.lastscan < currTick - 10 * 1000;
end

function Creature:isCheckingGuardField()
	return self.guardPatch and self.guardPatch.scanInProgress;
end

function Creature:beginCheckGuardField()
	self.guardPatch.scanInProgress = true;
	local plan = self.guardPatch.tScanplan;
	if not plan then
		plan = {};
		local tKnown = {};
		self.guardPatch.tScanplan = plan;
		
		-- plan: check all fields + the border fields that don't belong to the guardTiles
		for tile in pairs( self.guardPatch.tTiles ) do
			table.insert( plan, { X = tile.X, Y = tile.Y } );
			for _, nextOffset in pairs( { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } } ) do
				local adjTile = gtMap[tile.X + nextOffset[1]] and gtMap[tile.X + nextOffset[1]][tile.Y + nextOffset[2]];
				if adjTile and not self.guardPatch.tTiles[adjTile] and not tKnown[adjTile] then
					tKnown[adjTile] = true;
					table.insert( plan, { X = tile.X + nextOffset[1], Y = tile.Y + nextOffset[2] } );
				end
			end
		end
	end
	
	self.guardPatch.scanstep = 1;
	self:goTo( plan[self.guardPatch.scanstep].X, plan[self.guardPatch.scanstep].Y );
end

function Creature:continueCheckGuardField()
	local myX, myY = self.currX, self.currY;
	
	-- did we reach the target field, or stopped before that?
	if myX ~= self.guardPatch.tScanplan[self.guardPatch.scanstep].X or myY ~= self.guardPatch.tScanplan[self.guardPatch.scanstep].Y then
		for i = 1, table.getn( self.guardPatch.tScanplan ) do
			if myX == self.guardPatch.tScanplan[i].X and myY == self.guardPatch.tScanplan[i].Y then
				table.remove( self.guardPatch.tScanplan, i );
				break;
			end
		end
	else
		--self.guardPatch.scanstep = self.guardPatch.scanstep + 1;
		table.remove( self.guardPatch.tScanplan, 1 );
	end
	if self.guardPatch.scanstep > table.getn( self.guardPatch.tScanplan ) then
		self.guardPatch.lastscan = currTick;
		self.guardPatch.scanInProgress = false;
		self:screen_message( "Return" );

		-- recreate it
		self:createGuardField();
		self:goTo( self.guardPatch.centerX, self.guardPatch.centerY );
	else
		self:goTo( self.guardPatch.tScanplan[self.guardPatch.scanstep].X, self.guardPatch.tScanplan[self.guardPatch.scanstep].Y );
	end
end

-- Called each round for every attacker on this
-- creature. No long-running methods here!
function Creature:onAttacked(attacker)
	if self:hasRoleExplorer() then
		self:screen_message( "*squeek*" );
	elseif self:hasRoleAspiringGuardian() then
		self:screen_message( "mean :-(" );
	elseif self:hasRoleGuardian() then
		self:screen_message( "*ouch*" );
	end
end


-- Called by typing 'r' in the console, after creation (after 
-- onSpawned) or by calling self:restart(). No long-running 
-- methods calls here!
function Creature:onRestart()
    print("Creature " .. self.id .. " restarting")
		
	local myX, myY = self:pos();
	self.targetX, self.targetX = math.floor(myX/256), math.floor(myY/256);
	self:takeRoleExplorer();
end


-- Called after beeing killed. Since the creature is already 
-- dead, self.id cannot be used to call the Lowlevel API.
function Creature:onKilled(killer)
    if killer == self.id then
        print("Creature " .. self.id .. " suicided")
    elseif killer then 
        print("Creature " .. self.id .. " killed by Creature " .. killer)
    else
        print("Creature " .. self.id .. " died")
    end
	
	gtCreatures[self] = nil;
end

-- Your Creature Logic here :-)
function Creature:main()
--	self:screen_message( self.id.." of "..player_number );
	
	if not gtMap then
		gtMap = true;
		gtMap = scanMap( self );
	end
	
	-- wait for map creation
	while gtMap == true do
		self:wait_for_next_round();
	end
	
	-- update vars
	local myX, myY = self:pos();
	myX, myY = math.floor( myX / 256 ), math.floor( myY / 256 );
	self.currX, self.currY = myX, myY;
	currTick = game_time();
	
	-- check fields & update map
	local isNewField = self:checkForNewField();
	local foodOnField, foundNewFoodSource = self:checkForFoodOnField();
	
	if get_cpu_usage() > 95 then
		return;	-- avoid getting killed by the cpu limit
	end

	if self:hasNoRole() then
		self:takeRoleExplorer();
	end
	
	self:checkForArrival();

	if self:hasRoleExplorer() then
		-- emergency heal
		if self:health() < 20 and self:food() > 0 and not self:is_healing() then
			self:screen_message( "Emergncy" );
			self:begin_healing();
			return;
		end
		
		if foundNewFoodSource then
			self:stopMoving();
			if self:hasGuardField() then
				self:screen_message( "Yay :-)" );
				self:createGuardField();
				self:takeRoleGuardian();
			end

			
			self:begin_eating();
			
			local thefood = self:estimateNearbyFood();
			self:screen_message( "Yummy!" );
			if thefood + self:food() > 8100 and self:health() > 80 then
				self:begin_idling();
				self:takeRoleAspiringGuardian();
				self:screen_message( "AGuard" );
				assert( self:hasRoleAspiringGuardian() );
				return;
			end
		end

		if self:food() > 8100 and self:health() > 80 then
			self:begin_idling();
			self:takeRoleAspiringGuardian();
			self:screen_message( "AGuard" );
			assert( self:hasRoleAspiringGuardian() );
			return;
		end
		
		if self:is_idle() then
			if self:health() < 95 and self:food() > 0 then
				self:begin_healing();
				return;
			end
			-- find an interesting field to visit
			local cannotMove = true;
			local intX, intY;
			if get_cpu_usage() < 80 then
				intX, intY = self:findInterestingUnexploredField();
				if intX then
					if self:goTo( intX, intY ) then
						cannotMove = false;
						self:screen_message( "Explore" );
					end
				else
					-- no unexplored fields left
					-- TODO: Switch to another role
				end
			end
			
			-- can't move? There *must* be a field we can move to
			while cannotMove do
				self:screen_message( "AuxMove" );
				intX = myX - 2 + math.random(4);
				intY = myY - 2 + math.random(4);
				cannotMove = not self:goTo( intX, intY );
			end
		else
			self:wait_for_next_round();
		end
		
	elseif self:hasRoleAspiringGuardian() then
		if self:health() < 90 and self:food() > 0 and not self:is_healing() and not self:is_converting() then
			if not self:is_eating() or ( self:is_eating() and self:health() < 20 ) then
				self:begin_healing();
			end
		end
	
		if self:is_idle() then
			if self:food() > 8000 and self:health() >= 80 then
				self:screen_message( "Evolving" );
				self:convert( 1 );
				self:screen_message( "Guard" );
				self:takeRoleGuardian();
				return;
			elseif self:estimateNearbyFood() + self:food() > 8100 then
				if foodOnField > 0 then
					self:begin_eating();
				else
					local intX, intY = self:findNearbyFood();
					if intX then
						if not self:goTo( intX, intY ) then
							self:takeRoleExplorer();
						end
					else
						self:takeRoleExplorer();
					end
				end
			else
				self:takeRoleExplorer();
			end
		end
		
	elseif self:hasRoleGuardian() then	
		if self:is_walking() and self:isCheckingGuardField() and foodOnField > 0 and self:food() < self:max_food() then
			self:stopMoving();
		end
		
		if self:is_idle() then
			self:screen_message( "Guard :)" );

			if self:health() < 40 and self:food() < 200 then
				local thefood = self:estimateNearbyFood();
				print( "EstFood: " .. thefood );
				if thefood < 1000 then
					self:takeRoleExplorer(); 	-- become explorer before starving
					return;
				end
			end

			
			if self:health() < 20 and self:food() > 0 then
				self:screen_message( "Emergncy" );
				self:begin_healing();
			
			elseif not self:hasGuardField() then
				if gtMap[myX][myY].seenfood then
					self:createGuardField();
				else
					local intX, intY = self:findNearbyFood();
					if not intX then
						intX, intY = self:findNextFoodField();
					end
					
					self:screen_message( "GoFood" );
					self:goTo( intX, intY );
				end				
			
			elseif self:health() < 98 and self:food() > 0 then
				self:screen_message( "StayFit" );
				self:begin_healing();
			
			elseif self:health() > 95 and self:food() > 9000 then
				self:screen_message( "Multiply" );
				self:begin_spawning();
			
			elseif self:needsToCheckGuardField() or ( self:food() == 0 and not self:isCheckingGuardField() ) then
				self:screen_message( "ChkFood" );
				self:beginCheckGuardField();
			
			elseif foodOnField > 0 and self:food() < self:max_food() then
				self:screen_message( "AllMine" );
				self:begin_eating();
				
			elseif self:isCheckingGuardField() then
				self:screen_message( "ChkFood" );
				self:continueCheckGuardField();
				
			end	
		end
	end

	--local kohx, kohy = get_koth_pos();
	--print( string.format( "Tick:%d, ID:%d, X:%d, Y:%d, Food: %d, type:%d, tilefood:%d", game_time(), self.id, myX/256, myY/256, self:food(), self:type(), self:tile_food() ) );
	
	--print( "CPU usage: " .. get_cpu_usage() );
	-- print( currTick )
end
