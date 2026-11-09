#ifndef __BT_PROTOCOL_H
#define __BT_PROTOCOL_H

#include <stdint.h>
#include "bt_config.h"

/* 队列元素：中断里从 DMA 缓冲拷贝出来，避免被下一次接收覆写 */
struct BtFrame
{
  uint16_t len;
  uint8_t  data[BT_FRAME_MAX];
};

/* 协议层接口：蓝牙调试与树莓派通信各提供一个实现，由 CMake 选编 */
class BtProtocol
{
public:
  virtual bool parse(const BtFrame &frame) = 0;
  virtual uint16_t make_reply(uint8_t *out, uint16_t capacity) = 0;
};

/* 由 bt_proto_debug.cpp / bt_proto_rpi.cpp 之一实现 */
BtProtocol &BT_GetProtocol();

#endif /* __BT_PROTOCOL_H */
