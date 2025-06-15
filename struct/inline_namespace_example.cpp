#include <iostream>

namespace NS {
    namespace V1 {
        struct Data {
            Data() {std::cout << "DataV1\n";}
        };
    }
    inline namespace V2 {
        struct Data {
            Data() {std::cout << "DataV2\n";}
        };
    }
}

int main() {
    NS::Data d;
    NS::V1::Data dv1;
}