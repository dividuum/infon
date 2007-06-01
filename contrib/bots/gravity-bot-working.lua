--------------kbot version 0.5-----------------
-- this bot is GPL
-- questions & stuff -> kazamatzuri@informatik.uni-wuerzburg.de
-- author kazamatzuri
-----------------------------------------------


--global communication
COUNT=0
HUNTRANGE=6*256
NOHUNTTHRESHOLD=3
DECAYFACTOR=2
--FOODWEIGHT_A=FOODWEIGHT
FOODWEIGHT=20
FOODWAIT=60*1000
--food above threshold -> into map
FOODTHRESHOLD=50
ENEMYWEIGHT=3
ENEMYWEIGHTAT=100
EATWEIGHT=110
GROWBARRIER=6 -- below this we just grow, no war
MIN_GROUNDFOOD=400 --  minimum availabe food before relocating swarm 
SWARMTARGET={}
--count BOTAVERAGE last steps in speedkeep
BOTAVERAGE=4
GRAVWEIGHT=10
--bots not bound by swarm law
ROGUEBOTSMOD=10
--RANDOMWALK max step size
STEPSIZE=1*256
--
CLUMBMODE=1
CLUMBSIZE=1.5*256
--
EMERGENCYRANGE=6*256
INNERRADIUS=3*256
FEEDMODE=false
FEEDMODETHRESHOLD=0.5
KOTHBOTLIMIT=12
KOTHBOT=-1

gravupdate=0

x1, y1, x2, y2 = world_size()
gridx=math.floor(x2/256)
gridy=math.floor(y2/256)
foodmap={}
timemap={}
foodlist={}
grid={}
enemys={}
a_enemys={}
-- first gravity center

gx,gy=get_koth_pos()
g={gx,gy}


function Creature:kpos()
    local rx,ry=self:pos()
    return {rx,ry}
end

-- initialization
function Creature:onSpawned()
    print("new bot enters the world ")
    COUNT=COUNT+1
    self.wt=self:getrandomstep()
    self.steps={}
    local i = 0    
    for i=1,BOTAVERAGE do
        self.steps[i]={}
    end
    self.cstep=0
end

-- overwriting global info (keypress i in telnetserver)
-- to be a bit more talkative
function info()
    local chkd=0
    for id, creature in pairs(creatures) do
    	posx,posy=get_pos(id)
        print(id .. ":" ..get_type(id) .. " on "..posx..":"..posy .. "==> hp:"..get_health(id)..". food:"..get_food(id) .." state:" ..
	get_state(id).." punkte: "..player_score(player_number))
        chkd=chkd+1
    end
    print("colony state: "..COUNT.." gravitycenter: "..g[1]..":"..g[2])
    COUNT=chkd
end

function suicideall()
    for id,creature in pairs(creatures) do
        suicide(id)
    end
end


function Creature:onAttacked(attacker)
    local ex,ey = get_pos(attacker)
    enemys[attacker]={ex,ey}
end


function Creature:onKilled(killer)
    if killer == self.id then
        print("kbot " .. self.id .. " suicided")
    elseif killer then 
        print("kbot " .. self.id .. " killed by Creature " .. killer)
    else
        print("kbot  " .. self.id .. " starved")
    end
    if KOTHBOT==self.id then KOTHBOT=-1 end    
    --foodmap[killer]=nil
end

function Creature:checkcount_hardcore()
    -- sometimes the own counting isn't right, so we have to check hardcore style
	local scnum=0
	for i,id in pairs(creatures) do
		scnum=scnum+1
	end
	return scnum
end

function Creature:printloc()
    -- talkative debug
	self.mx,self.my=self:pos()
	return ( self.id..":"..get_type(self.id).." on  "..self.mx .. ":"..self.my.." ")
end

function Creature:checkdata()
    -- per round data 
    self.loc=self:kpos()    
    self.t=self:type()
    self.hp = self:health()
end

