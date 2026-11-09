/**
 * @file imu_task.c
 * @brief BMI088 polling bring-up + periodic sample (smoke / live data).
 *
 * On init failure: blink blue LED rapidly via BoardLed (best-effort).
 * Samples land in g_bmi088 for debugger watch / future Bridge export.
 */
#include "imu_task.h"
#include "bmi088.h"
#include "board_led.h"
#include "cmsis_os.h"

bmi088_t g_bmi088;
volatile uint8_t g_bmi088_last_err;
volatile uint32_t g_bmi088_ok_count;

void StartImuTask(void *argument)
{
  bmi088_err_t err;
  (void)argument;

  BMI088_StructInit(&g_bmi088);

  for (;;) {
    err = BMI088_Init(&g_bmi088);
    g_bmi088_last_err = (uint8_t)err;
    if (err == BMI088_OK) {
      break;
    }
    /* Fail pulse: blue toggle while retrying */
    BoardLed_Toggle(BOARD_LED_B);
    osDelay(200);
  }

  for (;;) {
    BMI088_Read(&g_bmi088);
    g_bmi088_ok_count++;
    osDelay(10); /* ~100 Hz polling */
  }
}
