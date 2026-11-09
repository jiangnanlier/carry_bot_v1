#ifndef MOTOR_DUMP_H
#define MOTOR_DUMP_H

#include <stdint.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Non-blocking dump mirror on 串口2 (USART1 TX @115200, PA9).
 * Enqueue-only APIs never touch the UART. MotorDump_Poll() drains
 * short HEX lines when TX is idle (call from ZDT task only). */

void MotorDump_Init(void);
void MotorDump_Printf(const char *fmt, ...);
void MotorDump_Hex(const char *tag, const uint8_t *data, uint16_t len);
void MotorDump_TxHex(const uint8_t *data, uint16_t len); /* ">aa bb.." */
void MotorDump_RxHex(const uint8_t *data, uint16_t len); /* "<aa bb.." */
void MotorDump_BootBanner(void);
void MotorDump_Poll(void);
/* Hold TX mutex + wait idle before ZDT DMA; unlock right after DMA start. */
HAL_StatusTypeDef MotorDump_PrepareBinaryTx(void);
void MotorDump_FinishBinaryTx(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_DUMP_H */