function Creature:dist(x0,y0,x1,y1)
	return (math.sqrt((x0-x1)^2+(y0-y1)^2))
end

function Creature:reactattack()
    --p("------under attack-----")
    -- i am attacked
    local enemy_id, enemy_x, enemy_y, enemy_playernum, enemy_dist = nearest_enemy(self.id)
    a_enemys[enemy_id]=true
    enemys[enemy_id]={enemy_x,enemy_y}
    --todo: need attacked bool??
end

function Creature:expand()
    -- spawn/evolve
    if (self:food() > 8020 and get_type(self.id)==0 and not FEEDMODE) then
        set_convert(self.id,1)
    	set_state(self.id,CREATURE_CONVERT)
        --p("evolving <<<<<<<<<<<<<<")
        return true    
    end
    
    if (self:food() > 8020 and get_type(self.id)==1 and get_health(self.id)>40)then
    	set_state(self.id,CREATURE_SPAWN)
        return true 
    end
    return false
end



function Creature:hunt()
    if get_type(self.id)==0 then 
        return false
    end    

    local enemy_id, enemy_x, enemy_y, enemy_playernum, enemy_dist = nearest_enemy(self.id)
    if enemy_id==nil then
        --p("no enemy")
        return false
    end
        
    local enemy_type = get_type(enemy_id)   
    local enemy_health=get_health(enemy_id)
    enemys[enemy_id]={enemy_x,enemy_y}
    -- enemy to near? not my fault *G
    if enemy_dist < 512 then
        self:set_target(enemy_id)
    	set_state(self.id,CREATURE_ATTACK)
        --self:kattack(enemy_id,enemy_dist,enemy_x,enemy_y)
        self.huntmode=true
        return true
    end    
    -- easy prey anywhere?

    if enemy_dist < HUNTRANGE and enemy_type==0 and self.t==1 and self.hp>20  and COUNT>NOHUNTTHRESHOLD then    	
        --p("gone hunting------------")
        self.wt={enemy_x,enemy_y}
        set_path(self.id,enemy_x,enemy_y)
        self.huntmode=true
        return false
    end
    return false
end

function Creature:meal()
 --p(self:printloc() .. "feeding with from " .. self.actfood .. " food, now have: " .. self:food() .."f, " .. self:health() .."h")
-- have a nice meal
 self.actfood = get_tile_food(self.id)
-- trigger circle search around it 
  local x,y=7
    if self.actfood>50 then
        --self.wt=self:kpos()
        self:set_path(self.wt[1],self.wt[2])
    end
    if self.actfood>0 then
        set_state(self.id,CREATURE_EAT)        
        return true
    else 
        return false
    end
 end

	
function Creature:rndcircle(c,r)
	local dx=math.random(r*2+1)-r
	local dy=math.random(r*2+1)-r
	return {c[1]+dx,c[2]+dy}
end

-- run forest run, but whereto?
function Creature:escaperoute()
-- todo update to new format 
 local enemy_id, enemy_x, enemy_y, enemy_playernum, enemy_dist = nearest_enemy(self.id)
 local ex,ey=self:pos()
 nx,ny=self:getvalidtarget(math.floor(ex/256),math.floor(ey/256))
 return nx,ny
end

function Creature:updategravitycenter()
--p("updategrav")
local ng={}
local lcount=1
local realcount=0
local warriorcount=0
ng[1]=0
ng[2]=0
local blubid=0
local avfood=0
for id, creature in pairs(creatures) do
    blubid=id
    realcount=realcount+1
    if COUNT>KOTHBOTLIMIT and get_type(id)==1 and KOTHBOT==-1 and get_health(id)>50 then
        KOTHBOT=id
    end

    if get_type(id)==1 then 
        warriorcount=warriorcount+1 
        avfood=avfood+get_tile_food(id)
    end

    local x,y=get_pos(id)
    if get_state(id)==CREATURE_EAT and get_type(id)==0 then    
        ng[1],ng[2]=ng[1]+x*EATWEIGHT,ng[2]+y*EATWEIGHT
        lcount=lcount+EATWEIGHT-1
	if get_tile_food(id)> 1000 then
        	ng[1],ng[2]=ng[1]+x*EATWEIGHT,ng[2]+y*EATWEIGHT
        	lcount=lcount+EATWEIGHT-1	
	end
    else 
        ng[1],ng[2]=ng[1]+x,ng[2]+y
        lcount=lcount+1
    end
