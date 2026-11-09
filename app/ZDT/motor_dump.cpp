#include "motor_dump.h"
#include "zdt_config.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifndef MOTOR_DUMP_ASCII
#define MOTOR_DUMP_ASCII 1
#endif

namespace {

constexpr uint16_t kLineMax = 48U;
constexpr uint8_t kRingDepth = 24U;

struct Line {
  uint8_t len;
  char buf[kLineMax];
};

Line ring[kRingDepth];
volatile uint8_t head = 0U;
volatile uint8_t tail = 0U;
SemaphoreHandle_t tx_mu = nullptr;
bool draining = false;

bool uart1_tx_idle()
{
  if (huart1.gState != HAL_UART_STATE_READY) return false;
  if (huart1.hdmatx != nullptr && (huart1.hdmatx->Instance->CR & DMA_SxCR_EN) != 0U)
    return false;
  return true;
}

bool ring_empty() { return head == tail; }

bool ring_full() { return static_cast<uint8_t>((head + 1U) % kRingDepth) == tail; }

void enqueue_raw(const char *s, uint16_t n)
{
#if !MOTOR_DUMP_ASCII
  (void)s; (void)n;
  return;
#else
  if (s == nullptr || n == 0U) return;
  if (n >= kLineMax) n = kLineMax - 1U;
  taskENTER_CRITICAL();
  if (!ring_full())
  {
    Line &L = ring[head];
    std::memcpy(L.buf, s, n);
    L.buf[n] = '\0';
    L.len = static_cast<uint8_t>(n);
    head = static_cast<uint8_t>((head + 1U) % kRingDepth);
  }
  taskEXIT_CRITICAL();
#endif
}

void format_hex_line(char *out, uint16_t cap, char prefix, const uint8_t *data, uint16_t len)
{
  if (out == nullptr || cap < 4U) return;
  uint16_t pos = 0U;
  out[pos++] = prefix;
  const uint16_t n = (len > 12U) ? 12U : len;
  static const char kHex[] = "0123456789ABCDEF";
  for (uint16_t i = 0U; i < n; ++i)
  {
    if (pos + 3U >= cap) break;
    out[pos++] = ' ';
    out[pos++] = kHex[(data[i] >> 4) & 0x0FU];
    out[pos++] = kHex[data[i] & 0x0FU];
  }
  if (len > n && pos + 4U < cap)
  {
    out[pos++] = ' ';
    out[pos++] = '.';
    out[pos++] = '.';
    out[pos++] = '.';
  }
  if (pos + 2U < cap)
  {
    out[pos++] = '\r';
    out[pos++] = '\n';
  }
  out[pos] = '\0';
  enqueue_raw(out, pos);
}

} // namespace

extern "C" void MotorDump_Init(void)
{
  if (tx_mu == nullptr)
    tx_mu = xSemaphoreCreateMutex();
  head = 0U;
  tail = 0U;
  draining = false;
}

extern "C" void MotorDump_Printf(const char *fmt, ...)
{
#if !MOTOR_DUMP_ASCII
  (void)fmt;
  return;
#else
  if (fmt == nullptr) return;
  char buf[kLineMax];
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf, sizeof(buf) - 2U, fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  uint16_t len = static_cast<uint16_t>((n < static_cast<int>(sizeof(buf) - 2U)) ? n : static_cast<int>(sizeof(buf) - 2U));
  buf[len++] = '\r';
  buf[len++] = '\n';
  enqueue_raw(buf, len);
#endif
}

extern "C" void MotorDump_Hex(const char *tag, const uint8_t *data, uint16_t len)
{
  /* Prefer TxHex/RxHex; keep tag path as short enqueue. */
  char prefix = '?';
  if (tag != nullptr)
  {
    if (tag[0] == 'T' || tag[0] == 't') prefix = '>';
    else if (tag[0] == 'R' || tag[0] == 'r') prefix = '<';
  }
  char line[kLineMax];
  format_hex_line(line, sizeof(line), prefix, data, len);
}

extern "C" void MotorDump_TxHex(const uint8_t *data, uint16_t len)
{
  char line[kLineMax];
  format_hex_line(line, sizeof(line), '>', data, len);
}

extern "C" void MotorDump_RxHex(const uint8_t *data, uint16_t len)
{
  char line[kLineMax];
  format_hex_line(line, sizeof(line), '<', data, len);
}

extern "C" void MotorDump_BootBanner(void)
{
  MotorDump_Printf(">RDY dump");
}

extern "C" void MotorDump_Poll(void)
{
#if !MOTOR_DUMP_ASCII
  return;
#else
  if (ring_empty()) return;
  if (!uart1_tx_idle()) return;
  if (tx_mu != nullptr && xSemaphoreTake(tx_mu, 0) != pdTRUE) return;
  if (!uart1_tx_idle() || ring_empty())
  {
    if (tx_mu != nullptr) (void)xSemaphoreGive(tx_mu);
    return;
  }

  Line L;
  taskENTER_CRITICAL();
  L = ring[tail];
  tail = static_cast<uint8_t>((tail + 1U) % kRingDepth);
  taskEXIT_CRITICAL();

  /* Short line only (~3ms @115200). Never called from Bridge path. */
  (void)HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(L.buf), L.len, 10);
  if (tx_mu != nullptr) (void)xSemaphoreGive(tx_mu);
#endif
}

extern "C" HAL_StatusTypeDef MotorDump_PrepareBinaryTx(void)
{
  if (tx_mu != nullptr)
  {
    if (xSemaphoreTake(tx_mu, pdMS_TO_TICKS(40)) != pdTRUE) return HAL_BUSY;
  }
  const TickType_t t0 = xTaskGetTickCount();
  while (!uart1_tx_idle())
  {
    if ((xTaskGetTickCount() - t0) >= pdMS_TO_TICKS(40))
    {
      if (tx_mu != nullptr) (void)xSemaphoreGive(tx_mu);
      return HAL_BUSY;
    }
    vTaskDelay(1);
  }
  return HAL_OK;
}

extern "C" void MotorDump_FinishBinaryTx(void)
{
  if (tx_mu != nullptr) (void)xSemaphoreGive(tx_mu);
}
