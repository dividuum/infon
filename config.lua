-- ip and port to bind listen socket
listenaddr = "0.0.0.0"
listenport = 1234

-- password for the 'shell' command. disabled if empty or missing.
debugpass  = ""

-- password to disable no-client kicking. by default, a player is
-- kicked, once no client is connected for 120 seconds. using this
-- password, players can disable this limit. disabled if empty or 
-- missing.
nokickpass = ""

-- message displayed every 10 seconds
join_info  = "this message can be changed in config.lua"

-- maps to rotate
maps       = {"foo", "gpn", "water", "cn", "owl", "stripeslice", "castle", "infon" }

-- rules file to use. 
rules      = "default"

-- filename prefix for demo files. disabled if unset.
-- demo       = "infond"

-- time limit for each map
time_limit = nil 

-- score limit for each map
score_limit = 500 

-- access control list. default is to allow connections 
-- from all clients. 
acl        = {{ pattern = "^ip:127\.0\..*"             }, 
              { pattern = "^special:.*"                }, 
              { pattern = ".*"                         },
              { pattern = ".*", deny = "access denied" }}

-- available highlevel apis
highlevel  =  { 'oo', 'state' }

-- disable joining?
-- disable_joining = "no joining!"

-- show banner?              
banner = [[
                    .--..-.
                   /   / \ \'-.  _
                  /   /  _( o)_ ; \
                  O  O  /      ' -.\
                        \___;_/-__ ;)
                          __/   \ //-.
                         ; /     //   \
                         ' (INFON  )   )
                          \(       )   )
                           '      /   /
                            '.__.- -'
                          _ \\   // _
                         (_'_/   \_'_)
]] 

-- allowed files for dofile
dofile_allowed = { 
    menu = "Botcode Menu System";
}

-- Disable Linehook Function.
-- This function offers some advanced debugging features,
-- but has not been tested enough. Do not enable, unless
-- you know what you're doing.
linehook = false

-- DO NOT ENABLE THE DEBUGGER IF OTHER PLAYERS CAN CONNECT
-- TO YOUR SERVER. The debugger is not yet secured against
-- malicious users.
debugger = false

-- competition = {
--     log  = "competition.log";
--     bots = {
--         { source = "bot1.lua" }, 
--         { source = "bot2.lua", 
--           log    = "bot2.log", 
--           name   = "2ndbot" }
--     }
-- }
-- 
-- maps             = { "foo", "foo", "foo" }
-- time_limit       = 10 * 60 * 1000
-- score_limit      = nil
-- listenaddr       = nil

-- Things to do once after the first game started
--
-- function autoexec()
--     start_bot{ source   = "contrib/bots/easybot.lua",
--                name     = "foo",
--                password = "secret",
--                api      = "state" }
-- end

-- function checking user/password combinations.
-- if pass is nil, return a boolean to indicate
-- if the username requires a password. If password
-- is set, return true if the password is correct.
--
-- function check_name_password(user, pass)
-- end

-- Uncomment the following lines to announce your
-- server to the Master Server at infon.dividuum.de.
-- This will send a tiny UDP packet every minute.
--
-- master_ip  = '217.28.96.154' -- infon.dividuum.de
-- servername = 'An Infon Server'

