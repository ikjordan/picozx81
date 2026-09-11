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

#ifdef SOUND_DMA_SEPARATE
static int audio_dma_left = -1;
static bool audio_dma_dual_slice = false;
static uint dual_dma_completed = 0;
#endif

static void __not_in_flash_func(dmaInterruptHandler)()
{
#ifdef SOUND_DMA_SEPARATE
    bool right_completed = dma_channel_get_irq1_status(DMA_CHANNEL_SOUND_1ST);
    bool left_completed = audio_dma_dual_slice && dma_channel_get_irq1_status(audio_dma_left);

    if (right_completed)
    {
        dma_channel_acknowledge_irq1(DMA_CHANNEL_SOUND_1ST);
        dma_channel_set_read_addr(DMA_CHANNEL_SOUND_1ST, SFIRST ? SBUFFER2 : SBUFFER16, true);

        if (audio_dma_dual_slice)
        {
            dual_dma_completed |= 1u << DMA_CHANNEL_SOUND_1ST;
        }
        else
        {
            // A single GPIO always uses the right-channel buffer.
            SFIRST = !SFIRST;
            SEM_REL;
#ifdef TIME_SPARE
            int_count++;
#endif
        }
    }

    if (left_completed)
    {
        dma_channel_acknowledge_irq1(audio_dma_left);
        dma_channel_set_read_addr(audio_dma_left,
                                  SFIRST ? SBUFFER2 + NUMSAMPLES : SBUFFER16 + NUMSAMPLES,
                                  true);
        dual_dma_completed |= 1u << audio_dma_left;
    }

    if (audio_dma_dual_slice &&
        dual_dma_completed == ((1u << DMA_CHANNEL_SOUND_1ST) | (1u << audio_dma_left)))
    {
        // Both channels completed, so swap the buffers and Signal the 50Hz semaphore
        dual_dma_completed = 0;
        SFIRST = !SFIRST;
        SEM_REL;
#ifdef TIME_SPARE
        int_count++;
#endif
    }
#else
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
#endif
}

#ifndef SOUND_DMA_SEPARATE
static void config_DMA(uint channel, uint slice, const volatile void* write, uint count)
{
    dma_channel_config dmaconfig = dma_channel_get_default_config(channel);
    channel_config_set_read_increment(&dmaconfig, true);
    channel_config_set_write_increment(&dmaconfig, false);
    channel_config_set_dreq(&dmaconfig, DREQ_PWM_WRAP0 + slice);
    channel_config_set_transfer_data_size(&dmaconfig, DMA_SIZE_32);

    // Set up dma
    dma_channel_configure(channel,
                          &dmaconfig,
                          &pwm_hw->slice[slice].cc,
                          write,
                          count,
                          false);
}
#endif

void initAudio_dma(int audio_pin_slice_r, int audio_pin_slice_l)
{
    dma_channel_claim(DMA_CHANNEL_SOUND_1ST);

#ifdef SOUND_DMA_SEPARATE
    {
        volatile uint16_t* right_cc = (volatile uint16_t *)&pwm_hw->slice[audio_pin_slice_r].cc;
        if (pwm_gpio_to_channel(AUDIO_PIN_R) == PWM_CHAN_B)
        {
            right_cc++;
        }

        dma_channel_config right_config = dma_channel_get_default_config(DMA_CHANNEL_SOUND_1ST);
        channel_config_set_read_increment(&right_config, true);
        channel_config_set_write_increment(&right_config, false);
        channel_config_set_dreq(&right_config, DREQ_PWM_WRAP0 + audio_pin_slice_r);
        channel_config_set_transfer_data_size(&right_config, DMA_SIZE_16);
        dma_channel_configure(DMA_CHANNEL_SOUND_1ST, &right_config,
                              right_cc, SBUFFER16, NUMSAMPLES, false);

        if (AUDIO_PIN_R != AUDIO_PIN_L)
        {
            audio_dma_dual_slice = true;
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
            dma_channel_configure(DMA_CHANNEL_SOUND_2ND, &left_config,
                                  left_cc, SBUFFER16 + NUMSAMPLES, NUMSAMPLES, false);
        }
    }
    #else
    {
        (void)audio_pin_slice_l;
        config_DMA(DMA_CHANNEL_SOUND_1ST, audio_pin_slice_r, soundBuffer16, NUMSAMPLES);
    }
#endif

    // Set the DMA interrupt handler
    irq_set_exclusive_handler(DMA_IRQ_1, dmaInterruptHandler);
    uint dma_mask = 1u << DMA_CHANNEL_SOUND_1ST;
#ifdef SOUND_DMA_SEPARATE
    if (audio_dma_dual_slice)
        dma_mask |= 1u << audio_dma_left;
#endif
    dma_set_irq1_channel_mask_enabled(dma_mask, true);
    irq_set_enabled(DMA_IRQ_1, true);
}

void startAudio_dma(void)
{
    uint dma_mask = 1u << DMA_CHANNEL_SOUND_1ST;
#ifdef SOUND_DMA_SEPARATE
    if (audio_dma_dual_slice)
    {
        dma_mask |= 1u << audio_dma_left;
    }
#endif
    dma_start_channel_mask(dma_mask);
}
