#include <Arduino.h>
// action
#include "action/offence.hpp"
#include "action/defence.hpp"
// common
#include "common/serial_packet.hpp"
#include "common/bus_instance.hpp"
// device
#include "device/bno.hpp"
#include "device/button.hpp"
#include "device/led.hpp"
#include "device/toggle.hpp"
// module
#include "module/ui.hpp"

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

void setup()
{
  // m5
  mySerial3.begin(115200);
  ui::attach(mySerial3);

  // pc
  mySerial2.begin(115200);

  // btn
  left_btn.begin(PC11, INPUT_PULLDOWN);
  right_btn.begin(PB15, INPUT_PULLDOWN);

  // toggle
  action_toggle.begin(PB14, INPUT_PULLDOWN);
  // led
  red_led.begin(PB5);
  yellow_led.begin(PA11);
  green_led.begin(PC8);
}

void loop()
{
  // btn更新
  left_btn.update();
  right_btn.update();
  // toggle更新
  action_toggle.update();

  // ui更新
  ui::process(action_toggle.isTurnedOn());

  // led初期化
  if (ui::cur_state == ui::STATE::HOME)
    red_led.lightdown();
  else
    red_led.lightup();
  yellow_led.lightdown();
  green_led.lightdown();

  if (ui::ACTION::run)
  {
    green_led.lightup();

    switch (ui::cur_state)
    {
    case ui::STATE::ACTION_OFFENCE:
      offence();
      break;
    case ui::STATE::ACTION_DEFENCE:
      defence();
      break;
    case ui::STATE::ACTION_RADIOCONTROL:
      break;
    }
  }
  else
  {
    switch (ui::cur_state)
    {
    case ui::STATE::HOME:
      break;
    case ui::STATE::TEST_KICKER:
      if (ui::TEST_KICKER::btn)
        green_led.lightup();
      if (ui::TEST_KICKER::front)
        yellow_led.lightup();
      break;
    case ui::STATE::TEST_DRIBBLER:
      if (ui::TEST_DRIBBLER::toggle)
        green_led.lightup();
      if (ui::TEST_DRIBBLER::front)
        yellow_led.lightup();
      break;
    case ui::STATE::TEST_MOTOR:
      if (ui::TEST_MOTOR::toggle)
        green_led.lightup();
      if (left_btn.isPushing() || right_btn.isPushing())
        yellow_led.lightup();
      break;
    case ui::STATE::SENSORMONITOR_BALL:
      break;
    case ui::STATE::SENSORMONITOR_LINE:
      break;
    case ui::STATE::SENSORMONITOR_GYRO:
      break;
    case ui::STATE::SENSORMONITOR_GOAL:
      break;
    case ui::STATE::SENSORMONITOR_LIDAR:
      break;
    case ui::STATE::COMMUNICATION_TRANSMIT:
      break;
    case ui::STATE::COMMUNICATION_RECEIVE:
      break;
    }
  }

  delay(50);
}