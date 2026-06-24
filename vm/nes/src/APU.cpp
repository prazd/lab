#include  "APU.hpp"

void APU::frame_counter_step4(Bus& bus) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus);
    } else if (cycle_counter == 7457 * 2){
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    } else if (cycle_counter == 7457 * 4) {
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
        if (!irq_inhibit) {
            bus.apu_irq_interrupt_flip_flop = true;
            bus.apu_control |= 0x40;
        }
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
    if (pulse_ch1.length_counter && !pulse_ch1.is_infinite_play)
        --pulse_ch1.length_counter;
}

void APU::handle_frame_counter_update(Bus& bus) {
    if (bus.apufc_requested) {
        frame_counter_mode = static_cast<FrameCounterMode>((bus.apufc >> 7) & 1);
        if (frame_counter_mode == Step5){
            frame_counter_quarter_update(bus);
            frame_counter_half_update(bus);
        }
        irq_inhibit = (bus.apufc >> 6) & 1;
        if (irq_inhibit && frame_counter_mode == Step4) bus.apu_control &= 0xbf;
        frame_counter_reset = true;
        frame_counter_reset_ticks = (current_tick & 1) ? 2 : 3; // todo: check (3,4)
        bus.apufc_requested = false;
    } else {
        if (frame_counter_mode == Step4)
            frame_counter_step4(bus);
        else
            frame_counter_step5(bus);
    }
}

float APU::gen_sample(Bus &bus) {
    float res = 0.f;
    auto ch1 = pulse_ch1.get_dac(bus);
    res += ch1;
    return res;
}

void APU::update_control(Bus& bus){
    bus.apu_control &= 0xee;
    bus.apu_control |= (!!pulse_ch1.length_counter);
    bus.apu_control |= (!!dmc_ch.bytes_remaining) << 4;
}

void APU::tick(Bus& bus) {
    if (bus.apu_enable_req){
// TODO:
//        if (dmc_ch.dmc_dma_counter <= 2)
        pulse_ch1.length_counter = bus.apu_is_enable(Bus::Pulse1) ? 0 : pulse_ch1.length_counter;
        if (bus.apu_is_enable(Bus::DMC))
            dmc_ch.restart(bus);
        else
            dmc_ch.bytes_remaining = 0;
        bus.apu_control &= 0x7f;
        bus.apu_enable_req = false;
    }

    if (bus.apu_pulse1_envelope_update){
        pulse_ch1.update_envelope(bus, bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE));
        bus.apu_pulse1_envelope_update = false;
    }

    if (bus.apu_pulse1_timer_update){
        if (bus.apu_is_enable(Bus::Pulse1)) {
            pulse_ch1.length_counter = length_table[((bus.unsafe_read_prg(constants::PULSE1_TIMER_HIGH) >> 3) & 0x1f)];
            pulse_ch1.current_duty_step = 0;
            pulse_ch1.envelope = 15; // set start flag
        }
        bus.apu_pulse1_timer_update = false;
    }

    if (current_tick & 1){
        pulse_ch1.tick_timer(bus);
        if (bus.apu_clear_frame_irq_req){
            bus.apu_control &= 0xbf;
            bus.apu_clear_frame_irq_req = false;
        }
    }
    if (bus.apu_is_enable(Bus::DMC)) dmc_ch.tick(bus);

    if (frame_counter_reset){
        if (frame_counter_reset_ticks) {--frame_counter_reset_ticks;}
        if (!frame_counter_reset_ticks){
            cycle_counter = 0;
            frame_counter_reset = false;
            // TODO: check
            if (frame_counter_mode == Step4)
                frame_counter_step4(bus);
            else
                frame_counter_step5(bus);
        }
    }
    handle_frame_counter_update(bus);
#if APU_ON
    sample_acc += 44100;
    if (sample_acc >= constants::CPU_SPEED){
        sample_acc-=constants::CPU_SPEED;
        auto s = gen_sample(bus);
        buffer[buffer_i] = s * 32768.f;
        buffer[buffer_i+1] = s * 32768.f;
        buffer_i = buffer_i + 2 == 1024 ? 0 : buffer_i + 2;
    }
#endif
    ++current_tick;
    update_control(bus);
}