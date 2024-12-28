#include <iostream>
#include <lua.hpp>

int main(){
    auto l = luaL_newstate();
    luaL_openlibs(l);
    if (luaL_loadfile(l, "scripts/test.lua") || lua_pcall(l, 0,0,0)){
        std::cout << "error:" << lua_tostring(l, -1) << "\n";
        lua_pop(l,1);
    }
    //std::cout << "Lua version number is:" << lua_version(l) << "\n";
    lua_close(l); 
}
