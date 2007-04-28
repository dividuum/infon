-- At initialization, ldb.lua attempts to require "ldb-config", and if it
-- works, it uses it for the initial configuration. You can use this to
-- customize ldb's functions for input and output (for example to use
-- lrcon); the prompts; and to configure an external editor. If it doesn't
-- find an ldb-config module, it uses sensible defaults and the external
-- editor command won't do anything.

-- Anything in this file will be executed before ldb sets up anything,
-- including its command table. In the future, there will be an option
-- for adding custom commands to ldb using this or a similar mechanism.

-- Until I get around to writing documentation, you can use this
-- file as an example :)

----- CONFIGURATION (see below for various uses of these locals)
--
-- -- These will be used in the configuration of lrcon.
-- local REMOTE = false
-- local PORT = 7022
--
-- This is *my* configuration; it will not work in general. It's
-- only been tested on one system (xubuntu 6.10 with vim-full;
-- I had to apt-get wmctrl for the raise command). The :Move
-- command is defined in ldb.vim, included as well.
-- local FN = "<C-\\><C-N>:Move %s<CR>"
-- local FNLN = "<C-\\><C-N>:Move %s | :%d<CR>"
-- local RAISE = "wmctrl -R ldbvim"
-- local VIEW = 'gvim --servername ldbvim --remote-send %q'
-- local EDIT = VIEW .. " && " .. RAISE
-- local function FL(filename, lineno)
--   if lineno then return FNLN:format(filename, lineno)
--   else return FN:format(filename)
--   end
-- end

-- This only works on xubuntu, and maybe not on xubuntu installs other than mine.
-- But I left it in here, so you can gaze at the nuttiness of doing simple things
-- like "start an application without giving it focus"
-- In practice, the best bet is to just write a shell script which fires up
-- gvim with the --servername option, and then fires up a shell running Lua,
-- in that order. You'll still quite possibly get them overlaid if you don't
-- specify the -geometry.

-- This is all one line:
-- local STARTVIM = [[gvim --serverlist | grep -q ^LDBVIM$ || ]]
--                ..[[{ id=`xwininfo -id $WINDOWID -children | grep Parent]]
--                ..[[ | sed 's/.*\(0x[0-9a-fA-F]\+\) .*/\1/'` ;]]
--                ..[[gvim --servername ldbvim; wmctrl -i -a $id ; }]]
-- 
-- -- If you need globals, this would be a good time to capture them as
-- -- upvalues
--
-- local osexec = os.execute

-- -- PREINITIALIZATION
-- -- If you're using lrcon, you'd probably want to initialize it here:
--
-- local con = require"lrcon"("loopback", 7022)
-- con:setbanner"Connected to ldb on port 7022\r\n"
-- con:get()   -- force a connection
-- 
-- osexec(STARTVIM)

-- -- RETURN THE CONFIGURATION

return {
-- The prompts used for commands and continuations, respectively:
  PROMPT =  "(ldb) ",
  PROMPT2 = "  >>> ",
--
-- Functions used for input, output, and errors. These are the defaults:
  input = coroutine.yield,

  output = function (...) 
      return print(...)
  end,

  error = function (...) 
      return print(...)
  end,

--   If you're using lrcon, 
--   -- you might want:
--
--   input = function(prompt)
--     con:write(prompt)
--     return con:read()
--   end,
--   output = function(...)
--     return con:print(...)
--   end,
--   error = function(...)
--     return con:print(...)
--   end,

-- External editor interface. The edit function does what is necessary
-- to fire up an external editor given a filename and (optional) line
-- number. This function must return a true value if it works. If it
-- returns <false, error>, the error message will be shown; if it just
-- returns nil or false, the default error message will be shown.
-- ("No editor configured").
--
-- The default function returns nil.
-- This is the one I use:
--
  edit = function(filename, lineno)
    return false, "external editor not available within infon"
    -- return osexec(EDIT:format(FL(filename, lineno)))
  end,

-- This function is called whenever a new callframe is entered,
-- which almost always means there is a new line number at least,
-- and maybe filename. The following is what I use to keep the
-- gvim window in synch with the debugger.
  enterframe = function(state)
    -- local filename = state.here.source:match"^@(.*)"
    -- if filename then
    --   osexec(VIEW:format(FL(filename, state.here.currentline or 1)))
    -- end
  end,

  -- This function is called at the end of the ldb.lua file,
  -- to load any plugins. The plugins can freely call
  -- require"ldb" and require"ldb-config" (even if the corresponding
  -- files were not loaded through the module system, or at all),
  -- which will give them access to ldb internals.
  load_plugins = function()
    -- Does not work in infon (yet)
    -- require"ldb-break"
  end
  
}

-- Here are some other possibilities for external editor configuration,
-- mostly untested. If you've got more, put them on the CVSTrac. All of
-- these use the following
--
--   local FL(filename, lineno)
--     if lineno then return FNLN:format(lineno, filename)
--     else return FN:format(filename)
--     end
--   end
--   function edit(filename, linenumber)
--     return osexec(EDIT:format(FL(filename, lineno))
--   end

-- vim
-- ---
-- To just fire up vim in a subprocess, which works fine, you could
-- use:
--   local FN = "%q"
--   local FNLN = "+%d %q"
--   local EDIT = "vim %s"
--
--   Alternatively, if you've got a vim running in clientserver
--   mode (available on Windows or X11 -- in the case of X11, although
--   vim uses X11 messaging, I believe it will still work with a vim
--   in a terminal shell):
-- 
--   local FN = "%q"
--   local FNLN = "+%d %q"
--   local EDIT = 'vim --servername ldbvim --remote-tab-silent %s %q'
-- 
-- Getting enterframe to work is trickier, which is why I went to the
-- trouble of writing the stuff above.

-- emacs, courtesy of my friends in #oasis. I haven't tried this.
-- -----
-- You'll need to put (start-server) in your .emacs file, or you can
-- get it to fallback to vim (as shown here)
--
-- local EDIT = 'emacsclient --alternate-editor vim --no-wait %s'
--
-- Use the same definitions of FN and FNLN as for vim.
-- See:
-- http://www.gnu.org/software/emacs/manual/html_node/Invoking-emacsclient.html

-- Visual Studio, courtesy of Slade from #lua
--
-- local FN = "%q"
-- local FNLN = '/command "Edit.Goto %d" %q'
-- local EDIT = '"c:\program files\microsoft visual studio 8\common7\ide\devenv" %s'
--
-- --  Note: the above apparently does not work if you use .lua as a file
-- --  extension; possibly, a VS bug of some form.

