/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Traffic Light + Pedestrian FSM using LCD + 74HC595 (SPI1)
  *
  * Hardware summary:
  *   LCD (HD44780, 4-bit):
  *     PA8 = RS, PA9 = E, PC0..PC3 = D4..D7 (data nibble)
  *
  *   74HC595 shift register (LEDs for lights):
  *     SPI1 (Master): PA5 = SCK, PA7 = MOSI
  *     PB12 = RCLK (latch). OE' tied LOW, SRCLR' tied HIGH on your breadboard.
  *
  *   Inputs:
  *     PA0 = Walk button (internal pulldown, pressed = 1)
  *     PA1 = North sensor (external pulldown)
  *     PA2 = East  sensor (external pulldown)
  *
  * LED bit map in the 74HC595 byte (MSB..LSB = QH..QA):
  *   QA (bit0)=E_G, QB=E_Y, QC=E_R, QD=N_G, QE=N_Y, QF=N_R, QG=WALK, QH=DONT
  *
  * Design summary:
  *   - Pure Moore FSM in ROM (table-driven: name, outputs, wait, 8 next states).
  *   - Every state has 8 transitions for inputs [W,N,E] (3 inputs => 2^3).
  *   - Two traffic cycles (N side and E side), plus a latched “walk requested”
  *     set that branches into a pedestrian sequence at the all-red boundary.
  *   - The walk button must be held ~2 seconds (confirm chain) to latch a request.
  *   - If a walk request finishes confirming AND no cars are waiting,
  *     we insert a quick yellow → all-red → WALK immediately.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "LCD.h"   // your LCD driver (PA8/PA9 + PC0..PC3)

/* ================= HAL Handles ================= */
SPI_HandleTypeDef hspi1;   // CubeMX provides the storage for SPI1

/* ================= 74HC595 OUTPUT MAP =================
   Byte -> QA..QH (LSB..MSB). Adjust if your wires differ.
*/
#define OUT_E_G   (1U<<0)  /* QA = East Green */
#define OUT_E_Y   (1U<<1)  /* QB = East Yellow */
#define OUT_E_R   (1U<<2)  /* QC = East Red */
#define OUT_N_G   (1U<<3)  /* QD = North Green */
#define OUT_N_Y   (1U<<4)  /* QE = North Yellow */
#define OUT_N_R   (1U<<5)  /* QF = North Red */
#define OUT_WALK  (1U<<6)  /* QG = Walk (pedestrian) */
#define OUT_DONT  (1U<<7)  /* QH = Don't Walk (pedestrian) */

#define OUT_ALLRED (OUT_E_R | OUT_N_R) /* both approaches red */

/* Latch (RCLK) pin for 74HC595 */
#define SR_LATCH_GPIO_Port   GPIOB
#define SR_LATCH_Pin         GPIO_PIN_12

/* ================== PROTOTYPES FROM CUBEMX ================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* ================== SHIFT REGISTER HELPERS ================== */

/** Toggle RCLK HIGH then LOW so QA..QH update from shift register. */
static inline void SR_Latch(void) {
  HAL_GPIO_WritePin(SR_LATCH_GPIO_Port, SR_LATCH_Pin, GPIO_PIN_SET);
  __NOP(); __NOP(); __NOP(); __NOP();   // short pulse is enough
  HAL_GPIO_WritePin(SR_LATCH_GPIO_Port, SR_LATCH_Pin, GPIO_PIN_RESET);
}

/** Send one byte (MSB first) over SPI1 to 74HC595. */
static inline void Shift595_WriteByte(uint8_t data) {
  HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
  SR_Latch(); // move shifted bits to the output latches
}

/** Drive all traffic/crosswalk LEDs by writing one byte. */
static inline void ApplyOutputs(uint8_t out_byte) {
  Shift595_WriteByte(out_byte);
}

