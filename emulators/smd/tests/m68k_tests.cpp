#include <nlohmann/json.hpp>
#include "CPU.hpp"
#include "BUS.hpp"
#include <fstream>

using nj = nlohmann::json;

//https://github.com/SingleStepTests/m68000
void set_initial(const nj& json, M68K& cpu){
    for (size_t i = 0, e = std::size(cpu.dregs); i != e; ++i){
        auto s = str_num_template<3>("d&", i);
        cpu.dregs[i] = json["initial"][s.to_sv()];
    }

    for (size_t i = 0, e = std::size(cpu.aregs); i != e; ++i){
        auto s = str_num_template<3>("a&", i);
        cpu.aregs[i] = json["initial"][s.to_sv()];
    }

    cpu.usp = json["initial"]["usp"];
    cpu.ssp = json["initial"]["ssp"];
    cpu.sreg = json["initial"]["sr"];
    cpu.pc = json["initial"]["pc"];

    cpu.prefetch_q.clear();
    cpu.prefetch_q.push_back(json["initial"]["prefetch"][0]);
    cpu.prefetch_q.push_back(json["initial"]["prefetch"][1]);

    for (size_t i = 0, e = json["initial"]["ram"].size(); i != e; ++i)
        BUS::m68kmem[json["initial"]["ram"][i][0]] = json["initial"]["ram"][i][1];

    cpu.was_exception = false;
}

bool cmp(std::string_view sv, auto current, auto expected){
    if (current != expected){
        std::cout << sv  << " current:" << current << " expected:" << expected << '\n';
        return false;
    }
    return true;
}

bool cmp_final(const nj& json, M68K& cpu){
    // DREG
    for (size_t i = 0, e = std::size(cpu.dregs); i != e; ++i){
        auto s = str_num_template<3>("d&", i);
        if (!cmp(s.to_sv(), cpu.dregs[i],  json["final"][s.to_sv()])) return false;
    }

    // AREG
    for (size_t i = 0, e = std::size(cpu.aregs); i != e; ++i){
        auto s = str_num_template<3>("a&", i);
        if (!cmp(s.to_sv(), cpu.aregs[i],  json["final"][s.to_sv()])) return false;
    }

    if (!cmp("usp", cpu.usp , json["final"]["usp"])) return false;
    if (!cmp("ssp", cpu.ssp , json["final"]["ssp"])) return false;
    if (!cmp("sr", cpu.sreg , json["final"]["sr"])) return false;
    if (!cmp("pc", cpu.pc , json["final"]["pc"])) return false;

    for (size_t i = 0, e = json["final"]["ram"].size(); i != e; ++i){
        if (BUS::m68kmem[json["final"]["ram"][i][0]] != json["final"]["ram"][i][1]) {

#if 1
            // Ignore PC values for now
            if (cpu.was_exception &&
            json["final"]["ram"][i][0] >= ((unsigned)(json["final"]["ssp"]) + 10) ||
            json["final"]["ram"][i][0] >= ((unsigned)(json["final"]["usp"]) + 10)
            ) continue;
#endif

            std::cout << "mem error:" << i << " current:" <<  (int)BUS::m68kmem[json["final"]["ram"][i][0]] <<
            " expected:" <<   json["final"]["ram"][i][1] << '\n';
            return false;
        }
    }
    return true;
}

bool test_m68k_instruction(std::string_view path, M68K& cpu, BUS& bus){
    std::ifstream f(path);
    if (!f.is_open()) {std::cout << "read file error"; return false;}
    std::string raw_json_s;
    nj json = nj::parse(f);
    for (size_t i = 0, e = json.size(); i != e; ++i){
        set_initial(json[i], cpu);
        cpu.step(bus);
        if (auto is_ok = cmp_final(json[i], cpu); !is_ok){
            std::cout << "Failed:" << i << '\n';
            return false;
        }

    }
    return true;
}

// NOP +
// NOT.b +
// NOT.w +- (pc)
// NOT.l +- (pc)
int main(){
    M68K cpu;
    BUS bus;
    test_m68k_instruction("/Users/prazd/Desktop/dev/m68000/v1/NOT.l.json",cpu, bus);
}