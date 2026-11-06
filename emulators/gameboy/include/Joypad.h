#ifndef RAM_H_JOYPAD_H
#define RAM_H_JOYPAD_H
#include "Memory.h"
#include "raylib.h"

#if CONTROLLER
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>

namespace NintendoJoystick {
    enum Button {
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

    struct KeyPressed {
        unsigned short state{};
        void set(Button b, bool val) {
            state &= ~(1 << b);
            state |= (val << b);
        }

        bool get(Button b) const {
            return (state >> static_cast<int>(b)) & 1;
        }
    };

    class HIDDevice {
        static constexpr int vendor_id = 8406;
        static constexpr int product_id = 42784;
        static constexpr int page = kHIDPage_GenericDesktop;
        static constexpr int usage_game_pad = kHIDUsage_GD_GamePad;
        static inline __IOHIDManager* manager = nullptr;
        static inline unsigned long prev_report_buffer{};
        static inline unsigned long report_buffer{};
        static inline KeyPressed kp;

        static void update_buttons(){
            if (prev_report_buffer == report_buffer) return;
            auto rb0 = report_buffer & 0xff;
            auto rb1 = report_buffer & 0xff00;
            auto rb2 = report_buffer & 0xff0000;
            auto rb3 = report_buffer & 0xff000000;
            auto rb4 = report_buffer & 0xff00000000;

            kp.set(L1, (rb0 == 0x10));
            kp.set(R1, (rb0 == 0x20));
            kp.set(L2, (rb0 == 0x40));
            kp.set(R2, (rb0 == 0x80));

            kp.set(DPadUp,    (rb2 == 0x0) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 && rb4 <= 0x4000000000));
            kp.set(DPadRight, (rb2 == 0x020000) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 >= 0xc0000000 && rb4 <= 0xf000000000));
            kp.set(DPadDown,  (rb2 == 0x040000)|| (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 && rb4 >= 0xc000000000));
            kp.set(DPadLeft,  (rb2 == 0x060000) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 <= 0x40000000 && rb4 <= 0xf000000000));

            kp.set(A, (rb0 & 0x2));
            kp.set(B, (rb0 & 0x4));
            kp.set(Select, (rb2 == 0x8f0000));
            kp.set(Start, (rb1 == 0x1000));

            prev_report_buffer = report_buffer;
        }

        static void device_matching_callback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
            if (result != kIOReturnSuccess) return;
            auto product = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
            auto open_res = IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
            if (open_res != kIOReturnSuccess) {
                std::abort();
            }
            IOHIDDeviceRegisterInputReportCallback(device, (unsigned char*)&report_buffer, sizeof(report_buffer), input_report_callback, nullptr);
            IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        }

        static void input_report_callback(void* context, IOReturn result, void* sender, IOHIDReportType type,
                                          unsigned reportID, unsigned char* report, CFIndex reportLength) {
            if (result != kIOReturnSuccess || reportLength == 0) return;
            update_buttons();
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

        static bool is_pressed(Button b) {
            return kp.get(b);
        }

        static bool is_key_released(Button b) {
            return !kp.get(b);
        }
    };
}
#endif

struct Joypad {
    enum JoypadKeys {
        A_Right, B_Left, Select_Up, Start_Down,
        NotPressed
    } button_state = NotPressed;

    enum Keys {
        Right, Left, Up,Down,
        A, B, Select, Start
    };

    KeyboardKey keyboard_keys[8]{
            KEY_RIGHT,     // RIGHT
            KEY_LEFT,     // LEFT
            KEY_UP,     // UP
            KEY_DOWN,     // DOWN

            KEY_Q,     // B
            KEY_E,     // A
            KEY_ENTER, // Start
            KEY_SPACE, // Select
    };



#if CONTROLLER
    NintendoJoystick::HIDDevice device;
    NintendoJoystick::Button gamepad_keys[8]{
            NintendoJoystick::DPadRight,
            NintendoJoystick::DPadLeft,
            NintendoJoystick::DPadUp,
            NintendoJoystick::DPadDown,

            NintendoJoystick::A,
            NintendoJoystick::B,
            NintendoJoystick::Select,
            NintendoJoystick::Start,
    };
#endif

    JoypadKeys key_shifts[8]{
            A_Right,
            B_Left,
            Select_Up,
            Start_Down,

            B_Left,
            A_Right,
            Start_Down,
            Select_Up,
    };

    void update_state(Memory&);
    std::pair<unsigned char, bool> current_pressed_values(bool);
};
#endif //RAM_H_JOYPAD_H
