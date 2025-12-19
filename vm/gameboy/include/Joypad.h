#ifndef RAM_H_JOYPAD_H
#define RAM_H_JOYPAD_H
#include "Memory.h"
#include "raylib.h"

#if CONTROLLER
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>

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

    template <typename PressedBuffer = std::vector<Buttons>>
    class HIDDevice {
        static constexpr int vendor_id = 8406;
        static constexpr int product_id = 42784;
        static constexpr int page = kHIDPage_GenericDesktop;
        static constexpr int usage_game_pad = kHIDUsage_GD_GamePad;
        static inline __IOHIDManager* manager = nullptr;
        static inline unsigned char report_buffer[8]{};
        static inline PressedBuffer pressed = PressedBuffer(8);

        static void update_buttons(){
            pressed.clear();
            if (report_buffer[2] == 0x0)  pressed.push_back(DPadUp);
            if (report_buffer[2] == 0x2)  pressed.push_back(DPadRight);
            if (report_buffer[2] == 0x4)  pressed.push_back(DPadDown);
            if (report_buffer[2] == 0x6)  pressed.push_back(DPadLeft);

            if (report_buffer[0] & 0x2)  pressed.push_back(A);
            if (report_buffer[0] & 0x4)  pressed.push_back(B);
            if (report_buffer[2] == 0x8f) pressed.push_back(Select);
            if (report_buffer[1] == 0x10) pressed.push_back(Start);

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
        }

        static void device_matching_callback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
            if (result != kIOReturnSuccess) return;
            auto product = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
            auto open_res = IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
            if (open_res != kIOReturnSuccess) {
                std::abort();
            }
            IOHIDDeviceRegisterInputReportCallback(device, report_buffer, sizeof(report_buffer), input_report_callback, nullptr);
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

        static bool is_pressed(Buttons b) {
            return std::any_of(pressed.begin(), pressed.end(), [b](auto el){return b == el;});
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
    NintendoJoystick::HIDDevice<> device;
    NintendoJoystick::Buttons gamepad_keys[8]{
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
