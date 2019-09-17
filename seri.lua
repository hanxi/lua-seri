local ok, seri = pcall(require, "seri.core")
if not ok then
    seri = require "pure-seri"
end

return seri
