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
        if (!irq_inhibit) {bus.irq_interrupt_flip_flop = true; bus.apu_control |= 0x80;}
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
    if (bus.apu_is_enable(Bus::APUChan::Pulse1))
        pulse_ch1.envelope_tick(bus);
}

void APU::frame_counter_half_update(Bus& bus) {
    if (bus.apu_is_enable(Bus::APUChan::Pulse1) && pulse_ch1.length_counter && bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE) & 0x20)
        --pulse_ch1.length_counter;
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

void APU::update_control(Bus& bus){
    bus.apu_control = 0;
    bus.apu_control |= (pulse_ch1.length_counter > 0) << 1;
}

void APU::tick(Bus& bus) {
#if !APU_ON
    return;
#endif
    if (bus.apu_enable_req){
        pulse_ch1.length_counter = bus.apu_is_enable(Bus::Pulse1) ? 0 : pulse_ch1.length_counter;
        bus.apu_enable_req = false;
    }

    if (bus.apu_pulse1_envelope_update){
        pulse_ch1.update_envelope(bus, bus.read_prg(bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE)));
        bus.apu_pulse1_envelope_update = false;
    }
    if (bus.apu_clock_len_counter){
        frame_counter_half_update(bus);
        bus.apu_clock_len_counter = false;
    }
    if (bus.apu_pulse1_timer_update){
        if (bus.apu_is_enable(Bus::Pulse1)) {
            pulse_ch1.length_counter = length_table[((bus.unsafe_read_prg(constants::PULSE1_TIMER_HIGH) >> 3) & 0x1f)];
            pulse_ch1.current_duty_step = 0;
            pulse_ch1.envelope = 15; // set start flag
        }
        bus.apu_pulse1_timer_update = false;
    }
    handle_frame_counter_update(bus);
    if (current_tick & 1){
        if (frame_counter_mode == Step4)
            frame_counter_step4(bus);
        else
            frame_counter_step5(bus);
        pulse_ch1.tick_timer(bus);
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
    update_control(bus);
}