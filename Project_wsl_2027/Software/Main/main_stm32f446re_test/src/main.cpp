#include <Arduino.h>
#include "action/offence.hpp"
#include "action/defence.hpp"

#include "sensor/serial_packet.hpp"
#include "sensor/bno.hpp"
#include "sensor/button.hpp"
#include "sensor/m5_module.hpp"

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

uint8_t red_led_pin = PB5;
uint8_t yellow_led_pin = PA11;
uint8_t green_led_pin = PC8;

void setup()
{
  uiInit();

  left_btn.begin(PC11, INPUT_PULLDOWN);
  right_btn.begin(PB15, INPUT_PULLDOWN);

  pinMode(red_led_pin, OUTPUT);
  pinMode(yellow_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(toggle_pin, INPUT_PULLDOWN);

  mySerial2.begin(115200);
}

void loop()
{
  // ボタン更新
  left_btn.update();
  right_btn.update();

  // 動作
  digitalWrite(yellow_led_pin, LOW);
  digitalWrite(green_led_pin, LOW);

  if (action_run)
  {
    digitalWrite(yellow_led_pin, HIGH);

    switch (ui_state)
    {
    case ACTION_OFFENCE:
      break;
    case ACTION_DEFENCE:
      break;
    case ACTION_RADIOCONTROL:
      break;
    }

    if (left_btn.isPushing() || right_btn.isPushing())
    {
      digitalWrite(green_led_pin, HIGH);
    }
  }
  else if (ui_state == HOME)
  {
  }
  else
  {
    digitalWrite(yellow_led_pin, HIGH);

    switch (ui_state)
    {
    case TEST_KICKER:
      if (testkicker_btn)
      {
        digitalWrite(green_led_pin, HIGH);
      }
      break;
    case TEST_DRIBBLER:
      if (testdribbler_toggle)
      {
        digitalWrite(green_led_pin, HIGH);
      }
      break;
    case TEST_MOTOR:
      if (testmotor_toggle)
      {
        digitalWrite(green_led_pin, HIGH);
      }
      break;
    }
  }
}