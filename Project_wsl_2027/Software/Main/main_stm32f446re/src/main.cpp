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
#include "module/camera.hpp"
#include "module/lidar.hpp"
#include "module/line.hpp"
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

HardwareTimer *myTim1;
void tim1Callback()
{
  reset_btn.update();
  sub_btn.update();
  action_toggle.update();
  sub1_toggle.update();
  sub2_toggle.update();
  sub3_toggle.update();

  /*
  mySerial1.print("gyro:");
  mySerial1.print(gyro.deg());
  mySerial1.print(" posi:");
  mySerial1.print(lidar::posi_x);
  mySerial1.print(",");
  mySerial1.println(lidar::posi_y);
  */
}

HardwareTimer *myTim2;
void tim2Callback()
{
  // bno更新
  gyro.update(reset_btn.isPushing());

  // ui更新
  ui::process(action_toggle.isTurnedOn());
  // line更新
  line::process();
  // camera更新
  camera::process(ui::ACTION::meter_type);
  // lidar更新
  lidar::process((int16_t)gyro.deg());
}

void setup()
{
  // gyro
  gyro.begin(Wire3, 0x28);

  // pc
  mySerial1.begin(115200);
  // ui
  mySerial3.begin(115200);
  ui::attach(mySerial3);
  // line
  mySerial4.begin(115200);
  line::attach(mySerial4);
  // camera
  mySerial2.begin(115200);
  camera::attach(mySerial2);
  // lidar
  mySerial6.begin(115200);
  lidar::attach(mySerial6);

  // btn
  reset_btn.begin(PB15, INPUT_PULLDOWN);
  sub_btn.begin(PC8, INPUT_PULLDOWN);
  // toggle
  action_toggle.begin(PC2, INPUT_PULLDOWN);
  sub1_toggle.begin(PA15, INPUT_PULLDOWN);
  sub2_toggle.begin(PC3, INPUT_PULLDOWN);
  sub3_toggle.begin(PC4, INPUT_PULLDOWN);

  // Tim1
  myTim1 = new HardwareTimer(TIM1);
  myTim1->setOverflow(5, HERTZ_FORMAT); // 200ms
  myTim1->attachInterrupt(tim1Callback);
  myTim1->resume();
  // Tim2
  myTim2 = new HardwareTimer(TIM2);
  myTim2->setOverflow(100, HERTZ_FORMAT); // 10ms
  myTim2->attachInterrupt(tim2Callback);
  myTim2->resume();
}

void loop()
{
  // action実行
  if (ui::ACTION::run)
  {
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
}