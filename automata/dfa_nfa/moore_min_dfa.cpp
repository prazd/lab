#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>
#include <array>
#include <cassert>

#define LOGS 1
#define COMPTIME 0

#if COMPTIME
#define RES_SPEC constexpr
#define TEST_SPEC consteval
#undef LOGS
#define LOGS 0
#else
#define RES_SPEC
#define TEST_SPEC
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

void print_arr(auto& vec){
    std::cout << "{";
    for (auto&& v: vec){
        if (arr_empty(v)) continue;
        std::cout << "{";
        for (auto it = std::begin(v), ite = std::end(v); it != ite; ++it){
            if (it->has_value()){
                std::cout << it->value();
                if (std::next(it) != ite && std::next(it)->has_value())
                    std::cout << ' ';
            }
        }
        std::cout << "}";
    }
    std::cout << "}\n";
}


void print_sarr(auto& vec){
    std::cout << "{";
    for (auto it = std::begin(vec), ite = std::end(vec); it != ite; ++it){
        if (it->has_value()){
            std::cout << it->value();
            if (std::next(it) != ite && std::next(it)->has_value())
                std::cout << ' ';
        }
    }
    std::cout << "} ";
}


constexpr int get_unique_index(const auto& matrix){
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


// todo: make O(n^2)
template <size_t states_count, size_t inputs_count>
constexpr int need_split(const size_t (&transition)[states_count][inputs_count], const auto& vec_el, const auto& res_vec) {
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
                print_sarr(res_vec[x[i].value()]);
            }
        }
        if (!arr_empty(x)) std::cout << '\n';
    }
#endif
    return get_unique_index(rv);
}


template <size_t states_count, size_t inputs_count>
constexpr auto moore_algo(const size_t (&transition)[states_count][inputs_count], const auto& finals_states){
    // todo: use array + size instead of optional
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
        print_arr(result);
#endif

        bool nsplit = false;
        int split_index = -1;
        int vec_index = -1;
        for (int i = 0, e = std::size(result); i != e; ++i){
            if (arr_empty(result[i]))
                continue;
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
                std::cout << "EQUAL\n\n";
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
            break;
        }
    }
    return result;
}


template <size_t states_count, size_t inputs_count, size_t final_count> 
struct TestCase {
    const size_t states[states_count][inputs_count];
    const size_t final[final_count];
    const std::array<std::array<std::optional<size_t>, states_count>, states_count> expected;
};

template <size_t states_count, size_t inputs_count, size_t final_count, size_t tests_count> 
constexpr void run_tests(TestCase<states_count,inputs_count,final_count>(&tests)[tests_count]){
#if LOGS
    size_t t_count = 0;
#endif    
    for (auto& t: tests){
#if LOGS
        std::cout << "CASE " << t_count++ << ":\n";
#endif        
        auto r = moore_algo<states_count,inputs_count>(t.states, t.final);
        assert(std::equal(std::begin(r), std::end(r), std::begin(t.expected)));        
#if LOGS
        std::cout << "Final res:\n";
        print_arr(r);
        std::cout << "\n\n\n";
#endif
    }
}

TEST_SPEC void tests(){
#if LOGS
    std::cout << "----------------------------------\n";
    std::cout << "Tests 6 states 2 inputs:\n";
#endif
// A->0
// B->1
// C->2
// D->3
// E->4
// F->5
    using arr_so6 = std::array<std::optional<size_t>, 6>;
    TestCase<6,2,2> tests_6s_2i_2f[] = {
        {
// A: B, C
// B: D, A
// C: A, D
// D: C, B
// E: B, F
// F: A, E
            {{1,2},{0,3},{0,3},{2,1},{1,5},{0,4}},
            {0,1},
            {arr_so6{1},arr_so6{5},arr_so6{3},arr_so6{0},arr_so6{2},arr_so6{4}},
        },
    };
    run_tests(tests_6s_2i_2f);
#if LOGS
    std::cout << "----------------------------------\n\n\n";
#endif

#if LOGS
    std::cout << "----------------------------------\n";
    std::cout << "Tests 5 states 2 inputs:\n";
#endif

// A->0
// B->1
// C->2
// D->3
// E->4
    using arr_so5 = std::array<std::optional<size_t>, 5>;
    TestCase<5,2,2> tests_5s_2i_2f[] = {
{
// A: B, C
// B: A, D
// C: D, A
// D: D, D
// E: D, D
            {{1,2},{0,3},{3,0},{3,3},{3,3}},
            {0,2},
            {arr_so5{2},arr_so5{3,4},arr_so5{1},arr_so5{0}},
        }
    };
    run_tests(tests_5s_2i_2f);
#if LOGS
    std::cout << "----------------------------------\n\n\n";
#endif

#if LOGS
    std::cout << "----------------------------------\n";
    std::cout << "Tests 4 states 2 inputs:\n";
#endif
// A->0
// B->1
// C->2
// D->3
    using arr_so4 = std::array<std::optional<size_t>, 4>;
    TestCase<4,2,2> tests_4s_2i_2f[] = {
// A: B,C
// B: A,D
// C: D,A
// D: D,D           
        {
            {{1,2},{0,3},{3,0},{3,3}},
            {0,2},
            {arr_so4{2},arr_so4{3},arr_so4{1},arr_so4{0}},
        },
// A: B,C
// B: A,D
// C: D,A
// D: C,B
        {    
            {{1,2},{0,3},{3,0},{2,1}},
            {0,1},
            {arr_so4{0,1}, arr_so4{2,3}}
        },
    };
    run_tests(tests_4s_2i_2f);
#if LOGS
    std::cout << "----------------------------------\n\n\n";
#endif
}

int main(){
    tests();
}