/* ================== INPUTS ==================
   We always produce a 3-bit index: [W,N,E] (WALK, North, East).
   This gives 8 possible input codes and matches next[] per state.
*/
static inline uint8_t ReadInputs3(void){
  uint8_t w = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) ? 1 : 0; // Walk button
  uint8_t n = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) ? 1 : 0; // North sensor
  uint8_t e = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) ? 1 : 0; // East sensor
  return (uint8_t)((w<<2) | (n<<1) | e);
}

/* ================== FSM CORE ==================
   Table-driven Moore machine, each state has:
     - a printable name (for LCD)
     - an 8-bit output for the 74HC595
     - a dwell time in 10ms units (HAL_Delay(10) loops)
     - 8 next-state entries (for all 3-bit input patterns)
*/

/* Timings (demo-friendly; tweak to taste) */
#define T_G      300   // 3.0 s green
#define T_Y      150   // 1.5 s yellow
#define T_AR      50   // 0.5 s all-red (safety)
#define T_WALK   200   // 2.0 s steady WALK
#define T_HURRY   40   // 0.4 s per hurry blink ON or OFF
#define T_DONT   150   // 1.5 s solid DON'T WALK
#define T_CF      30   // 0.3 s per confirm step (4 steps ≈ 1.2 s). Increase for longer hold.

/* State object layout */
typedef struct {
  const char *name;     // shown on LCD line 1
  uint8_t     out;      // 74HC595 byte
  uint16_t    t10ms;    // dwell (10ms ticks)
  const uint8_t next[8];// 8 next-state indices for [W N E]
} State;

/* Enumerate states so we can index the table and write transitions clearly */
enum {
  /* Traffic (normal demand-driven) */
  S_N_G=0, S_N_Y, S_AR_N2E,
  S_E_G,   S_E_Y, S_AR_E2N,

  /* Traffic with a walk-request latched (ignore further W) */
  S_rN_G, S_rN_Y, S_rAR_N2E,
  S_rE_G, S_rE_Y, S_rAR_E2N,

  /* Pedestrian sequences (two variants so we return to the correct side) */
  S_WALK_N2E, S_HON1_N2E, S_HOFF1_N2E, S_HON2_N2E, S_HOFF2_N2E, S_DONT_N2E,
  S_WALK_E2N, S_HON1_E2N, S_HOFF1_E2N, S_HON2_E2N, S_HOFF2_E2N, S_DONT_E2N,

  /* 4-step confirm chains (≈ 4 * T_CF) used to "hold" the walk button */
  S_ConfN1, S_ConfN2, S_ConfN3, S_ConfN4,
  S_ConfE1, S_ConfE2, S_ConfE3, S_ConfE4,

  S__NUM
};

/* Helper for the 8-way next[] initializer (W=0 rows then W=1 rows) */
#define NEXT8(w0n0e0,w0n0e1,w0n1e0,w0n1e1, w1n0e0,w1n0e1,w1n1e0,w1n1e1) \
  { w0n0e0,w0n0e1,w0n1e0,w0n1e1, w1n0e0,w1n0e1,w1n1e0,w1n1e1 }

