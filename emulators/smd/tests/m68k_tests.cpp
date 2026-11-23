#include <nlohmann/json.hpp>
#include "CPU.hpp"
#include "BUS.hpp"
#include <fstream>

using nj = nlohmann::json;

//https://github.com/SingleStepTests/m68000
void set_initial(const nj& json, M68K& cpu){
    cpu.dregs[M68K::D0] = json["initial"]["d0"];
    cpu.dregs[M68K::D1] = json["initial"]["d1"];
    cpu.dregs[M68K::D2] = json["initial"]["d1"];
    cpu.dregs[M68K::D3] = json["initial"]["d3"];
    cpu.dregs[M68K::D4] = json["initial"]["d4"];
    cpu.dregs[M68K::D5] = json["initial"]["d5"];
    cpu.dregs[M68K::D6] = json["initial"]["d6"];
    cpu.dregs[M68K::D7] = json["initial"]["d7"];

    cpu.aregs[M68K::A0] = json["initial"]["a0"];
    cpu.aregs[M68K::A1] = json["initial"]["a1"];
    cpu.aregs[M68K::A2] = json["initial"]["a1"];
    cpu.aregs[M68K::A3] = json["initial"]["a3"];
    cpu.aregs[M68K::A4] = json["initial"]["a4"];
    cpu.aregs[M68K::A5] = json["initial"]["a5"];
    cpu.aregs[M68K::A6] = json["initial"]["a6"];
    cpu.usp = json["initial"]["usp"];
    cpu.ssp = json["initial"]["ssp"];
    cpu.sreg = json["initial"]["sr"];
    cpu.pc = json["initial"]["pc"];

    cpu.prefetch_q.clear();
    cpu.prefetch_q.push_back(json["initial"]["prefetch"][0]);
    cpu.prefetch_q.push_back(json["initial"]["prefetch"][1]);

    for (size_t i = 0, e = json["initial"]["ram"].size(); i != e; ++i)
        BUS::m68kmem[json["initial"]["ram"][i][0]] = json["initial"]["ram"][i][1];

}

std::pair<bool, std::string_view> cmp_final(const nj& json, M68K& cpu){
    if (cpu.dregs[M68K::D0] != json["final"]["d0"])   return {false, "d0"};
    if (cpu.dregs[M68K::D1] != json["final"]["d1"]) return {false, "d1"};
    if (cpu.dregs[M68K::D2] != json["final"]["d1"]) return {false, "d2"};
    if (cpu.dregs[M68K::D3] != json["final"]["d3"]) return {false, "d3"};
    if (cpu.dregs[M68K::D4] != json["final"]["d4"]) return {false, "d4"};
    if (cpu.dregs[M68K::D5] != json["final"]["d5"]) return {false, "d5"};
    if (cpu.dregs[M68K::D6] != json["final"]["d6"]) return {false, "d6"};
    if (cpu.dregs[M68K::D7] != json["final"]["d7"]) return {false, "d7"};

    if (cpu.aregs[M68K::A0] != json["final"]["a0"]) return {false, "a0"};
    if (cpu.aregs[M68K::A1] != json["final"]["a1"]) return {false, "a1"};
    if (cpu.aregs[M68K::A2] != json["final"]["a1"]) return {false, "a2"};
    if (cpu.aregs[M68K::A3] != json["final"]["a3"]) return {false, "a3"};
    if (cpu.aregs[M68K::A4] != json["final"]["a4"]) return {false, "a4"};
    if (cpu.aregs[M68K::A5] != json["final"]["a5"]) return {false, "a5"};
    if (cpu.aregs[M68K::A6] != json["final"]["a6"]) return {false, "a6"};

    if (cpu.usp  !=  json["final"]["usp"]) return {false, "usp"};
    if (cpu.ssp  !=  json["final"]["ssp"]) return {false, "ssp"};
    if (cpu.sreg !=  json["final"]["sr"])  return {false, "sreg"};
    if (cpu.pc   !=  json["final"]["pc"])  return {false, "pc"};

    for (size_t i = 0, e = json["final"]["ram"].size(); i != e; ++i){
        if (BUS::m68kmem[json["final"]["ram"][i][0]] != json["final"]["ram"][i][1]) {
            std::cout << "mem error:" << i << '\n';
            return {false, "mem"};
        }
    }
    return {true, "ok"};
}

bool test_m68k_instruction(std::string_view path, M68K& cpu, BUS& bus){
    std::ifstream f(path);
    if (!f.is_open()) {std::cout << "read file error"; return false;}
    std::string raw_json_s;
    nj json = nj::parse(f);
    for (size_t i = 0, e = json.size(); i != e; ++i){ // TODO: check 3
        set_initial(json[i], cpu);
        cpu.step(bus);
        auto [is_ok, txt] = cmp_final(json[i], cpu);
        if (!is_ok) {std::cout << "Failed:" << txt << '\n'; return false;}
    }
    return true;
}

int main(){
    M68K cpu;
    BUS bus;
    test_m68k_instruction("/Users/prazd/Desktop/dev/m68000/v1/NOP.json",cpu, bus);
}