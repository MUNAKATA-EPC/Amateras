#include <Arduino.h>
// common
#include "common/serial_packet.hpp"
#include "common/bus_instance.hpp"
#include "common/const_number.hpp"
// device
#include "device/bno.hpp"
#include "device/button.hpp"
#include "device/cyclic_timer.hpp"
#include "device/led.hpp"
#include "device/toggle.hpp"

extern "C" void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while (1)
      ;
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    while (1)
      ;
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    while (1)
      ;
  }
}

struct t_data
{
  int16_t ball_deg = UNDETECTED;
  int16_t ball_dis = UNDETECTED;
  int16_t yellow_goal_deg = UNDETECTED;
  int16_t yellow_goal_dis = UNDETECTED;
  int16_t blue_goal_deg = UNDETECTED;
  int16_t blue_goal_dis = UNDETECTED;
} __attribute__((packed));
struct r_data
{
} __attribute__((packed));
serial_packet<t_data, r_data> packet;

struct t_data_from_cam
{
} __attribute__((packed));
struct r_data_from_cam
{
  int16_t obj_x = 0, obj_y = 0;
} __attribute__((packed));
serial_packet<t_data_from_cam, r_data_from_cam> camera_packet[4]; // {front,right,back,left}

void setup()
{
  // pc
  mySerial1.begin(115200);
  // stm
  mySerial6.begin(115200);
  packet.begin(mySerial6);
  // cam
  mySerial4.begin(115200);
  camera_packet[0].begin(mySerial4);

  mySerial2.begin(115200);
  camera_packet[1].begin(mySerial2);

  mySerial5.begin(115200);
  camera_packet[2].begin(mySerial5);

  mySerial3.begin(115200);
  camera_packet[3].begin(mySerial3);

  // timer
  cyclic_timer_1ms.begin(TIM9, 1);     // 1ms周期
  cyclic_timer_10ms.begin(TIM6, 10);   // 10ms周期
  cyclic_timer_200ms.begin(TIM7, 200); // 200ms周期
}

void loop()
{
  // 200ms周期
  if (cyclic_timer_200ms.called())
  {
    mySerial1.print("ff");
  }

  // 10ms周期
  if (cyclic_timer_10ms.called())
  {
    // cam更新
    camera_packet[0].update();
    camera_packet[1].update();
    camera_packet[2].update();
    camera_packet[3].update();

    // stm更新
    packet.update();
  }
}