/* ================== FSM TABLE ==================
   Read left→right:
     name, outputs (which LEDs), dwell (how long), then transitions:
     next[ W=0,N=0,E=0 ], next[0,0,1], next[0,1,0], next[0,1,1],
     next[ W=1,N=0,E=0 ], next[1,0,1], next[1,1,0], next[1,1,1 ].
*/
static const State FSM[S__NUM] = {
/* ===== Traffic: normal ===== */
[S_N_G] = {
  .name="N_G",                    // North green, East red
  .out  = OUT_N_G | OUT_E_R,
  .t10ms = T_G,
  .next = NEXT8(
    /*W=0*/ /*N E:00*/ S_N_G,   /*01*/ S_N_Y,   /*10*/ S_N_G,   /*11*/ S_N_Y,
    /*W=1*/ /*N E:00*/ S_ConfN1,/*01*/ S_ConfN1,/*10*/ S_ConfN1,/*11*/ S_ConfN1)
},
[S_N_Y] = {
  .name="N_Y",                    // North yellow (East stays red)
  .out  = OUT_N_Y | OUT_E_R,
  .t10ms = T_Y,
  .next = NEXT8(S_AR_N2E,S_AR_N2E,S_AR_N2E,S_AR_N2E, S_AR_N2E,S_AR_N2E,S_AR_N2E,S_AR_N2E)
},
[S_AR_N2E] = {
  .name="AR_N2E",                 // All-red between N and E
  .out  = OUT_ALLRED,
  .t10ms = T_AR,
  .next = NEXT8(S_E_G,S_E_G,S_E_G,S_E_G, S_E_G,S_E_G,S_E_G,S_E_G)
},

[S_E_G] = {
  .name="E_G",                    // East green, North red
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_G,
  .next = NEXT8(
    /*W=0*/ /*N E:00*/ S_E_G,   /*01*/ S_E_G,   /*10*/ S_E_Y,   /*11*/ S_E_Y,
    /*W=1*/ /*N E:00*/ S_ConfE1,/*01*/ S_ConfE1,/*10*/ S_ConfE1,/*11*/ S_ConfE1)
},
[S_E_Y] = {
  .name="E_Y",
  .out  = OUT_E_Y | OUT_N_R,
  .t10ms = T_Y,
  .next = NEXT8(S_AR_E2N,S_AR_E2N,S_AR_E2N,S_AR_E2N, S_AR_E2N,S_AR_E2N,S_AR_E2N,S_AR_E2N)
},
[S_AR_E2N] = {
  .name="AR_E2N",
  .out  = OUT_ALLRED,
  .t10ms = T_AR,
  .next = NEXT8(S_N_G,S_N_G,S_N_G,S_N_G, S_N_G,S_N_G,S_N_G,S_N_G)
},

/* ===== Traffic: walk-request latched (ignore further W) ===== */
[S_rN_G] = {
  .name="rN_G",
  .out  = OUT_N_G | OUT_E_R,
  .t10ms = T_G,
  .next = NEXT8(
    /*W ignored*/ /*N E:00*/ S_rN_G, /*01*/ S_rN_Y, /*10*/ S_rN_G, /*11*/ S_rN_Y,
    /*W ignored*/ /*N E:00*/ S_rN_G, /*01*/ S_rN_Y, /*10*/ S_rN_G, /*11*/ S_rN_Y)
},
[S_rN_Y] = {
  .name="rN_Y",
  .out  = OUT_N_Y | OUT_E_R,
  .t10ms = T_Y,
  .next = NEXT8(S_rAR_N2E,S_rAR_N2E,S_rAR_N2E,S_rAR_N2E, S_rAR_N2E,S_rAR_N2E,S_rAR_N2E,S_rAR_N2E)
},
[S_rAR_N2E] = {
  .name="rAR_N2E",
  .out  = OUT_ALLRED,
  .t10ms = T_AR,
  .next = NEXT8(S_WALK_N2E,S_WALK_N2E,S_WALK_N2E,S_WALK_N2E, S_WALK_N2E,S_WALK_N2E,S_WALK_N2E,S_WALK_N2E)
},

[S_rE_G] = {
  .name="rE_G",
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_G,
  .next = NEXT8(
    /*W ignored*/ /*N E:00*/ S_rE_G, /*01*/ S_rE_G, /*10*/ S_rE_Y, /*11*/ S_rE_Y,
    /*W ignored*/ /*N E:00*/ S_rE_G, /*01*/ S_rE_G, /*10*/ S_rE_Y, /*11*/ S_rE_Y)
},
[S_rE_Y] = {
  .name="rE_Y",
  .out  = OUT_E_Y | OUT_N_R,
  .t10ms = T_Y,
  .next = NEXT8(S_rAR_E2N,S_rAR_E2N,S_rAR_E2N,S_rAR_E2N, S_rAR_E2N,S_rAR_E2N,S_rAR_E2N,S_rAR_E2N)
},
[S_rAR_E2N] = {
  .name="rAR_E2N",
  .out  = OUT_ALLRED,
  .t10ms = T_AR,
  .next = NEXT8(S_WALK_E2N,S_WALK_E2N,S_WALK_E2N,S_WALK_E2N, S_WALK_E2N,S_WALK_E2N,S_WALK_E2N,S_WALK_E2N)
},

/* ===== Pedestrian: N→E version ===== */
[S_WALK_N2E] = {
  .name="WALK_N2E",               // show WALK steady
  .out  = OUT_ALLRED | OUT_WALK,
  .t10ms = T_WALK,
  .next = NEXT8(S_HON1_N2E,S_HON1_N2E,S_HON1_N2E,S_HON1_N2E, S_HON1_N2E,S_HON1_N2E,S_HON1_N2E,S_HON1_N2E)
},
[S_HON1_N2E] = {
  .name="H1_ON_N2E",              // hurry: DON'T blinking (ON)
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HOFF1_N2E,S_HOFF1_N2E,S_HOFF1_N2E,S_HOFF1_N2E, S_HOFF1_N2E,S_HOFF1_N2E,S_HOFF1_N2E,S_HOFF1_N2E)
},
[S_HOFF1_N2E] = {
  .name="H1_OFF_N2E",             // hurry: DON'T blinking (OFF)
  .out  = OUT_ALLRED,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HON2_N2E,S_HON2_N2E,S_HON2_N2E,S_HON2_N2E, S_HON2_N2E,S_HON2_N2E,S_HON2_N2E,S_HON2_N2E)
},
[S_HON2_N2E] = {
  .name="H2_ON_N2E",
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HOFF2_N2E,S_HOFF2_N2E,S_HOFF2_N2E,S_HOFF2_N2E, S_HOFF2_N2E,S_HOFF2_N2E,S_HOFF2_N2E,S_HOFF2_N2E)
},
[S_HOFF2_N2E] = {
  .name="H2_OFF_N2E",
  .out  = OUT_ALLRED,
  .t10ms = T_HURRY,
  .next = NEXT8(S_DONT_N2E,S_DONT_N2E,S_DONT_N2E,S_DONT_N2E, S_DONT_N2E,S_DONT_N2E,S_DONT_N2E,S_DONT_N2E)
},
[S_DONT_N2E] = {
  .name="DONT_N2E",               // solid DON'T before traffic resumes
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_DONT,
  .next = NEXT8(S_E_G,S_E_G,S_E_G,S_E_G, S_E_G,S_E_G,S_E_G,S_E_G) // resume East cycle
},

