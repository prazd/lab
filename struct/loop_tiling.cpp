#include <iostream>
#include <thread>
#include <functional>
#include <utility>

// g++ -std=c++20 -O3 -ffast-math -mavx2 -mfma loop_tiling.cpp

constexpr int SIZE = 1024;

std::array<std::array<float, SIZE>, SIZE> m1;
std::array<std::array<float, SIZE>, SIZE> m2;

alignas(64) std::array<std::array<float, SIZE>, SIZE> mres1;
alignas(64) std::array<std::array<float, SIZE>, SIZE> mres2;
alignas(64) std::array<std::array<float, SIZE>, SIZE> mres3;

void init_ms() {
    for (auto i = 0; i < SIZE; ++i){
        for (auto j = 0; j < SIZE; ++j) {
            m1[i][j] = float(rand() % 1000);
            m2[i][j] = float(rand() % 1000);
        }
    }
    std::fill(mres1.begin(), mres1.end(), std::array<float, SIZE>{});
    std::fill(mres2.begin(), mres2.end(), std::array<float, SIZE>{});
    std::fill(mres3.begin(), mres2.end(), std::array<float, SIZE>{});
}

template <typename Arr>
void matmul_naive(const Arr& a, const Arr& b, Arr& result) {
    for (auto i = 0; i < SIZE; ++i) {
        for (auto j = 0; j < SIZE; ++j) {
            for (int k = 0; k < SIZE; ++k) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

// Loop Tiling (Cache Blocking)
template <typename Arr, int BLOCK = 64>
void matmul_blocked(const Arr&  a, const Arr&  b, Arr&  result) {
    for (int bj = 0; bj < SIZE; bj += BLOCK) {  
        for (int bk = 0; bk < SIZE; bk += BLOCK) {  
            for (int bi = 0; bi < SIZE; bi += BLOCK) {  
                for (int j = bj; j < bj + BLOCK; ++j) {  
                    for (int k = bk; k < bk + BLOCK; k += 4) {  
                        for (int i = bi; i < bi + BLOCK; ++i) {  
                            result[i][j] += a[i][k] * b[k][j] + 
                                            a[i][k+1] * b[k+1][j] +
                                            a[i][k+2] * b[k+2][j] +
                                            a[i][k+3] * b[k+3][j];  
                        }  
                    }  
                }  
            }  
        }  
    }
}

// SIMD + Loop Tiling (Cache Blocking)
template <typename Arr, int BLOCK = 256>
void matmul_blocked_simd(const Arr& __restrict__ a, const Arr& __restrict__ b, Arr& __restrict__ result) {
    #pragma omp parallel for collapse(2) schedule(dynamic) // OpenMP 
    for (int bj = 0; bj < SIZE; bj += BLOCK) {  
        for (int bk = 0; bk < SIZE; bk += BLOCK) {  
            for (int bi = 0; bi < SIZE; bi += BLOCK) {  
                for (int i = bi; i < bi + BLOCK; ++i) {  
                    for (int k = bk; k < bk + BLOCK; k += 4) {  
                        #pragma omp simd // (AVX/SSE)
                        for (int j = bj; j < bj + BLOCK; ++j) {  
                            result[i][j] += a[i][k] * b[k][j] + 
                                            a[i][k+1] * b[k+1][j] +
                                            a[i][k+2] * b[k+2][j] +
                                            a[i][k+3] * b[k+3][j];  
                        }  
                    }  
                }  
            }  
        }  
    }
}


template <typename Func, typename... Args>
auto elapsed_call(Func&& func, Args&&... args) {
    auto start = std::chrono::steady_clock::now();
    func(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int main() {
    std::cout << "Loop Tiling (Cache Blocking)" << '\n';

    init_ms();

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            auto res_naive = elapsed_call([&]{matmul_naive(m1, m2, mres1);});
            std::cout << "matmul_naive: " << SIZE << " res: " << res_naive << "ms\n";    
        }
        #pragma omp section
        {
            auto res_blocked = elapsed_call([&]{matmul_blocked(m1, m2, mres2);});
            std::cout << "matmul_blocked: " << SIZE << " res: " << res_blocked << "ms\n";    
        }
        #pragma omp section
        {
            auto res_simd = elapsed_call([&]{matmul_blocked_simd(m1, m2, mres3);});
            std::cout << "matmul_blocked_simd: " << SIZE << " res: " << res_simd << "ms\n";
        }
    }

}
