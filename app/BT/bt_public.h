#ifndef __BT_PUBLIC_H
#define __BT_PUBLIC_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FreeRTOS 任务入口，由 freertos.c 创建 */
void StartBTTask(void *argument);

/* 发送一帧；数据会拷贝进模块内发送缓冲，调用者无需保持有效 */
HAL_StatusTypeDef BT_Send(const uint8_t *data, uint16_t len);

/* 运行时修改串口6波特率并重启接收 */
HAL_StatusTypeDef BT_SetBaudRate(uint32_t baudrate);

#ifdef __cplusplus
}
#endif

#endif /* __BT_PUBLIC_H */
