#include  "APU.h"

void APU::frame_counter_step4(Bus& bus) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus);
    } else if (cycle_counter == 7457 * 2){
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    } else if (cycle_counter == 7457 * 4) {
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
        if (!irq_inhibit) bus.irq_interrupt_flip_flop = true;
    }
    cycle_counter = (cycle_counter + 1) % ((7457 * 4) + 1);
}

void APU::frame_counter_step5(Bus& bus) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus);
    } else if (cycle_counter == 7457 * 2){
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    } else if (cycle_counter == 7457 * 5) {
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    }
    cycle_counter = (cycle_counter + 1) % ((7457 * 5) + 1);
}

void APU::frame_counter_quarter_update(Bus& bus) {
    pulse_ch1.update_envelope(bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE));
    pulse_ch2.update_envelope(bus.unsafe_read_prg(constants::PULSE2_DUTY_ENVELOPE));
    noise_ch.update_envelope(bus.unsafe_read_prg(constants::NOISE_CHAN_ENVELOPE));
    // todo: update triangle linear counter
}

void APU::frame_counter_half_update(Bus& bus) {
    pulse_ch1.update_length_counter(bus.unsafe_read_chr(constants::PULSE1_TIMER_HIGH));
    pulse_ch2.update_length_counter(bus.unsafe_read_prg(constants::PULSE2_TIMER_HIGH));
    noise_ch.update_length_counter(bus.unsafe_read_prg(constants::NOISE_CHAN_LENGTH_COUNTER));
    triangle_ch.update_length_counter(bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_HIGH));
    // todo: update sweep
}

void APU::handle_frame_counter_update(Bus& bus) {
    if (bus.apufc_requested) {
        frame_counter_mode = static_cast<FrameCounterMode>((bus.apufc >> 7) & 1);
        if (frame_counter_mode == Step5){
            frame_counter_quarter_update(bus);
            frame_counter_half_update(bus);
        }
        cycle_counter = 0;
        irq_inhibit = (bus.apufc >> 6) & 1;
        bus.apufc_requested = false;
    }
}

float APU::gen_sample(Bus &bus) {
    float res = 0.f;
    auto ch1 = pulse_ch1.get_dac(bus);
    res += ch1;
    return res;
}

void APU::tick(Bus& bus) {
#if !APU_ON
    return;
#endif
    handle_frame_counter_update(bus);
    if (current_tick & 1){
        if (frame_counter_mode == Step4)
            frame_counter_step4(bus);
        else
            frame_counter_step5(bus);
        pulse_ch1.tick_timer(bus);
        //    pulse_ch2.tick_timer(bus, cpu);
    }
    sample_acc += 44100;
    if (sample_acc >= constants::CPU_SPEED){
        sample_acc-=constants::CPU_SPEED;
        auto s = gen_sample(bus);
        buffer[buffer_i] = s * 32768.f;
        buffer[buffer_i+1] = s * 32768.f;
        buffer_i = buffer_i + 2 == 1024 ? 0 : buffer_i + 2;
    }
    ++current_tick;
}