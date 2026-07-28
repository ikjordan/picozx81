#include "pico.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "common.h"
#include "emupriv.h"
#include "emulinein.h"
#include "linein.pio.h"

#define PIO_INSTRUCTIONS_PER_BIT    3
#define PIO_INSTRUCTIONS_PER_SAMPLE (32.0f * PIO_INSTRUCTIONS_PER_BIT)

#define LINEIN_RX_SM    0

static PIO linein_pio = pio2;                       // Hard coded to avoid clashes with existing SMs on 0 and 1
static uint32_t linein_frame_tstates = 0;           // tstates at start of frame

// DMA 
#define LINEIN_BUFF_SIZE        (LINEIN_RATE_HZ / 50)   // 20ms at 48000Hz
#define LINEIN_DMA_IRQ_INDEX    2                       // Hard coded to DMA 2 as 0 and 1 used elsewhere
#define LINEIN_DMA_CHANNEL      DMA_CHANNEL_LINEIN      // Hard coded to avoid channels used elsewhere
#define LINEIN_DMA_IRQ          (DMA_IRQ_0 + LINEIN_DMA_IRQ_INDEX)

static uint32_t linein_buffer[2][LINEIN_BUFF_SIZE];
static int linein_buffer_available = 0;             // The buffer that has data that the application can read
static uint rx_offset = 0;
static float linein_clkdiv = 0.0f;

#ifdef TIME_SPARE
int32_t linein_count = 0;
#endif


static void die_if_i2c_err(int rc, const char *what) {
    if (rc < 0) {
        printf("I2C error during %s: %d\n", what, rc);
        while (true) sleep_ms(1000);
    }
}

static void es8311_write(uint8_t reg, uint8_t val) {
    uint8_t b[2] = {reg, val};
    int rc = i2c_write_blocking(i2c0, PICO_ES8311_ADDR, b, 2, false);
    die_if_i2c_err(rc, "write");
}

static void codec_power_on(void) {
    gpio_init(PICO_CODEC_PWR_DIS_PIN);
    gpio_set_dir(PICO_CODEC_PWR_DIS_PIN, GPIO_OUT);
    gpio_put(PICO_CODEC_PWR_DIS_PIN, 1);
    sleep_ms(100);
}

#if 0
static void codec_power_off(void) {
    gpio_put(PICO_CODEC_PWR_DIS_PIN, 1);
}
#endif


static void i2c_setup(void) {
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
}

static void es8311_init_capture(void) {
    // Reset sequence
    es8311_write(0x00, 0x1f);
    sleep_ms(6);
    es8311_write(0x00, 0x80);
    sleep_ms(6);

    // Clocking
    es8311_write(0x01, 0xBF);       // Derive MCLK from BCLK enable BCLK, ADC digital clk, ADC analog clk
    es8311_write(0x02, 0x18);       // pre-mult = x8 pre-div = /1

    // ADC clock divider
    es8311_write(0x03, 0x10);       // 64*fs(ss) / 32*fs(ds)

    // DAC clock divider (not used but keep sane)   
    es8311_write(0x04, 0x10);       // 64*fs

    es8311_write(0x05, 0x00);       // default adc_mclk=dig_mclk/(DIV_CLKADC+1) -> adc_mclk=dig_mclk

    es8311_write(0x09, 0x0c);       // 16-bit serial audio data word length, I2S serial audio data format
    
    // Power analog/bias/vref/ADC path
    es8311_write(0x0d, 0x01);       // VMID startup normal
    sleep_ms(50);
    es8311_write(0x0d, 0x02);       // VMID normal operation

    es8311_write(0x0e, 0x00);       // power up PGA + ADC modulator
    sleep_ms(20);
    
    // Input: MIC1P-MIC1N differential, DMIC disabled, PGA gain = 0 dB
    es8311_write(0x14, 0x10);

    // ADC volume full-scale, HPF on, EQ bypass
    es8311_write(0x16, 0x20);       // Synchronise filter counter, ADC gain scale up 0dB - was 0
    es8311_write(0x17, 0xf0);       // ADC volume: +32dB - (15-3) * 0.5 = +26dB
    es8311_write(0x18, 0x00);       // ALC disabled 
    es8311_write(0x1c, 0x6f);       // ADCEQ bypass, Dynamic HPF, ADCHPF stage2 coeff = 0x0c
}

