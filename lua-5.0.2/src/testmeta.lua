

function test_plain_coro()

    local a = {}

    function a.func(a, b)
        local val, v2 = coroutine.yield()
        return val .. v2
    end

    local function c()
        local r = ""
        for i=1, 3 do
            local q = a.func()
            r = r .. q
        end
        return r
    end

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== plain coroutine passed")

end


function test_OP_GETTABLE()

    local a = {}
    local b = {}

    function b.__index(a, b)
        local val, v2 = coroutine.yield()
        return val .. v2
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            r = r .. a.zoo
        end
        return r
    end

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== OP_GETTABLE passed")

end


function test_OP_GETTABLE_tail()

    local a = {}
    local b = {}

    function b.__index(a, b)
        return coroutine.yield()
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            r = r .. a.zoo
        end
        return r
    end

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc")
    coroutine.resume(r, "xyz")
    local succ, val = coroutine.resume(r, "123")
    
    assert(succ and val == "abcxyz123")
    print("==== OP_GETTABLE (tail-call) passed")

end


function test_OP_GETGLOBAL()

    local a = {}
    local b = {}

    function b.__index(a, b)
        local val, v2 = coroutine.yield()
        return val .. v2
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            r = r .. zoo
        end
        return r
    end
    
    setfenv(c, a)

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== OP_GETGLOBAL passed")

end


function test_OP_SELF()

    local a = {}
    local b = {}

    function b.__index(a, b)
        local val, v2 = coroutine.yield()
        return function()
           return val .. v2
	end
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            local q = a:zoo()
            r = r .. q
        end
        return r
    end

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== OP_SELF passed")

end


function test_OP_SETTABLE()

    local a = { groucho="" }
    local b = {}

    function b.__newindex(self, k, v)
        local val, v2 = coroutine.yield()
        a.groucho = a.groucho .. val .. v2
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            a.zoo = "x"
        end
        return a.groucho
    end

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== OP_SETTABLE passed")

end


function test_OP_SETGLOBAL()

    local a = { groucho="" }
    local b = {}

    function b.__newindex(self, k, v)
        local val, v2 = coroutine.yield()
        a.groucho = a.groucho .. val .. v2
    end

    setmetatable(a, b)

    local function c()
        local r = ""
        for i=1, 3 do
            zoo = "x"
        end
        return groucho
    end
    
    setfenv(c, a)

    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "ABC")
    coroutine.resume(r, "xyz", "XYZ")
    local succ, val = coroutine.resume(r, "012345", "6789")
    
    assert(succ and val == "abcABCxyzXYZ0123456789")
    print("==== OP_SETGLOBAL passed")

end


function test_OP_ADD()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__add(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a + a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_ADD passed")

end    
    

function test_OP_SUB()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__sub(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a - a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_SUB passed")

end    


function test_OP_MUL()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__mul(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a * a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_MUL passed")

end    


function test_OP_DIV()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__div(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a / a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_DIV passed")

end    


function test_OP_POW()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__pow(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a ^ a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_POW passed")

end    


function test_OP_POW_global()

    local function c()
       return 2 ^ 5
    end
    
    function __pow(x, y)
       return coroutine.yield()
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 100)
    
    assert(succ and val == 100)
    print("==== OP_POW_global passed")
    
end    


function test_OP_LT()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__lt(self, other)
       return coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       local r = ""
       for i=1, 3 do
          if a < a2 then
             r = r .. "yes"
          else
             r = r .. "no"
          end
       end
       return r
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, true)
    local succ, val = coroutine.resume(r, false)
    local succ, val = coroutine.resume(r, true)
    
    assert(succ and val == "yesnoyes")
    print("==== OP_LT passed")

end    


function test_OP_LE()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__le(self, other)
       return coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       local r = ""
       for i=1, 3 do
          if a <= a2 then
             r = r .. "yes"
          else
             r = r .. "no"
          end
       end
       return r
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, true)
    local succ, val = coroutine.resume(r, false)
    local succ, val = coroutine.resume(r, true)
    
    assert(succ and val == "yesnoyes")
    print("==== OP_LE passed")

end    


function test_OP_EQ()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__eq(self, other)
       return coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       local r = ""
       for i=1, 3 do
          if a == a2 then
             r = r .. "yes"
          else
             r = r .. "no"
          end
       end
       return r
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, true)
    local succ, val = coroutine.resume(r, false)
    local succ, val = coroutine.resume(r, true)
    
    assert(succ and val == "yesnoyes")
    print("==== OP_EQ passed")

end    


function test_OP_CONCAT()

    local a = { value=10 }
    local a2 = { value=15 }
    local b = {}
    
    function b.__concat(self, other)
       return self.value + other.value + coroutine.yield()
    end
    
    setmetatable(a, b)
    setmetatable(a2, b)
    
    local function c()
       return a .. a2
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 11)
    
    assert(succ and val == (10 + 15 + 11))
    print("==== OP_CONCAT passed")

