/*******************************************************************
 Sound
*******************************************************************/
#include "pico.h"
#include <stdlib.h>
#include <limits.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/irq.h"
#ifdef SOUND_DMA
#include "hardware/dma.h"
#endif
#ifdef I2S
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "audio_i2s.pio.h"
#else
#ifndef SOUND_HDMI
#include "hardware/pwm.h"
#endif
#endif
#include "hardware/clocks.h"
#include "pico/sync.h"
#ifdef SOUND_HDMI
#include "audio_ring.h"
#include "display.h"
#endif
#include "sound.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound.h"
#include "iopins.h"

semaphore_t timer_sem;

static const uint16_t NUMSAMPLES = (SAMPLE_FREQ / 50); // samples in 50th of second

static uint16_t soundBuffer16[NUMSAMPLES << 2]; // Effectively two stereo buffers
static uint16_t* soundBuffer2 = &soundBuffer16[NUMSAMPLES << 1];

static bool soundCreated = false;
static volatile bool first = true;   // True if the first buffer is playing
static bool genSound = false;

static int queued_sound_type;           // new sound type requested
static int change_count;                // count down to frame to change sound type
static bool queued_play;

static void beginAudio(void);

#ifdef SOUND_DMA
static int dma_channel_sound;
#endif

#ifdef SOUND_HDMI
#define FIFTYHZMS   20    // ms between 50 HZ ticks
#define TICKMS      2     // ms ticks to service hdmi audio ring buffer
#define TICKCOUNT   (FIFTYHZMS/TICKMS)  // number of ring buffer ticks per 50Hz tick
#define TICK_SAMPLES        (NUMSAMPLES / TICKCOUNT)

struct repeating_timer audio_timer;
audio_ring_t* ring;
audio_sample_t* hdmi_buffer;
int hdmi_buffer_size;
#endif

#ifdef I2S
PIO audio_pio = pio0;
int dma_irq = DMA_IRQ_1;    // Hard code to DMA_IRQ_1, as scanvideo claims 0
int i2s_dma;
int irq_num;
int i2s_pio_sm;
int i2s_dreq = DREQ_PIO0_TX0;
gpio_function_t i2s_gpio_func = GPIO_FUNC_PIO0;
#endif

#ifdef TIME_SPARE
int32_t sound_count = 0;
int64_t int_count = 0;
#endif

uint16_t emu_sndGetSampleRate(void)
{
  return SAMPLE_FREQ;
}

void emu_sndInit(bool playSound, bool reset)
{
  genSound = playSound;
  change_count = 0;   // in case a changed was queued

  // This can be called multiple times...
  if (!soundCreated)
  {
    soundCreated = sound_create(NUMSAMPLES);
  }

  // Call each time, as sound type may have changed
  if (soundCreated)
  {
    sound_init(emu_ACBRequested(), reset);
  }

  // Begin sound regardless, as needed for 50 Hz
  beginAudio();
}

void emu_sndQueueChange(bool playSound, int new_sound_type)
{
  queued_sound_type = new_sound_type;
  queued_play = playSound;
  change_count = 3; // Allow final save bytes to propagate
}

// Calls to this function are synchronised to 50Hz through main timer interrupt
void emu_sndGenerateSamples(void)
{
  if (genSound && soundCreated)
  {
    sound_frame(first ? soundBuffer2 : soundBuffer16);
#ifdef TIME_SPARE
    sound_count++;
#endif

    // process any queued sound change
    if (change_count)
    {
      if (--change_count == 0)
      {
        sound_change_type(queued_sound_type);
        emu_sndInit(queued_play, false);
      }
    }
  }
}

#ifndef SOUND_HDMI
#ifdef I2S
static inline void i2s_start_dma_transfer()
{
    dma_channel_config c = dma_get_channel_config(i2s_dma);
    channel_config_set_read_increment(&c, true);
    dma_channel_set_config(i2s_dma, &c, false);
    dma_channel_transfer_from_buffer_now(i2s_dma,
                                         first ? soundBuffer2 : soundBuffer16,
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
        first = !first;
        sem_release(&timer_sem);
#ifdef TIME_SPARE
        int_count++;
#endif
    }
}

