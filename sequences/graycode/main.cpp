#include <iostream>

std::vector<int> bitseq(int n) {
    auto count = std::pow(2,n);
    std::vector<int> res;
    res.reserve(count);
    for(int i = 0; i < count; ++i){
        res.push_back(i);
    }
    return res;
}

template <typename T>
void print(const T& val) {
    for(auto& el: val)
        std::cout << el << " ";
    std::cout << "\n";
}

template <typename T, int n>
void print_bits(const T& val) {
    for(auto& el: val) {
        std::cout << std::bitset<n>(el).to_string() << " ";
    }
    std::cout << "\n";
}

int hamming_weight(int n) {
    int res{};
    while(n) {
        n &= n - 1;
        ++res;
    }
    return res;
}

bool onebitdiff(int x, int y) {
    if (hamming_weight(x ^ y) == 1) {
        return true;
    }
    if (hamming_weight(x) == 1 && hamming_weight(y) == 1) {
        return true;
    }
    return false;
}


int lastbitpos(int n) {
    int res = 0;
    for (int i = 0; i < CHAR_BIT * sizeof(n); ++i) {
        if (n & (1 << i)) {
            res = i;
        }
    }
    return res;
}

int firstbitpos(int n) {
    int res = 0;
    for (int i = 0; i < CHAR_BIT * sizeof(n); ++i) {
        if (n & (1 << i)) {
            return i;
        }
    }
    return res;
}
// auto btostr = [](auto& el) {
//     return std::bitset<4>(el).to_string();
// };

auto find_pos(auto el, std::vector<int>& res) {
    for (auto it = res.begin()+1, ite = res.end(); it != ite; ++it) {
        // auto diff1 = std::abs(lastbitpos(*el) - lastbitpos(*it));
        // auto diff2 = std::abs(lastbitpos(*el) - lastbitpos(*(std::prev(it))));
        // auto diff3 = std::abs(firstbitpos(*el) - firstbitpos(*it));
        // auto diff4 = std::abs(firstbitpos(*el) - firstbitpos(*(std::prev(it))));
        // auto d = std::abs(hamming_weight(*it) - hamming_weight(*el));
        // auto dprev = std::abs(hamming_weight(*std::prev(it)) - hamming_weight(*el));
        // (d == 1 || d == 0) && (dprev == 1 || dprev == 0) &&
//        if ((d == 1 || d ==0) && (dprev == 1 || dprev == 0) && (diff1 == 0 || diff1 == 1) && (diff2 == 0 || diff2 == 1)){
        // if ((hamming_weight(*el ^ *it) == 1 && hamming_weight(*el ^ *std::prev(it)) == 1)) {
        //     std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
        //     return it;
        // }
        // if ((hamming_weight(*el ^ *it) == 1) && (hamming_weight(*el) == hamming_weight(*std::prev(it)) && (diff2 == 1 || diff2 == 0) && (diff4 == 0 || diff4 == 1))) {
        //     std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
        //     return it;
        // }

        // if ((hamming_weight(*el ^ *std::prev(it)) == 1) &&(hamming_weight(*el) == hamming_weight(*it) && (diff3 == 1 || diff3 == 0) && (diff1 == 0 || diff1 == 1))) {
        //     std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
        //     return it;
        // }
        if (onebitdiff(*el, *it) && onebitdiff(*el, *std::prev(it))) {
            return it;
        }
    }
    return res.end();
}

std::vector<int> graycode2(int n) {
    std::vector<int> res{0,1};
    if (n == 1) {
        return res;
    }
    auto fullseq = bitseq(n);
    fullseq.erase(fullseq.begin(), fullseq.begin()+2);
    while(!fullseq.empty()) {
        for(auto it = fullseq.begin(), ite = fullseq.end(); it != ite; ++it) {
            auto pos = find_pos(it, res);
            if (pos == res.end()) {
                continue;
            }
            res.insert(pos, *it);
            fullseq.erase(it);
            std::cout << "Res:" << "\n";
            print_bits<decltype(res), 3>(res);
            std::cout << "Res:" << "\n";
            print_bits<decltype(res), 3>(fullseq);
            break;
        }
    }
    return res;
}