/* ===== Pedestrian: E→N version ===== */
[S_WALK_E2N] = {
  .name="WALK_E2N",
  .out  = OUT_ALLRED | OUT_WALK,
  .t10ms = T_WALK,
  .next = NEXT8(S_HON1_E2N,S_HON1_E2N,S_HON1_E2N,S_HON1_E2N, S_HON1_E2N,S_HON1_E2N,S_HON1_E2N,S_HON1_E2N)
},
[S_HON1_E2N] = {
  .name="H1_ON_E2N",
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HOFF1_E2N,S_HOFF1_E2N,S_HOFF1_E2N,S_HOFF1_E2N, S_HOFF1_E2N,S_HOFF1_E2N,S_HOFF1_E2N,S_HOFF1_E2N)
},
[S_HOFF1_E2N] = {
  .name="H1_OFF_E2N",
  .out  = OUT_ALLRED,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HON2_E2N,S_HON2_E2N,S_HON2_E2N,S_HON2_E2N, S_HON2_E2N,S_HON2_E2N,S_HON2_E2N,S_HON2_E2N)
},
[S_HON2_E2N] = {
  .name="H2_ON_E2N",
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_HURRY,
  .next = NEXT8(S_HOFF2_E2N,S_HOFF2_E2N,S_HOFF2_E2N,S_HOFF2_E2N, S_HOFF2_E2N,S_HOFF2_E2N,S_HOFF2_E2N,S_HOFF2_E2N)
},
[S_HOFF2_E2N] = {
  .name="H2_OFF_E2N",
  .out  = OUT_ALLRED,
  .t10ms = T_HURRY,
  .next = NEXT8(S_DONT_E2N,S_DONT_E2N,S_DONT_E2N,S_DONT_E2N, S_DONT_E2N,S_DONT_E2N,S_DONT_E2N,S_DONT_E2N)
},
[S_DONT_E2N] = {
  .name="DONT_E2N",
  .out  = OUT_ALLRED | OUT_DONT,
  .t10ms = T_DONT,
  .next = NEXT8(S_N_G,S_N_G,S_N_G,S_N_G, S_N_G,S_N_G,S_N_G,S_N_G) // resume North cycle
},

