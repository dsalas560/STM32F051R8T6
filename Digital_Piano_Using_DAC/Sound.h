#ifndef __SOUND_H__
#define __SOUND_H__

#include <stdint.h>

/*
 * Sound driver for 3-note digital piano.
 *
 * Uses:
 *   - TIM3 as the sample-rate timer (update interrupt)
 *   - DAC driver on PC0..PC3
 *
 * API:
 *   Sound_Init() must be called once at startup.
 *   Sound_Play(note) selects which tone to play:
 *      NOTE_OFF  = silence
 *      NOTE_LOW  = first note
 *      NOTE_MED  = second note
 *      NOTE_HIGH = third note
 */

#define NOTE_OFF   0
#define NOTE_LOW   1   // e.g., C4
#define NOTE_MED   2   // e.g., E4
#define NOTE_HIGH  3   // e.g., G4

void Sound_Init(void);
void Sound_Play(uint8_t note);

#endif /* __SOUND_H__ */
