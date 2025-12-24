#include <iostream>
#include <bitset>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>
#include <iomanip>
#include <thread>
#include <algorithm>

// g++ -framework IOKit -framework CoreFoundation -framework Foundation -std=c++2b
namespace NintendoJoystick {
    enum Buttons {
        DPadRight,
        DPadLeft,
        DPadUp,
        DPadDown,

        A, B, 
        Start, 
        Select,

        L1, R1,
        L2, R2,

        Unknown
    };

    template <typename Callback, typename PressedBuffer = std::vector<Buttons>>
    class HIDDevice {
        static constexpr int vendor_id = 8406;
        static constexpr int product_id = 42784;
        static constexpr int page = kHIDPage_GenericDesktop;
        static constexpr int usage_game_pad = kHIDUsage_GD_GamePad;
        static inline __IOHIDManager* manager = nullptr;
        static inline unsigned char report_buffer[8]{};
        static inline Callback callback;
        static inline PressedBuffer pressed =         PressedBuffer(12);
        static inline PressedBuffer prev_pressed =    PressedBuffer(12);
        static inline PressedBuffer keys_released_event = PressedBuffer(12);

        static void update_buttons_and_run_callback(){
            prev_pressed = pressed;
            pressed.clear();
            if (report_buffer[0] == 0x10)  pressed.push_back(L1);
            if (report_buffer[0] == 0x20)  pressed.push_back(R1);
            if (report_buffer[0] == 0x40)  pressed.push_back(L2);
            if (report_buffer[0] == 0x80)  pressed.push_back(R2);

            if (report_buffer[2] == 0x0)   pressed.push_back(DPadUp);
            if (report_buffer[2] == 0x2)   pressed.push_back(DPadRight);
            if (report_buffer[2] == 0x4)   pressed.push_back(DPadDown);
            if (report_buffer[2] == 0x6)   pressed.push_back(DPadLeft);

            if (report_buffer[0] & 0x2)    pressed.push_back(B);
            if (report_buffer[0] & 0x4)    pressed.push_back(A);
            if (report_buffer[2] == 0x8f)  pressed.push_back(Select);
            if (report_buffer[1] == 0x10)  pressed.push_back(Start);


            if (!(report_buffer[3] == 0x80 && report_buffer[4] == 0x80)){
                if (report_buffer[3] <= 0x40 && report_buffer[4] <= 0xf0)
                    pressed.push_back(DPadLeft);

                if (report_buffer[3] >= 0xc0 && report_buffer[4] <= 0xf0)
                    pressed.push_back(DPadRight);

                if (report_buffer[3] && report_buffer[4] >= 0xc0)
                    pressed.push_back(DPadDown);

                if (report_buffer[3] && report_buffer[4] <= 0x40)
                    pressed.push_back(DPadUp);
            }

            if (prev_pressed.size() > pressed.size()) {
                for(size_t i = 0, e = prev_pressed.size(); i != e; ++i){
                    if (!std::any_of(pressed.begin(), pressed.end(),[&](auto el){ return el == prev_pressed[i];})){
                        keys_released_event.push_back(prev_pressed[i]);
                    }
                }
            }

            //     (4)
            //     00
            // 00      FF (3)
            //     FF
            callback(pressed);

            for (int i = 0; i < 8; ++i)
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned)report_buffer[i] << ' ';
            std::cout << '\n';
        }

        static void device_matching_callback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
            if (result != kIOReturnSuccess) return;
            auto product = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
            auto open_res = IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
            if (open_res != kIOReturnSuccess) {
                std::abort();
                return;
            }
            IOHIDDeviceRegisterInputReportCallback(device, report_buffer, sizeof(report_buffer), input_report_callback, nullptr);
            IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        }

        static void input_report_callback(void* context, IOReturn result, void* sender, IOHIDReportType type,
                                unsigned reportID, unsigned char* report, CFIndex reportLength) {
            if (result != kIOReturnSuccess || reportLength == 0) return;
            update_buttons_and_run_callback();
        }

        static CFArrayRef create_matching_array() {
            CFMutableDictionaryRef match = CFDictionaryCreateMutable(
                    kCFAllocatorDefault,
                    0,
                    &kCFTypeDictionaryKeyCallBacks,
                    &kCFTypeDictionaryValueCallBacks
            );

            auto vid = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendor_id);
            auto pid = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &product_id);

            CFDictionarySetValue(match,  CFSTR("idVendor"), vid);
            CFDictionarySetValue(match,  CFSTR("idProduct"), pid);

            CFDictionarySetValue(match, CFSTR(kIOHIDDeviceUsagePageKey),
                                CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page));
            CFDictionarySetValue(match, CFSTR(kIOHIDDeviceUsageKey),
                                CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_game_pad));

            const void* keys[] = { match };
            CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, keys, 1, &kCFTypeArrayCallBacks);
            CFRelease(match);
            return array;
        }
        
    public:
        static void init() {
            manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
            if (!manager) {
                std::abort();
            }

            auto matching = create_matching_array();
            IOHIDManagerSetDeviceMatchingMultiple(manager, matching);
            CFRelease(matching);

            IOHIDManagerRegisterDeviceMatchingCallback(manager, device_matching_callback, nullptr);
            IOHIDManagerRegisterDeviceRemovalCallback(manager, [](auto...) {std::cout << "Disable controller";}, nullptr);

            auto open_mgr = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
            if (open_mgr != kIOReturnSuccess) {
                std::abort();
            }
            IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        }
        static void run() {
            CFRunLoopRun();
            IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
            CFRelease(manager);
        }

        static void step(double ms) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, ms, false);
        }

        static void release() {
            IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
            CFRelease(manager);
        }
        static bool is_key_released(Buttons b) {
            auto res = std::any_of(keys_released_event.begin(), keys_released_event.end(), [b](auto& el){return el == b;});
            if (!res) return false;
            keys_released_event.erase(std::remove(keys_released_event.begin(), keys_released_event.end(), b), keys_released_event.end());
            return true;
        }
    };
};

void example() {
    using namespace NintendoJoystick;
    auto callback = [](auto& pressed) {
          for (auto el: pressed) {
                switch(el){
                    case DPadDown: {std::cout << "Down ";    break;}
                    case DPadUp:   {std::cout << "Up ";      break;}
                    case DPadLeft: {std::cout << "Left ";    break;}
                    case DPadRight:{std::cout << "Right ";   break;}

                    case A:        {std::cout << "A ";       break;}
                    case B:        {std::cout << "B ";       break;}
                    case Select:   {std::cout << "Select ";  break;}
                    case Start:    {std::cout << "Start ";   break;}

                    case L1:   {std::cout << "L1 ";  break;}
                    case L2:    {std::cout << "L2 ";   break;}
                    case R1:   {std::cout << "R1 ";  break;}
                    case R2:    {std::cout << "R2 ";   break;}
                    default:       {std::cout << "Unknown "; break;}
                }
            }
           
            if (!pressed.empty()) std::cout << "\n";
    };
    HIDDevice<decltype(callback)>::init();
    for(;;){
        HIDDevice<decltype(callback)>::step(0.01);
        if (HIDDevice<decltype(callback)>::is_key_released(L1))
                std::cout << "L1 down\n";
    }
    HIDDevice<decltype(callback)>::release();
}

int main() {
    example();
    return 0;
}