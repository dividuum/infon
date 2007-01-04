-- ip and port to bind listen socket
listenaddr = "0.0.0.0"
listenport = 1234

-- password for the 'shell' command. disabled if empty or missing.
debugpass  = ""

-- message displayed every 10 seconds
join_info  = "this message can be changed in config.lua"

-- maps to rotate
maps       = {"foo", "gpn", "water", "cn", "owl" }

-- rules file to use. 
rules      = "default"

-- filename prefix for demo files. disabled if unset.
-- demo       = "infond"

-- time limit for each map
time_limit = nil 

-- score limit for each map
score_limit= 500 

-- access control list. default is to allow connections 
-- from all clients. 
acl        = {{ pattern = "^ip:127\.0\..*"             }, 
              { pattern = "^special:.*"                }, 
              { pattern = ".*"                         },
              { pattern = ".*", deny = "access denied" }}

-- available highlevel apis
highlevel  =  { 'player-highlevel' }

-- show banner?              
banner = [[
                     .___             `.
        ___              `~\\           |               \\
      o~   `.               |         /'                |
    .----._ `|             ,'       /'              _./'
    `o     `\\|___       __,|----'~~~~T-----,__  _,'~
          /~~o   `~>-/|~ '   ' ,   '      '   ~~\\_
         |_      <~   |   ' ,   ' '   '  ' , '     \\
           `-...-'~\./' '     '     '   '   '  , '  >
                     `-, __'  ,  '  '  , ' ,   '_,'-'
                       /'   `~~~~~~~|`--------~~\\
                     /'            ,'            `.
              ~~`---'             /               |
                               ,-'              _/'
]] -- von http://www.asciiworld.com/animals_birds.html

-- competition = true
-- competition_log  = "competition.log"
-- competition_bots = { { source = "bot1.lua" }, { source = "bot2.lua", log = "bot2.log", name = "2ndbot" } }
-- time_limit       = 10 * 60 * 1000
-- score_limit      = nil
-- maps             = { "foo", "foo", "foo" }
-- listenaddr       = nil

-- Things to do once after the first game started
--
-- function autoexec()
--    start_bot("bot1.lua")
-- end

-- function checking user/password combinations.
-- if pass is nil, return a boolean to indicate
-- if the username requires a password. If password
-- is set, return true if the password is correct.
--
-- function check_name_password(user, pass)
-- end


