#include <Arduino.h>
#include <Ps3Controller.h>

/*m5stamp-picoのアドレスを取得する*/

uint8_t btmac[6];

void setup()
{
  Serial.begin(9600);

  // m5stamp-picoのアドレスを取得
  btmac[6];
  esp_read_mac(btmac, ESP_MAC_BT);
}

void loop()
{
  Serial.printf("[Bluetooth] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", btmac[0], btmac[1], btmac[2], btmac[3], btmac[4], btmac[5]);
}