end    


function test_OP_UNM()

    local a = { value=10 }
    local b = {}
    
    function b.__unm(self)
       return coroutine.yield()
    end
    
    setmetatable(a, b)
    
    local function c()
       return -a
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, 52)
    
    assert(succ and val == 52)
    print("==== OP_UNM passed")

end    


function test_OP_TFORLOOP()

    local function iter()
        return coroutine.yield()
    end
    
    local function c()
        local r = ""
        for a, b, c in iter do
            r = r .. a .. b .. c
        end
        return r
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc", "DEF", "000")
    coroutine.resume(r, "ghi", "JKLM", "111")
    local succ, val = coroutine.resume(r)
    
    assert(succ and val == "abcDEF000ghiJKLM111")
    print("==== OP_TFORLOOP passed")

end


function test_table_foreachi()

    local t = {"abc", "XYZ", "12345"}
    
    local function c()
        table.foreachi(t, function (k,v)
            return coroutine.yield(v)
        end)
        coroutine.yield("ALL DONE!")
    end
    
    local r = coroutine.create(c)
    local succ, val
    
    succ, val = coroutine.resume(r)
    assert(succ and val == "abc")

    succ, val = coroutine.resume(r)
    assert(succ and val == "XYZ")

    succ, val = coroutine.resume(r, true)
    assert(succ and val == "ALL DONE!")
    
    print("==== table.foreachi() passed")
    
end


function test_table_foreach()

    local t = {"abc", "XYZ", "12345"}
    
    local function c()
        table.foreach(t, function (k,v)
            return coroutine.yield(k, v)
        end)
        coroutine.yield(500, "ALL DONE!")
    end
    
    local r = coroutine.create(c)
    local succ, k, v
    
    succ, k, v = coroutine.resume(r)
    assert(succ)
    t[k] = "hit"

    succ, k, v = coroutine.resume(r)
    assert(succ)
    t[k] = "hit"

    succ, k, v = coroutine.resume(r)
    assert(succ)
    t[k] = "hit"

    succ, k, v = coroutine.resume(r)
    assert(succ)
    t[k] = "hit"
    
    assert(t[1] == "hit")
    assert(t[2] == "hit")
    assert(t[3] == "hit")
    assert(t[500] == "hit")

    print("==== table.foreach() passed")
    
end


function test_tostring()

    local x = {}
    local y = { __tostring = function() return coroutine.yield() end }
    
    setmetatable(x, y)
    
    local function c()
        return tostring(x)
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    local succ, val = coroutine.resume(r, "hello")
    
    assert(succ and val == "hello")
    print("==== tostring() passed")
    
end


function test_print()

    local x = {}
    local y = { __tostring = function() return coroutine.yield() end }
    
    setmetatable(x, y)
    
    local function c()
        print(x, x, x)
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    assert(coroutine.resume(r, "hello"))
    assert(coroutine.resume(r, "out"))
    assert(coroutine.resume(r, "there"))
    
    print("==== print() passed")
    
end


function test_dofile()

    local function c()
       return dofile("testdofile.lua")
    end
    
    local r = coroutine.create(c)
    coroutine.resume(r)
    coroutine.resume(r, "abc")
    local succ, val = coroutine.resume(r, "def")
    
    assert(succ and val == "abcdef")
    print("==== dofile() passed")
    
end    


function test_string_gsub()

    local function c()
        return string.gsub("xxx yyy zzz X Y Z", "(%w+)", function (s) return coroutine.yield(s) end)
    end
    
    local r = coroutine.create(c)
    local succ, val = coroutine.resume(r)
    assert(succ and val == "xxx")
    
    succ, val = coroutine.resume(r, "a")
    assert(succ and val == "yyy")
    
    succ, val = coroutine.resume(r, "lua")
    assert(succ and val == "zzz")

    succ, val = coroutine.resume(r, "zoo")
    assert(succ and val == "X")
    
    succ, val = coroutine.resume(r, "in")
    assert(succ and val == "Y")

    succ, val = coroutine.resume(r, "a")
    assert(succ and val == "Z")
    
    succ, val = coroutine.resume(r, "shoe")
    assert(succ and val == "a lua zoo in a shoe")
    
    print("==== gsub() passed")
end
    

test_plain_coro()
test_OP_GETTABLE()
test_OP_GETTABLE_tail()
test_OP_GETGLOBAL()
test_OP_SELF()
test_OP_SETTABLE()
test_OP_SETGLOBAL()
test_OP_ADD()
test_OP_SUB()
test_OP_MUL()
test_OP_DIV()
test_OP_POW()
test_OP_POW_global()
test_OP_LT()
test_OP_LE()
test_OP_EQ()
test_OP_CONCAT()
test_OP_UNM()
test_OP_TFORLOOP()
test_table_foreachi()
test_table_foreach()
test_tostring()
test_print()
test_dofile()
test_string_gsub()
