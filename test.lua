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
    h = {1,2,3,4,5,6},
    i = function () end,
}

seri.limit(4000)
for i=1,10000 do
str = seri.encode(t1, 2, 3, "a", nil, "b", function () end)
end
print(str)

local t2,a,b,c,d,e,f = seri.decode(str)
print(t2,a,b,c,d,e,f)