end


if lcount==1 then
	ng[1],ng[2]=get_pos(blubid)
end


COUNT=realcount
WCOUNT=warriorcount
INNERCIRCLE=1*256+(1.011^WCOUNT)*256

--todo move gravity in clumbmode better
--if COUNT<CLUMBMODE then
--    g=self:getrandomstep()
--    return
--end
--lcount=1
if WCOUNT>GROWBARRIER then
for i,c in pairs(enemys) do
    if creature_exists(i) then
		local x,y =get_pos(i)
		ng[1]=(ng[1]+ENEMYWEIGHT*x)
    	ng[2]=(ng[2]+ENEMYWEIGHT*y)
	else
		enemys[i]=nil
	end
    --p(ng[1]..":"..ng[2])
    lcount=lcount+ENEMYWEIGHT
end
end

for i,c in pairs(foodmap) do
    if foodmap[i]>FOODTHRESHOLD then
    local y= math.mod(i,gridx)
    local x= math.floor((i-y)/gridx)
    --print(i.."=>"..x..":"..y.." food: "..foodmap[i])
    ng[1]=(ng[1]+(c/100)*(x*256+128))
    ng[2]=(ng[2]+(c/100)*(y*256+128))
    lcount=lcount+(c/100)
    end
end

g[1],g[2]=ng[1]/lcount,ng[2]/lcount
if COUNT<CLUMBMODE then 
    STEPSIZE=CLUMBSIZE
    ROGUEBOTSMOD=5
elseif COUNT>10 and COUNT<20 then 
    STEPSIZE=5*256
    ROGUEBOTSMOD=6
elseif COUNT>20 and COUNT<40 then 
    STEPSIZE=7*256
    ROGUEBOTSMOD=8
else
    STEPSIZE=5*256
    ROGUEBOTSMOD=7
end
if foodcounter== nil then foodcounter=1 end
foodcounter=0
local blub=game_time()
for u=1,gridx do
    for v=1,gridy do
        if foodmap[u*gridx+v]~= nil then
            if foodmap[u*gridx+v]>FOODTHRESHOLD and math.abs(blub-timemap[u*gridx+v])> FOODWAIT then
                foodlist[foodcounter]={u,v}
                foodcounter=foodcounter+1
            end
        end
    end
end
--p((foodcounter).." places with food mapped")

avfood=avfood/warriorcount
--print(avfood/warriorcount.."    "..MIN_GROUNDFOOD)
if not swarmonthemove then
    SWARMTARGET=g
end
local kx,ky=get_koth_pos()
local nwfood = self:getnearestfood_swarm(kx,ky,1000) 
--print (nwfood[1].."   "..nwfood[2])
if avfood<MIN_GROUNDFOOD and nwfood~=nil then    
--    print("move")
    SWARMTARGET=nwfood  
    swarmonthemove=true
end
g[1]=math.floor(g[1])
g[2]=math.floor(g[2])
--print(g[1]..":"..g[2].." "..SWARMTARGET[1]..":"..SWARMTARGET[2])

local swarmdelta = self:dist(g[1],g[2],SWARMTARGET[1],SWARMTARGET[2])

if swarmdelta < 5*256 then
    swarmonthemove=false
end

if swarmonthemove  then
--    p("moving swarm")
    g=SWARMTARGET  
end
--print (WCOUNT.."    "..COUNT)
if (WCOUNT/COUNT)>FEEDMODETHRESHOLD and COUNT>3 then
--    p("feedmode on")
    FEEDMODE=true