/* ===== Confirm chains (hold WALK ≥ ~4*T_CF) =====
   If the user keeps W=1 through Conf* steps, we "latch" a request by
   jumping into the r* (walk-requested) set. If W drops, we bail back to the
   current green (no walk request latched).
   Special case: if W is confirmed AND N=E=0, we branch to a quick r*_Y so
   the system can enter WALK immediately (no cars to serve).
*/
[S_ConfN1] = {
  .name="ConfN1",
  .out  = OUT_N_G | OUT_E_R, // keep current outputs during confirm
  .t10ms = T_CF,
  .next = NEXT8(S_N_G,S_N_G,S_N_G,S_N_G,  S_ConfN2,S_ConfN2,S_ConfN2,S_ConfN2)
},
[S_ConfN2] = {
  .name="ConfN2",
  .out  = OUT_N_G | OUT_E_R,
  .t10ms = T_CF,
  .next = NEXT8(S_N_G,S_N_G,S_N_G,S_N_G,  S_ConfN3,S_ConfN3,S_ConfN3,S_ConfN3)
},
[S_ConfN3] = {
  .name="ConfN3",
  .out  = OUT_N_G | OUT_E_R,
  .t10ms = T_CF,
  .next = NEXT8(S_N_G,S_N_G,S_N_G,S_N_G,  S_ConfN4,S_ConfN4,S_ConfN4,S_ConfN4)
},
[S_ConfN4] = {
  .name="ConfN4",
  .out  = OUT_N_G | OUT_E_R,
  .t10ms = T_CF,
  // If W=0 (released) → abort back to N_G.
  // If W=1:
  //   - If no cars (N=0,E=0), jump to rN_Y so we WALK right away.
  //   - Else latch rN_G (finish current N phase, WALK at all-red).
  .next = NEXT8(
    /*W=0*/ /*N E*/ S_N_G, S_N_G, S_N_G, S_N_G,
    /*W=1*/ /*00*/  S_rN_Y, /*01*/ S_rN_G, /*10*/ S_rN_G, /*11*/ S_rN_G)
},

[S_ConfE1] = {
  .name="ConfE1",
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_CF,
  .next = NEXT8(S_E_G,S_E_G,S_E_G,S_E_G,  S_ConfE2,S_ConfE2,S_ConfE2,S_ConfE2)
},
[S_ConfE2] = {
  .name="ConfE2",
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_CF,
  .next = NEXT8(S_E_G,S_E_G,S_E_G,S_E_G,  S_ConfE3,S_ConfE3,S_ConfE3,S_ConfE3)
},
[S_ConfE3] = {
  .name="ConfE3",
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_CF,
  .next = NEXT8(S_E_G,S_E_G,S_E_G,S_E_G,  S_ConfE4,S_ConfE4,S_ConfE4,S_ConfE4)
},
[S_ConfE4] = {
  .name="ConfE4",
  .out  = OUT_E_G | OUT_N_R,
  .t10ms = T_CF,
  // If W=0 → abort back to E_G.
  // If W=1:
  //   - If no cars (N=0,E=0), jump to rE_Y so we WALK right away.
  //   - Else latch rE_G.
  .next = NEXT8(
    /*W=0*/ /*N E*/ S_E_G, S_E_G, S_E_G, S_E_G,
    /*W=1*/ /*00*/  S_rE_Y, /*01*/ S_rE_G, /*10*/ S_rE_G, /*11*/ S_rE_G)
},
};