#else // I2S
#ifdef SOUND_DMA
static void __not_in_flash_func(dmaInterruptHandler)()
{
  if (dma_channel_get_irq1_status(dma_channel_sound))
  {
    dma_channel_acknowledge_irq1(dma_channel_sound);
    dma_channel_set_read_addr(dma_channel_sound, first ? soundBuffer2 : soundBuffer16, true);

    // Swap the buffers and Signal the 50Hz semaphore
    first = !first;
    sem_release(&timer_sem);
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

#else // SOUND_DMA
static void __not_in_flash_func(pwmInterruptHandler)()
{
  static int cnt = 0;
  pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN_R));

  pwm_set_gpio_level(AUDIO_PIN_R, soundBuffer16[cnt++]);
#if (AUDIO_PIN_L != AUDIO_PIN_R)
  pwm_set_gpio_level(AUDIO_PIN_L, soundBuffer16[cnt++]);
#else
  cnt++;
#endif
#ifdef TIME_SPARE
  int_count++;
#endif

  if (cnt == (NUMSAMPLES << 2))
  {
    cnt = 0;
    first = true;
    sem_release(&timer_sem);
  }
  else if (cnt == NUMSAMPLES << 1)
  {
    first = false;
    sem_release(&timer_sem);
  }
}
#endif // SOUND_DMA
#endif // I2S
#else  // SOUND_HDMI

static bool __not_in_flash_func(audio_timer_callback)(struct repeating_timer *t)
{
  (void)(t);
  static uint32_t call_count = 0;
  static int cnt = 0;

  // write in chunks
  int size = get_write_size(ring, true);
  if (size >= TICK_SAMPLES)
  {
    int audio_offset = get_write_offset(ring);
    if ((size >= ((3*hdmi_buffer_size)>>2)) &&
        (cnt <= ((NUMSAMPLES << 2)-(TICK_SAMPLES << 1))))
    {
      // Allow to refill buffer
      size = (TICK_SAMPLES<<1);
    }
    else
    {
      size = TICK_SAMPLES;
    }

    for (int c = 0; c < size; c++)
    {
      hdmi_buffer[audio_offset].channels[0] = soundBuffer16[cnt++];
      hdmi_buffer[audio_offset].channels[1] = soundBuffer16[cnt++];
      audio_offset = (audio_offset + 1) & (hdmi_buffer_size-1);
    }
    set_write_offset(ring, audio_offset);

    if (cnt >= (NUMSAMPLES << 2))
    {
      cnt -= (NUMSAMPLES << 2);
    }
  }

  if (++call_count == TICKCOUNT)
  {
    call_count = 0;

    // resync the play pointer
    if ((!first) & (cnt!=0))
    {
      cnt=0;
    }

    // Swap the buffers and Signal the 50Hz semaphore
    first = !first;
    sem_release(&timer_sem);
  }
  return true;
}

#endif // SOUND_HDMI

