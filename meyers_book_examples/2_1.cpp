#include <iostream>

template <typename It>
void print(It b, It e) {
    while (b!=e)
    {
      
      typename std::iterator_traits<It>::value_type val = *b;
      // auto val = *b;
      std::cout << "el:" << val << std::endl;
      ++b;
    }
    
}

int main() {
  std::vector<int> vec = {1,2,3,4,5,6};
  print(vec.begin(), vec.end());
}