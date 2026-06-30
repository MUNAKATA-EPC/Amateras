#include <Arduino.h>
#include "sensor/serial_packet.hpp"
#include "sensor/bno.hpp"

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

HardwareSerial mySerial1(PA10, PA9); // ピンは外には出ていない
HardwareSerial mySerial3(PC5, PB10); // 動作確認済み
HardwareSerial mySerial5(PD2, PC12); // 動作確認済み
HardwareSerial mySerial4(PA1, PA0);  // 動作確認済み
HardwareSerial mySerial2(PA3, PA2);  // 動作確認済み
HardwareSerial mySerial6(PC7, PC6);  // 動作確認済み

TwoWire Wire1(PB9, PB8);
TwoWire Wire3(PC9, PA8);

struct t_data
{
  int16_t gyro_deg = 0;
  int16_t ball_deg = 0;
};
struct r_data
{
};
serial_packet<t_data, r_data> packet(20);

bno gyro(20);

void setup()
{
  gyro.begin(&Wire1, 0x29);

  mySerial4.begin(115200);
  packet.begin(mySerial4);

  mySerial1.begin(115200);
}

void loop()
{
  gyro.update(false);
  packet.tx.gyro_deg = static_cast<int16_t>(gyro.deg());
  packet.tx.ball_deg = 10;

  packet.update();

  mySerial1.println("aa");
}