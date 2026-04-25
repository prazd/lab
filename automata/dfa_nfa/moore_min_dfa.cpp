#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>
#include <array>

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

#define LOGS 1
#define COMPTIME 1

#if COMPTIME
#define RES_SPEC constexpr
#undef LOGS
#define LOGS 0
#else
#define RES_SPEC
#endif

constexpr bool arr_empty(const auto& arr){
    return std::count_if(std::begin(arr), std::end(arr), [](auto& el){return !el.has_value();}) == std::size(arr);
}

constexpr size_t arr_sz(const auto& arr){
    return std::count_if(std::begin(arr), std::end(arr), [](auto& el){return el.has_value();});
}

constexpr size_t matrix_sz(const auto& m){
    size_t sz = 0;
    for (auto&& arr: m)
        sz += !arr_empty(arr);
    return sz;
}


void print_arr2(auto& vec){
    std::cout << "{";
    for (auto&& v: vec){
        if (arr_empty(v)) continue;
        std::cout << "{";
        for (auto it = std::begin(v), ite = std::end(v); it != ite; ++it){
            if (it->has_value()){
                std::cout << it->value();
                if (std::next(it) != ite)
                    std::cout << ' ';
            }
        }
        std::cout << "}";
    }
    std::cout << "}\n";
}

void print_arr(auto& vec){
    std::cout << "{";
    for (auto&& v: vec){
        std::cout << "{";
        for (auto it = std::begin(v), ite = std::end(v); it != ite; ++it){
            std::cout << *it;
            if (std::next(it) != ite)
                std::cout << ' ';
        }
        std::cout << "}";
    }
    std::cout << "}\n";
}

void print_sarr(auto& vec){
    std::cout << "{";
    for (auto it = std::begin(vec), ite = std::end(vec); it != ite; ++it){
        std::cout << *it;
        if (std::next(it) != ite)
            std::cout << ' ';
    }
    std::cout << "} ";
}

void print_sarr2(auto& vec){
    std::cout << "{";
    for (auto it = std::begin(vec), ite = std::end(vec); it != ite; ++it){
        if (it->has_value()){
            std::cout << it->value();
            if (std::next(it) != ite)
                std::cout << ' ';
        }
    }
    std::cout << "} ";
}

int get_unique_index(const auto& matrix){
    size_t mismatch_count = 0;
    size_t index = 0;
    size_t equals_count = 0;
    size_t sz = std::size(matrix);
    for (size_t i = 0; i != sz; ++i){
        size_t local_count = 0;
        size_t local_count_eq = 0;
        for (size_t j = 0; j != sz; ++j){
            if (j == i) continue;
            bool val = (!std::equal(std::begin(matrix[i]), std::end(matrix[i]), std::begin(matrix[j])));
            local_count += val;
            local_count_eq += !val;
        }
        equals_count += local_count_eq == sz - 1;
        mismatch_count = (local_count > mismatch_count) ? (index = i, local_count) : mismatch_count;
    }
    return equals_count == sz ? -1 : index;
}

constexpr int get_unique_index2(const auto& matrix){
    size_t mismatch_count = 0;
    size_t index = 0;
    size_t equals_count = 0;
    size_t sz = matrix_sz(matrix);
    if (sz == 1) return -1;
    for (size_t i = 0; i != std::size(matrix); ++i){
        size_t local_count = 0;
        size_t local_count_eq = 0;
        for (size_t j = 0; j != std::size(matrix); ++j){
            if (j == i || arr_empty(matrix[i]) || arr_empty(matrix[j])) continue;
            bool val = (!std::equal(std::begin(matrix[i]), std::end(matrix[i]), std::begin(matrix[j])));
            local_count += val;
            local_count_eq += !val;
        }
        equals_count += local_count_eq == sz - 1;
        mismatch_count = (local_count > mismatch_count) ? (index = i, local_count) : mismatch_count;
    }
    return equals_count == sz ? -1 : index;
}


// TODO: finish
template <size_t states_count, size_t inputs_count>
int need_split(const size_t (&transition)[states_count][inputs_count], const auto& vec_el, const auto& res_vec) {
    int res = -1;
    std::vector<std::vector<size_t>> rv;
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
#if LOGS    
    std::cout << "RV:\n";
    for (auto&& x: rv){
        for (int i = 0; i < x.size(); ++i){
            print_sarr(res_vec[x[i]]);
        }
        std::cout << '\n';
    }
#endif
    return get_unique_index(rv);
}

