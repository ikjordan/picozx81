#ifndef IOPINS_H
#define IOPINS_H

// Speaker
#ifdef PICO_AUDIO_PWM_L_PIN
#define AUDIO_PIN_L       PICO_AUDIO_PWM_L_PIN
#ifdef PICO_AUDIO_PWM_R_PIN
// Both defined, stereo
#define AUDIO_PIN_R       PICO_AUDIO_PWM_R_PIN
#else
// Only left defined - use for both as mono
#define AUDIO_PIN_R       PICO_AUDIO_PWM_L_PIN
#endif
#else
#ifdef PICO_AUDIO_PWM_R_PIN
// Only right defined - use for both as mono
#define AUDIO_PIN_R       PICO_AUDIO_PWM_R_PIN
#define AUDIO_PIN_L       PICO_AUDIO_PWM_R_PIN
#else   // Nothing defined, default to mono
#define AUDIO_PIN_L       28
#define AUDIO_PIN_R       28
#endif
#endif

#endif
