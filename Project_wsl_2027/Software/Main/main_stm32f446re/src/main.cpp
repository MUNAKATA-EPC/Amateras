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
#include "device/cyclic_timer.hpp"
#include "device/led.hpp"
#include "device/toggle.hpp"
// module
#include "module/camera.hpp"
#include "module/lidar.hpp"
#include "module/line.hpp"
#include "module/motordriver.hpp"
#include "module/ps3.hpp"
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
  // resolution
  analogWriteResolution(12);

  // gyro
  gyro.begin(Wire3, 0x28);

  // pc
  mySerial1.begin(115200);
  // ui
  mySerial3.begin(115200);
  ui::attach(mySerial3);
  // ps3
  mySerial5.begin(115200);
  ps3::attach(mySerial5);
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

  // motordriver // {right_front, right_back, left_back, left_front} // 全て正1000で右回転
  motordriver::attach({PB6, PB7}, {PA6, PA7}, {PB8, PB9}, {PB0, PB1});
  motordriver::move(0, 0, 0, 0);

  cyclic_timer_1ms.begin(TIM9, 1);     // 1ms周期
  cyclic_timer_10ms.begin(TIM2, 10);   // 10ms周期
  cyclic_timer_200ms.begin(TIM1, 200); // 200ms周期
}

void loop()
{
  // 200ms周期
  if (cyclic_timer_200ms.called())
  {
    // btn更新
    reset_btn.update();
    sub_btn.update();

    // toggle更新
    action_toggle.update();
    sub1_toggle.update();
    sub2_toggle.update();
    sub3_toggle.update();

    mySerial1.print("gyro:");
    mySerial1.print(gyro.deg());
    mySerial1.print(" posi:");
    mySerial1.print(lidar::posi_x);
    mySerial1.print(",");
    mySerial1.println(lidar::posi_y);
  }

  // 10ms周期
  if (cyclic_timer_10ms.called())
  {
    // bno更新
    gyro.update(reset_btn.isPushing());

    // ui更新
    ui::TRANSMIT_DATA::gyro_deg = (int16_t)gyro.deg();
    ui::process(action_toggle.isTurnedOn());
    // ps3更新
    ps3::process();
    // line更新
    line::process();
    // camera更新
    camera::process(ui::ACTION::meter_type);
    // lidar更新
    lidar::process((int16_t)gyro.deg());
  }

  // 1ms周期
  if (cyclic_timer_1ms.called())
  {
    motordriver::process();
  }

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
      motordriver::move(400, 400, 400, 400);
      break;
    default:
      motordriver::move(0, 0, 0, 0);
    }
  }
  else
  {
    motordriver::move(0, 0, 0, 0);
  }
}