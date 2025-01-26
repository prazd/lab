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

// todo: finish
std::vector<int> graycode(int n) {
    std::vector<int> res{0,1};
    if (n == 1) {
        return res;
    }
    auto fullseq = bitseq(n);
    fullseq.erase(std::remove_if(fullseq.begin(), fullseq.end(), [](auto a){return a == 0 || a == 1;}), fullseq.end());
    int last = 1;
    while(!fullseq.empty()) {
        auto& current = *(res.end()-last);
        auto pos = std::find_if(fullseq.begin(), fullseq.end(), [&](auto& el){
            auto diff = std::abs(lastbitpos(el) - lastbitpos(*(res.end()-last)));
            return (hamming_weight(el ^ current) == 1 && (diff == 1 || diff == 0));
        });
        if (pos == fullseq.end()) {
            std::abort();
        }
        res.insert(res.end()-last++, *pos);
        fullseq.erase(pos);

    }

    return res;
}

auto btostr = [](auto& el) {
    return std::bitset<4>(el).to_string();
};

auto find_pos(auto el, std::vector<int>& res) {
    for (auto it = res.begin()+1, ite = res.end(); it != ite; ++it) {
        auto diff1 = std::abs(lastbitpos(*el) - lastbitpos(*it));
        auto diff2 = std::abs(lastbitpos(*el) - lastbitpos(*(std::prev(it))));
        auto diff3 = std::abs(firstbitpos(*el) - firstbitpos(*it));
        auto diff4 = std::abs(firstbitpos(*el) - firstbitpos(*(std::prev(it))));
        auto d = std::abs(hamming_weight(*it) - hamming_weight(*el));
        auto dprev = std::abs(hamming_weight(*std::prev(it)) - hamming_weight(*el));
        // (d == 1 || d == 0) && (dprev == 1 || dprev == 0) &&
//        if ((d == 1 || d ==0) && (dprev == 1 || dprev == 0) && (diff1 == 0 || diff1 == 1) && (diff2 == 0 || diff2 == 1)){
        if ((hamming_weight(*el ^ *it) == 1 && hamming_weight(*el ^ *std::prev(it)) == 1)) {
            std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
            return it;
        }
        if ((hamming_weight(*el ^ *it) == 1) && (hamming_weight(*el) == hamming_weight(*std::prev(it)) && (diff2 == 1 || diff2 == 0) && (diff4 == 0 || diff4 == 1))) {
            std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
            return it;
        }

        if ((hamming_weight(*el ^ *std::prev(it)) == 1) &&(hamming_weight(*el) == hamming_weight(*it) && (diff3 == 1 || diff3 == 0) && (diff1 == 0 || diff1 == 1))) {
            std::cout << "It:" << btostr(*it) << "El:" << btostr(*el) << "PrevIt:" << btostr(*(std::prev(it))) << "\n";
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
            print_bits<decltype(res), 4>(res);
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
    // std::cout << ( << std::endl;
    // print(bitseq(4));
    // auto s = bitseq(1);
    // std::cout << (std::abs(lastbitpos(2) - lastbitpos(3)) == 1) << std::endl;
//    print_bits<std::vector<int>, 4>(graycode2(4));
////    print_bits<std::vector<int>, 3>(std::vector<int>{0,4,6,2,5,7,3,1});
//    print_bits<std::vector<int>, 4>(std::vector<int>{0,1,3,2,6,7,5,4,12,13,15,14,10,11,9,8});
    std::vector<int> v{0,1,3,2,6,7,5,4,12,13,15,14,10,11,9,8};
    bool all = true;
    for (auto it = v.begin(), ite = v.end(); it != ite; ++it) {
        auto next = std::next(it);
        if (next  == ite) {
            next = v.begin();
        }
        if (hamming_weight(*next ^ *it)!= 1) {
            all = false;
            break;
        }
    }

    std::cout << "ALL:" << all << "\n";

    // std::cout << lastbitpos(10) << std::endl;
    // print_bits<decltype(s), 2>(s);
}