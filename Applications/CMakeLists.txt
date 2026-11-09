# Applications — FreeRTOS tasks (board IO + IMU)
add_library(applications STATIC
  Src/board_task.c
  Src/imu_task.c
)

target_include_directories(applications PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_link_libraries(applications PUBLIC
  stm32cubemx
  bsp_board
  userlib_bmi088
)
