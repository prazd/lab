#include <iostream>
#include <lua.hpp>

int main(){
    auto l = luaL_newstate();
    std::cout << "Lua version number is:" << lua_version(l) << "\n";
    lua_close(l); 
}