static inline void linein_start_dma_transfer()
{
    dma_channel_config c = dma_get_channel_config(LINEIN_DMA_CHANNEL);
    channel_config_set_write_increment(&c, true);
    dma_channel_set_config(LINEIN_DMA_CHANNEL, &c, false);
    dma_channel_transfer_to_buffer_now(LINEIN_DMA_CHANNEL,
                                       &linein_buffer[linein_buffer_available],
                                       LINEIN_BUFF_SIZE);
}

// irq handler for LineIn DMA
static void __isr __time_critical_func(linein_dma_irq_handler)()
{
    if (dma_irqn_get_channel_status(LINEIN_DMA_IRQ_INDEX, LINEIN_DMA_CHANNEL))
    {
        dma_irqn_acknowledge_channel(LINEIN_DMA_IRQ_INDEX, LINEIN_DMA_CHANNEL);
        linein_start_dma_transfer();

        // Swap the buffers and Signal the 50Hz semaphore
        linein_buffer_available = 1 - linein_buffer_available;
        linein_count++;
    }
}

// External API

// Initialise the linein capture
void emu_linein_initialise(void) 
{
    codec_power_on();
    i2c_setup();

    es8311_init_capture();

    rx_offset = pio_add_program(linein_pio, &linein_rx_program);

    // Get a free DMA channel - has to be outside of the range claimed by PicoDVI & DMA sound
    dma_channel_claim(LINEIN_DMA_CHANNEL);

    dma_channel_config dma_config = dma_channel_get_default_config(LINEIN_DMA_CHANNEL);

    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);

    // Pace transfers from the PIO RX FIFO
    channel_config_set_dreq(&dma_config, pio_get_dreq(linein_pio, LINEIN_RX_SM, false));

    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
 
    dma_channel_configure(LINEIN_DMA_CHANNEL,
                          &dma_config,
                          NULL,                             // destination, set later
                          &linein_pio->rxf[LINEIN_RX_SM],   // src
                          0,                                // count, set later
                          false);                           // Do not start yet

    irq_set_exclusive_handler(LINEIN_DMA_IRQ, linein_dma_irq_handler);
    dma_irqn_set_channel_enabled(LINEIN_DMA_IRQ_INDEX, LINEIN_DMA_CHANNEL, true);

    linein_buffer_available = 1 - linein_buffer_available;

    irq_set_enabled(LINEIN_DMA_IRQ, true);

    // Trigger the DMA, but nothing will happen until the PIO state machaines are enabled
    linein_start_dma_transfer();

    // Calculate the LineIn SM clock frequency
    linein_clkdiv = clock_get_hz(clk_sys)/ (LINEIN_RATE_HZ * PIO_INSTRUCTIONS_PER_SAMPLE);
    printf("Sys clock %lu LineIn Divide: %f\n", clock_get_hz(clk_sys), linein_clkdiv);
}

// Start the linein capture
void emu_linein_start(void)
{
    linein_rx_program_init(linein_pio, LINEIN_RX_SM, rx_offset, PICO_I2S_DOUT_PIN, PICO_I2S_LRCK_PIN, linein_clkdiv);
}

void emu_linein_set_frame_tstate(uint32_t tstates)
{
    // Store the tstate count at the start of the frame
    linein_frame_tstates = tstates;
}

int16_t emu_linein_value(uint32_t tstates)
{
    // Calculate the tstate offset
    uint32_t tstate_offset = tstates - linein_frame_tstates;
    uint32_t linein_index =  tstate_offset * LINEIN_BUFF_SIZE / tsmax;

    // clamp
    if (linein_index >= LINEIN_BUFF_SIZE) {
        linein_index = LINEIN_BUFF_SIZE - 1;
    }
    return (int16_t)(linein_buffer[linein_buffer_available][linein_index] & 0xffff);
}

void emu_linein_get_buffer(uint32_t** buffer, uint32_t* buffer_size)
{
    *buffer = linein_buffer[linein_buffer_available];
    *buffer_size = LINEIN_BUFF_SIZE;
}