auto find_pos2(auto el, std::vector<int>& res) {
    for (auto it = res.begin()+1, ite = res.end(); it != ite; ++it) {
        auto [first, second] = *el;
        if ((onebitdiff(first, *std::prev(it)) && onebitdiff(second, *it))) {
            return std::tuple{it, std::pair<int, int>{first, second}};
        }
        if ((onebitdiff(second, *std::prev(it)) && onebitdiff(first, *it))) {
            return std::tuple{it, std::pair<int, int>{second, first}};
        }
        if (std::next(it) == ite && onebitdiff(first, *it)) {
            return std::tuple{it, std::pair<int, int>{first, second}};
        }
        if (std::next(it) == ite && onebitdiff(second, *it)){
            return std::tuple{it, std::pair<int, int>{second, first}};
        }
    }
    return std::tuple{res.end(), std::pair<int, int>{}};
}


std::vector<int> graycode3(int n) {
    std::vector<int> res{0,1};
    if (n == 1) {
        return res;
    }
    auto fullseq = bitseq(n);
    fullseq.erase(fullseq.begin(), fullseq.begin()+2);
    std::vector<std::pair<int, int>> pairs;

    // generate bits pairs
    while(!fullseq.empty()) {
        auto first = fullseq.begin();
        for(auto it = fullseq.begin() + 1, ite = fullseq.end(); it != ite; ++it) {
            if (hamming_weight(*it ^ *first) == 1) {
                pairs.emplace_back(*it, *first);
                fullseq.erase(it);
                fullseq.erase(first);
                break;
            }
        }
    }

    while(!pairs.empty()) {
        for(auto it = pairs.begin(), ite = pairs.end(); it != ite; ++it) {
            auto pos = find_pos2(it, res);
            if (std::get<0>(pos) == res.end()) {
                continue;
            }

            res.insert(std::get<0>(pos), {std::get<1>(pos).first, std::get<1>(pos).second});
            pairs.erase(it);
            break;
        }
    }
    return res;
}


auto find_pos3(auto el, std::vector<int>& res) {
    for (auto it = res.begin(), ite = res.end(); it != ite; ++it) {
    if (onebitdiff(*el, *it) && *el % 2 == 0) {
            return it;
        }
    }
    return res.end();
}

auto find_pos4(auto el, std::vector<int>& res) {
    for (auto it = res.begin(), ite = res.end(); it != ite; ++it) {
    if (onebitdiff(*el, *it) && *el % 2 == 0) {
            return it;
        }
    }
    return res.end();
}


std::vector<int> graycode4(int n) {
    std::vector<int> res{0,1};
    if (n == 1) {
        return res;
    }
    auto fullseq = bitseq(n);
    std::vector<int> d1;
    d1.reserve(fullseq.size()/2);
    std::vector<int> d2;
    d1.reserve(fullseq.size()/2);
    std::for_each(fullseq.begin(), fullseq.end(), [&](auto el){el%2==0 ? d2.push_back(el) : d1.push_back(el);});

    // 0000 0010 0110 0100 1100 1000 1010 1110 1111 1011 1001 1101 0101 0111 0011 0001
    // 0 2 6 4 12 8 10 14 15 11 9 13 5 7 3 1

    // 1 3 7 5 13 9 11 15

    // 1 3 5 7 9 11 13 15

    print(d1);
    print_bits<std::vector<int>, 4>(d1);
    print(d2);
    print_bits<std::vector<int>, 4>(d2);

    for (auto it = d1.begin()+1 ,ite = d1.end(); it != ite; ++it) {
        if (onebitdiff(*it, *std::prev(it))) {
            continue;
        }
        for (auto it2 = d1.begin()+1, ite2 = d1.end(); it2 != ite2; ++it2) {
            if (*it == *it2) {
                continue;
            }
            if (onebitdiff(*it2, *std::prev(it))) {
                std::swap(*it, *it2);
                print(d1);
                print_bits<std::vector<int>, 4>(d1);
                it = d1.begin();
                break;
            }
        }
    }

    std::cout << "After:" << "\n";
    print(d1);
    print_bits<std::vector<int>, 4>(d1);
    return res;
}

