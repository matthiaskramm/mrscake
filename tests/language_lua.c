#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <errno.h>
#include "language_interpreter.h"

typedef struct _lua_internal {
    language_interpreter_t*li;
    lua_State* state;
} lua_internal_t;

static const luaL_reg lualibs[] =
{
    {"base", luaopen_base},
    {NULL,   NULL}
};

static int f_trace(lua_State *l)
{
    if(lua_gettop(l) >= 0) {
        const char*str = lua_tostring(l, -1);
        printf("%s\n", str);
        lua_pushnumber(l, 1);
        return 1;
    }
    return 0;
}

static void openlualibs(lua_State *l)
{
    const luaL_reg *lib;
    for(lib = lualibs; lib->func != NULL; lib++)
    {
        lib->func(l);
        lua_settop(l, 0);
    }
}

static void show_error(lua_State *l)
{
    size_t len = 0;
    const char *s = lua_tolstring (l, -1, &len);
    printf("%s\n", s);
}

bool init_lua(lua_internal_t*lua)
{
    lua->state = lua_open();
    openlualibs(lua->state);
    lua_pushcfunction(lua->state, f_trace);
    lua_setglobal(lua->state, "trace");
}

static bool define_function_lua(language_interpreter_t*li, const char*script)
{
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua_State*l = lua->state;

    int error = luaL_loadbuffer(l, script, strlen(script), "@file.lua");
    if(!error) {
        error = lua_pcall(l, 0, LUA_MULTRET, 0);
    }
    if(error) {
        show_error(l);
        printf("Couldn't compile: %d\n", error);
        return 0;
    }

    return 1;
}

static int call_function_lua(language_interpreter_t*li, row_t*row)
{
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua_State*l = lua->state;
    lua_getfield(l, LUA_GLOBALSINDEX, "predict"); // push function
    int i;
    for(i=0;i<row->num_inputs;i++) {
        variable_t*v = &row->inputs[i];
        switch(v->type) {
            case CATEGORICAL:
                lua_pushinteger(l, row->inputs[i].category);
            break;
            case CONTINUOUS:
                lua_pushnumber(l, row->inputs[i].value);
            break;
            case TEXT:
                lua_pushstring(l, row->inputs[i].text);
            break;
            default:
                return 0;
        }
    }
    int error = lua_pcall(l, /*nargs*/row->num_inputs, /*nresults*/1, 0);
    if(error) {
        show_error(l);
        printf("Couldn't run: %d\n", error);
        return 0;
    }
    int ret = lua_tointeger(l, -1);
    lua_pop(l, 1);
    return ret;
}

static void destroy_lua(language_interpreter_t* li)
{
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua_close(lua->state);
    free(lua);
    free(li);
}

language_interpreter_t* lua_interpreter_new()
{
    language_interpreter_t * li = calloc(1, sizeof(language_interpreter_t));
    li->name = "lua";
    li->define_function = define_function_lua;
    li->call_function = call_function_lua;
    li->destroy = destroy_lua;
    li->internal = calloc(1, sizeof(lua_internal_t));
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua->li = li;
    init_lua(lua);
    return li;
}
