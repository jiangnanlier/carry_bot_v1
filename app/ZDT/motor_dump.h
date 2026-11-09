#ifndef MOTOR_DUMP_H
#define MOTOR_DUMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Short ASCII mirror on 串口2 (USART1 TX / PA9 @115200).
 * Lines start with "# " so Emm drivers usually ignore them as non-addr frames.
 * Set MOTOR_DUMP_ASCII 0 in zdt_config.h to disable. */

void MotorDump_Printf(const char *fmt, ...);
void MotorDump_Hex(const char *tag, const uint8_t *data, uint16_t len);
void MotorDump_BootBanner(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_DUMP_H */
