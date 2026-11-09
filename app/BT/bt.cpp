#include "bt_public.h"
#include "bt_config.h"
#include "bt_protocol.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <cstring>

namespace {

uint8_t bt_rx_buf[BT_RX_BUF_SIZE];
uint8_t bt_tx_buf[BT_FRAME_MAX];
QueueHandle_t bt_queue = nullptr;

void start_receive()
{
  HAL_UARTEx_ReceiveToIdle_DMA(&huart6, bt_rx_buf, sizeof(bt_rx_buf));
  /* HAL 启动时会重新使能 HT 中断，每次重启后都要关掉 */
  __HAL_DMA_DISABLE_IT(huart6.hdmarx, DMA_IT_HT);
}

}

extern "C" HAL_StatusTypeDef BT_Send(const uint8_t *data, uint16_t len)
{
  if (data == nullptr || len == 0U || len > static_cast<uint16_t>(sizeof(bt_tx_buf))) return HAL_ERROR;
  if (huart6.gState != HAL_UART_STATE_READY) return HAL_BUSY;
  if ((huart6.hdmatx->Instance->CR & DMA_SxCR_EN) != 0U) return HAL_BUSY;
  /* 确认可发送后才写缓冲，避免覆盖上一帧尚未发完的数据 */
  std::memcpy(bt_tx_buf, data, len);
  return HAL_UART_Transmit_DMA(&huart6, bt_tx_buf, len);
}

extern "C" HAL_StatusTypeDef BT_SetBaudRate(uint32_t baudrate)
{
  if (baudrate < 1200U || baudrate > 5000000U) return HAL_ERROR;
  if (huart6.Init.BaudRate == baudrate) return HAL_OK;

  HAL_UART_DeInit(&huart6);
  huart6.Init.BaudRate = baudrate;
  if (HAL_UART_Init(&huart6) != HAL_OK) return HAL_ERROR;
  start_receive();
  return HAL_OK;
}

extern "C" void StartBTTask(void *argument)
{
  (void)argument;
  bt_queue = xQueueCreate(BT_QUEUE_LEN, sizeof(BtFrame));
  BtProtocol &protocol = BT_GetProtocol();
  BtFrame frame{};

  if (huart6.Init.BaudRate != BT_DEFAULT_BAUDRATE)
    BT_SetBaudRate(BT_DEFAULT_BAUDRATE);
  else
    start_receive();

  for (;;)
  {
    if (xQueueReceive(bt_queue, &frame, portMAX_DELAY) == pdTRUE)
    {
      if (protocol.parse(frame))
      {
        const uint16_t n = protocol.make_reply(bt_tx_buf, sizeof(bt_tx_buf));
        if (n > 0U) BT_Send(bt_tx_buf, n);
      }
    }
  }
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance != USART6) return;

  BaseType_t woken = pdFALSE;
  if (bt_queue != nullptr && Size > 0U && Size <= BT_FRAME_MAX)
  {
    BtFrame frame{};
    frame.len = Size;
    std::memcpy(frame.data, bt_rx_buf, Size);
    xQueueSendToBackFromISR(bt_queue, &frame, &woken);
  }

  std::memset(bt_rx_buf, 0, sizeof(bt_rx_buf));
  start_receive();
  portYIELD_FROM_ISR(woken);
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART6) return;
  __HAL_UNLOCK(huart);
  start_receive();
}
