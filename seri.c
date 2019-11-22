#define LUA_LIB

#include <stdlib.h>
#include <ctype.h>

#include "lua.h"
#include "lauxlib.h"

#define default_limit (1024*1024) // result has 1M element

static void append_val(lua_State *L, int index, luaL_Buffer *b, int *elecnt);

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
append_string(lua_State *L, int index, luaL_Buffer *b) {
    size_t len;
    const char *str = lua_tolstring(L, index, &len);
    const char *escstr;

    luaL_addchar(b, '\"');
    for (size_t i = 0; i < len; i++) {
        escstr = char2escape[(unsigned char)str[i]];
        if (escstr) {
            luaL_addstring(b, escstr);
        } else {
            luaL_addchar(b, str[i]);
        }
    }
    luaL_addchar(b, '\"');
}

#define MAXNUMBER2STR	50
static void
append_number(lua_State *L, int index, luaL_Buffer *b) {
    char buff[MAXNUMBER2STR];
    size_t len;
    lua_Number value = lua_tonumber(L, index);
    len = lua_number2str(buff, sizeof(buff), value);
    luaL_addlstring(b, buff, len);
}

/*
static void 
dump_stack(lua_State *L) {
    int top = lua_gettop(L);
    for (int i=1; i<=top; i++) {
        int tp = lua_type(L, i);
        switch (tp) {
            case LUA_TNUMBER:
                printf("n:%7g\n", lua_tonumber(L, i));
                break;
            default:
                printf("s:%s\n", lua_tostring(L, i));
                break;
        }
    }
}
*/

static void
append_string_key(lua_State *L, int index, luaL_Buffer *b) {
    size_t len;
    const char *str = lua_tolstring(L, index, &len);
    int is_var_str = 1;

    if (len>=1 && isalpha(str[0])) {
        for (size_t i = 1; i < len; i++) {
            char c = str[i];
            if (!isalpha(c) && !isdigit(c)) {
                is_var_str = 0;
                break;
            }
        }
    } else {
        is_var_str = 0;
    }
    
    if (is_var_str) {
        luaL_addlstring(b, str, len);
        luaL_addchar(b, '=');
    } else {
        luaL_addchar(b, '[');
        append_string(L, index, b);
        luaL_addstring(b, "]=");
    }
}
static void
append_table(lua_State *L, int index, luaL_Buffer *b, int *elecnt) {
    int limit = lua_tonumber(L, lua_upvalueindex(1));

    luaL_addchar(b, '{');

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
            luaL_addchar(b, ',');
        } else {
            comma = 1;
        }

        /* table, key, value */
        keytype = lua_type(L, -2);
        if (keytype == LUA_TNUMBER) {
            luaL_addchar(b, '[');
            append_number(L, -2, b);
            luaL_addstring(b, "]=");
        } else if (keytype == LUA_TSTRING) {
            append_string_key(L, -2, b);
        } else {
            luaL_error(L, "table key must be a number or string, but type is %s", lua_typename(L, keytype));
            /* never returns */
        }

        /* table, key, value */
        append_val(L, -1, b, elecnt);
        lua_pop(L, 1);
        /* table, key */
        (*elecnt)++;
        if (*elecnt > limit) {
            break;
        }
    }
    luaL_addchar(b, '}');
}

static void
append_val(lua_State *L, int index, luaL_Buffer *b, int *elecnt) {
    int vtype = lua_type(L, index);
    switch (vtype) {
        case LUA_TSTRING:
            append_string(L, index, b);
            break;
        case LUA_TNUMBER:
            append_number(L, index, b);
            break;
        case LUA_TTABLE:
            append_table(L, index, b, elecnt);
            break;
        default:
            luaL_addstring(b, "nil");
            (*elecnt)++;
            break;
    }
}

static int 
lsetlimit(lua_State *L) {
    luaL_checkinteger(L, -1);
	lua_replace(L, lua_upvalueindex(1));
    return 0;
}

static int 
lencode(lua_State *L) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);

    size_t n = lua_gettop(L);
    int elecnt = 0;
    for (size_t i = 1; i <= n; i++) {
        lua_pushvalue(L, i);
        append_val(L, -1, &b, &elecnt);
        lua_pop(L, 1);

        if (i != n) {
            luaL_addchar(&b, ',');
        }
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
