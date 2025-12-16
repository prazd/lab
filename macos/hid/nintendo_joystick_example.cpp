#include <iostream>
#include <bitset>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>
#include <thread>

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
        static inline PressedBuffer pressed = PressedBuffer(8);

        static void update_buttons_and_run_callback(){
            pressed.clear();
            if (report_buffer[2] == 0x0)  pressed.push_back(DPadUp);
            if (report_buffer[2] == 0x2)  pressed.push_back(DPadRight);
            if (report_buffer[2] == 0x4)  pressed.push_back(DPadDown);
            if (report_buffer[2] == 0x6)  pressed.push_back(DPadLeft);

            if (report_buffer[0] == 0x2)  pressed.push_back(B);
            if (report_buffer[0] == 0x4)  pressed.push_back(A);
            if (report_buffer[2] == 0x8f) pressed.push_back(Select);
            if (report_buffer[1] == 0x10) pressed.push_back(Start);
            callback(pressed);
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
                    default:       {std::cout << "Unknown "; break;}
                }
            }
            if (!pressed.empty()) std::cout << "\n";
    };
    HIDDevice<decltype(callback)>::init();
    for(;;){
        HIDDevice<decltype(callback)>::step(0.01);
    }
    HIDDevice<decltype(callback)>::release();
}

int main() {
    example();
    return 0;
}