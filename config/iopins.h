#ifndef IOPINS_H
#define IOPINS_H

// Speaker
#ifdef PICO_AUDIO_PWM_R_PIN
#define AUDIO_PIN       PICO_AUDIO_PWM_R_PIN
#else
#define AUDIO_PIN       28
#endif

#endif