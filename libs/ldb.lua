-- ldb - A Lua debugger
-- $Id: ldb.lua,v 1.17 2007/04/18 21:24:24 jbloggs Exp $
--
-- Requires Lua 5.1
--
-- Written in a fit of boredom by Rici Lake. The author hereby
-- releases the contents of this file into the public domain,
-- disclaiming all rights and responsibilities. Do with it
-- as you will. Caution: do not operate while under the influence
-- of heavy machinery.
-- 
local VERSION = ([[$Revision: 1.17 $]]):match":([^$]*)"
local VERSION_DATE = ([[$Date: 2007/04/18 21:24:24 $]]):match":([^$]*)"

local function Memoize(func)
  return setmetatable({}, {__index = function(t, k)
    local v = func(k); t[k] = v; return v
  end})
end

local function words(s)
  local rv = {}
  for w in s:gmatch"%S+" do rv[w] = w end
  return rv
end

-- PLUGIN datastructures ----------------------------------------------------
--
-- SPLAIN -- the help/command parser

-- command[name] -> function(state, args)
-- The command functions
-- By convention, the functions are written in O-O style,
-- so that `self` is the state object.
local command = {}

-- splain[name] -> string
-- If the key is present in command[], then this is help for a
-- command. The first line of the string *must* have the format:
-- "args - summary". The `- ` is mandatory, but `args` and the
-- space which follows are optional. If an argument is optional,
-- it should be surrounded by []. By convention, argument names
-- are written in CAPITAL LETTERS.
--
-- If the key is not present in command[], then this is a topic,
-- and the first line of the string *must* have the format:
-- "X summary" where `X` is either a `*` or a `-`. If it is
-- a `*` then this is an "important" topic and the help system
-- will print it first in the topics summary.
--
-- If there is a long description, it should be separated from
-- the summary line by a blank line, and be indented two spaces.
-- Please keep lines to 78 characters including the indent.
local splain = {}

-- alias[string] -> string
-- Commands can have at most one alias, and aliases should either
-- be a single non-alphabetic character or a short alphabetic
-- string (one or two characters). Topics cannot have aliases.
-- If a non-alphabetic alias is more than one character long,
-- it will not be recognized unless followed by a space.
local alias = {}

-- HANDLER hooks 

-- prehandler[string] -> function(arg2)
-- handler[string] -> function(state, arg1, arg2)

-- ldb() is always entered with two arguments, both of which are
-- optional. When entered as an error function, the first argument
-- will be the error object (usually a simple string message) and
-- the second argument will usually be nil, but may be a hint from
-- error() about which callframe is the user-visible error. When 
-- entered as a hook function, the first argument will be one of
-- Lua's hook strings (line, call, count, return, 'tail return').
-- When called directly, the first argument could be anything, but
-- all strings starting ldb_ are reserved. The only handler defined
-- by the core system is ldb_cmd, which is used by the C glue.
--
-- The prehandler is primarily used by the breakpoint system, in order
-- to continue execution as fast as possible. It is called immediately
-- when ldb is entered, and should either return nothing/nil/false as
-- fast as possible, or return a new arg1, arg2 pair. In the first case,
-- ldb is not entered at all; this is used to filter unwanted hook calls.
-- It's probably not of much use otherwise.
--
-- The handler is called once the debugger state has been set up, but
-- before anything else has happened. It should either return:
--   nil or nothing --> ldb will enter interactive mode
--   true, value    --> ldb will return the specified value.
-- Note the different prototypes. prehandler was deliberately stripped
-- to the minimum possible calling interface.
--
local prehandler, handler = {}, {}

-- STARTUP ------------------------------------------------------------------
local ldb

-- Grab debug and other globals now, in case they get modified
local _G = getfenv()
local debug = debug
local getfenv, getinfo = debug.getfenv, debug.getinfo
local getupvalue, setupvalue = debug.getupvalue, debug.setupvalue
local getlocal, setlocal = debug.getlocal, debug.setlocal
local getmetatable, setmetatable = getmetatable, setmetatable
local pcall = pcall
local concat = table.concat

local function prefer(pkgname)
  local ok, pkg = pcall(require, pkgname)
  return ok and pkg
end

-- valid keys in a stackframe (debug.getinfo + a few of our own)
local infokeys = words[[
    source short_src linedefined
    lastlinedefined what currentline
    name namewhat nups
    func var level
]]

local infofuncs = {}
function infofuncs.fenv(info)
  if info.func then return getfenv(info.func)
  else return {}
  end
end

