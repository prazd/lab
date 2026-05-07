#include  "APU.h"

void APU::frame_counter_step4(Bus& bus, CPU& cpu) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus, cpu);
    } else if (cycle_counter == 7457 * 2){
        frame_counter_quarter_update(bus, cpu);
        frame_counter_half_update(bus, cpu);
    } else if (cycle_counter == 7457 * 4) {
        frame_counter_quarter_update(bus, cpu);
        frame_counter_half_update(bus, cpu);
        if (!irq_inhibit) bus.irq_interrupt_flip_flop = true;
    }
    cycle_counter = (cycle_counter + 1) % ((7457 * 4) + 1);
}

void APU::frame_counter_step5(Bus& bus, CPU& cpu) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus, cpu);
    } else if (cycle_counter == 7457 * 2){
        frame_counter_quarter_update(bus, cpu);
        frame_counter_half_update(bus, cpu);
    } else if (cycle_counter == 7457 * 5) {
        frame_counter_quarter_update(bus, cpu);
        frame_counter_half_update(bus, cpu);
    }
    cycle_counter = (cycle_counter + 1) % ((7457 * 5) + 1);
}

void APU::frame_counter_quarter_update(Bus& bus, CPU& cpu) {
    pulse_ch1.update_envelope(cpu.mem.unsafe_read(bus, constants::PULSE1_DUTY_ENVELOPE));
    pulse_ch2.update_envelope(cpu.mem.unsafe_read(bus, constants::PULSE2_DUTY_ENVELOPE));
    noise_ch.update_envelope(cpu.mem.unsafe_read(bus, constants::NOISE_CHAN_ENVELOPE));
    // todo: update triangle linear counter
}

void APU::frame_counter_half_update(Bus& bus, CPU& cpu) {
    pulse_ch1.update_length_counter(cpu.mem.unsafe_read(bus, constants::PULSE1_TIMER_HIGH));
    pulse_ch2.update_length_counter(cpu.mem.unsafe_read(bus, constants::PULSE2_TIMER_HIGH));
    noise_ch.update_length_counter(cpu.mem.unsafe_read(bus, constants::NOISE_CHAN_LENGTH_COUNTER));
    triangle_ch.update_length_counter(cpu.mem.unsafe_read(bus, constants::TRIANGLE_CHAN_TIMER_HIGH));
    // todo: update sweep
}

void APU::handle_frame_counter_update(Bus& bus, CPU& cpu) {
    if (bus.apufc) {
        frame_counter_mode = static_cast<FrameCounterMode>((bus.apufc >> 7) & 1);
        if (frame_counter_mode == Step5){
            frame_counter_quarter_update(bus, cpu);
            frame_counter_half_update(bus, cpu);
        }
        cycle_counter = 0;
        irq_inhibit = (bus.apufc >> 6) & 1;
    }
}

void APU::tick(Bus& bus, CPU& cpu) {
    handle_frame_counter_update(bus, cpu);
    if (frame_counter_mode == Step4)
        frame_counter_step4(bus, cpu);
    else
        frame_counter_step5(bus, cpu);
    pulse_ch1.tick_timer(bus, cpu);
    pulse_ch2.tick_timer(bus, cpu);
    // todo
}