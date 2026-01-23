#include "Sound.h"
#include "DAC.h"
#include "main.h"      // for htim3

extern TIM_HandleTypeDef htim3;  // TIM3 handle created in main.c

// ===== Waveform table =====
// 32-sample 4-bit-ish sine wave (values 0..15)
#define WAVE_SIZE 32

static const uint8_t SineWave[WAVE_SIZE] = {
    8, 10, 12, 13, 14, 15, 15, 15,
    14, 13, 12, 10, 8, 6, 4, 3,
    2, 1, 0, 0, 0, 1, 2, 3,
    4, 6, 8, 10, 12, 13, 14, 15
};

static volatile uint8_t waveIndex = 0;
static volatile uint8_t currentNote = NOTE_OFF;

// Precomputed ARR values for TIM3 (PSC = 47, tick = 1MHz)
#define ARR_NOTE_LOW   118u   // ~262 Hz
#define ARR_NOTE_MED    94u   // ~329 Hz
#define ARR_NOTE_HIGH   79u   // ~391 Hz

void Sound_Init(void)
{
    waveIndex = 0;
    currentNote = NOTE_OFF;

    DAC_Init();  // ensure DAC is ready

    // Start with timer stopped; Sound_Play() will start it when needed
    HAL_TIM_Base_Stop_IT(&htim3);
    DAC_Out(0);  // silence
}

void Sound_Play(uint8_t note)
{
    currentNote = note;
    waveIndex = 0;

    switch (note)
    {
    case NOTE_LOW:
        __HAL_TIM_SET_AUTORELOAD(&htim3, ARR_NOTE_LOW);
        HAL_TIM_Base_Start_IT(&htim3);
        break;

    case NOTE_MED:
        __HAL_TIM_SET_AUTORELOAD(&htim3, ARR_NOTE_MED);
        HAL_TIM_Base_Start_IT(&htim3);
        break;

    case NOTE_HIGH:
        __HAL_TIM_SET_AUTORELOAD(&htim3, ARR_NOTE_HIGH);
        HAL_TIM_Base_Start_IT(&htim3);
        break;

    case NOTE_OFF:
    default:
        HAL_TIM_Base_Stop_IT(&htim3);
        DAC_Out(0);   // force output to 0
        break;
    }
}

// ===== Timer ISR callback =====
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        // Only output wave if a note is active
        if (currentNote == NOTE_OFF)
        {
            DAC_Out(0);
            return;
        }

        DAC_Out(SineWave[waveIndex]);
        waveIndex++;
        if (waveIndex >= WAVE_SIZE)
        {
            waveIndex = 0;
        }
    }
}
