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
};
struct r_data
{
  UI_STATE ui_state = HOME;
  bool test_kicker_btn = false;
  bool test_dribbler_toggle = false;
  bool test_motor_toggle = false;
};
serial_packet<t_data, r_data> packet(20);
struct action_run_t_data
{
  bool action_run = false;
  int my_posi_x = 0;
  int my_posi_y = 0;
};
struct action_run_r_data
{
  int peer_posi_x = 0;
  int peer_posi_y = 0;
};
serial_packet<action_run_t_data, action_run_r_data> action_run_packet(20);
bool action_run = false;
bool test_kicker_btn = false;
bool test_dribbler_toggle = false;
bool test_motor_toggle = false;

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
  static bool prev_action_run = false;
  bool action_toggle = (digitalRead(toggle_pin) == HIGH);

  if (!action_run)
  {
    if (prev_action_run)
    {
      while (mySerial3.available())
        mySerial3.read();
    }
    prev_action_run = false;

    digitalWrite(green_led_pin, LOW);

    packet.tx.action_run = action_run;
    packet.update();
    ui_state = packet.rx.ui_state;

    if (isActionState(ui_state) && action_toggle)
    {
      action_run = true;
    }
    test_kicker_btn = packet.rx.test_kicker_btn;
    test_dribbler_toggle = packet.rx.test_dribbler_toggle;
    test_motor_toggle = packet.rx.test_motor_toggle;
  }
  else
  {
    if (!prev_action_run)
    {
      while (mySerial3.available())
        mySerial3.read();
    }
    prev_action_run = true;

    digitalWrite(green_led_pin, HIGH);

    action_run_packet.update();

    action_run = action_toggle;
    test_kicker_btn = false;
    test_dribbler_toggle = false;
    test_motor_toggle = false;
  }

  digitalWrite(red_led_pin, LOW);
  digitalWrite(yellow_led_pin, LOW);

  switch (ui_state)
  {
  case HOME:
    break;
  case ACTION_OFFENSE:
    if (action_run)
      digitalWrite(red_led_pin, HIGH);
    break;
  case ACTION_DEFENSE:
    if (action_run)
      digitalWrite(red_led_pin, HIGH);
    break;
  case ACTION_RADIOCONTROL:
    if (action_run)
      digitalWrite(red_led_pin, HIGH);
    break;
  case TEST_KICKER:
    if (test_kicker_btn)
      digitalWrite(yellow_led_pin, HIGH);
    break;
  case TEST_DRIBBLER:
    if (test_dribbler_toggle)
      digitalWrite(yellow_led_pin, HIGH);
    break;
  case TEST_MOTOR:
    if (test_motor_toggle)
      digitalWrite(yellow_led_pin, HIGH);
    break;
  case SENSORMONITOR_BALL:
  case SENSORMONITOR_LINE:
  case SENSORMONITOR_GYRO:
  case SENSORMONITOR_GOAL:
  case SENSORMONITOR_LIDAR:
  case COMMUNICATION_TRANSMIT:
  case COMMUNICATION_RECEIVE:
    break;
  }

  left_btn.update();
  right_btn.update();

  static uint32_t _last_tx_time = millis();
  if (millis() - _last_tx_time >= 20)
  {
    _last_tx_time = millis();
  }
}