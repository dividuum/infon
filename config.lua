-- IP and port to listen for incoming connections. Change the listenaddr
-- to "0.0.0.0" to listen to all addresses. This allows other players to 
-- join your server.
listenaddr = "127.0.0.1"
listenport = 1234

-- password for the 'shell' command. disabled if empty or missing.
debugpass = ""

-- password to disable no-client kicking. by default, a player is
-- kicked, once no client is connected for 120 seconds. using this
-- password, players can disable this limit. disabled if empty or 
-- missing.
nokickpass = ""

-- message displayed every 10 seconds
-- join_info = "join #infon on irc.freenode.net for help"

-- maps to rotate
maps = {"foo", "gpn", "water", "cn", "owl", "stripeslice", "castle", "infon", "pacman" }

-- rules file to use. 
rules = "default"

-- filename prefix for demo files. disabled if unset.
-- demo       = "infond"

-- time limit for each map (in ms). default is 10 minutes.
time_limit = 10 * 60 * 1000

-- score limit for each map
score_limit = 500 

-- access control list. default is to allow connections 
-- from all clients. 
acl = {{ pattern = "^ip:127\.0\..*"             }, 
       { pattern = "^special:.*"                }, 
       { pattern = ".*"                         },
       { pattern = ".*", deny = "access denied" }}

-- available highlevel apis
highlevel = { 'oo', 'state' }

-- disable joining? If set, no *new* players can join the game.
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
    pp   = "A Pretty Printer";
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

-- Allows changing the speed from realtime to maximum speed 
-- using the 'R' command. You should probably keep it 
-- disabled on a public server.
speedchange = false

-- Use this section to automate competitions. information about
-- the fight will be written into 'log'. the bots in 'bots' will
-- automatically join the game. Each of them takes the same 
-- parameters as shown in the autoexec example below.
-- Set the parameters 'maps', 'time_limit', 'score_limit' as you
-- wish. 'listenaddr' should be set to nil so nobody can join
-- this server. the server will terminate once all maps were 
-- played once. The infond command line options are available
-- in the table 'argv'.
--
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
--     -- Change runner speed
--     creature_config.runner_speed = 1000
--
--     -- Join a bot
--     start_bot{ source   = "contrib/bots/queener.lua",
--                name     = "Queener",
--                password = "topsecret",
--                log      = "queener.log",
--                api      = "oo" }
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

