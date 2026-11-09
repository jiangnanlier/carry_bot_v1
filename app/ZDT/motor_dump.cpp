#include "motor_dump.h"
#include "zdt_config.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifndef MOTOR_DUMP_ASCII
#define MOTOR_DUMP_ASCII 1
#endif

namespace {

constexpr uint16_t kLineMax = 96U;

bool uart1_tx_ready()
{
  if (huart1.gState != HAL_UART_STATE_READY) return false;
  if (huart1.hdmatx != nullptr && (huart1.hdmatx->Instance->CR & DMA_SxCR_EN) != 0U)
    return false;
  return true;
}

bool wait_tx_ready(uint32_t timeout_ms)
{
  const TickType_t t0 = xTaskGetTickCount();
  while (!uart1_tx_ready())
  {
    if ((xTaskGetTickCount() - t0) >= pdMS_TO_TICKS(timeout_ms)) return false;
    vTaskDelay(1);
  }
  return true;
}

void send_line(const char *buf, uint16_t len)
{
  if (len == 0U) return;
  if (!wait_tx_ready(30U)) return;
  (void)HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(const_cast<char *>(buf)), len, 40);
}

} // namespace

extern "C" void MotorDump_Printf(const char *fmt, ...)
{
#if !MOTOR_DUMP_ASCII
  (void)fmt;
  return;
#else
  if (fmt == nullptr) return;
  char buf[kLineMax];
  buf[0] = '#';
  buf[1] = ' ';
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf + 2, sizeof(buf) - 4U, fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  uint16_t len = static_cast<uint16_t>(2 + ((n < static_cast<int>(sizeof(buf) - 4U)) ? n : static_cast<int>(sizeof(buf) - 4U)));
  buf[len++] = '\r';
  buf[len++] = '\n';
  send_line(buf, len);
#endif
}

extern "C" void MotorDump_Hex(const char *tag, const uint8_t *data, uint16_t len)
{
#if !MOTOR_DUMP_ASCII
  (void)tag; (void)data; (void)len;
  return;
#else
  char hex[72];
  uint16_t pos = 0U;
  const uint16_t n = (len > 16U) ? 16U : len;
  static const char kHex[] = "0123456789ABCDEF";
  for (uint16_t i = 0U; i < n; ++i)
  {
    if (pos + 3U >= sizeof(hex)) break;
    if (i > 0U) hex[pos++] = ' ';
    hex[pos++] = kHex[(data[i] >> 4) & 0x0FU];
    hex[pos++] = kHex[data[i] & 0x0FU];
  }
  hex[pos] = '\0';
  if (len > n)
    MotorDump_Printf("%s n=%u %s ...", tag ? tag : "HEX", static_cast<unsigned>(len), hex);
  else
    MotorDump_Printf("%s n=%u %s", tag ? tag : "HEX", static_cast<unsigned>(len), hex);
#endif
}

extern "C" void MotorDump_BootBanner(void)
{
  MotorDump_Printf("dump ready 串口2@115200 (PA9) MOTOR_DUMP=%d", MOTOR_DUMP_ASCII);
  MotorDump_Printf("listen TXD; 串口1@9600 Bridge cmds; drivers may ignore # lines");
}
