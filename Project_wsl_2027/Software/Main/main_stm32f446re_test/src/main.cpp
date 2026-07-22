#include <Arduino.h>
#include "sensor/serial_packet.hpp"
#include "sensor/bno.hpp"
#include "sensor/button.hpp"

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

HardwareSerial mySerial1(PA10, PA9);
HardwareSerial mySerial3(PC5, PB10);
HardwareSerial mySerial5(PD2, PC12);
HardwareSerial mySerial4(PA1, PA0);
HardwareSerial mySerial2(PA3, PA2);
HardwareSerial mySerial6(PC7, PC6);

TwoWire Wire1(PB9, PB8);
TwoWire Wire3(PC9, PA8);

enum UI_STATE : uint8_t
{
  HOME,
  ACTION_OFFENSE,
  ACTION_DEFENSE,
  ACTION_RADIOCONTROL,
  TEST_KICKER,
  TEST_DRIBBLER,
  TEST_MOTOR,
  SENSORMONITOR_BALL,
  SENSORMONITOR_LINE,
  SENSORMONITOR_GYRO,
  SENSORMONITOR_GOAL,
  SENSORMONITOR_LIDAR,
  COMMUNICATION_TRANSMIT,
  COMMUNICATION_RECEIVE
};
enum UI_STATE ui_state = HOME;

struct t_data
{
  bool action_run = false;
} __attribute__((packed));

struct r_data
{
  bool action_run = false;
  int action_meter_type = 0;
  UI_STATE ui_state = HOME;
  bool testkicker_btn = false;
  bool testkicker_front = false;
  bool testdribbler_toggle = false;
  bool testdribbler_front = false;
  bool testmotor_toggle = false;
  int testmotor_meter_type = 0;
} __attribute__((packed));

struct action_run_t_data
{
  bool action_run = false;
  int16_t my_posi_x = 0;
  int16_t my_posi_y = 0;
} __attribute__((packed));

struct action_run_r_data
{
  bool action_run = false;
  int16_t my_posi_x = 0;
  int16_t my_posi_y = 0;
} __attribute__((packed));

serial_packet<t_data, r_data> packet;
serial_packet<action_run_t_data, action_run_r_data> action_run_packet;

bool action_run = false;
bool testkicker_btn = false;
bool testkicker_front = false;
bool testdribbler_toggle = false;
bool testdribbler_front = false;
bool testmotor_toggle = false;
int testmotor_meter_type = 0;

bool isActionState(UI_STATE state)
{
  return (state == ACTION_OFFENSE || state == ACTION_DEFENSE || state == ACTION_RADIOCONTROL);
}

button left_btn;
button right_btn;

uint8_t red_led_pin = PB5;
uint8_t yellow_led_pin = PA11;
uint8_t green_led_pin = PC8;
uint8_t toggle_pin = PB14;

int toggle_stable_judge(bool cur_toggle)
{
  static bool last_state = false;
  static uint8_t consecutive_count = 0;

  if (cur_toggle == last_state)
  {
    if (consecutive_count < 5)
    {
      consecutive_count++;
    }
  }
  else
  {
    last_state = cur_toggle;
    consecutive_count = 1;
  }

  if (consecutive_count >= 5)
  {
    return cur_toggle ? 1 : 0;
  }

  return -1;
}

void setup()
{
  mySerial3.begin(115200);
  packet.begin(mySerial3);
  action_run_packet.begin(mySerial3);

  mySerial2.begin(115200);

  left_btn.begin(PC11, INPUT_PULLDOWN);
  right_btn.begin(PB15, INPUT_PULLDOWN);

  pinMode(red_led_pin, OUTPUT);
  pinMode(yellow_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(toggle_pin, INPUT_PULLDOWN);
}

void loop()
{
  // ボタン更新
  left_btn.update();
  right_btn.update();

  // トグルスイッチ
  static bool prev_action_run = false;
  int action_toggle = toggle_stable_judge(digitalRead(toggle_pin) == HIGH);

  digitalWrite(red_led_pin, (action_toggle == 1) ? HIGH : LOW);

  // アップデート
  static uint32_t last_time = 0;
  if (millis() - last_time > 20)
  {
    bool prev_action_run = action_run;

    // M5のデータを受送信
    if (action_run)
    {
      action_run_packet.update();

      testkicker_btn = false;
      testkicker_front = false;
      testdribbler_toggle = false;
      testdribbler_front = false;
      testmotor_toggle = false;
      testmotor_meter_type = 0;

      // アクションを走らせるか
      action_run_packet.tx.action_run = (action_toggle == 1);
      action_run = action_run_packet.rx.action_run;
    }
    else
    {
      packet.update();
      ui_state = packet.rx.ui_state;
      testkicker_btn = packet.rx.testkicker_btn;
      testkicker_front = packet.rx.testkicker_front;
      testdribbler_toggle = packet.rx.testdribbler_toggle;
      testdribbler_front = packet.rx.testdribbler_front;
      testmotor_toggle = packet.rx.testmotor_toggle;
      testmotor_meter_type = packet.rx.testmotor_meter_type;

      // アクションを走らせるか
      packet.tx.action_run = (isActionState(ui_state) && (action_toggle == 1));
      action_run = packet.rx.action_run;
    }

    // 切替時にシリアルバッファをリセット
    if (action_run && !prev_action_run)
    {
      action_run_packet.reset();
    }
    else if (!action_run && prev_action_run)
    {
      packet.reset();
    }

    last_time = millis();
  }

  digitalWrite(yellow_led_pin, LOW);
  digitalWrite(green_led_pin, LOW);

  if (action_run)
  {
    digitalWrite(yellow_led_pin, HIGH);

    switch (ui_state)
    {
    case ACTION_OFFENSE:
      break;
    case ACTION_DEFENSE:
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