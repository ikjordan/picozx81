#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "pico/sync.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound_common.h"

static void __not_in_flash_func(dmaInterruptHandler)()
{
    if (dma_channel_get_irq1_status(DMA_CHANNEL_SOUND))
    {
        dma_channel_acknowledge_irq1(DMA_CHANNEL_SOUND);
        dma_channel_set_read_addr(DMA_CHANNEL_SOUND, SFIRST ? SBUFFER2 : SBUFFER16, true);

        // Swap the buffers and Signal the 50Hz semaphore
        SFIRST = !SFIRST;
        SEM_REL;
#ifdef TIME_SPARE
        int_count++;
#endif
    }
}

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

void initAudio_dma(int audio_pin_slice_r, int audio_pin_slice_l)
{
    dma_channel_claim(DMA_CHANNEL_SOUND);

    // Cannot use DMA if have two channels on different slices
    if (audio_pin_slice_r != audio_pin_slice_l)
    {
        printf("Audio on different slices: l slice = %i, r slice = %i\n",
                audio_pin_slice_l, audio_pin_slice_r);
        printf("Cannot use DMA: aborting\n");
        exit(-1);
    }
    config_DMA(DMA_CHANNEL_SOUND, audio_pin_slice_r, soundBuffer16, NUMSAMPLES);

    // Set the DMA interrupt handler
    irq_set_exclusive_handler(DMA_IRQ_1, dmaInterruptHandler);
    dma_set_irq1_channel_mask_enabled(0x01 << DMA_CHANNEL_SOUND, true);
    irq_set_enabled(DMA_IRQ_1, true);
}

void startAudio_dma(void)
{
    dma_start_channel_mask(0x1 << DMA_CHANNEL_SOUND);
}
