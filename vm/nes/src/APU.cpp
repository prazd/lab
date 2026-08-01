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

    if (bus.apu_noise_start_flag){
        noise_ch.divider = noise_ch.prev_divider;
        noise_ch.decay_lvl = 15;
        bus.apu_noise_start_flag = false;
    } else {
        noise_ch.envelope_tick(bus);
    }

    if (triangle_ch.linear_counter_reload){
        triangle_ch.linear_counter = triangle_ch.next_linear_counter & 0x7f;
        triangle_ch.control_flag = triangle_ch.next_linear_counter & 0x80;
        triangle_ch.linear_counter_reload = triangle_ch.control_flag;
    } else if (triangle_ch.linear_counter) --triangle_ch.linear_counter;
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

    if (noise_ch.length_counter && !noise_ch.is_infinite_play)
        --noise_ch.length_counter;

    if (triangle_ch.length_counter && !triangle_ch.control_flag)
        --triangle_ch.length_counter;
}

void APU::handle_frame_counter_update(Bus& bus) {
    if (bus.frame_counter_mode == Bus::Step4)
        frame_counter_step4(bus);
    else
        frame_counter_step5(bus);
}

template <int hz>
static inline float low_pass_filter(float x){
    static float prev_y = 0.f;
    // TODO: check
    static constexpr float a = (2.0f * 3.14f * (float)hz / 44100.f) / ((1 + 2.0f * 3.14f * (float)hz / 44100.f));
    return prev_y = prev_y + a * (x - prev_y);
}

template <int hz>
static inline float high_pass_filter(float x){
    static float prev_y = 0.f;
    static float prev_x = 0.f;
    // TODO: check
    static constexpr float a = (1.0f/(1.0f + 2.0f * 3.14f * (float)hz / 44100.f));
    auto res = a * (prev_y + x - prev_x);
    prev_x = x;
    return prev_y = res ;
}

// https://www.nesdev.org/wiki/APU_Mixer
float APU::gen_sample(Bus& bus) {
    auto ch1 = pulse_ch1.get_dac(bus);
    auto ch2 = pulse_ch2.get_dac(bus);
    auto triangle = triangle_ch.get_dac();
    auto noise = noise_ch.get_dac(bus);
    auto dmc = bus.dmc_output_level;
#if DEBUG_MODE
    if (!(internal_enable & Bus::APUChan::Pulse1)) ch1 = 0;
    if (!(internal_enable & Bus::APUChan::Pulse2)) ch2 = 0;
    if (!(internal_enable & Bus::APUChan::Triangle)) triangle = 0;
    if (!(internal_enable & Bus::APUChan::Noise)) noise = 0;
    if (!(internal_enable & Bus::APUChan::DMC)) dmc = 0;
#endif
    auto pulse_res = 95.88f/((8128.f/(ch1+ch2)) + 100.f);
    auto tnd_res = 159.79f/((1.f/((triangle/8227.f) + (noise/12241.f) + dmc/22638.f)) + 100.f); // TODO: debug
#if 0
    return low_pass_filter<14'000>(high_pass_filter<440>(high_pass_filter<90>(pulse_res + tnd_res)));
#else
    return pulse_res + tnd_res;
#endif
}

void APU::update_control(Bus& bus){
    bus.apu_control &= 0xee;
    bus.apu_control |= (!!pulse_ch1.length_counter);
    bus.apu_control |= (!!pulse_ch2.length_counter) << 1;
    bus.apu_control |= (!!triangle_ch.length_counter) << 2;
    bus.apu_control |= (!!noise_ch.length_counter) << 3;
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
        triangle_ch.length_counter = bus.apu_is_enable(Bus::Triangle) ? triangle_ch.length_counter : 0;
        noise_ch.length_counter = bus.apu_is_enable(Bus::Noise) ? noise_ch.length_counter : 0;
        bus.apu_control &= 0x7f;
        bus.apu_enable_req = false;
    }

    if (bus.apu_pulse1_envelope_update){
        pulse_ch1.update_envelope(bus.unsafe_read_prg(constants::PULSE1_DUTY_ENVELOPE));
        bus.apu_pulse1_envelope_update = false;
    }

    if (bus.apu_pulse2_envelope_update){
        pulse_ch2.update_envelope(bus.unsafe_read_prg(constants::PULSE2_DUTY_ENVELOPE));
        bus.apu_pulse2_envelope_update = false;
    }

    if (bus.apu_noise_envelope_update){
        noise_ch.update_envelope(bus.unsafe_read_prg(constants::NOISE_CHAN_ENVELOPE));
        bus.apu_noise_envelope_update = false;
    }

    if (bus.apu_pulse1_timer_update){
        if (bus.apu_is_enable(Bus::Pulse1)){
            pulse_ch1.update_lc_and_timer(bus);
            bus.apu_pulse1_start_flag = true; // TODO: check
        }
        bus.apu_pulse1_timer_update = false;
    }

    if (bus.apu_pulse2_timer_update){
        if (bus.apu_is_enable(Bus::Pulse2)){
            pulse_ch2.update_lc_and_timer(bus);
            bus.apu_pulse2_start_flag = true; // TODO: check
        }
        bus.apu_pulse2_timer_update = false;
    }

    if (bus.apu_triangle_timer_update){
        if (bus.apu_is_enable(Bus::Triangle))
            triangle_ch.update_lc_and_timer(bus);
        triangle_ch.linear_counter_reload = true;
        bus.apu_triangle_timer_update = false;
    }

    if (bus.apu_noise_timer_update){
        if (bus.apu_is_enable(Bus::Noise))
            noise_ch.update_lc_and_timer(bus);
        bus.apu_noise_timer_update = false;
    }

    if (bus.apu_triangle_linear_counter_update){
        triangle_ch.next_linear_counter = bus.unsafe_read_prg(constants::TRIANGLE_CHAN_LINEAR_COUNTER);
        bus.apu_triangle_linear_counter_update = false;
    }


    if (bus.apu_noise_chan_loop_update){
        noise_ch.loop = bus.unsafe_read_prg(constants::NOISE_CHAN_LOOP);
        noise_ch.mode = noise_ch.loop & 0x80;
        noise_ch.timer = noise_ch.period[(noise_ch.loop & 0xf)];
        bus.apu_noise_chan_loop_update = false;
    }

    if (bus.apu_dmc_dma_address_update) {
        auto s_addr = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_ADDRESS);
        dmc_ch.dma_base_address = 0xc000 | (s_addr << 6);
        bus.apu_dmc_dma_address_update = false;
    }

    if (!(bus.apu_current_tick & 1)){
        pulse_ch1.tick_timer(bus);
        pulse_ch2.tick_timer(bus);
        triangle_ch.tick_timer(bus);
        noise_ch.tick_timer(bus);
        if (bus.apu_clear_frame_irq_req){
            bus.apu_control &= 0xbf;
            bus.apu_irq_interrupt_flip_flop = false;
            bus.apu_clear_frame_irq_req = false;
        }
    }

