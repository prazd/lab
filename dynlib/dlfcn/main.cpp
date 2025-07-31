#include <iostream>
#include <dlfcn.h>

using printFunc = void(*)();

int main() {
    void* handle = dlopen("libprint.dylib", RTLD_LAZY);
    if (!handle){
        std::cout << "dlopen failure:" << dlerror();
        return 1;
    }
    auto pf = reinterpret_cast<printFunc>(dlsym(handle, "print"));
    pf();
    dlclose(handle);
}