else
--    p("feedmode off")
    FEEDMODE=FALSE
end

end

function Creature:getrandomstep()
    local x,y=self:pos()
    if self.v==nil then 
        self.v={}
    end

    if self.v[1]==nil then 
        self.v[1]=0 
        self.v[2]=0
    end
   
    if self.id==KOTHBOT then
        return get_koth_pos()
    end
    local pivot={}
   
    pivot[1]=self.v[1]*2 + (x+GRAVWEIGHT*g[1])/(GRAVWEIGHT+1)
    pivot[2]=self.v[2]*2 + (y+GRAVWEIGHT*g[2])/(GRAVWEIGHT+1)

    local s = STEPSIZE
    local n = {}
    local sc=1    
    repeat
    if get_type(self.id)==1 then
        n = self:rndcircle(g,INNERCIRCLE+sc)
        sc=sc+16
    else
        n = self:rndcircle(pivot,s)
        s=s+32
    end
    --p("calc "..pivot[1]..":"..pivot[2].."__"..s)
    until set_path(self.id,math.floor(n[1]),math.floor(n[2]))
	
    local delta={}    
    delta[1]=math.floor(-n[1]+x)
    delta[2]=math.floor(-n[2]+y)
    n[1]=math.floor(n[1])
    n[2]=math.floor(n[2])
    
    self.v[1]=(self.v[1]*BOTAVERAGE+delta[1])/(BOTAVERAGE+1)
    self.v[2]=(self.v[2]*BOTAVERAGE+delta[2])/(BOTAVERAGE+1)
    
    return n
end

function Creature:getnearestfood_swarm(x,y,thold)
    local max =0
    local actmax = -1
    local blub=game_time()
    for i=1,foodcounter-1 do
        if math.abs(blub-timemap[foodlist[i][1]*gridx+foodlist[i][2]])<FOODWAIT  and foodmap[foodlist[i][1]*256+foodlist[i][2]]>max then
            actmax=i
            max = foodmap[foodlist[i][1]*256+foodlist[i][2]]
        end
    end
    
    if actmax==-1 then return {math.random(x2),math.random(y2)} end 
    if {foodlist[actmax][1]*256+128,foodlist[actmax][2]*256+128}==nil then
        p("inner nil")
        return {math.random(x2),math.random(y2)}
    end
    return {foodlist[actmax][1]*256+128,foodlist[actmax][2]*256+128}

end


function Creature:getnearestfood()    
    local min =gridx*256
    local actmin = -1
    local minx,miny=0,0    
    local blub=game_time()
    --for i=1,foodcounter-1 do
    --    local d = self:dist(foodlist[i][1],foodlist[i][2],self.gridloc[1],self.gridloc[2])
        --if d < min and math.abs(blub-timemap[foodlist[i][1]*gridx+foodlist[i][2]])<FOODWAIT  and foodmap[foodlist[i][1]*256+foodlist[i][2]]>FOODTHRESHOLD then
        --    actmin=i
        --    min = d
        --end
    --end

    for id,creature in pairs(creatures) do
        local d = get_distance(self.id,id)
        if d<min and get_state(id)==CREATURE_EAT then
            min=d
            actmin=id
            minx,miny=get_pos(id)
        end
    end
    
    if actmin==-1 then return nil end 

    return {minx,miny}
end

function Creature:getnearesteating()
    local actmin=-1
    local min = 64*256
    local minx,miny=0,0    
    for id,creature in pairs(creatures) do
        local d = get_distance(self.id,id)
        if d<min and get_state(id)==CREATURE_EAT then
            min=d
            actmin=id
            minx,miny=get_pos(id)
        end
    end
    
    if actmin==-1 then return nil end 

    return {minx,miny}
end


function Creature:getvalidtarget()
	local blub=game_time()
	return get_koth_pos()
end