template <size_t states_count, size_t inputs_count>
constexpr int need_split2(const size_t (&transition)[states_count][inputs_count], const auto& vec_el, const auto& res_vec) {
    int res = -1;
    std::array<std::array<std::optional<size_t>, states_count>, states_count> rv;
    size_t res_i = 0;
    for (auto& el: vec_el){
        if (el.has_value()){
            std::array<std::optional<size_t>, states_count> steps;
            size_t steps_i = 0;
            for (int i = 0; i < inputs_count; ++i) {
                for (int j = 0; j < std::size(res_vec); ++j) {
                    if (arr_empty(res_vec[j])) continue;
                    if (std::any_of(std::begin(res_vec[j]), std::end(res_vec[j]),
                                    [x = transition[el.value()][i]](auto &el) {
                                        return el.has_value() && el.value() == x;
                                    })) {
                        steps[steps_i++] = j;
                    }
                }
            }
            rv[res_i++] = steps;
      }
    }
#if LOGS
    std::cout << "RV:\n";
    for (auto&& x: rv){
        for (int i = 0; i < x.size(); ++i){
            if (x[i].has_value()){
                print_sarr2(res_vec[x[i].value()]);
            }
        }
        if (!arr_empty(x)) std::cout << '\n';
    }
#endif
    return get_unique_index2(rv);
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


    while(true) {
#if LOGS
        std::cout << "Result:\n";
        print_arr(result);
#endif
        bool nsplit = false;
        int split_index = -1;
        int vec_index = -1;
        for (int i = 0, e = result.size(); i != e; ++i){
            auto x = need_split<states_count, inputs_count>(transition, result[i], result);
            if (x != -1){
                nsplit = true;
                split_index = x;
                vec_index = i;
#if LOGS                
                std::cout << "SPLIT:" << x << "\n\n";
#endif                
                break;
            } else {
#if LOGS                
                std::cout << "EQUAL" << "\n\n";
#endif
            }
        }
        if (nsplit){
            std::vector<size_t> n = {result[vec_index][split_index]};
            result[vec_index].erase(std::remove(result[vec_index].begin(), result[vec_index].end(), result[vec_index][split_index]),result[vec_index].end());
            result.push_back(n);
        } else {
#if LOGS            
            std::cout <<"Finish\n";
#endif            
            break;
        }
    }
    return result;
}

template <size_t states_count, size_t inputs_count>
constexpr auto moore_algo2(const size_t (&transition)[states_count][inputs_count], const auto& finals_states){
    std::array<std::array<std::optional<size_t>, states_count>, states_count> result;
    std::array<std::optional<size_t>, states_count> tmp;
    std::array<std::optional<size_t>, states_count> other;
    size_t result_i = 0;

    std::copy(std::begin(finals_states), std::end(finals_states), std::begin(tmp));
    result[result_i++] = tmp;

    for (size_t i = 0, j = 0; i < states_count; ++i){
        if (!std::any_of(std::begin(finals_states), std::end(finals_states), [i](auto el){return i == el;}))
            other[j++] = i;
    }
    result[result_i++] = other;

    while(true) {

#if LOGS
        std::cout << "Result:\n";
        print_arr2(result);
#endif

        bool nsplit = false;
        int split_index = -1;
        int vec_index = -1;
        for (int i = 0, e = std::size(result); i != e; ++i){
            if (arr_empty(result[i]))
                continue;
            auto x = need_split2<states_count, inputs_count>(transition, result[i], result);
            if (x != -1){
                nsplit = true;
                split_index = x;
                vec_index = i;
#if LOGS
                std::cout << "SPLIT:" << x << "\n\n";
#endif                
                break;
            } else {
#if LOGS                
                std::cout << "EQUAL" << x << "\n\n";
#endif
            }
        }
        if (nsplit){
            std::array<std::optional<size_t>, states_count> n;
            n[0] = result[vec_index][split_index];
            result[vec_index][split_index] = std::nullopt;
            std::sort(std::begin(result[vec_index]), std::end(result[vec_index]), [](auto& a, auto& b){ return a.has_value() && !b.has_value();});
            result[result_i++] = n;
        } else {
#if LOGS            
            std::cout <<"Finish\n";
#endif
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
    RES_SPEC auto r = moore_algo2<5,2>(states4, final4);
    auto r2 = moore_algo<5,2>(states4, final4);
    std::cout << "Result:\n";
    print_arr2(r);
    print_arr(r2);
}