static void beginAudio(void)
{
  static bool begun = false;

  if (!begun) // Only begin once...
  {
    begun = true;
    sem_init(&timer_sem, 0, 1);
#ifndef SOUND_HDMI
#ifdef I2S
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
    i2s_start_dma_transfer();
    pio_sm_set_enabled(audio_pio, i2s_pio_sm, true);
#else // I2S
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
    int audio_pin_slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);
    int audio_pin_slice_l = audio_pin_slice_r;

#if (AUDIO_PIN_L != AUDIO_PIN_R)
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    audio_pin_slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
#endif // AUDIO_PIN_L != AUDIO_PIN_R

#ifdef SOUND_DMA
    dma_channel_sound = dma_claim_unused_channel(true);

    // Cannot use DMA if have two channels on different slices
    if (audio_pin_slice_r != audio_pin_slice_l)
    {
      printf("Audio on different slices: l slice = %i, r slice = %i\n",
             audio_pin_slice_l, audio_pin_slice_r);
      printf("Cannot use DMA: aborting\n");
      exit(-1);
    }
    config_DMA(dma_channel_sound, audio_pin_slice_r, soundBuffer16, NUMSAMPLES);

    // Set the DMA interrupt handler
    irq_set_exclusive_handler(DMA_IRQ_1, dmaInterruptHandler);
    dma_set_irq1_channel_mask_enabled(0x01 << dma_channel_sound, true);
    irq_set_enabled(DMA_IRQ_1, true);
#else // SOUND_DMA
    // Setup PWM interrupt to fire when PWM cycle is complete on right channel
    pwm_clear_irq(audio_pin_slice_r);
    pwm_set_irq_enabled(audio_pin_slice_r, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmInterruptHandler);
    irq_set_priority (PWM_IRQ_WRAP, PICO_DEFAULT_IRQ_PRIORITY);
    irq_set_enabled(PWM_IRQ_WRAP, true);
#endif // SOUND_DMA

    pwm_config config = pwm_get_default_config();

    // Want to generate samples at a ratio of the
    // system clock, wrap at 1000 to allow 32kHz samples
    // At 250 MHz, 32K samples per second with range 1000 gives 7.8125
    // At 252 MHz, 32K samples per second with range 1000 gives 7.875
    // At 270 MHz, 32K samples per second with range 1000 gives 8.4375
    // int_frac has 4 bit frac, so multiply int by 16 (4 bits)
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = (((system_clock_frequency  / RANGE) << 4) / SAMPLE_FREQ);
    printf("Sys clock %lu Divide: %lu\n", system_clock_frequency, divider);
    pwm_config_set_clkdiv_int_frac(&config, divider >> 4u, divider & 0xfu);
    pwm_config_set_wrap(&config, RANGE - 1);

    // Set buffer and initial pwm to silence
    emu_sndSilence();

    pwm_set_gpio_level(AUDIO_PIN_R, ZEROSOUND);   // mid point to wrap
    pwm_init(audio_pin_slice_r, &config, false);

#if (AUDIO_PIN_L != AUDIO_PIN_R)
    // Can have left and right PWM on different slices (e.g. the Pimoroni VGA boards)
    if (audio_pin_slice_l != audio_pin_slice_r)
    {
      pwm_set_gpio_level(AUDIO_PIN_L, ZEROSOUND);   // mid point to wrap
      pwm_init(audio_pin_slice_l, &config, false);
    }
#endif // AUDIO_PIN_L != AUDIO_PIN_R

#ifdef SOUND_DMA
    dma_start_channel_mask(0x1 << dma_channel_sound);
#endif // SOUND_DMA
    // Cannot use mask here, as other libs may have already enabled PWM slices
    pwm_set_enabled(audio_pin_slice_r, true);
    if (audio_pin_slice_r != audio_pin_slice_l)
      pwm_set_enabled(audio_pin_slice_l, true);
#endif // I2S
#else // SOUND_HDMI
    // Create the timer callback
    emu_sndSilence();
    getAudioRing(&ring);
    hdmi_buffer = ring->buffer;
    hdmi_buffer_size = ring->size;
    add_repeating_timer_ms(-TICKMS, audio_timer_callback, NULL, &audio_timer);
#endif // SOUND_HDMI

    printf("sound initialized\n");
  }
  else
  {
    emu_sndSilence();
  }
}

void emu_sndSilence(void)
{
  // Set buffers to silence
  for (int i = 0; i < (NUMSAMPLES<<2); ++i)
  {
    soundBuffer16[i] = ZEROSOUND;
  }
}

bool emu_sndSaveSnap(void)
{
  if (!sound_save_snap()) return false;
  if (!emu_FileWriteBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;
  if (!emu_FileWriteBytes(&queued_play, sizeof(queued_play))) return false;

  return true;
}

bool emu_sndLoadSnap(void)
{
  if (!sound_load_snap()) return false;
  if (!emu_FileReadBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;
  if (!emu_FileReadBytes(&queued_play, sizeof(queued_play))) return false;

  return true;
}
