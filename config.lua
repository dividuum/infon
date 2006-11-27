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
