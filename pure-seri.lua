local gsub = string.gsub
local match = string.match

local _M = {}

local tbl_to_str
local limit = 1024*1024 -- result has 1M element

-- set result array limit element
_M.limit = function(_limit)
    limit = _limit
end

-- fork from http://lua-users.org/wiki/TableUtils
local function append_result(result, ...)
    local n = select('#', ...)
    for i=1,n do
        result.i = result.i + 1
        result[result.i] = select(i, ...)
    end
end

local support_value_type = {
    string = function (vtype, v, result)
        v = gsub(v, "\n", "\\n")
        if match(gsub(v, "[^'\"]", ""), '^"+$') then
            append_result(result, "'")
            append_result(result, v)
            append_result(result, "'")
        else
            append_result(result, '"')
            v = gsub(v, '"', '\\"')
            append_result(result, v)
            append_result(result, '"')
        end
    end,
    number = function (vtype, v, result)
        append_result(result, tostring(v))
    end,
    table = function (vtype, v, result)
        tbl_to_str(v, result)
    end,
}
local function val_to_str(vtype, v, result)
    local func = support_value_type[vtype]
    if func then
        func(vtype, v, result)
    else
        append_result(result, "nil")
    end
end

local function key_to_str(ktype, k, result)
    if "string" == ktype and match(k, "^[_%a][_%a%d]*$") then
        append_result(result, k)
    else
        append_result(result, "[")
        val_to_str(ktype, k, result)
        append_result(result, "]")
    end
end

local support_key_type = {
    string = true,
    number = true,
}
tbl_to_str = function (tbl, result)
    append_result(result, "{")
    local is_first_ele = true
    for k,v in pairs(tbl) do
        if result.i > limit then
            break
        end
        local ktype = type(k)
        if support_key_type[ktype] then
            local vtype = type(v)
            if support_value_type[vtype] then
                if not is_first_ele then
                    append_result(result, ",")
                end
                key_to_str(ktype, k, result)
                append_result(result, "=")
                val_to_str(vtype, v, result)
                is_first_ele = false
            end
        end
    end
    append_result(result, "}")
end

_M.encode = function(...)
    local ret = {}
    ret.i = 0
    local n = select('#', ...)
    for i=1,n do
        local v = select(i, ...)
        local vtype = type(v)
        val_to_str(vtype, v, ret)
        if i ~= n then
            append_result(ret, ",")
        end
    end
    ret.i = nil
    return table.concat(ret, "")
end

return _M

