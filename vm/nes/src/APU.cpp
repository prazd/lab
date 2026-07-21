#include  "APU.hpp"

void APU::frame_counter_step4(Bus& bus) {
    if (cycle_counter == 7457 || cycle_counter == (7457 * 3)) {
        frame_counter_quarter_update(bus);
    } else if (cycle_counter == 14913){
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    } else if (cycle_counter == 29829) {
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
        if (!bus.apu_irq_inhibit) {
            bus.apu_irq_interrupt_flip_flop = true;
            bus.apu_fc_irq_set_cycle = bus.master_clock;
        }else {
            irq_set_counter = 2;
        }
        bus.apu_control |= 0x40;
    }
    cycle_counter = (cycle_counter + 1) % (29830);
}

void APU::frame_counter_step5(Bus& bus) {
    if (cycle_counter == 7457 || cycle_counter == 7457 * 3) {
        frame_counter_quarter_update(bus);
    } else if (cycle_counter == 14913){
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    } else if (cycle_counter == 37281) {
        frame_counter_quarter_update(bus);
        frame_counter_half_update(bus);
    }
    cycle_counter = (cycle_counter + 1) % (37282);
}

void APU::frame_counter_quarter_update(Bus& bus) {
    if (bus.apu_pulse1_start_flag){
        pulse_ch1.divider = pulse_ch1.prev_divider;
        pulse_ch1.decay_lvl = 15;
        bus.apu_pulse1_start_flag = false;
    } else {
        pulse_ch1.envelope_tick(bus);
    }
    if (bus.apu_pulse2_start_flag){
        pulse_ch2.divider = pulse_ch2.prev_divider;
        pulse_ch2.decay_lvl = 15;
        bus.apu_pulse2_start_flag = false;
    } else {
        pulse_ch2.envelope_tick(bus);
    }
}

void APU::frame_counter_half_update(Bus& bus) {
    if (pulse_ch1.length_counter && !pulse_ch1.is_infinite_play)
        --pulse_ch1.length_counter;
    if (pulse_ch1.sweep_counter) --pulse_ch1.sweep_counter;
    if (!pulse_ch1.sweep_counter || pulse_ch1.sweep_reset)
        pulse_ch1.sweep_update_timer();

    if (pulse_ch2.length_counter && !pulse_ch2.is_infinite_play)
        --pulse_ch2.length_counter;
    if (pulse_ch2.sweep_counter) --pulse_ch2.sweep_counter;
    if (!pulse_ch2.sweep_counter || pulse_ch2.sweep_reset)
        pulse_ch2.sweep_update_timer();
}

void APU::handle_frame_counter_update(Bus& bus) {
    if (bus.frame_counter_mode == Bus::Step4)
        frame_counter_step4(bus);
    else
        frame_counter_step5(bus);
}

static inline float low_pass_filter(float x){
    static float prev_y = 0.f;
    static float a = 0.864f;
    return prev_y = prev_y + a * (x - prev_y);
}

static inline float high_pass_filter(float x){
    static float prev_y = 0.f;
    static float prev_x = 0.f;
    static float a = 0.987f;
    auto res = a * (prev_y + x - prev_x);
    prev_x = x;
    return prev_y = res ;
}


// https://www.nesdev.org/wiki/APU_Mixer
float APU::gen_sample(Bus& bus) {
    auto ch1 = pulse_ch1.get_dac(bus);
    auto ch2 = pulse_ch2.get_dac(bus);
    auto triangle = 0.f; // TODO
    auto noise = 0.f; // TODO
    auto dmc = bus.dmc_output_level;
#if DEBUG_MODE
    if (!(internal_enable & Bus::APUChan::Pulse1)) ch1 = 0;
    if (!(internal_enable & Bus::APUChan::Pulse2)) ch2 = 0;
    if (!(internal_enable & Bus::APUChan::Triangle)) triangle = 0;
    if (!(internal_enable & Bus::APUChan::Noise)) noise = 0;
    if (!(internal_enable & Bus::APUChan::DMC)) dmc = 0;
#endif
    auto pulse_res = 95.88f/((8128.f/(ch1+ch2)) + 100.f);
    auto tnd_res = 159.79f/(1/((triangle/8227.f) + (noise/12241.f) + dmc/22638.f) + 100.f); // TODO: debug
    return low_pass_filter(high_pass_filter(pulse_res));
}

