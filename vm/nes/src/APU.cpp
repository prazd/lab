#include  "APU.h"

void APU::frame_counter_step(Bus& bus, CPU& cpu) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus, cpu);
    } else if (cycle_counter == 7457 * 2 || cycle_counter == 7457 * frame_counter_mode){
        frame_counter_quarter_update(bus, cpu);
        frame_counter_half_update(bus, cpu);
        // + IRQ gen (if 4step mode)
    }
    cycle_counter = cycle_counter >= 7457 * frame_counter_mode ? 0 : cycle_counter + 1;
}

void APU::frame_counter_quarter_update(Bus& bus, CPU& cpu) {
    pulse_ch1.update_envelope(cpu.mem.read(bus, constants::PULSE1_DUTY_ENVELOPE));
    pulse_ch2.update_envelope(cpu.mem.read(bus, constants::PULSE2_DUTY_ENVELOPE));
    noise_ch.update_envelope(cpu.mem.read(bus, constants::NOISE_CHAN_ENVELOPE));
    // todo: update triangle linear counter
}

void APU::frame_counter_half_update(Bus& bus, CPU& cpu) {
    pulse_ch1.update_length_counter(cpu.mem.read(bus, constants::PULSE1_TIMER_HIGH));
    pulse_ch2.update_length_counter(cpu.mem.read(bus, constants::PULSE2_TIMER_HIGH));
    noise_ch.update_length_counter(cpu.mem.read(bus, constants::NOISE_CHAN_LENGTH_COUNTER));
    triangle_ch.update_length_counter(cpu.mem.read(bus, constants::TRIANGLE_CHAN_TIMER_HIGH));
    // todo: update sweep
}

void APU::tick(Bus& bus, CPU& cpu) {
    // todo
}