-- Create an id line from an info table. This is similar
-- but not identical to debug.traceback, but it can be customized
-- TODO: Move this into the config system.
local id do
  local function lineno(l, pfx)
    if l and l > 0 then return pfx..tostring(l) else return "" end
  end
  -- The default id doesn't use self
  function id(info)
    if info.what == "tail" then return "[tailcall]" end
    local segone
    if info.what == "main" then
      if info.source:sub(1,1) == "@" then segone = "file"
      elseif info.source:sub(1,1) == "=" then segone = info.source:sub(2)
      else segone = "??"
      end
    elseif info.namewhat and info.namewhat ~= "" then segone = info.namewhat
    else segone = "function"
    end
    local segs = { ("%-8s"):format(segone) }
    segs[#segs+1] = info.name
    local source = info.source and info.source:sub(1,1) == "@" and info.source:sub(2)
                   or info.short_src
    segs[#segs+1] = source and 
                    ("<%s%s>"):format(source, lineno(info.linedefined, ":"))
    segs[#segs+1] = lineno(info.currentline, "at line ")
    return concat(segs, " ")
  end
  infofuncs.id = id
end

-- CONFIGURATION ------------------------------------------------------------
-- This is the pseudo-environment table which will be used
-- by all instances of the debugger. It's not actually
-- the debugger's environment table; rather, it's bound
-- into the metamethods for debugger contexts, but it's
-- pretty similar. A future version may allow multiple
-- debuggers with different environments.
local env = prefer"ldb-config" or {}
if type(env) ~= "table" then
  print"ldb-config.lua must return a table of configuration settings"
  print"Ignoring ldb-config and using defaults"
  env = {}
end
-- Make it look like the config was loaded even if it wasn't.
package.loaded["ldb-config"] = env

-- Add the configuration hooks
env.splain = splain
env.alias = alias
env.command = command
env.prehandler = prehandler
env.handler = handler

-- Set up the default configuration
env.PROMPT = env.PROMPT   or "(ldb) "
env.PROMPT2 = env.PROMPT2 or "  >>> "
env.input = env.input or function(prompt) io.write(prompt); return io.read"*l" end
env.output = env.output or print
env.error = env.error or env.output
do
  local function donothing() end
  local function selfidentity(self, ...) return ... end
  env.enterframe = env.enterframe or selfidentity
  env.leave = env.leave or selfidentity
  env.edit = env.edit or donothing
  env.load_plugins = env.load_plugins or donothing
end
env.g = _G
env.id = env.id or id

setmetatable(env, {__index = _G})

-- This function finds the frame which invoked the debugger.
-- It starts by finding the sentinel frame (placed on the
-- callstack by the debugger) and then works backwards,
-- skipping over the debugger frame itself and also
-- error or assert, if present. It also tries to identify
-- and skip the trampoline pushed onto the stack by lua.c.
-- That would be easier if lua.c used a constant closure which
-- we knew about.
--

--[[
local function getoffset(sentinel)
  for base = 3, 1e6 do
    if getinfo(base, "f").func == sentinel then
      base = base + 2
      local _, val = getlocal(base, 2)
      if val == debug and not getlocal(base, 3) then
        base = base + 1
      end
      local errfunc = getinfo(base, "f").func
      if errfunc == error or errfunc == assert then
        base = base + 1
      end
      return base - 2
    end
  end
end
]]

local function getoffset(sentinel)
  for base = 3, 1e6 do
    if getinfo(base, "f").func == sentinel then return base end
  end
end

local function geterroffset(sentinel)
  local base = getoffset(sentinel)
  if base then
    local rv = 0
    local _, val = getlocal(base+1, 2)
    if val == debug and not getlocal(base+1, 3) then
      rv = rv + 1
    end
    local errfunc = getinfo(base+1+rv, "f").func
    if errfunc == error or errfunc == assert then
      rv = rv + 1
    end
    return base - 1, rv
  end
end

-- Create the debugger State, which encapsulates everything needed
-- during a single invocation of the debugger. This was called
-- Context in earlier versions, but that was too confusing.
--
-- The State does not persist between calls, so it can't be used
-- for things which have to persist (breakpoints, for example).
-- Such things could probably go into env; we'll see when I
-- actually write the breakpoint handling stuff.

-- FIXME There's only one local being protected by this whole
-- do block, now that I've hiked statemeta out of it. But
-- reindenting will create too big a diff for now. Also,
-- change statemeta to statemethods.

local statemeta = {}; statemeta.__index = statemeta

local State do
  -- Methods on the state object
  -- The double indirection here is to allow run-time customization
  -- of the functions. Maybe it's dumb...
  --
  -- CUSTOMIZABLE CONFIGURATION FUNCTIONS
  -- We reflect the configurable functions into the state object,
  -- throwing away self (in most cases) to make the state object's
  -- interface consistent.
  --
  -- input(prompt) should display prompt to the user and return one
  -- line of input.
  function statemeta:input(prompt) return self.var.input(prompt) end
  -- output should display its arguments in a fashion similar to
  -- print(). There is no guarantee about the arguments; they may
  -- be simple values or multi-line help-text.
  function statemeta:output(...) return self.var.output(...) end
  -- error should display its argument in a fashion similar to print.
  -- Indeed, it's possible to use the same function for output and
  -- error; however, it may be desirable to distinguish style in
  -- some way.
  function statemeta:error(...)
    self.var.error(...)
    if not self.ignore_error then return false end
  end
  -- edit is called to implement the edit command.
  -- IMPORTANT: edit must return true if it handled the edit; otherwise,
  -- false and an error message. If nothing or only false is returned,
  -- ldb will produce a "no editor configured" message.
  function statemeta:edit(name, line) return self.var.edit(name, line) end
  -- enterframe is called whenever the debugger enters a different call frame
  function statemeta:enterframe() return self.var.enterframe(self) end
  -- leave is called with a value when control is about to return to the program
  -- the value should be returned as an argument
  function statemeta:leave(val) return self.var.leave(self, val) end
  -- id is called to produce an id line for a stackframe.
  function statemeta:id(info) return self.var.id(info) end

  -- END OF CUSTOMIZABLE INTERFACE

  -- Get the offset using getoffset and the cached erroffset
  function statemeta:getoffset()
    return getoffset(self.sentinel) + self.erroffset - 1
  end

  -- This is used to read expressions, chunks, etc. from commands
  function statemeta:getchunk(cmd)
    local chunk, err = loadstring(cmd, "arg")
    if chunk then return true, chunk
    elseif self.interactive and err:match"<eof>" then
      local more = self:input(self.var.PROMPT2)
      if more then
        return self:getchunk(cmd.."\n"..more)
      end
    end
    return false, err
  end

  function statemeta:do_in_context(flag, chunk)
    if flag then
      setfenv(chunk, self.context.var or {})
      return pcall(chunk)
    else
      return false, chunk
    end
  end

  -- Announce where we are in the callframe
  function statemeta:announce()
    self:enterframe()
    self:output( ("*%2d %s"):format(self.here.level, self.here.id) )
  end

  -- The proxy stack contains information about the current stack
  -- (all of it :) ) It probably should be constructed more lazily,
  -- and with more caching. Otherwise, debugging deep stack frames
  -- will be painful.

  -- The stack is indexed by 'level'; currently, that's the same as
  -- the Lua level, but it might not hurt to remove empty tailcall
  -- frames from the level count. Each level contains the full
  -- information returned by debug.getinfo (except available
  -- breakpoints), plus some computed information.

  -- We export the stack as a (memoised) array of proxy info
  -- objects, which are defined here.  The proxy object can be
  -- indexed by the fixed info fields (the complete list of
  -- available fields is in infokeys at the top of this file); any
  -- other key is resolved in the context of the callframe. (If an
  -- infokey collides with a variable, use .env.varname to get at
  -- the actual variable name.) The proxy objects understand simple
  -- addition and subtraction as though they were C pointers.  It
  -- seemed convenient.

  -- Uses an explicit argument name to avoid confusion with the
  -- meta methods below.
  function statemeta.proxystack(state)
    local stack -- defined below
    local meta = {}
    local function StackFrame()
      return setmetatable({}, meta)
    end
    function meta:__add(offset)
      if type(offset) == "number" then
        return stack[stack[self] + offset]
      end
    end
    function meta:__sub(offset)
      if type(offset) == "number" then
        return stack[stack[self] - offset]
      end
    end
    function meta:__index(key)
      local info = state[stack[self]]
      if infokeys[key] then return info[key]
      elseif infofuncs[key] then return infofuncs[key](info)
      else return info.var[key]
      end
    end
    function meta:__newindex(key, val)
      local info = state[stack[self]]
      if infokeys[key] or infofuncs[key] then
        error(key.." cannot be modified")
      else info.var[key] = val
      end
    end
    
    stack = Memoize(function(level)
      if type(level) == "number" and state[level] then
        local rv = StackFrame()
        stack[rv] = level
        return rv
      end
    end)
    return stack
  end

  -- This is the "debugger context", which can be used as an
  -- alternative to the local context. A few debugger internals
  -- are exported.
  --
  -- For historical reasons, the context is actually `state.var`,
  -- which matches the current context being `state.here.var`. It
  -- might have been better to have called it `context`
  --
  -- Any other key is resolved in the `env` table and then in the
  -- globals table, but setting is always done in the `env` table.
  --
  -- To expose new internals, you must define both a getter and a
  -- setter; readonly internals should have an empty setter (although
  -- I suppose there's nothing wrong with throwing an error). This
  -- can also be used to "lock" settings in the `env` table as is
  -- done with `g` (to avoid typos, mostly).
  -- 

  function statemeta:makecontext(env)
    local getter, setter = {}, {}
    function getter.here()    return self.here   end
    function getter.stack()   return self.stack  end
    function getter.g()       return env.g       end
    
    function setter.here(new)
      local info = self.stack[new]
      if type(info) == "number" then self.here = new
      elseif info then self.here = info
      end
    end
    function setter.stack() end
    function setter.g() end

    local meta = {}
    function meta:__index(key)
      local g = getter[key]
      if g then return g() else return env[key] end
    end
    function meta:__newindex(key, val)
      local s = setter[key]
      if s then s(val) else env[key] = val end
    end

    return setmetatable({}, meta)
  end

  -- This function builds the local context from information
  -- hopefully cached by State.
  --
  -- TODO Construct the context lazily
  local function makeinfovar(info, sentinel, erroffset)
    info.var = setmetatable({}, {
      __index = function(_, key)
        local name, val
        local index = info.visible[key]
        if index then
          if index < 0 then
            name, val = getupvalue(info.func, -index)
          else
            name, val = getlocal(getoffset(sentinel) + erroffset + info.level, index)
          end
        else
          local word, index = key:match"^(%l+)(%d+)$"
          if word == "upval" then
            name, val = getupvalue(info.func, tonumber(index))
          elseif word == "local" then
            name, val = getlocal(getoffset(sentinel) + erroffset + info.level,
                                       tonumber(index))
          end
          if not name then val = getfenv(info.func)[key] end
        end
        return val
      end,
      __newindex = function(_, key, val)
        local index = info.visible[key]
        if index then
          if index < 0 then
            setupvalue(info.func, -index, val)
          else
            setlocal(getoffset(sentinel) + erroffset + info.level, index, val)
          end
        else
          local word, index = key:match"^(%l+)(%d+)$"
          if word == "upval" then
            setupvalue(info.func, tonumber(index), val)
          elseif word == "local" then
            setlocal(getoffset(sentinel) + erroffset + info.level, tonumber(index), val)
          else
            getfenv(info.func)[key] = val
          end
        end
      end
     })
  end

  function State(sentinel, env)
    local base, erroffset = geterroffset(sentinel)
    --[[ DEBUG
      print ("State", base, erroffset)
    --]]
    local self
    if base then
      base = base + erroffset
      self = setmetatable({sentinel = sentinel, erroffset = erroffset}, statemeta)
      self.stack = self:proxystack()
      self.var = self:makecontext(env)
      for level = 1, 1e6 do
        local info = getinfo(base + level, "nSluf")
        if not info then break end
        self[level] = info
        info.level = level
        local func = info.func
        if func then
          local visible = {}
          if info.what ~= "C" then
            for index = 1, 1000 do
              local name = getupvalue(func, index)
              if name == nil then break end
              visible[name] = -index
            end
            for index = 1, 1000 do
              local name = getlocal(base + level, index)
              if name == nil then break end
              if name:match"^[%a_]" then visible[name] = index end
            end
          end
          info.visible = visible
          makeinfovar(info, sentinel, erroffset)
        end -- if func (not a tailcall)
      end -- for level
      self.here = self.stack[1]
    end -- if base
    return self
  end
end

-- SPLAIN -- Command parser and help system ---------------------------------

-- Any unambiguous prefix of a command is allowed. Aliases must be
-- typed exactly, but may be otherwise ambiguous
local resolve_command = Memoize(function (cmd)
  if command[cmd] then return cmd end
  for real, short in pairs(alias) do
    if cmd == short then return real end
  end
  local possible
  for real in pairs(command) do
    if cmd == real:sub(1, #cmd) then
      if possible then return "Ambiguous" end
      possible = real
    end
  end
  return possible or "Unknown"
end)

function statemeta:do_a_command(input)
  local bang, dot, args = input:match"%s*(!?)%s*(%.?)%s*(.*)"
  if args ~= "" then
    local cmd = args:match"^%W"
    if cmd then
      if args:sub(2,2) == cmd then
        dot = cmd
        args = args:sub(3)
      else
        args = args:sub(2)
      end
    else
      cmd, args = args:match"(%S+)%s*(.*)"
      if args:sub(1,1) == '=' then
        args = cmd .. " " .. args:match".%s*(.*)"
        cmd = "set"
      end
    end
    self.context = dot == "" and self.here or self
    self.ignore_error = bang == "!"
    return command[resolve_command[cmd]](self, args)
  end
end

-- These save a couple of tests.
function command:Ambiguous()
  return self:error "Ambiguous command"
end

function command:Unknown()
  return self:error "Huh?"
end

-- CORE COMMANDS ------------------------------------------------------------
splain.backtrace = "- print a backtrace"
alias.backtrace = "bt"

function command:backtrace()
  local l = self.here.level
  for i, info in ipairs(self) do
    self:output((i == l and "*%2d %s" or "%3d %s"):format(i, self:id(info)))
  end
end

splain.up = "[N] - move up N levels, default 1"
alias.up = 'u'
function command:up(i)
  local here = self.here + (tonumber(i) or 1)
  if here then self.here = here end
end

splain.down = "[N] - move down N levels, default 1"
alias.down = 'd'
function command:down(i)
  local here = self.here - (tonumber(i) or 1)
  if here then self.here = here end
end

splain.edit = [[- open the file of the current context in an external editor

  For this command to work, `edit` must be defined in ldb-config. See the
  `ldb-config.example` file in the distribution for examples and more
  information.
]]

function command:edit(filename)
  local lineno
  if filename == "" then
    filename = self.here.source:match"^@(.*)"
    lineno = self.here.currentline or 1
  end
  if filename then
    if not self:edit(filename, lineno) then
      return self:error "No editor configured"
    end
  else
    return self:error "No current file"
  end
end

local function safestring(s)
  s = s:gsub("[%z\1-\31]", function(c) return ("\\%2x"):format(c:byte()) end)
  if #s > 40 then s = s:sub(1, 37).."..." end
  return "'"..s.."'"
end

local function safeshow(x)
  return (type(x) == "string" and safestring or tostring)(x)
end

splain.show = [[- show all locals and upvalues at the current level

  Locals and upvalues which are shadowed are marked with an
  exclamation point (!).
]]

function command:show(cmd)
  local maxclocals = tonumber(cmd) or 40
  local currentlevel = self.here.level
  local info = self[currentlevel]
  if info then
    local level = self:getoffset() + currentlevel
    local func = info.func
    if func == nil then
      self:output"[tailcall]"
    elseif info.what == "C" then
      for index = 1, 1000 do
        local name, val = getupvalue(func, index)
        if name == nil then break end
        if index == 1 then self:output"Upvalues:" end
        self:output( ("%4d: %s"):format(index, safeshow(val)) )
      end
      for index = 1, maxclocals do
        local name, val = getlocal(level, index)
        if name == nil then break end
        if index == 1 then self:output"Locals:" end
        self:output ( ("%4d: %s"):format(index, safeshow(val)) )
      end
    else
      local visible = info.visible
      local upvalue = {}
      for index = 1, 1000 do
        local name, val = getupvalue(func, index)
        if name == nil then break end
        local vis = index == -visible[name] and " " or "!"
        upvalue[#upvalue+1] = ("%s    %-12s: %s"):format(vis, name, safeshow(val)) 
      end
      if upvalue[1] then
        self:output"Upvalues:"
        table.sort(upvalue)
        for _, l in ipairs(upvalue) do self:output(l) end
      end
      for index = 1, 1000 do
        local name, val = getlocal(level, index)
        if name == nil then break end
        if index == 1 then self:output"Locals:" end
        local vis = index == visible[name] and " " or "!"
        self:output( ("%s%3d %-12s: %s"):format(vis, index, name, safeshow(val)) )
      end
    end
  end
end

splain.set = [[VAR EXPR - set debugger variable VAR to EXPR

  EXPR is evaluated in the indicated context (that is, the debugger context if
  invoked as `.set` and otherwise the current context. The named VAR (which
  currently must be a simple name) is set in the debugger context. You cannot
  use this command to set global variables.

  Normally you don't need to use the `set` command, because ldb interprets any
  command line whose second token is `=` as a set command. For example, the
  following are equivalent:

    foo = getmetamethod(local1)
    set foo getmetamethod(local1)

  and the following three commands are equivalent:

    .here = here+1
    .set here here+1
    ,,here=here + 1

  Note that the `=` must be preceded by a space to be recognized as a `set`
  command.
]]

function command:set(cmd)
  local var, chunk = cmd:match"(%S+)%s*(.*)"
  if not var:match"^[%a_][%w_]*$" then
    return self:error "Only simple variable names can be set"
  elseif chunk == "" then
    return self:error "No value specified for set"
  else
    local ok, val = self:do_in_context(self:getchunk("return "..chunk))
    if ok then
      self.var[var] = val
    else
      return self:error(val)
    end
  end
end

splain.print = [[EXPR - evaluate and print EXPR

  If invoked as `print` or `=`, the EXPR is evaluated in the current context.
  If invoked as `.print` or `==`, the EXPR is evaluated in the debugger
  context.

  See `help context` for more information.
]]
alias.print = '='
function command:print(cmd)
  self:output(select(2, self:do_in_context(self:getchunk("return "..cmd))))
end

splain.exec = [[CHUNK - evaluate CHUNK

  If invoked as `exec` or `,`, the CHUNK is evaluated in the current context.
  If invoked as `.exec` or `..` the CHUNK is evaluated in the debugger
  context.

  See `help context` for more information.
]]
alias.exec = ","
function command:exec(cmd)
  local ok, err = self:do_in_context(self:getchunk(cmd))
  if ok then
    if self.interactive then self:output"Ok!" end
  else
    return self:error(err)
  end
end

splain.quit = [=[[EXPR] - return from ldb, with an optional value

  Causes the current invocation of ldb to return. If an EXPR is provided,
  it will be returned from the call to ldb (which is only useful if ldb
  is invoked directly).  Currently, only a single value may be returned.

  If the evaluation of EXPR throws an error and the command was not
  being executed interactively, ldb will return <false, error message>
]=]
function command:quit(cmd)
  if cmd:match"%S" then
    local ok, val = self:do_in_context(self:getchunk("return "..cmd))
    if ok then
      return true, val
    elseif not self.interactive then
      return true, false, val
    else
      return self:error(val)
    end
  end
  return true
end

splain["if"] = [=[EXPR - return from ldb if EXPR is a true value

  Causes the current invocation of ldb to return with value EXPR
  if EXPR evaluates to a value other than `false` or `nil`. If an error
  is thrown, the invocation does not return.
]=]
command["if"] = function(self, cmd)
  local ok, val = self:do_in_context(self:getchunk("return "..cmd))
  if ok and val then return ok, val end
end

splain.unless = [=[EXPR - return from ldb if EXPR is `false` or `nil`

  Causes the current invocation of ldb to return (with no return value)
  if EXPR evalutes to `false` or `nil`, or throws an error.
]=]
function command:unless(cmd)
  local ok, val = self:do_in_context(self:getchunk("return "..cmd))
  return not ok or not val
end

splain.continue = [[- return from ldb]]
alias.continue = 'c'
function command:continue()
  return true
end
  
-- Topics

splain.intro = [[* quick introduction to ldb

  ldb is a very basic interactive debugger. Currently it only allows stack
  inspection, although breakpoints and watches will be supported in the
  future.

  The usual way of setting ldb up is:
     ldb = require"ldb"
     debug.traceback = ldb

  This will trigger ldb when any error occurs. You can also call ldb directly
  in your code, as a sort of manual breakpoint.
  
  There are only a few commands (so far) and all of them are documented in the
  help system. Take a few minutes to review them. The most common ones are:
  
  bt   print a backtrace
  =    print the value of an expression in the current context
  ==   print the value of an expression in the debug context
  ,    execute a statement in the current context
  ,,   execute a statement in the debug context
  show show all of the variables in the current context
  
  Read `help context` for an explanation of contexts.
  It's really important.
]]

splain.context = [[- how contexts work in ldb

  A context is essentially a variable scope; the combination of (visible)
  locals, upvalues and "globals" (i.e. the function environment.) ldb always
  has two contexts: the current context and the debugger context.
  
  When ldb is called, it sets the current context to the context of the
  caller, or the point at which an error was thrown, if ldb is being used as
  an error function (such as debug.traceback).

  You can move up and down in the call stack with the `up` and `down`
  commands; as you move, the current context will shift to reflect where
  you are.

  ldb also has its own context, which is persistent between invocations of
  ldb. You can set values in this context which will be available on
  subsequent invocations of the debugger.  The ldb context also contains ldb
  configuration variables, such as `PROMPT` and `output`, and `g`, which
  refers to the global environment in which `ldb` was originally loaded.
  There are also a few 'magic' variables.

  ldb commands which take an expression or a chunk as an argument evaluate
  the expression or chunk in the current context by default. If you prefix
  the command with `.`, then the debugger context will be used instead. For
  convenience, `.,` can be typed as `,,` and `.=` can be typed as `==`.

  For more details, see `help magic`, `help here`, `help clocal` and
  `help tailcall`.
]]

splain.clocal = [[- debugging inside C frames

  In C frames, there are no named variables -- ldb doesn't integrate with gdb,
  unfortunately -- but the stack and upvalues are still available as numbered
  variables in the form `local#` and `upval#` where # is a decimal number,
  possibly with leading zeros.
  
  For example, `local1` is stack slot 1 in the C frame, and `upval1` is the
  first upvalue (if there is one). You can use these as though they were
  ordinary variables.
  
  You can also use this format to get at shadowed locals and in a Lua frame.
  Note that the variable is first looked up in the context, so you if you have
  a local variable called `local1`, you would need to use `local01` to get at
  the first stack slot.

  Another issue you may run into with C frames is that ldb uses the
  CFunction's environment, not the thread environment, as the environment
  when the current context is a C frame. Most C functions operate on
  `LUA_GLOBALSINDEX` rather than `LUA_ENVIRONINDEX`, so the results might
  be surprising.
]]

splain.tailcall = [[- gotchas with tailcall frames

  The Lua debug interface includes the concept of 'deleted' tailcall stack
  frames; i.e. the frames which have been recycled by a tail call of the form:

    return other_func()
    
  These deleted frames have no information -- they really have been deleted --
  but they show up in backtraces anyway. At some point this might irritate me
  enough to remove them from the backtraces, but for now they are there.

  Deleted frames have any empty context. There are no variables, not even
  standard library functions. If you get an error that some global which
  obviously exists is `nil`, you should make sure that the current frame is
  not a tailcall frame.  (ldb doesn't insert "the" globals table, because it's
  not at all clear what that would mean -- had the frame not been deleted, the
  function would have had an environment table which might well not be the
  "global" environment.)
]]

splain.magic = [[- 'magic' fields in the ldb context

  ldb maintains its own environment, which you can use as a scratchpad;
  assignments to the ldb environment persist between calls to ldb, but do not
  affect the global environment. For convenience, the ldb environment has an
  __index metamethod which provides access to the initial global environment,
  but the __newindex metamethod does not direct modification of the global
  environment. The global environment is available as the variable `g` in the
  ldb environment, so you can set a global variable like this:
  
     (ldb) ,, g.some_global = 42
     
  Here is a list of all 'magic' variables in the ldb context:
    here       information about the current stack frame. See
               `help here` for details.
    g     (RO) The value of _G when the debugger was entered
    stack (RO) An array of stack frame info objects, as above.
               stack[here.level] == here
    PROMPT     The debugger comand prompt
    PROMPT2    The debugger continuation prompt
]]

splain.here = [[- the 'here' field in the ldb context

  The variable `here` in the ldb context is a reference to the 'current' stack
  frame. When ldb is invoked the current stackframe is the place where ldb was
  invoked (either directly or via an error function); you can view the stack
  with `backtrace` (usually written `bt`, in which the current stackframe is
  marked with a '*', and you can move up and down in the stack with the `up`
  and `down` commands.
  
  `here` contains all of the fields returned by `debug.getinfo` except for the
  table of valid breakpoints. It also contains some variables added by ldb
  itself. See `help getinfo` for a list.
 
  `here` is special; it can be added to or subtracted from to get at other
  levels. So instead of writing

     (ldb) == stack[here.level - 1].foo

  (to see the value of 'foo' in the calling frame), you can simply type:
  
     (ldb) == (here-1).foo

  You can assign `here` to any valid stack frame, or to the number of a valid
  stack frame, allowing you to navigate in the stack.  For example,

     (ldb) ,, here = here + 1

  is exactly the same as the `up` command, and:

     (ldb) ,, here = 7

  takes you to stackframe 7 directly.
]]
  
splain.getinfo = [[- fields in the getinfo table

  The getinfo fields are described in the Lua reference manual, although ldb
  adds a few of its own. Here is a complete list:

  currentline     The line currently being executed by the function
  fenv            The function's environment (that is, getfenv(func) )
  func            The function itself
  id              The id line ldb uses to identify the stackframe
  linedefined     The line in the source where the function is defined
  lastlinedefined The line where the definition of the function ends
  level           Level of the frame in the call stack
  name            A possible name for the function, if Lua can figure it out.
                  This will be `nil` in a stackframe discarded by a tailcall;
                  otherwise it will be the empty string if Lua can't find a
                  name.
  namewhat        "global", "local", "method", "field", "upvalue" or ""
  nups            The number of upvalues of the function
  short_src       A shorter version of `source`
  source          `@` followed by the filename, or "=stdin", or whatever was
                  provided as the last argument to loadfile
  var             The stackframe context; locals, upvalues and the fenv are
                  reflected into this table. You can also use local# and
                  upval# (where # is any integer) to get at locals and
                  upvalues by number. Assignment is allowed (and will be to
                  the fenv if there is no local or upvalue with the given
                  name.)
  what            "Lua", "C", "main" or "tail"
]]

splain.copyright = [[* ldb is public domain

  ldb was originally written by Rici Lake, and released into the public
  domain. Do with it as you see fit. The author disclaims all rights and
  responsibilities.
]]

splain.version = [[* this is version]]..VERSION.."-"..VERSION_DATE

splain.help = "[CMD] - show help for CMD or all commands"
alias.help = "?"

do
  local function sorted(filter)
    local t = {}
    for k, v in pairs(splain) do
      if filter(k, v) then t[#t+1] = k end
    end
    table.sort(t)
    local i = 0
    return function() i = i + 1; if t[i] then return t[i], splain[t[i]] end end
  end

  local function align(col1, col2)
    local intro = ""
    if #col1 > 15 then intro, col1 = col1.."\n", intro end
    return ("%s%-16s %s"):format(intro, col1, col2)
  end

  function command:help(cmd)
    cmd = cmd:gsub("%s", "")
    if cmd == "topics" then
      self:output"topics - help <topic> for details"
      self:output""
      for key, descr in sorted(
        function(key, descr) return not command[key] and descr:sub(1,1) == "*" end
      ) do
        self:output(align(key, descr:match"^%*[ \t]*([^\n]*)"))
      end
      self:output""
      for key, descr in sorted(
        function(key, descr) return not command[key] and descr:sub(1,1) ~= "*" end
      ) do
        self:output(align(key, descr:match"^.[ \t]*([^\n]*)"))
      end
    elseif cmd == "" then
      for cmd, descr in sorted(function(key) return command[key] end) do
        local arg, desc = descr:match"^([^-\n]-) ?%- ([^\n]*)"
        self:output(align( ("%-3s%s"):format(alias[cmd] or "", cmd.." "..arg),
                              desc) )
      end
      self:output"\nuse `help topics` for help on particular issues"
    elseif splain[cmd] then self:output(cmd.." "..splain[cmd])
    else
      local real = resolve_command[cmd]
      self:output(cmd.." ("..real..") "..(splain[real] or ""))
    end
  end
end

-- Do a single command and return
function handler:ldb_cmd(token, cmd)
  self:do_a_command(cmd)
  return true
end

function env.ldb (arg1, arg2)
  local check = prehandler[arg1]
  if check then
    local newarg1, newarg2 = check(arg2)
    if not newarg1 then return end
    arg1 = newarg1; arg2 = newarg2
  end
  local function sentinel(arg1, arg2)
    local self = State(sentinel, env)
    assert(self, "Couldn't find myself in the backtrace!")
    self.var.arg1 = arg1
    self.var.arg2 = arg2
    local f = handler[arg1]
    if f then
      local flag, val = f(self, arg1, arg2)
      if flag then return self:leave(val) end
    else
      if type(arg2) == "string" then
        local flag, val = self:do_a_command(arg2)
        if flag then return self:leave(val) end
      elseif type(arg2) == "table" then
        for _, cmd in ipairs(arg2) do
          local flag, val = self:do_a_command(cmd)
          if flag ~= nil then return self:leave(val) end
        end
      end
      if arg1 then self:output(arg1) end
    end
    self.interactive = true
    local here
    while true do
      if here ~= self.here then
        self:announce()
        here = self.here
      end
      local flag, val = self:do_a_command(self:input(env.PROMPT) or "continue")
      if flag then return self:leave(val) end
    end
  end
  -- the tailcall will put a pseudoframe on the stack, but it
  -- should still be skipped over by getinfo
  return sentinel(arg1, arg2)
end

-- In case we were started in some other way than "require'ldb'":
package.loaded.ldb = env.ldb
env.load_plugins()
return env.ldb
