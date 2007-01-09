--[[
  
  Menu system written by Rici Lake in order to demonstrate
  some interesting syntactic possibilities in Lua

  This program is released into the public domain. But if you find
  it useful, you could certainly buy me a coffee sometime.

  Modified by Florian Wesch to fit into infon.

]]--

------------------------------------------------------------------
-- menu.lua
------------------------------------------------------------------

do

  local curriedMethod, method, meta = {}, {}, {}

  -- __index either executes a method from method or curries a method from
  -- curriedMethod with its self argument. This allows all calls to be with
  -- "." rather than ":" and also allows you to write obj.foo instead of
  -- obj.foo() for methods which don't require arguments. It might
  -- not be great design, but it is interesting. :)
  
  function meta:__index(key)
    local func = method[key]
    if func then
      return func(self)
     else
      func = curriedMethod[key]
      if func then
        local rv = function(a, b) return func(self, a, b) end
        self[key] = rv
        return rv
      end
    end
  end

  -- quick and dirty display routine.
  local DASHES = string.rep('-', 80)
  local DOUBLES = string.rep('=', 80)

  local function drawmenu(self)
    local maxsize = string.len(self.name) + 2
    local item = 0
    for i = 1, self.n do
      local sz = 6 + string.len(self[1][i])
      if maxsize < sz then maxsize = sz end
    end
    if maxsize > 75 then maxsize = 75 end
    local sepformat = "  +%-"..maxsize.."."..maxsize.."s+"
    local nameformat = "  | %-"..(maxsize - 2).."."..(maxsize-2).."s |"
    local itemformat = "  | %2i. %-"..(maxsize - 6).."."..(maxsize-6).."s |"
    local sepline = string.format(sepformat, DASHES)
    print(string.format(sepformat, DOUBLES))
    print(string.format(nameformat, self.name))
    print(string.format(sepformat, DOUBLES))
    for i = 1, self.n do
      if self[2][i] then
        item = item + 1
        print(string.format(itemformat, item, self[1][i]))
      else
        print(sepline)
      end
    end
    print(sepline)
  end

  -- Equally quick and dirty menu execution. Tail calls the function
  -- associated with the selected menu item.
  local function domenu(self)
    while true do
      print()
      drawmenu(self)
      print()
      local choice = coroutine.yield()
      if choice == nil then return false end
      local _, _, item = string.find(choice, "^%s*(%d+)%s*$")
      if item then
        item = item + 0 -- force numeric conversion
        for i = 1, self.n do
          if self[2][i] then
            if item == 1 then return self[2][i]() end
            item = item - 1
          end
        end
      end
      print("Selection '" .. choice .. "' not valid.")
    end
  end

  -- Create a new menu with given name and back reference.
  local function newmenu(name, back)
    return setmetatable({
      {}, {},  -- [1] is the menu label, [2] is the associated function
      name = name,
      back = back,
      n = 0
    },
    meta)
  end

  -- insert a label and a function at the end of a menu
  local function put(self, name, action)
    local n = self.n + 1
    self.n = n
    self[1][n] = name
    self[2][n] = action
    return self
  end

  -- Now the actual menu methods.
  -- add(label, id)
  function curriedMethod:add(name, id)
    return put(self, name, type(id) == "function" and id or function() return id end)
  end
  
  -- create and open a submenu with the given name
  function curriedMethod:sub(name)
    local submenu = newmenu(self.name .. " / " .. name, self)
    put(self, name.." -->", function() return domenu(submenu) end)
    return submenu
  end

  -- create a new, unrelated menu. You cannot use super afterwards
  function curriedMethod:new(name)
    return newmenu(name)
  end
  
  -- go back to the previous level, after introducing the automatic Back label
  -- unless this is a top-level menu
  function method:super()
    local mom = self.back
    if mom then
      put(self, "-")
      put(self, "<-- Back", function() return domenu(self.back) end)
      return self.back
     else return self
    end
  end

  -- insert a separator line
  function method:sep()
    return put(self, "-")
  end

  -- and a function to actually execute the thing
  curriedMethod.run = domenu

  function menu(name)
    local coro, input
    local menu = newmenu("").new(name)

    function input(key)
      if not key or not coro or coroutine.status(coro) == "dead" then
        coro = coroutine.create(menu.run)
      end
      local ok, msg = coroutine.resume(coro, key)
      if not ok then 
        error(_TRACEBACK(coro, msg))
      end
    end

    function onInput0() input()  end
    function onInput1() input(1) end
    function onInput2() input(2) end
    function onInput3() input(3) end
    function onInput4() input(4) end
    function onInput5() input(5) end
    function onInput6() input(6) end
    function onInput7() input(7) end
    function onInput8() input(8) end
    function onInput9() input(9) end
    return menu
  end
end
