function Creature:onSpawned( )
	kx, ky = get_koth_pos()
	if not set_path( self.id, kx, ky ) then suicide( self.id ); print( 'Killed myself: ' .. self.id) end -- it's a trap
	self.status = CREATURE_IDLE
end

function Creature:onKilled( killer )
	print( self.id .. ' killed by ' .. killer )
end

function Creature:onAttacked( attacker )
	if get_type( self.id ) == 1 then
		set_target( self.id, attacker ); set_state( self.id, CREATURE_ATTACK )
	end
end

function Creature:walkAndKill()
	victim, self.vX, self.vY, p, d = nearest_enemy( self.id )
	if get_distance( self.id, victim ) <= 256 then
		set_target( self.id, victim )
		set_state( self.id, CREATURE_ATTACK )
	else
		set_path( self.id, self.vX, self.vY )
		set_state( self.id, CREATURE_WALK )
	end
end

function getRandomCoords()
	return math.random( 256,12544 ), math.random( 256,8448 )
end

function Creature:eatAndGrow()
	x,y = get_pos( self.id )
	if get_state( self.id ) == CREATURE_CONVERT then	return
	elseif get_food(self.id) >= 8000 and get_state(self.id) == CREATURE_IDLE then
		set_convert( self.id, 1 ); set_state( self.id, CREATURE_CONVERT )
	elseif get_tile_food( self.id ) > 0 then		set_state( self.id, CREATURE_EAT )
	elseif get_state( self.id ) ~= CREATURE_WALK then
		self.destX, self.destY = getRandomCoords()
		set_path( self.id, self.destX, self.destY )
		set_state( self.id, CREATURE_WALK )
	end
end

function Creature:main()
	if get_type( self.id ) == 0 then self:eatAndGrow(); set_message( self.id, 'LittleBot' )
	else                             self:walkAndKill(); set_message( self.id, 'StupiBot' )
	end
	
end
