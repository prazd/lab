#include <iostream>
#include <thread>

constexpr int ELEMS_SZ = 80'000'000;

struct Point {
    float x, y, z;
};

// AoS (Array of Structures)
std::vector<Point> points;

void dummy_rand_points_init(size_t n) {
    points.reserve(n);
    for (int i = 0; i < n; ++i) {
        points.push_back({float(rand() % 100), float(rand() % 100), float(rand() % 100)});
    }
}

auto compute_distance_aos() {
    std::vector<float> result(points.size());
    for (size_t i = 0; i < points.size(); ++i)
        result[i] = std::sqrt(points[i].x * points[i].x + points[i].y * points[i].y + points[i].z * points[i].z);
    return result;
}

struct PointsSoA {
    std::vector<float> x, y, z;
};

PointsSoA ps;

void dummy_rand_points_soa_init(size_t n) {
    ps.x.reserve(n);
    ps.y.reserve(n);
    ps.z.reserve(n);
    for (int i = 0; i < n; ++i) {
        ps.x.push_back(float(rand() % 100));
        ps.y.push_back(float(rand() % 100));
        ps.z.push_back(float(rand() % 100));
    }
}
auto compute_distance_soa() {
    std::vector<float> result(ps.x.size());
    for (auto i = 0; i < ps.x.size(); ++i)
        result[i] = std::sqrt(ps.x[i] * ps.x[i] + ps.y[i] * ps.y[i] + ps.z[i] * ps.z[i]);
    return result;
}


int main() {
    std::cout << "Running SoA and AoS..." << '\n';
    std::mutex mt;
    dummy_rand_points_init(ELEMS_SZ);
    dummy_rand_points_soa_init(ELEMS_SZ);

    auto elapsed_call = [&](auto&& func){
        auto start = std::chrono::steady_clock::now();
        func();
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    std::thread aos([&]{
        auto res = elapsed_call(compute_distance_aos);
        std::lock_guard lg(mt);
        std::cout << "AoS " << ELEMS_SZ << " distance:" << res << "ms" << '\n';
    });

    std::thread soa([&]{
        auto res = elapsed_call(compute_distance_soa);
        std::lock_guard lg(mt);
        std::cout << "SoA " << ELEMS_SZ << " distance:" << res << "ms" << '\n';
    });

    aos.join();
    soa.join();
}