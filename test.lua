local seri = require "seri"

local t1 = {
    a = "a",
    b = 1,
    c = 1.1,
    d = {
        e = "e",
        f = 2,
        g = 2.2,
    },
}

local str = seri.encode(t1, 2, 3, "a", nil, "b")
print(str)

local t2,a,b,c,d,e = seri.decode(str)
print(t2,a,b,c,d,e)

