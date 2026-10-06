#include <Arduino.h>
#include <Ps3Controller.h>
#include "sensor/serial_packet.hpp"

struct t_data
{
  int8_t stick_lx, stick_ly, stick_rx, stick_ry;
  uint16_t buttons_data_bit_mask = 0;
} __attribute__((packed));
struct r_data
{
} __attribute__((packed));
serial_packet<t_data, r_data> packet;

// PS3が接続されると呼ばれる関数
void connect_success();
// PS3が切断されると呼ばれる関数
void connect_false();
// PS3のどれが反応すると呼ばれる関数
void data_update();

void setup()
{
  Serial.begin(115200); // 115200bpsでシリアル通信を開始
  packet.begin(Serial);

  Ps3.attach(data_update);               // PS3のどれが反応すると呼ばれる関数
  Ps3.attachOnConnect(connect_success);  // 接続されたときに呼び出す関数
  Ps3.attachOnDisconnect(connect_false); // 切断されたときに呼び出す関数
  Ps3.begin("14:08:08:55:9D:FA");        // ps3_setupで取得したアドレスを記入
  // マーク1アドレス : 14:08:08:55:9D:FA

  while (!Ps3.isConnected()) // 接続されるまで待つ
  {
    Serial.println("waitng for connection...");
    delay(10);
  };
}

void loop()
{
  packet.update();
}

// それぞれの関数のプログラム

void connect_success()
{
  Ps3.setRumble(1, 1000); // 振動1を1000msする
}

void connect_false()
{
  while (!Ps3.isConnected()) // 接続されるまで待つ
    ;
}

void data_update()
{
  // アナログスティックの情報を取得
  packet.tx.stick_lx = Ps3.data.analog.stick.lx; // 左スティックのX方向
  packet.tx.stick_ly = Ps3.data.analog.stick.ly; // 左スティックのY方向
  packet.tx.stick_rx = Ps3.data.analog.stick.rx; // 右スティックのX方向
  packet.tx.stick_ry = Ps3.data.analog.stick.ry; // 右スティックのY方向

  // ボタンの情報を取得(押されたら加算)
  packet.tx.buttons_data_bit_mask = 0; // 初期化

  if (Ps3.data.button.up) // 上
    packet.tx.buttons_data_bit_mask |= (1 << 0);
  if (Ps3.data.button.down) // 下
    packet.tx.buttons_data_bit_mask |= (1 << 1);
  if (Ps3.data.button.left) // 左
    packet.tx.buttons_data_bit_mask |= (1 << 2);
  if (Ps3.data.button.right) // 右
    packet.tx.buttons_data_bit_mask |= (1 << 3);

  if (Ps3.data.button.triangle) // △
    packet.tx.buttons_data_bit_mask |= (1 << 4);
  if (Ps3.data.button.circle) // ○
    packet.tx.buttons_data_bit_mask |= (1 << 5);
  if (Ps3.data.button.cross) // ×
    packet.tx.buttons_data_bit_mask |= (1 << 6);
  if (Ps3.data.button.square) // □
    packet.tx.buttons_data_bit_mask |= (1 << 7);

  if (Ps3.data.button.l1) // L1
    packet.tx.buttons_data_bit_mask |= (1 << 8);
  if (Ps3.data.button.l2) // L2
    packet.tx.buttons_data_bit_mask |= (1 << 9);
  if (Ps3.data.button.l3) // L3
    packet.tx.buttons_data_bit_mask |= (1 << 10);

  if (Ps3.data.button.r1) // R1
    packet.tx.buttons_data_bit_mask |= (1 << 11);
  if (Ps3.data.button.r2) // R2
    packet.tx.buttons_data_bit_mask |= (1 << 12);
  if (Ps3.data.button.r3) // R3
    packet.tx.buttons_data_bit_mask |= (1 << 13);
}