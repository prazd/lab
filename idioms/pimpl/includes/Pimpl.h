#ifndef PIMPL
#define PIMPL
#include <memory>
struct Pimpl {
    Pimpl();
    ~Pimpl();
    Pimpl(Pimpl&&);
    Pimpl& operator=(Pimpl&&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
#endif