-- Breakpoint support.

-- Most ldb modules will start like this (although they may
-- not require ldb itself). The setfenv gives us access to
-- all of the ldb configs and hooks, including the splain
-- system.
local ldb = require"ldb"
setfenv(1, require"ldb-config")

splain.breakpoints = [[- Some issues with breakpoints

  The breakpoint code has not been exhaustively debugged, and it has a few
  glitches and inconveniences. I'll try to fix these (or work around them)
  before the beta release.

  * ldb does not check that the file or line specified in the `break`
    command are correct. Furthermore, the file must be typed exactly
    as it was specified in the dofile() function (or equivalent). This
    is particularly awkward for files loaded through require().

    It's usually easiest to use the `ldb` command-line script, which
    will trigger ldb at the beginning of a lua file (or you can use
    the `debugf` function exported by `require"ldb-break"`). If you
    omit the filename in the `break` command, then ldb will use the filename
    from the current callframe. This is almost always the most convenient.
    You can move up and down in the callframe to find a callframe with
    the correct filename.

  You can now use the `next`, `step` and `finish` commands even if you're
  not in an ldb call initiated by a breakpoint. However, there are a
  couple of issues:

  * `finish` will act as though it were `next`, so you'll need to do
    a second `finish` to actually finish the callframe which called
    ldb directly. However, I believe that `continue <linenumber>` will
    work correctly.

  * If ldb is being used as an error function (either by assigning
    `debug.traceback = ldb` or by using something like `xpcall(myfunc, ldb)`
    then `next` and `step` will be relative to the frame where Lua will throw
    the error. ldb does not know where that is, so `continue <linenumber>`
    will most likely produce an error telling you that the line is not
    correct. Also, `finish` will, as above, be treated as though it were
    `next`. If the error is not in the scope of an `xpcall`, then Lua
    will simply terminate as usual; ldb can't do anything about that.
    It would be really nice if there were a way to restart a Lua program
    at the place where the error was thrown, having fixed a variable or
    something, but there isn't and ldb can't invent mechanisms which
    don't exist in the Lua debug interface.

  Any suggestions for overcoming these issues are more than welcome,
  of course.
]]

local getinfo, sethook = debug.getinfo, debug.sethook

-- To enable sethook tracing, you need to change the 2 to 3
-- in leave, and change the above from sethook to _sethook.
-- local function sethook(_, f) print("sethook", f) _sethook(_, f) end
--
-- Also: %s/--\[\[ DEBUG/--[[ DEBUG/

-- A table of tables.
-- breaklines[lno][source] is true if there is a breakpoint at
-- source:lno. It would be better if we could get
-- at the proto, really.
local breaklines

-- If we're tracking call levels to do 'next' or 'finish', this is
-- the current virtual level. When it hits 0, we need to break on
-- the next line (or the line in breaknextline). If we're not
-- doing 'next' or 'finish', this will be nil.
local breaklevel

-- True if a 'next' should be restored when breaklevel reaches 0.
local breaknext

-- If 'next' has a target line, this is the line number. If it's
-- set, breaknext must also be set.
local breaknextline

-- breakcalls and breakrets are independent, but they work the same
-- way: each is a table of functions we want to break at call
-- or return (not tail return) respectively, with a command table
-- (or true) as value. 
local breakcalls, breakrets

-- There are times when stray breaknexts are still around when
-- we're on our way out, or when an error is raised. This table
-- contains internal functions whose execution indicates that
-- a breaknext ought to be ignored. (Usually a 'step' command.)
-- The value is a function to execute when the stray hook fires.

-- In the case of ldb itself, we just kill the hook; it will get
-- re-enabled if necessary on exit.
local breakignores = {[ldb] =  sethook}

-- There's only one leave entry point, so it needs to be chained
-- by tail-calling the old one. Unfortunately, we can't get away
-- with this here (see long comment below) so we require that
-- this be the *first* plugin loaded.
do
  --local _leave = leave
  function leave(self, ...)
    --[[ DEBUG
      print("leave", self.in_hook, breaklevel, breaknext)
    --]]
    if not self.in_hook and (breaknext or breaklevel) then
      -- Boy, would it be handy to have an error hook :)
      -- Also, this is why I hate the tailcall faking behaviour.
      --
      -- At this point, we're on our way out from an error
      -- or a direct call. Since we're not in a hook, we
      -- have to adjust breaklevel to not go off until we
      -- get back to the caller. Everything from here on
      -- out is a tailcall except maybe for the trampoline
      -- inserted by the standalone interpreter, which we
      -- can ignore because it's a C frame. The problem is
      -- that we don't know how many tail call frames are
      -- going to be added by the chained calls to leave.
      --
      -- We also don't really know how far back to go up
      -- the call chain, but that's less important. What's left
      -- here is tailcalls and C-frames (hopefully), and behind
      -- that there will be either the frame which directly called
      -- ldb, or a luaD_throw back to a pcall or xpcall, which is
      -- also a C-frame. The only issue with not knowing where to
      -- go back to is that it makes the semantics of `finish` a
      -- bit odd: you can't `finish` a frame entered with a direct
      -- call to ldb.
      --
      -- Undoubtedly, there is a clever way of solving this
      -- problem, but since I'm pressed for time, I'm going for
      -- the simple solution, which is to ignore the value of
      -- breaklevel and just set it to something which will work,
      -- and to insist that this be the first module loaded.
      --
      -- So we need to ignore the return from sethook() and then our
      -- own (pseudo)return. We'll just set breaklevel to 2,
      -- kill breaknext, and hope for the best.
      breaklevel = 2
      breaknext = nil
    end
    local c = breaklevel or breakcalls
    local r = breaklevel or breakrets
    local l = breaklines or breaknext
    --[[ DEBUG
      print("leave-post adjust", self.in_hook, breaklevel, breaknext, c, r, l)
    --]]
    if c or r or l then
      sethook(ldb, (c and "c" or "")..(r and "r" or "")..(l and "l" or ""))
    else
      sethook()
    end
    return ...
    --return _leave(self, ...)
  end
end


local function setbreakpoint(source, lno, cmds)
  breaklines = breaklines or {}
  local sources = breaklines[lno] or {}
  sources[source] = cmds or true
  breaklines[lno] = sources
end

local function setcallbreak(f, cmds)
  breakcalls = breakcalls or {}
  breakcalls[f] = cmds or true
end

local function setretbreak(f, cmds)
  breakrets = breakrets or {}
  breakrets[f] = cmds or true
end

local function clearbreakcall(f)
  if not f then
    breakcalls = nil
  elseif breakcalls then
    oldcalls = breakcalls[f]
    breakcalls[f] = nil
    if not next(breakcalls) then breakcalls = nil end
    return oldcalls
  end
end

local function clearbreakret(f)
  if not f then
    breakrets = nil
  elseif breakrets then
    oldrets = breakrets[f]
    breakrets[f] = nil
    if not next(breakrets) then breakrets = nil end
    return oldrets
  end
end

local function clearbreakpoint(source, lno)
  if not source then
    breaklines = nil
  elseif breaklines then
    local sources = breaklines[lno]
    local oldcmds = sources and sources[source]
    if oldcmds then
      sources[source] = nil
      if not next(sources) then
        breaklines[lno] = nil
      end
      if not next(breaklines) then
        breaklines = nil
      end
    end
    return oldcmds
  end
end


function prehandler.line(lno)
  --[[ DEBUG
    print("line hook", breaklines, breaklevel, breaknext, breaknextline)
  --]]
  local sources = breaklines and breaklines[lno]
  local info
  if sources then
    -- There's no sentinel yet so we have to count levels,
    -- which should just be ldb and us.
    info = getinfo(3)
    local cmds = sources[info.source]
    if cmds then
      return "ldb_dohook", cmds
    end
  end
  if breaknext then
    if not breaknextline or breaknextline == lno then
      if not info then info = getinfo(3) end
      if breakignores[info.func] then
        return breakignores[info.func]()
      end
      return "ldb_dohook"
    end
  end
end

function prehandler.call()
  --[[ DEBUG
    do
      local info = getinfo(3)
      print("call hook", breakcalls, breaklevel, breaknext, breaknextline,
            info.what, info.name)
    end
  --]]
  if breakcalls then
    local func = getinfo(3, "f").func
    local cmds = breakcalls[func]
    if cmds then return "ldb_dohook", cmds end
  end
  breaknext = false
  if breaklevel == 0 then
    breaklevel = 1
    sethook(ldb, breaklines and "crl" or "cr")
  elseif breaklevel then
    breaklevel = breaklevel + 1
  end
end

prehandler["return"] = function()
  --[[ DEBUG
  do
    local info = getinfo(3)
    print("return hook", breakrets, breaklevel, breaknext, breaknextline,
          info.what, info.name)
  end
  --]]
  if breakrets then
    local func = getinfo(3, "f").func
    local cmds = breakrets[func]
    if cmds then return "ldb_dohook", cmds end
  end
  if breaklevel then
    breaklevel = breaklevel - 1
    if breaklevel <= 0 then
      sethook(ldb, "crl")
      breaknext = true
    end
  end
end

prehandler["tail return"] = function()
  --[[ DEBUG
    print("tail return hook", breakrets, breaklevel, breaknext)
  --]]
  if breaklevel then
    breaklevel = breaklevel - 1
    if breaklevel <= 0 then
      sethook(ldb, "crl")
      breaknext = true
    end
  end
end

-- Do a command or a table of commands, and then fall into
-- interactive mode unless the command(s) include continue
-- The test is for `nil` specifically because self:error()
-- returns `false` if there is an error (unless self.ignore_error
-- is set, which happens if the command starts with a `!`)
function handler:ldb_dohook(token, cmds)
  breaknext = nil
  breaknextline = nil
  breaklevel = nil
  self.in_hook = true
  local t = type(cmds)
  if t == "string" then
    return self:do_a_command(cmds)
  elseif t == "table" then
    for _, cmd in ipairs(cmds) do
      local flag, val = self:do_a_command(cmd)
      if flag ~= nil then return flag, val end
    end
  end
end

-- Breakpoint commands

splain.breakpoint = [=[[FILE] LINE - Set a breakpoint at FILE:LINE

If only a line number is specified, the breakpoint is set in the file
of the current context.

Note: ldb compares filenames for strict equality; it does not know
that ./foo.lua, /usr/home/rici/foo.lua and foo.lua are all the same
file. (It just uses the source string from debug.getinfo and compares
for equality.)
]=]
alias.breakpoint = 'b'
function command:breakpoint(args)
  local file, lno = self.here.source, tonumber(args)
  if lno then
    lno = tonumber(args)
  elseif not args:match"%S" then
    lno = self.here.currentline
  else
    file, lno = args:match"(%S+)%s*(%d+)"
    if file then
      file = "@"..file
      lno = tonumber(lno)
    end
  end
  if not lno then
    return self:error"A line number must be specified"
  end
  if not file then
    return self:error"The current context has no associated source file"
  end
  setbreakpoint(file, lno)
end

splain.breakfunc = [[EXPR [LINE] - Set a breakpoint in an function

  EXPR is evaluated and must return a function value or a string.
  
  If EXPR is a string:
  
    EXPR is interpreted as the value of the `source` getinfo field, and it
    should come from something like:
  
    .bf  (here+3).source
    
    LINE defaults to 1.
  
  If EXPR is a Lua function:

    The breakpoint is not specific to the closure; it is set to the file
    position so it will apply to any closure using the same function
    prototype.

    LINE defaults to the first valid line in the function.

    In interactive mode, a warning is printed if the line is not inside the
    function's definition. If it is inside the definition, an error is
    produced if it is not a breakpointable line.

    If a warning is produced, the breakpoint is set anyway, so that you can
    use it to set breakpoints in the function's file.

  If the function is a CFunction:

    The breakpoint *is* specific to the closure.
    
    LINE may be 'call', 'return', or 'both' (which is the default).

  Note: unlike most ldb commands, the expression must fit in one line. It
  would not be possible to parse the LINE argument otherwise.
]]
alias.breakfunc = "bf"
do
  local calllines = {call = 'c', ["return"] = 'r', both = 'cr'}
  local function ls(s)
    local ok, err = loadstring(s, "arg")
    if ok then return true, ok
    else return ok, err
    end
  end

  function command:breakfunc(cmd)
    local func, line = cmd:match("^(.*)%s+(%S+)%s*$")
    if line then
      if tonumber(line) then line = tonumber(line)
      elseif calllines[line] then line = calllines[line]
      else func = cmd; line = nil
      end
    else
      func = cmd
    end
    local ok, f = self:do_in_context(ls("return "..func))
    if not ok then return self:error(f) end
    if type(f) == "function" then
      local info = getinfo(f, "SL")
      if info.what == "C" then
        line = line or 'cr'
        if type(line) ~= "string" then 
          return self:error"LINE for CFunction must be 'call', 'return' or 'both'"
        end
        if line:match'c' then setcallbreak(f) end
        if line:match'r' then setretbreak(f) end
        return
      else
        if not line then
          line = next(info.activelines)
          if not line then
            return self:error "No active lines in that function"
          end
          for l in next, info.activelines, line do
            if l < line then line = l end
          end
        elseif type(line) == "number" then
          if line < info.linedefined or line > info.lastlinedefined then
            if self.interactive then
              print "Warning: the line is not within the function"
            end
          elseif not info.activelines[line] then
            return self:error "not a valid line inside the function"
          end
        else
          return self:error "LINE for a Lua function must be a number"
        end
        f = info.source
      end
    elseif type(f) == "string" then
      line = line or 1
      if type(line) ~= "number" then
        return self:error "LINE must be a number"
      end
    else
      return self:error "EXPR must be function or string"
    end
    -- At this point, either we've handled it (error or cfunction) or
    -- f is the source and line is the line number
    setbreakpoint(f, line)
    return
  end
end

splain.delete = [=[- Delete the current breakpoint]=]
function command:delete()
  local source, line = self.here.source, self.here.currentline
  if not source or not line then
    return self:error"The current context has no associated source file"
  else
    local old = clearbreakpoint(source, line)
    if not old then
      return self:error"No breakpoint was set"
    end
  end
end

splain.clear = [=[[CLASS...] - clear all breakpoints

  CLASS may be any combination of "line", "call", "ret" or
  "all". If it's not specified, it defaults to "line"
]=]
function command:clear(arg)
  local o, e
  for opt in arg:gmatch"(%S)%S*" do
    local a
    if opt == "l" or opt == "a" then clearbreakpoint(); a = true end
    if opt == "c" or opt == "a" then clearbreakcall();  a = true end
    if opt == "r" or opt == "a" then clearbreakret();   a = true end
    o = o or a
    e = e or not a
  end
  if e then return self:error"Unrecognized option" end
  if not o then clearbreakpoint() end
end


splain.blist = [[- list all breakpoints]]
function command:blist()
  if breaklines then
    local list = {}
    for lno, sources in pairs(breaklines) do
      for source in pairs(sources) do
        list[#list+1] = ("%s:%d"):format(source, lno)
      end
    end
    table.sort(list)
    for _, e in ipairs(list) do self:output(e) end
  end
  local funcs = {}
  if breakcalls then
    for func in pairs(breakcalls) do funcs[func] = "call " end
  end
  if breakrets then
    for func in pairs(breakrets) do funcs[func] = (funcs[func] or "").."ret " end
  end
  for func, which in pairs(funcs) do self:output(func, which) end
end

-- Command line support
local function debugf(f, ...)
  local err, ok = "debugf expects a filename or a function"
  if type(f) == "string" then
    f, err = loadfile(f)
  end
  if f then
    sethook(function()
              if f == getinfo(2, "f").func then
                sethook(ldb, "l")
                breaknext = true
              end
            end,
            "c")
    local nargs, args = select("#", ...), {...}
    local function g() return f(unpack(args, 1, nargs)) end
    local function callit() return xpcall(g, ldb) end
    ok, err = callit()
    breaknext = nil
    breaknextline = nil
    sethook()
  end
  return ok, err
end
breakignores[debugf] = function() return "ldb_dohook", "continue" end

-- ldb.break redefines the continue command to allow an extra
-- argument

splain.continue = [=[[LINE]- continue execution until line LINE

  If LINE is not specified, execution will simply continue. If
  LINE is specified, it must be a valid linenumber in the current
  function.
  
  Note that LINE is associated with the current call frame, so that
  if the function calls itself recursively, the inner calls will not
  be interrupted. This is not the same as setting a breakpoint at the
  specified line, which would trigger at any calling level.
]=]
alias.continue = 'c'
function command:continue(cmd)
  local line = tonumber(cmd)
  if line then
    if getinfo(self.here.func, "L").activelines[line] then
      breaklevel = self.here.level - 1
      breaknext = true
      breaknextline = line
    else
      return self:error(cmd .. " is not a valid line number in this context")
    end
  elseif cmd:match"%S" then
    return self:error("numeric argument expected")
  end
  return true
end

splain.next = [[- step to the next line]]
function command:next()
  breaknext = true
  breaklevel = 0
  return true
end

splain.step = [[- step to the next line, stepping into functions]]
alias.step = 's'
function command:step()
  breaknext = true
  return true
end

splain.finish = [[- step to the end of this function]]
function command:finish()
  breaklevel = 1
  return true
end

return debugf
