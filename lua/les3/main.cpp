#include <iostream>
#include <lua.hpp>

class LuaExecutor{
public:    
    LuaExecutor(): L(luaL_newstate()){
        luaL_openlibs(L);
    };
    void execute_file(const std::string& path) {
        if (luaL_loadfile(L, path.c_str())){
            std::cerr << "Failed to prepare file:" << popString() << "\n";
        }
        pcall(0,0);
    }
    std::string popString() {
        std::string result(lua_tostring(L, -1));
        lua_pop(L,1);
        return result;
    }
    void pcall(int nargs, int nresults){
        if (lua_pcall(L, nargs, nresults, 0)) {
            std::cerr << "Failed to executed Lua code:" << popString() << "\n";
        }
    }
    void execute(const std::string& script) {
        if (luaL_loadstring(L, script.c_str())) {
            std::cerr << "Failed to prepare script:" << popString() << "\n";
        }
        pcall(0,0);
    }
    virtual ~LuaExecutor(){
        lua_close(L);
    };
private:
    lua_State* const L;
};

int main(){
    LuaExecutor le;
    le.execute_file("scripts/hello.lua"); 
    le.execute("print('second')");
}
