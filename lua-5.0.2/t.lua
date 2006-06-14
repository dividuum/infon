function b(u,i)
    print(u)
    error("bar")
end

function a()
    coroutine.yield()
    local a, b = pcall(b, "foo", "bar")
    print(a)
    print(b)
    print("xxxxxx")
end

x = coroutine.create(a)
coroutine.resume(x)
coroutine.resume(x)
