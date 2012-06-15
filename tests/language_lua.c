#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
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

static int my_increment(lua_State *l)
{
    int x;
    if(lua_gettop(l) >= 0) {
        x = (int)lua_tonumber(l, -1);
        lua_pushnumber(l, x + 1);
    }
    printf("increment\n");
    return 1;
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
    lua_pushcfunction(lua->state, my_increment);
    lua_setglobal(lua->state, "my_increment");
}

static bool define_function_lua(language_interpreter_t*li, const char*script)
{
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua_State*l = lua->state;
    //int error = luaL_dostring(l, "function run_me()\nthrow \"test\";\nprint(\"test \" .. my_increment(3))\nend");
    int error = luaL_dofile(l, "test.lua");
    if(error) {
        show_error(lua->state);
        printf("Couldn't compile: %d\n", error);
        return 0;
    }
}

static int call_function_lua(language_interpreter_t*li, row_t*row)
{
    lua_internal_t*lua = (lua_internal_t*)li->internal;
    lua_State*l = lua->state;
    lua_getfield(l, LUA_GLOBALSINDEX, "run_me"); // push function
    int error = lua_pcall(l, /*nargs*/0, /*nresults*/1, 0);
    if(error) {
        show_error(l);
        printf("Couldn't run: %d\n", error);
        return 0;
    }
    lua_pop(l, 1);

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

int main()
{
    language_interpreter_t* l = lua_interpreter_new();
    l->define_function(l, "function bla() return 3; end");
    int r = l->call_function(l, NULL);
}