bool is_pos(auto el, std::vector<int>& res, int count) {
    auto prev = res.begin() + count - 1;
    auto next = res.begin() + count;
    if (onebitdiff(el->first, *prev) && (onebitdiff(el->second, *next))) {
        return true;
    }
    return false;
}

int is_pos(auto el, std::vector<int>& res) {
    int count = 1;
    for (auto it = res.begin(), ite = res.end(); it != ite; ++it) {
        auto prev = res.begin() + count;
        auto next = res.end() - count;
        if (onebitdiff(el->first, *prev) && (onebitdiff(el->second, *next))) {
            return count;
        }
        ++count;
    }
   return -1;
}

std::vector<int> graycode5(int n) {
    std::vector<int> res{0,1};
    if (n == 1) {
        return res;
    }
    auto fullseq = bitseq(n);
    fullseq.erase(fullseq.begin(), fullseq.begin()+2);
    std::vector<std::pair<int, int>> pairs;

    // 0000 0010 0110 0100 1100 1000 1010 1110 1111 1011 1001 1101 0101 0111 0011 0001
    // 0 2 6 4 12 8 10 14 15 11 9 13 5 7 3 1
    // 0 2 6 4 12 8 10 14 15 11 9 13 5 7 3 1

    // generate bits pairs
    while(!fullseq.empty()) {
        auto first = fullseq.begin();
        for(auto it = fullseq.begin() + 1, ite = fullseq.end(); it != ite; ++it) {
            if (hamming_weight(*it ^ *first) == 1) {
                auto max = std::max(*first, *it);
                auto min = std::min(*first, *it);
                pairs.emplace_back(min, max);
                fullseq.erase(it);
                fullseq.erase(first);
                break;
            }
        }
    }
    int count = 1;

    while(!pairs.empty()) {
        for(auto it = pairs.begin(), ite = pairs.end(); it != ite; ++it) {
            if (is_pos(it, res, count)) {
                res.insert(res.begin() + count, {it->first, it->second});
                pairs.erase(it);
                ++count;
                break;
            }

            auto pos = is_pos(it, res);
            if (pos == -1)
                continue;
            res.insert(res.begin() + pos, it->first);
            res.insert(res.end() - pos, it->second);
            pairs.erase(it);
            ++count;
            break;
        }
    }
    return res;
}

// 0111 1101 xor = 1010


// 000 100 010

// res: 000 001 011 010 110 111 101 100

// start
// input: 010 011 100 101 110 111
// res: 000 001

// step1:
// input: -010- 011 100 101 110 111
// res: 000 -010- 001

// step2:
// input: -011- 100 101 110 111
// res: 000 010 -011- 001

// step3
// input: -100- 101 110 111
// res: 000 -100- 010 011 001

// input: 101 -110- 111
// res: 000 100 110 010 011 001

// input: -101- 111
// res: 000 100 101 110 010 011 001

// // input: -111-
// res: 000 100 101 111 110 010 011 001

// 0 4 5 7 6 2 3 1


// 010 100 011

//It:001El:010PrevIt:000
// 000 010 001
//It:001El:011PrevIt:010
// 000 010 011 001
//It:011El:100PrevIt:010
// if (bitcounts(x) > bitcounts(y) {lastbitpos}
// 100 011
//
// 000 010 100 011 001
//It:100El:110PrevIt:010
// 000 010 110 100 011 001

// 1 case - 000 -010- 001  (hw(x ^ y) == 1 && (bitcountx == bitcounty && lastbitpos == lastbity)
//

int main(){
    print(graycode5(7));
    print_bits<std::vector<int>, 7>(graycode5(7));

}