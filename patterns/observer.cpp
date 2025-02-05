#include <iostream>

struct IObserver {
    virtual void update(float value) = 0;
    virtual ~IObserver() = default;
};

class Sub1: public IObserver {
    float value;
public:
    Sub1(): value{0} {}
    Sub1(float value): value(value) {}
    void update(float v) override {
        value = v;
        std::cout << "Update value:" << value << "\n";
    }
};

class MainObject {
    std::vector<std::weak_ptr<IObserver>> observers;
    float value;
public:
    MainObject(): value(0) {}
    MainObject(float value): value(value) {}
    void add(const std::shared_ptr<IObserver>& observer) {
        observers.push_back(observer);
    }
    void notify_all() {
        for (auto it = observers.begin(), ite = observers.end(); it != ite; ++it) {
            if (auto observer = it->lock()) {
                observer->update(value);
            }
        }
    }
    void update_value(float v) {
        value = v;
        notify_all();
    }
};


int main() {
    MainObject obj;
    {
        auto sub1 = std::make_shared<Sub1>();
        obj.add(sub1);
        obj.update_value(10.f);
        obj.update_value(20.f);
        obj.update_value(30.f);
    }
    obj.update_value(30.f);
}