function Creature:move()
    local x,y=self:pos()

    if self.id==KOTHBOT then
        self.wt[1],self.wt[2]=get_koth_pos()
        set_path(self.id,self.wt[1],self.wt[2])
        set_state(self.id,CREATURE_WALK)
        if get_health(self.id)<50 then
            KOTHBOT=-1
        end
    elseif self:dist(x,y,self.wt[1],self.wt[2]) < 5 then
        local newc=nil
        newc=self:getnearesteating()
        if newc==nil then newc=self:getrandomstep() end 
        self.wt=newc
        self:set_path(newc[1],newc[2])
    end
    set_state(self.id,CREATURE_WALK)
    return true
end

function Creature:heal()
    if self:food()>2000 and get_health(self.id)< 65 then
        set_state(self.id,CREATURE_HEAL)
        return true
    else
        return false
    end
end

function Creature:updatefood()
    local t = self.gridloc[1]*gridx+self.gridloc[2]
    local blub=game_time()
    if foodmap[t] == nil then
       foodmap[t] = self.actfood
       timemap[t]=blub
    else
        if foodmap[t]< self.actfood then
            foodmap[t]=self.actfood
            timemap[t]=blub
        end
    end
end

function Creature:updategrid()
    local x,y=self:pos()
    local t=math.floor(x/256)*gridx+math.floor(y/256)
    if grid[t]==nil then 
        grid[t]={true,0}
    else        
    end        
end

function Creature:getnearestmaster()
    local actmin=-1
    local min=gridx*256
    for id,creature in pairs(creatures) do
        if get_type(id)==1 then
            local d=get_distance(self.id,id)
            if d < min then
                actmin=id
                min=d
            end
        end
    end
    
    if actmin==-1 then 
        return nil
    else
        local x,y=get_pos(actmin)
        return {x,y,actmin}
    end

end


function Creature:go_feed()
    if self:food()>5000 and self:health()>60 and get_type(self.id)==0 then
        local m=self:getnearestmaster()
        if m==nil then 
            return false
        end 
        if self:dist(m[1],m[2],self.loc[1],self.loc[2]) < 256 then
            set_target(self.id,m[3])
            set_state(self.id,CREATURE_FEED)
            return true
        else
            self.wt=m
            self:set_path(self.wt[1],self.wt[2])
            set_state(self.id,CREATURE_WALK)
        end
    end
end

function Creature:main()
     
--    local x,y=get_koth_pos()
--    set_path(self.id,x,y)
--    set_state(self.id,CREATURE_WALK)
--    self:wait_for_next_round()
        
    --set_message(self.id,self.id.."OF"..player_number)
    if self.wt==nil then self.wt=self:kpos() end
    self.loc=self:kpos()
    self.gridloc={math.floor(self.loc[1]/256),math.floor(self.loc[2]/256)}
    if self.foodstepping==nil then self.foodstepping=1 end
    self.huntmode=false
    self.t=self:type()
    self.hp = self:health()    
    if math.abs(gravupdate-game_time())>1000 then
        self:updategravitycenter()
        gravupdate=game_time()
    end
    
    while self:is_converting() or self:is_spawning() or self:is_healing() do
        self:wait_for_next_round()
    end


    while self:is_feeding() and self:food()>1000 do
        self:wait_for_next_round()
    end

    self.actfood=get_tile_food(self.id)
    self:updategrid()
    self:updatefood()    
--p("blub")    
    if self:hunt() then
--    p(self.id .. "is hunting ")
    elseif FEEDMODE and self:go_feed() then        
        set_message(self.id,"feeding")
    elseif self:expand() then
        set_message(self.id,"evolving")
    elseif self:heal() then
        set_message(self.id,"healing")
    elseif self:meal() then
        set_message(self.id,"eating")
    elseif self:move() then
        if KOTHBOT==self.id then
            set_message(self.id,"KINGBOT")
        else
            set_message(self.id,"moving")
        end
    else 
        set_message(self.id,"else ")
    end
    self:wait_for_next_round()    
    
end

