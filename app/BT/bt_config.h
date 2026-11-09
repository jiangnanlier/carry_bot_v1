#ifndef __BT_CONFIG_H
#define __BT_CONFIG_H

/* BT_LINK_DEBUG / BT_LINK_RPI 由 app/BT/CMakeLists.txt 的 BT_LINK_MODE 定义 */
#if defined(BT_LINK_RPI)
#define BT_DEFAULT_BAUDRATE   115200U
#else
#define BT_DEFAULT_BAUDRATE   9600U
#endif

#define BT_RX_BUF_SIZE        1024U             /* DMA 接收缓冲区 */
#define BT_FRAME_MAX          256U              /* 单帧最大字节数 */
#define BT_QUEUE_LEN          4U                /* 接收队列深度 */

/* 以下两项仅供 Core/Src/freertos.c 使用（需先包含 cmsis_os.h） */
#define BT_TASK_STACK_SIZE    (256U * 4U)
#define BT_TASK_PRIORITY      osPriorityNormal

#endif /* __BT_CONFIG_H */
