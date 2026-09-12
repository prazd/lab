#include <cerrno>
#include <iostream>
#include <cstring>

template <typename T, size_t n>
class PowerMap {
    T arr[n]{};
public:    
    PowerMap(auto& arr_v){
        for (size_t i = 1, j = 0; i != n; i <<= 1, ++j)
            arr[i] = arr_v[j];
    }
    T& get(size_t k){return arr[k];}
};

struct TLS {
    enum TSDOffset {
        THREAD_SELF=0x0,
        ERRNO=0x8,
        QoS=0x20,
    };

    pthread_t read_pthread() {
        unsigned long base = 0;
        asm volatile("movq %%gs:%p1, %0" : "=r"(base) :"i"(THREAD_SELF));
        return (pthread_t)base;
    }

    int read_errno(){
        unsigned long base = 0;
        asm volatile("movq %%gs:%p1, %0" : "=r"(base) :"i"(ERRNO));
        return *reinterpret_cast<int*>(base);
    }


    enum QoSClass {
        QOS_CLASS_MAINTENANCE=0x1,
        QOS_CLASS_BACKGROUND=0x2,
        QOS_CLASS_UTILITY=0x4,
        QOS_CLASS_DEFAULT=0x8,
        QOS_CLASS_USER_INITIATED=0x10,
        QOS_CLASS_USER_INTERACTIVE=0x20,
    };
    static constexpr std::string_view qos_str[] = {
        "QOS_CLASS_MAINTENANCE",
        "QOS_CLASS_BACKGROUND",
        "QOS_CLASS_UTILITY",
        "QOS_CLASS_DEFAULT",
        "QOS_CLASS_USER_INITIATED",
        "QOS_CLASS_USER_INTERACTIVE",
    };
    PowerMap<std::string_view, 64> qos_pm = PowerMap<std::string_view, 64>(qos_str);

    std::string_view qos_class_str(unsigned long v){
        return qos_pm.get((v >> 8) & 0x3f);
    }
    unsigned long read_qos(){
        unsigned long res = 0;
        asm volatile("movq %%gs:%p1, %0" : "=r"(res) :"i"(QoS));
        return res;
    }
};


int main(){
    TLS tls;
    std::cout << "errno:" <<  tls.read_errno() << '\n';
    errno = 1;
    std::cout << "errno:" <<  tls.read_errno() << "\n\n";

    auto pt = tls.read_pthread();
    std::cout << "PT sig:" << std::hex << *((unsigned long*)pt) << '\n';
    auto qos = tls.read_qos();
    std::cout << "QoS: " << std::hex << tls.qos_class_str(qos) << " prio:" << (qos & 0xff) << '\n';
}