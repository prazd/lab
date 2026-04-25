#include <iostream>
#include <vector>
#include <numeric>

// S = {A, B, C, D}
// Σ = {0, 1}

// A: 0 → B, 1 → C
// B: 0 → A, 1 → D
// C: 0 → D, 1 → A
// D: 0 → D, 1 → D

// F = {A, C}

// 1) {{A,C}, {B, D}}

// 2) A: (B,D), (A,C)
//    С: (B,D), (A,C)

// 3) B:  (A, C), (B, D) 
//    D:  (B, D), (A, C)
//    {{A, C}, {B}, {D}}
//    {{A, C}, {B, D, E, F}}


void print_arr(auto& vec){
    std::cout << "{";
    for (auto&& v: vec){
        std::cout << "{";
        for (auto el: v){
            std::cout << el << ',';
        }
        std::cout << "}";
    }
    std::cout << "}\n";
}

void print_sarr(auto& vec){
    std::cout << "{";
    for (auto v: vec){
        std::cout << v << ' ';
    }
    std::cout << "} ";
}


// TODO: finish
template <size_t states_count, size_t inputs_count>
int need_split(const size_t (&transition)[states_count][inputs_count], const auto& vec_el, const auto& res_vec) {
    int res = -1;
    std::vector<std::vector<size_t>> rv; // {{res_indexes...}}
    for (auto& el: vec_el){
        std::vector<size_t> steps;
        for (int i = 0; i < inputs_count; ++i){
            for (int j = 0; j < res_vec.size(); ++j){
                if (std::any_of(std::begin(res_vec[j]), std::end(res_vec[j]), [x = transition[el][i]](auto el){return el == x;}))
                    steps.push_back(j);
            }
        }
        rv.push_back(steps);
    }
    std::cout << "RV:\n";
    for (auto&& x: rv){
        for (int i = 0; i < x.size(); ++i){
            print_sarr(res_vec[x[i]]);
        }
        std::cout << '\n';
    }
    auto& r = rv[0]; // todo: finish
    int i = 0;
    std::vector<size_t> eq_states{};
    for (auto&& s: rv){
        if (std::equal(std::begin(s), std::end(s), std::begin(r))) eq_states.push_back(i);
        ++i;
    }
    if (eq_states.size() == vec_el.size()) return -1;
    
    for (int i = 0, e = vec_el.size(); i != e; ++i){
        if (!std::any_of(std::begin(eq_states), std::end(eq_states), [i](auto el){ return el == i;})) return i;
    }
    return res;
}


template <size_t states_count, size_t inputs_count>
auto moore_algo(const size_t (&transition)[states_count][inputs_count], const auto& finals_states){
    std::vector<std::vector<size_t>> result;
    std::vector<size_t> tmp;
    std::vector<size_t> other;
    other.reserve(states_count);

    tmp.resize(std::size(finals_states));
    std::copy(std::begin(finals_states), std::end(finals_states), std::begin(tmp));
    result.push_back(tmp);

    for (size_t i = 0; i < states_count; ++i){
        if (!std::any_of(std::begin(finals_states), std::end(finals_states), [i](auto el){return el == i;}))
            other.push_back(i);
    }
    result.push_back(other);

    std::cout << "Result:\n";
    print_arr(result);

    while(true) {
        bool nsplit = false;
        int split_index = -1;
        int vec_index = -1;
        for (int i = 0; i < result.size(); ++i){
            auto x = need_split<states_count, inputs_count>(transition, result[i], result);
            if (x != -1){
                nsplit = true;
                split_index = x;
                vec_index = i;
                std::cout << "SPLIT:" << x << "\n\n";
                break;
            } else {
                std::cout << "EQUAL" << x << "\n\n";
            }
        }
        if (nsplit){
            std::vector<size_t> n = {result[vec_index][split_index]};
            result[vec_index].erase(std::remove(result[vec_index].begin(), result[vec_index].end(), result[vec_index][split_index]),result[vec_index].end());
            result.push_back(n);
        } else {
            break;
        }    
    }
    return result;
}


// A->0
// B->1
// C->2
// D->3
int main(){
    // A: 0 → B, 1 → C
    // B: 0 → A, 1 → D
    // C: 0 → D, 1 → A
    // D: 0 → D, 1 → D

    constexpr size_t states1[4][2] = {
        {1,2},
        {0,3},
        {3,0},
        {3,3},
    };
    constexpr size_t final1[2] = {0, 2};

    // A: 0 → B, 1 → C
    // B: 0 → A, 1 → D
    // C: 0 → D, 1 → A
    // D: 0 → C, 1 → B

    constexpr size_t states2[4][2] = {
        {1,2},
        {0,3},
        {3,0},
        {2,1},
    };
    constexpr size_t final2[2] = {0, 1};


    // F -> A, C
    // N = {B, D, E, F}
    // A: B, C
    // B: D, A
    // C: A, D
    // D: C, B
    // E: B, F
    // F: A, E

    // A->0
    // B->1
    // C->2
    // D->3
    // E->4
    // F->5
    constexpr size_t states3[6][2] = {
        {1,2},
        {0,3},
        {0,3},
        {2,1},
        {1,5},
        {0,4},
    };
    constexpr size_t final3[2] = {0, 1};


    // A: 0 → B, 1 → C
    // B: 0 → A, 1 → D
    // C: 0 → D, 1 → A
    // D: 0 → D, 1 → D
    // E: 0 → D, 1 → D
    constexpr size_t states4[5][2] = {
        {1,2},
        {0,3},
        {3,0},
        {3,3},
        {3,3},
    };
    constexpr size_t final4[2] = {0, 2};


    auto r = moore_algo<5,2>(states4, final4);
    std::cout << "Result:\n";
    print_arr(r);
}