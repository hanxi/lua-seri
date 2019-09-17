local ok, seri = pcall(require, "seri.core")
if not ok then
    seri = require "pure-seri"
end

local loadstring = loadstring or load
seri.decode = function(str)
    return loadstring("return " .. str)()
end

return seri
