#include "Pimpl.h"

struct Pimpl::Impl {
    int x{};
    int y{};
};
Pimpl::Pimpl(): impl(std::make_unique<Pimpl::Impl>()){}
Pimpl::~Pimpl() = default;
Pimpl::Pimpl(Pimpl&&) = default;
Pimpl& Pimpl::operator=(Pimpl&&) = default;

int main() {
    Pimpl p;
}