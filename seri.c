#define LUA_LIB

#include <stdlib.h>
#include <ctype.h>

#include "lua.h"
#include "lauxlib.h"

#define default_limit (1024*1024) // result has 1M element

static void append_val(lua_State *L, int index, int ret_index, int *elecnt);

static const char *char2escape[256] = {
    "\\u0000", "\\u0001", "\\u0002", "\\u0003",
    "\\u0004", "\\u0005", "\\u0006", "\\u0007",
    "\\b", "\\t", "\\n", "\\u000b",
    "\\f", "\\r", "\\u000e", "\\u000f",
    "\\u0010", "\\u0011", "\\u0012", "\\u0013",
    "\\u0014", "\\u0015", "\\u0016", "\\u0017",
    "\\u0018", "\\u0019", "\\u001a", "\\u001b",
    "\\u001c", "\\u001d", "\\u001e", "\\u001f",
    NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\/",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\u007f",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static void
append(lua_State *L, const char * str, int ret_index) {
    int sz = lua_rawlen(L, ret_index);
    lua_pushstring(L, str);
    lua_rawseti(L, ret_index, sz+1);
}

static void
append_len(lua_State *L, const char * str, int ret_index, size_t len) {
    int sz = lua_rawlen(L, ret_index);
    lua_pushlstring(L, str, len);
    lua_rawseti(L, ret_index, sz+1);
}

static void
append_string(lua_State *L, int index, int ret_index) {
    size_t len;
    const char *str = lua_tolstring(L, index, &len);
    const char *escstr;

    append(L, "\"", ret_index);
    for (size_t i = 0; i < len; i++) {
        escstr = char2escape[(unsigned char)str[i]];
        if (escstr) {
            append(L, escstr, ret_index);
        } else {
            append_len(L, &str[i], ret_index, 1);
        }
    }
    append(L, "\"", ret_index);
}

static void
append_other(lua_State *L, int index, int ret_index) {
    size_t l;
    const char *s = lua_tolstring(L, index, &l);
    append_len(L, s, ret_index, l);
}

static void
append_table(lua_State *L, int index, int ret_index, int *elecnt) {
    int limit = lua_tonumber(L, lua_upvalueindex(1));

    append(L, "{", ret_index);

    lua_pushnil(L);
    /* table, startkey */
    int comma = 0;
    int keytype = 0;
    int valutype = 0;
    while (lua_next(L, -2) != 0) {
        valutype = lua_type(L, -1);
        if (valutype != LUA_TNUMBER && valutype != LUA_TSTRING && valutype != LUA_TTABLE) {
            lua_pop(L, 1);
            continue;
        }

        if (comma) {
            append(L, ",", ret_index);
        } else {
            comma = 1;
        }

        /* table, key, value */
        int kindex = lua_gettop(L) - 1; // -2:key
        int vindex = lua_gettop(L); // -1:value
        const char * keystr = luaL_tolstring(L, kindex, NULL);
        keytype = lua_type(L, kindex);
        if (keytype == LUA_TNUMBER) {
            lua_pushfstring(L, "[%s]=", keystr);
        } else if (keytype == LUA_TSTRING) {
            lua_pushfstring(L, "['%s']=", keystr);
        } else {
            luaL_error(L, "table key must be a number or string, but type is %s", lua_typename(L, keytype));
            /* never returns */
        }

        size_t skeylen = 0;
        const char * skey = lua_tolstring(L, -1, &skeylen);
        append_len(L, skey, ret_index, skeylen);

        /* table, key, value */
        append_val(L, vindex, ret_index, elecnt);
        lua_pop(L, 1);
        /* table, key */
        (*elecnt)++;
        if (*elecnt > limit) {
            break;
        }
    }
    append(L, "}", ret_index);
}

static void
append_val(lua_State *L, int index, int ret_index, int *elecnt) {
    int vtype = lua_type(L, index);
    switch (vtype) {
        case LUA_TSTRING:
            append_string(L, index, ret_index);
            break;
        case LUA_TNUMBER:
            append_other(L, index, ret_index);
            break;
        case LUA_TTABLE:
            append_table(L, index, ret_index, elecnt);
            break;
        default:
            append(L, "nil", ret_index);
            break;
    }
    (*elecnt)++;
}

static int
lsetlimit(lua_State *L) {
    luaL_checkinteger(L, -1);
	lua_replace(L, lua_upvalueindex(1));
    return 0;
}

static int
lencode(lua_State *L) {
    size_t n = lua_gettop(L);

    //ret = {}
    lua_newtable(L);
    int ret_index = lua_gettop(L);

    int elecnt = 0;
    int i;
    for (i = 1; i <= n; i++) {
        append_val(L, i, ret_index, &elecnt);

        if (i != n) {
            append(L, ",", ret_index);
        }
    }


    luaL_Buffer b;
    luaL_buffinit(L, &b);
    int sz = lua_rawlen(L, ret_index);
    for (i = 1; i <= sz; i++) {
        lua_rawgeti(L, ret_index, i);
        luaL_addvalue(&b);
    }
    luaL_pushresult(&b);
    return 1;
}

LUAMOD_API int
luaopen_seri_core(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "limit", lsetlimit },
        { "encode", lencode },
        { NULL, NULL },
    };

    lua_newtable(L);
    lua_pushinteger(L, default_limit);
    luaL_setfuncs(L, l, 0);
    return 1;
}
