#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/sync.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound_common.h"
#include "iopins.h"

static int audio_pin_slice_r = 0;
static int audio_pin_slice_l = 0;

void initAudio_dma(int audio_pin_slice_r, int audio_pin_slice_l);
void startAudio_dma(void);

static void createAudio_outputs(void)
{
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
    audio_pin_slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);
    audio_pin_slice_l = audio_pin_slice_r;

#if (AUDIO_PIN_L != AUDIO_PIN_R)
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    audio_pin_slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
#endif // AUDIO_PIN_L != AUDIO_PIN_R
}

static void configureAudio_outputs(void)
{
    pwm_config config = pwm_get_default_config();

    // Want to generate samples at a ratio of the
    // system clock, wrap at 1000 to allow 32kHz samples
    // At 252 MHz, 32K samples per second with range 1000 gives 7.875
    // At 270 MHz, 32K samples per second with range 1000 gives 8.4375
    // int_frac has 4 bit frac, so multiply int by 16 (4 bits)
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = (((system_clock_frequency  / RANGE) << 4) / SAMPLE_FREQ);
    printf("Sys clock %lu Divide: %lu\n", system_clock_frequency, divider);
    pwm_config_set_clkdiv_int_frac(&config, divider >> 4u, divider & 0xfu);
    pwm_config_set_wrap(&config, RANGE - 1);

    pwm_set_gpio_level(AUDIO_PIN_R, ZEROSOUND);   // mid point to wrap
    pwm_init(audio_pin_slice_r, &config, false);

#if (AUDIO_PIN_L != AUDIO_PIN_R)
    // Can have left and right PWM on different slices (e.g. OlimexPC board)
    if (audio_pin_slice_l != audio_pin_slice_r)
    {
        pwm_set_gpio_level(AUDIO_PIN_L, ZEROSOUND);   // mid point to wrap
        pwm_init(audio_pin_slice_l, &config, false);
    }
#endif // AUDIO_PIN_L != AUDIO_PIN_R
}

void initAudio_pwm(void)
{
    createAudio_outputs();
    initAudio_dma(audio_pin_slice_r, audio_pin_slice_l);
    configureAudio_outputs();
}

void startAudio_pwm(void)
{
    startAudio_dma();

    // Cannot use mask here, as other libs may have already enabled PWM slices
    pwm_set_enabled(audio_pin_slice_r, true);
    if (audio_pin_slice_r != audio_pin_slice_l)
    {
        pwm_set_enabled(audio_pin_slice_l, true);
    }
}

