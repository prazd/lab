#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H
#include "raylib.h"
#include "Constants.hpp"

#if JOYSTICK
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
        unsigned short down_state{};
        unsigned short pressed_state{};

        void set_down(Button b, bool val) {
            down_state &= ~(1 << b);
            down_state |= (val << b);
        }

        bool get_down(Button b) const {
            return (down_state >> static_cast<int>(b)) & 1;
        }

        bool get_pressed(Button b) const {
            return (pressed_state >> static_cast<int>(b)) & 1;
        }

        void set_pressed(Button b, bool val) {
            pressed_state &= ~(1 << b);
            pressed_state |= (val << b);
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

            kp.set_down(L1, (rb0 == 0x10));
            kp.set_down(R1, (rb0 == 0x20));
            kp.set_down(L2, (rb0 == 0x40));
            kp.set_down(R2, (rb0 == 0x80));

            kp.set_down(DPadUp,    (rb2 == 0x0) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 && rb4 <= 0x4000000000));
            kp.set_down(DPadRight, (rb2 == 0x020000) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 >= 0xc0000000 && rb4 <= 0xf000000000));
            kp.set_down(DPadDown,  (rb2 == 0x040000)|| (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 && rb4 >= 0xc000000000));
            kp.set_down(DPadLeft,  (rb2 == 0x060000) || (!(rb3 == 0x80000000 && rb4 == 0x8000000000) && rb3 <= 0x40000000 && rb4 <= 0xf000000000));

            kp.set_down(A, (rb0 & 0x2));
            kp.set_down(B, (rb0 & 0x4));
            kp.set_down(Select, (rb2 == 0x8f0000));
            kp.set_down(Start, (rb1 == 0x1000));

            kp.pressed_state |= kp.down_state;
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

        static bool is_down(Button b) {
            return kp.get_down(b);
        }

        static bool is_pressed(Button b) {
            if (kp.get_pressed(b)){
                kp.set_pressed(b, false);
                return true;
            }
            return false;
        }

    };
}
#endif

struct Controller {
    enum Button {
        A, B,
        Select, Start,
        Up, Down,
        Left, Right
    };

    unsigned char status1{};
    unsigned char status2{};
    unsigned char current1 = 0;
    unsigned char current2 = 0;
    bool prev_res1 = false;
    bool prev_res2 = false;
    bool latch1 = false;
    bool shift_req1 = false;
    bool shift_req2 = false;

    KeyboardKey keys1[8] = {
            KEY_LEFT_BRACKET,KEY_RIGHT_BRACKET,
            KEY_SPACE,KEY_ENTER,
            KEY_W,KEY_S,
            KEY_A, KEY_D
    };

#if JOYSTICK
    NintendoJoystick::HIDDevice hid_device;
    NintendoJoystick::Button keysj[8] = {
           NintendoJoystick::Button::A,
           NintendoJoystick::Button::B,
           NintendoJoystick::Button::Select,
           NintendoJoystick::Button::Start,
           NintendoJoystick::Button::DPadUp,
           NintendoJoystick::Button::DPadDown,
           NintendoJoystick::Button::DPadLeft,
           NintendoJoystick::Button::DPadRight,
    };
    Controller(){hid_device.init();}
#endif

#if DEBUG_MODE
    unsigned char custom_status{};
    static void cs_switch(Controller& c, int i, bool dir){
        i += (dir << 2);
        c.custom_status ^= (1 << i);
    }
    static void cs_set(Controller& c, int i, bool dir, bool value){
        i += (dir << 2);
        c.custom_status &= ((~(1 << i)) & 0xff);
        c.custom_status |= (value << i);
    }
    bool cs_read(Controller& c, int i, bool dir) {
        i += (dir << 2);
        return (c.custom_status >> i) & 1;
    }
#endif

    inline void update_status1() {
        if (latch1) {
            status1 = 0;
            for (int i = 0; i < 8; ++i)
                status1 |=
#if DEBUG_MODE
                        ((custom_status & (1 << i)) | (IsKeyDown(keys1[i])) << i);
#elif JOYSTICK
                    (hid_device.is_down(keysj[i]) << i);
#else

                        (IsKeyDown(keys1[i]) << i);
#endif
            current1 = 0;
        }
    }

    // TODO: finish
    inline void update_status2() {
        status2 = 0;
    }

    inline bool read_status1(){
        if (latch1) return IsKeyDown(keys1[0]);
        if (current1 < 8){
            prev_res1 = status1 & 1;
            return prev_res1;
        }
        return true;
    }

    // TODO: finish
    inline bool read_status2(){
        current2 = 0;
        prev_res2 = false;
        return false;
    }
};
#endif //NES_CONTROLLER_H