// TODO: debug
#if 0
    if (bus.apu_is_enable(Bus::DMC)) dmc_ch.tick(bus);
    if (bus.dmc_rate_counter) --bus.dmc_rate_counter;
#endif

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

void DMCChan::tick(Bus &bus) {
    auto chan_freq_val = bus.unsafe_read_prg(constants::DMC_CHAN_FREQ);
    is_loop = chan_freq_val & 0x40;
    irq_enable = chan_freq_val & 0x80;
    if (!irq_enable) bus.apu_control &= 0x7f;
    bus.dmc_rate_index = chan_freq_val & 0xf;

    if (!bus.dmc_bytes_remaining && is_loop){
        auto s_len = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_LENGTH);
        bus.dmc_bytes_remaining = (s_len << 4) | 1;
        bus.dmc_dma_address = dma_base_address;
        if (bus.dmc_sample_buffer.empty()){
            bus.apu_irq_interrupt_flip_flop = true;
            bus.dmc_dma_counter = 3 + bus.next_write;
            bus.dmc_need_sample_update = true;
        }
    } else if (!bus.dmc_bytes_remaining){
        return;
    }

    if (bus.dmc_delay){
        if (!(bus.dmc_delay == 1 && bus.next_write)) --bus.dmc_delay;
        if (!bus.dmc_delay) {bus.dmc_dma_counter = 3 + bus.next_write; bus.dmc_dma_in_progress = true;}
    }
    if (bus.dmc_dma_counter && bus.dmc_dma_in_progress) {
        if (bus.dmc_dma_counter == 3){
// TODO: check
//            if (bus.last_arg_addr == 0x4016){
//                if (!(bus.apu_current_tick & 1)) bus.unsafe_read_prg(bus.last_arg_addr);
                bus.dmc_dma_read_prg(bus.last_arg_addr); // TODO: debug
        }
        --bus.dmc_dma_counter;
        if (!bus.dmc_dma_counter){
            bus.dmc_sample_buffer.push_back(bus.dmc_dma_read_prg(bus.dmc_dma_address));
            bus.dmc_dma_address = bus.dmc_dma_address == 0xffff ? 0x8000 : bus.dmc_dma_address + 1;
            if (bus.dmc_bytes_remaining) --bus.dmc_bytes_remaining;
            bus.dmc_dma_in_progress = false;
            if (bus.dmc_bytes_remaining){
                if (bus.dmc_need_sample_update){
                    bus.dmc_shift_reg = bus.dmc_sample_buffer.back(); bus.dmc_sample_buffer.pop_back();
                    bus.dmc_shift_count = 8;
                    bus.dmc_need_sample_update = false;
                }
            } else if (!bus.dmc_bytes_remaining && !is_loop && irq_enable){
                bus.apu_control &= 0x7f;
                bus.apu_control |= 0x80;
                bus.apu_irq_interrupt_flip_flop = true;
                if (!bus.dmc_shift_count) {
                    bus.dmc_shift_reg = bus.dmc_sample_buffer.back(); bus.dmc_sample_buffer.pop_back();
                    bus.dmc_shift_count = 8;
                }
            }
        }
    }

    if (!bus.dmc_rate_counter){
        bus.dmc_output_level += (bus.dmc_shift_reg & 1) ? 2 : -2;
        if (bus.dmc_shift_count){
            bus.dmc_shift_reg >>= 1;
            --bus.dmc_shift_count;
            bus.dmc_output_level = std::clamp(bus.dmc_output_level, (char)0, (char)127);
            bus.dmc_rate_counter = Bus::ntsc_rates[bus.dmc_rate_index];
        }

        // TODO: debug
        if (!bus.dmc_shift_count){
            if (!(bus.dmc_sample_buffer.empty() && bus.dmc_bytes_remaining)){
                bus.dmc_shift_reg = bus.dmc_sample_buffer.back(); bus.dmc_sample_buffer.pop_back();
                bus.dmc_shift_count = 8;
            } else {
                bus.dmc_need_sample_update = true;
            }
            bus.dmc_dma_counter = 4; // TODO: check
            bus.dmc_dma_in_progress = true;
        }
    }
}