void APU::update_control(Bus& bus){
    bus.apu_control &= 0xee;
    bus.apu_control |= (!!pulse_ch1.length_counter);
    bus.apu_control |= (!!pulse_ch2.length_counter) << 1;
    bus.apu_control |= (!!bus.dmc_bytes_remaining) << 4;
}

void APU::tick(Bus& bus) {
    if (bus.apu_irq_inhibit && bus.apu_control & 0x40){
        if (irq_set_counter) --irq_set_counter;
        if (!irq_set_counter) bus.apu_control &= 0xbf;
    }
    update_control(bus);

    if (bus.apu_pulse1_sweep_update){
        pulse_ch1.update_sweep(bus.unsafe_read_prg(constants::PULSE1_SWEEP));
        bus.apu_pulse1_sweep_update = false;
    }

    if (bus.apu_pulse2_sweep_update){
        pulse_ch2.update_sweep(bus.unsafe_read_prg(constants::PULSE2_SWEEP));
        bus.apu_pulse2_sweep_update = false;
    }

    if (bus.apu_enable_req){
// TODO:
//        if (dmc_ch.dmc_dma_counter <= 2)
        pulse_ch1.length_counter = bus.apu_is_enable(Bus::Pulse1) ? pulse_ch1.length_counter : 0;
        pulse_ch2.length_counter = bus.apu_is_enable(Bus::Pulse2) ? pulse_ch2.length_counter : 0;

        bus.apu_control &= 0x7f;
        bus.apu_enable_req = false;
    }

    if (bus.apu_pulse1_envelope_update){
        pulse_ch1.update_envelope(bus, bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE));
        bus.apu_pulse1_envelope_update = false;
    }

    if (bus.apu_pulse2_envelope_update){
        pulse_ch2.update_envelope(bus, bus.unsafe_read_prg(constants::PULSE2_DUTY_ENVELOPE));
        bus.apu_pulse2_envelope_update = false;
    }


    if (bus.apu_pulse1_timer_update){
        if (bus.apu_is_enable(Bus::Pulse1)) {
            pulse_ch1.length_counter = Bus::length_table[((bus.unsafe_read_prg(constants::PULSE1_TIMER_HIGH) >> 3) & 0x1f)];
        }
        bus.apu_pulse1_timer_update = false;
    }

    if (bus.apu_pulse2_timer_update){
        if (bus.apu_is_enable(Bus::Pulse2)) {
            pulse_ch2.length_counter = Bus::length_table[((bus.unsafe_read_prg(constants::PULSE2_TIMER_HIGH) >> 3) & 0x1f)];
        }
        bus.apu_pulse2_timer_update = false;
    }


    if (!(bus.apu_current_tick & 1)){
        pulse_ch1.tick_timer(bus);
        pulse_ch2.tick_timer(bus);
        if (bus.apu_clear_frame_irq_req){
            bus.apu_control &= 0xbf;
            bus.apu_irq_interrupt_flip_flop = false;
            bus.apu_clear_frame_irq_req = false;
        }
    }

// TODO: debug
    if (bus.apu_is_enable(Bus::DMC)) dmc_ch.tick(bus);
    if (bus.dmc_rate_counter)
        --bus.dmc_rate_counter;


    if (bus.frame_counter_reset) {
        if (bus.frame_counter_reset_ticks) { --bus.frame_counter_reset_ticks; }
        if (!bus.frame_counter_reset_ticks) {
            cycle_counter = 0;
            bus.frame_counter_reset = false;
            if (bus.frame_counter_mode == Bus::Step5) {
                frame_counter_quarter_update(bus);
                frame_counter_half_update(bus);
            }
        }
    }
    handle_frame_counter_update(bus);
#if APU_ON
    sample_acc += 44100;
    if (sample_acc >= constants::CPU_SPEED){
        sample_acc-=constants::CPU_SPEED;
        auto s = gen_sample(bus);
        buffer[write_i] = (short)(s * 32768.f);
        write_i = write_i + 1 == 1024 ? 0 : write_i + 1;
        ++sample_count;
    }
#endif
    ++bus.apu_current_tick;
}