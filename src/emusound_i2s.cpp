#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "audio_i2s.pio.h"
#include "hardware/clocks.h"
#include "pico/sync.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound_common.h"

static PIO audio_pio = pio0;
static int dma_irq = DMA_IRQ_1;    // Hard code to DMA_IRQ_1, as scanvideo claims 0
static int i2s_dma;
static int irq_num;
static int i2s_pio_sm;
static int i2s_dreq = DREQ_PIO0_TX0;
static gpio_function_t i2s_gpio_func = GPIO_FUNC_PIO0;

static inline void i2s_start_dma_transfer()
{
    dma_channel_config c = dma_get_channel_config(i2s_dma);
    channel_config_set_read_increment(&c, true);
    dma_channel_set_config(i2s_dma, &c, false);
    dma_channel_transfer_from_buffer_now(i2s_dma,
                                         SFIRST ? SBUFFER2 : SBUFFER16,
                                         NUMSAMPLES);
}

// irq handler for I2S DMA
static void __isr __time_critical_func(i2s_dma_irq_handler)()
{
    if (dma_irqn_get_channel_status(irq_num, i2s_dma))
    {
        dma_irqn_acknowledge_channel(irq_num, i2s_dma);
        i2s_start_dma_transfer();

        // Swap the buffers and Signal the 50Hz semaphore
        SFIRST = !SFIRST;
        SEM_REL;
#ifdef TIME_SPARE
        int_count++;
#endif
    }
}

void initAudio_i2s(void)
{
    irq_num = (dma_irq == DMA_IRQ_1) ? 1 : 0;

    // Find a free state machine
    i2s_pio_sm = pio_claim_unused_sm(audio_pio, false);

    if (i2s_pio_sm == -1)
    {
        //Try the other state machine
        audio_pio = pio1;
        i2s_gpio_func = GPIO_FUNC_PIO1;
        i2s_pio_sm = pio_claim_unused_sm(audio_pio, false);

        if (i2s_pio_sm == -1)
        {
            printf("Cannot configure I2S sound - aborting\n");
            exit(-1);  // Cannot run without sound
        }
        else
        {
            i2s_dreq = DREQ_PIO1_TX0 + i2s_pio_sm;
        }
    }
    else
    {
        i2s_dreq = DREQ_PIO0_TX0 + i2s_pio_sm;
    }

    gpio_set_function(PICO_AUDIO_I2S_DATA_PIN, i2s_gpio_func);
    gpio_set_function(PICO_AUDIO_I2S_CLOCK_PIN_BASE, i2s_gpio_func);
    gpio_set_function(PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, i2s_gpio_func);

    uint offset = pio_add_program(audio_pio, &audio_i2s_program);
    audio_i2s_program_init(audio_pio, i2s_pio_sm, offset, PICO_AUDIO_I2S_DATA_PIN, PICO_AUDIO_I2S_CLOCK_PIN_BASE);

    // Set the SM clock frequency
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    assert(system_clock_frequency < 0x40000000);
    uint32_t divider = system_clock_frequency * 4 / (SAMPLE_FREQ * 3); // avoid arithmetic overflow
    assert(divider < 0x1000000);
    pio_sm_set_clkdiv_int_frac(audio_pio, i2s_pio_sm, divider >> 8u, divider & 0xffu);

    __mem_fence_release();
    i2s_dma = dma_claim_unused_channel(true);

    dma_channel_config dma_config = dma_channel_get_default_config(i2s_dma);

    channel_config_set_dreq(&dma_config,
                            i2s_dreq);

    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    dma_channel_configure(i2s_dma,
                            &dma_config,
                            &audio_pio->txf[i2s_pio_sm],  // dest
                            NULL,                         // src
                            0,                            // count
                            false);                       // trigger

    irq_set_exclusive_handler(dma_irq, i2s_dma_irq_handler);
    dma_irqn_set_channel_enabled(irq_num, i2s_dma, 1);

    irq_set_enabled(dma_irq , true);
}

void startAudio_i2s(void)
{
    i2s_start_dma_transfer();
    pio_sm_set_enabled(audio_pio, i2s_pio_sm, true);
}
