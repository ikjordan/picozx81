#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "iopins.h"
#include "pico/sync.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound_common.h"

static void __not_in_flash_func(dmaInterruptHandler)()
{
#if !defined(SOUND_DMA_SEPARATE) || (AUDIO_PIN_R == AUDIO_PIN_L)
    if (dma_channel_get_irq1_status(DMA_CHANNEL_SOUND_1ST))
    {
        dma_channel_acknowledge_irq1(DMA_CHANNEL_SOUND_1ST);
        dma_channel_set_read_addr(DMA_CHANNEL_SOUND_1ST, SFIRST ? SBUFFER2 : SBUFFER16, true);

        // Swap the buffers and Signal the 50Hz semaphore
        SFIRST = !SFIRST;
        SEM_REL;
#ifdef TIME_SPARE
        int_count++;
#endif
    }
#else
    static uint dual_dma_completed = 0;
    bool right_completed = dma_channel_get_irq1_status(DMA_CHANNEL_SOUND_1ST);
    bool left_completed = dma_channel_get_irq1_status(DMA_CHANNEL_SOUND_2ND);
    if (right_completed)
    {
        dma_channel_acknowledge_irq1(DMA_CHANNEL_SOUND_1ST);
        dma_channel_set_read_addr(DMA_CHANNEL_SOUND_1ST, SFIRST ? SBUFFER2 : SBUFFER16, true);

        dual_dma_completed |= 1u << DMA_CHANNEL_SOUND_1ST;
    }

    if (left_completed)
    {
        dma_channel_acknowledge_irq1(DMA_CHANNEL_SOUND_2ND);
        dma_channel_set_read_addr(DMA_CHANNEL_SOUND_2ND, SFIRST ? SBUFFER2 + NUMSAMPLES : SBUFFER16 + NUMSAMPLES, true);
        dual_dma_completed |= 1u << DMA_CHANNEL_SOUND_2ND;
    }

    if (dual_dma_completed == ((1u << DMA_CHANNEL_SOUND_1ST) | (1u << DMA_CHANNEL_SOUND_2ND)))
    {
        // Both channels completed, so swap the buffers and Signal the 50Hz semaphore
        dual_dma_completed = 0;
        SFIRST = !SFIRST;
        SEM_REL;
#ifdef TIME_SPARE
        int_count++;
#endif
    }
#endif
}

void initAudio_dma(int audio_pin_slice_r, int audio_pin_slice_l)
{
#if !defined(SOUND_DMA_SEPARATE) || (AUDIO_PIN_R == AUDIO_PIN_L)
    (void)audio_pin_slice_l;
#endif

    dma_channel_claim(DMA_CHANNEL_SOUND_1ST);
    dma_channel_config right_config = dma_channel_get_default_config(DMA_CHANNEL_SOUND_1ST);
    channel_config_set_read_increment(&right_config, true);
    channel_config_set_write_increment(&right_config, false);
    channel_config_set_dreq(&right_config, DREQ_PWM_WRAP0 + audio_pin_slice_r);

#ifdef SOUND_DMA_SEPARATE
    volatile uint16_t* right_cc = (volatile uint16_t *)&pwm_hw->slice[audio_pin_slice_r].cc;
    if (pwm_gpio_to_channel(AUDIO_PIN_R) == PWM_CHAN_B)
    {
        right_cc++;
    }

    channel_config_set_transfer_data_size(&right_config, DMA_SIZE_16);
    dma_channel_configure(DMA_CHANNEL_SOUND_1ST,
                          &right_config,
                          right_cc,
                          SBUFFER16,
                          NUMSAMPLES,
                          false);

#if AUDIO_PIN_R != AUDIO_PIN_L
    dma_channel_claim(DMA_CHANNEL_SOUND_2ND);

    volatile uint16_t* left_cc = (volatile uint16_t *)&pwm_hw->slice[audio_pin_slice_l].cc;
    if (pwm_gpio_to_channel(AUDIO_PIN_L) == PWM_CHAN_B)
    {
        left_cc++;
    }

    dma_channel_config left_config = dma_channel_get_default_config(DMA_CHANNEL_SOUND_2ND);
    channel_config_set_read_increment(&left_config, true);
    channel_config_set_write_increment(&left_config, false);
    channel_config_set_dreq(&left_config, DREQ_PWM_WRAP0 + audio_pin_slice_l);
    channel_config_set_transfer_data_size(&left_config, DMA_SIZE_16);
    dma_channel_configure(DMA_CHANNEL_SOUND_2ND,
                          &left_config,
                          left_cc,
                          SBUFFER16 + NUMSAMPLES,
                          NUMSAMPLES,
                          false);
#endif
#else
    channel_config_set_transfer_data_size(&right_config, DMA_SIZE_32);

    // Set up dma
    dma_channel_configure(DMA_CHANNEL_SOUND_1ST,
                          &right_config,
                          &pwm_hw->slice[audio_pin_slice_r].cc,
                          SBUFFER16,
                          NUMSAMPLES,
                          false);
#endif

    // Set the DMA interrupt handler
    irq_set_exclusive_handler(DMA_IRQ_1, dmaInterruptHandler);
    uint dma_mask = 1u << DMA_CHANNEL_SOUND_1ST;
#if defined(SOUND_DMA_SEPARATE) && (AUDIO_PIN_R != AUDIO_PIN_L)
    dma_mask |= 1u << DMA_CHANNEL_SOUND_2ND;
#endif
    dma_set_irq1_channel_mask_enabled(dma_mask, true);
    irq_set_enabled(DMA_IRQ_1, true);
}

void startAudio_dma(void)
{
    uint dma_mask = 1u << DMA_CHANNEL_SOUND_1ST;
#if defined(SOUND_DMA_SEPARATE) && (AUDIO_PIN_R != AUDIO_PIN_L)
    dma_mask |= 1u << DMA_CHANNEL_SOUND_2ND;
#endif
    dma_start_channel_mask(dma_mask);
}