/* ============== Small LCD helper so states print when they change ============== */
static inline void LCD_ShowState(const char *name) {
  LCD_Clear();
  LCD_OutString(name);
}

/* ================== MAIN ================== */
int main(void)
{
  /* HAL + clocks */
  HAL_Init();
  SystemClock_Config();

  /* GPIO + SPI init (from CubeMX) */
  MX_GPIO_Init();
  MX_SPI1_Init();

  /* LCD init (give it a moment to power up) */
  HAL_Delay(100);
  LCD_Init();
  LCD_Clear();
  LCD_OutString("Traffic Ctrl");

  /* Boot policy:
     If East sensor is already 1 at startup, begin with East green;
     else default to North green. (You can change this policy easily.)
  */
  uint8_t boot = ReadInputs3();
  uint8_t s = (boot & 0x01) ? S_E_G : S_N_G;

  /* Force LCD to update on first pass */
  uint8_t prev = 0xFF;

  while (1) {
    /* ----- Set outputs (LEDs via shift register) ----- */
    ApplyOutputs(FSM[s].out);

    /* ----- Update LCD only on state change ----- */
    if (prev != s) {
      LCD_ShowState(FSM[s].name);
      prev = s;
    }

    /* ----- Dwell in this state for t10ms * 10 ms ----- */
    for (uint16_t t = FSM[s].t10ms; t > 0; --t) {
      HAL_Delay(10);
    }

    /* ----- Compute next state using current inputs [W,N,E] ----- */
    s = FSM[s].next[ ReadInputs3() ];
  }
}

/* ================== CUBEMX-GENERATED FUNCTIONS ==================
   These match what CubeMX generated in your project, just kept here so this
   file builds on its own. If your project already has them identical, you
   can keep those.
*/
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) { Error_Handler(); }
}

/* SPI1: Master, 8-bit, CPOL=0, CPHA=1Edge, MSB-first, prescaler /32 is plenty */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) { Error_Handler(); }
}

/* GPIO directions that match our wiring:
   - PA0..PA2 = inputs (WALK, N, E)
   - PA8, PA9 = LCD control (RS, E)
   - PC0..PC3 = LCD data (D4..D7)
   - PB12     = 74HC595 latch (RCLK)
   - SPI1 takes care of PA5 (SCK) and PA7 (MOSI) alt-function setup
*/
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* ---- Default output levels ---- */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);         // LCD RS/E low
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET); // LCD D4..D7 low
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);                   // 595 latch low

  /* ---- Inputs: PA0 (WALK), PA1 (N), PA2 (E) ---- */
  // PA0 with internal pulldown (button to +3.3V gives 1)
  GPIO_InitStruct.Pin  = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // PA1, PA2 (external pulldowns as per your wiring)
  GPIO_InitStruct.Pin  = GPIO_PIN_1 | GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* ---- LCD control: PA8 (RS), PA9 (E) ---- */
  GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* ---- LCD data: PC0..PC3 = D4..D7 ---- */
  GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* ---- 74HC595 Latch: PB12 ---- */
  GPIO_InitStruct.Pin   = GPIO_PIN_12;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* NOTE:
     - SPI1 pins (PA5=SCK, PA7=MOSI) are configured by CubeMX
       as AF (alternate function) when MX_SPI1_Init() runs.
     - We do NOT use PB0..PB7 anymore for LEDs (shift register does that).
  */
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file; (void)line;
}
#endif
