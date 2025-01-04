#include <iostream>
#include <lua.hpp>
#include <cassert>

enum class LuaType: char {
    Nil, Boolean, String, Number
};

struct LuaNil final {
    const LuaType type = LuaType::Nil;
    const std::nullptr_t value = nullptr;
    static LuaNil make() {
        return LuaNil();
    }
private:
    LuaNil() = default;    
};

struct LuaString final {
    const LuaType type = LuaType::String;
    std::string value;
    static LuaString make(const std::string& value) {
        return LuaString(value);
    }
private:
    LuaString(const std::string& value): value(value){};
};

struct LuaBoolean final {
    const LuaType type = LuaType::Boolean;
    bool value;
    static LuaBoolean make(bool value) {
        return LuaBoolean(value);
    }
private:
    LuaBoolean(bool value): value(value){};
};

struct LuaNumber final {
    const LuaType type = LuaType::Number;
    const double value;
    static LuaNumber make(double value) {
        return LuaNumber(value);
    }
private:
    LuaNumber(double value): value(value) {};
};

using LuaValue = std::variant<LuaNumber, LuaBoolean, LuaNil, LuaString>;

LuaType get_type(const LuaValue& value) {
    return std::visit([](const auto& v){return v.type;}, value);
}

std::string get_lua_value_str(const LuaValue& value) {
    switch(get_type(value)){
        case LuaType::Nil:
            return "nil";
        case LuaType::Boolean:
            return std::get<LuaBoolean>(value).value ? "true" : "false";
        case LuaType::String:
            return std::get<LuaString>(value).value;
        case LuaType::Number:
            return std::to_string(std::get<LuaNumber>(value).value);
    }
}


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

    
    template <typename... Params>
    void push_value(const Params&... param) {
        auto push = [this](LuaType t, const LuaValue& p){
            switch(t) {
                case LuaType::Nil:
                    lua_pushnil(L);
                    break;
                case LuaType::String:
                    lua_pushstring(L, std::get<LuaString>(p).value.c_str());
                    break;
                case LuaType::Boolean:
                    lua_pushboolean(L, std::get<LuaBoolean>(p).value);
                    break;   
                case LuaType::Number:
                    lua_pushnumber(L, std::get<LuaNumber>(p).value);
                    break;       
            }
        };
        (push(get_type(param), param), ...);
    }

    LuaValue pop_value() {
        auto value = get_value(-1);
        lua_pop(L,1);
        return value;
    }

    LuaValue get_value(int index) {
        switch(lua_type(L, index)) {
            case LUA_TNIL:
                return LuaNil::make();
            case LUA_TBOOLEAN:
                return LuaBoolean::make(lua_toboolean(L, index) == 1);
            case LUA_TNUMBER:
                return LuaNumber::make((double)lua_tonumber(L, index));
            case LUA_TSTRING:
                return LuaString::make(lua_tostring(L, index));
            default:
                return LuaNil::make();
        }
    }

    template <typename... Params>
    LuaValue call(const std::string& function, const Params&... params) {
        int type = lua_getglobal(L, function.c_str());
        assert(LUA_TFUNCTION == type);
        push_value(params...);
        pcall(sizeof...(params), 1);
        return pop_value();
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
    std::cout << std::get<LuaString>(le.call("greetings", LuaString::make("123"), LuaString::make("456"))).value;
}
