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

HardwareSerial mySerial1(PA10, PA9); // ピンは外には出ていない // パソコンとの通信用
HardwareSerial mySerial3(PC5, PB10); // 動作確認済み
HardwareSerial mySerial5(PD2, PC12); // 動作確認済み
HardwareSerial mySerial4(PA1, PA0);  // 動作確認済み
HardwareSerial mySerial2(PA3, PA2);  // 動作確認済み // メインとの通信用
HardwareSerial mySerial6(PC7, PC6);  // 動作確認済み

TwoWire Wire1(PB9, PB8);
TwoWire Wire3(PC9, PA8);

const uint8_t led_pin = PA5;
const uint8_t vref_pin = PA4;

const uint8_t left_side_pin = PA6;
const uint8_t right_side_pin = PA7;

const uint8_t angel_pins[] = {
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
    PB8, PB9, PB10,
    PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7,
    PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
    PA0};

struct t_data
{
  uint32_t angel = 0UL;
  int16_t right_side_val = 0;
  int16_t left_side_val = 0;
};
struct r_data
{
};
serial_packet<t_data, r_data> packet(20);

void setup()
{
  mySerial1.begin(115200);

  mySerial2.begin(115200);
  packet.begin(mySerial2);

  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);

  analogWriteResolution(12);
  analogWrite(vref_pin, 2048);

  pinMode(left_side_pin, INPUT);
  pinMode(right_side_pin, INPUT);
  for (int i = 0; i < 32; i++)
    pinMode(angel_pins[i], INPUT);
}

void loop()
{
  uint32_t reg_a = GPIOA->IDR;
  uint32_t reg_b = GPIOB->IDR;
  uint32_t reg_c = GPIOC->IDR;

  uint32_t angel_bit = 0;
  angel_bit |= (reg_b & 0x07FF);         // bit 0 ~ 10 はそのまま (配列の 0~10 番目)
  angel_bit |= ((reg_b & 0xF000) >> 1);  // bit 12 ~ 15 は 1 ビット右シフトして詰める (配列の 11~14 番目へ)
  angel_bit |= ((reg_c & 0xFFFF) << 15); // bit 0 ~ 15 を 15 ビット左シフトして持ち上げる (配列の 15~30 番目へ)
  angel_bit |= ((reg_a & 0x0001) << 31); // bit 0 を 31 ビット左シフトして最上位へ (配列の 31 番目へ)

  packet.tx.angel = angel_bit;
  packet.tx.left_side_val = analogRead(left_side_pin);
  packet.tx.right_side_val = analogRead(right_side_pin);

  packet.update();

  static uint32_t _last_tx_time = millis();
  if (millis() - _last_tx_time >= 20)
  {
    _last_tx_time = millis();
    mySerial1.print("angel: ");
    mySerial1.print(packet.tx.angel, BIN);
    mySerial1.print(", left_side_val: ");
    mySerial1.print(packet.tx.left_side_val);
    mySerial1.print(", right_side_val: ");
    mySerial1.println(packet.tx.right_side_val);
  }
}