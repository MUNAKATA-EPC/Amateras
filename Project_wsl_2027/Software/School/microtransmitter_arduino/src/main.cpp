#include <Arduino.h>
#include <stdio.h>

#define INFRA_PACKET_MAX_PAYLOAD 5 + 2
#include "common/infra_packet.hpp"

struct Packet
{
  char text[5];
} __attribute__((packed));

infra_packet<Packet, Packet> infra;

void setup()
{
  Serial.begin(9600);

  infra.begin(3, -1, 1000);

  for (int i = 0; i < 5; i++)
  {
    infra.tx.text[i] = 'a' + i;
  }
}

void loop()
{
  infra.update();

  static uint32_t last_t = 0;
  uint32_t cur_t = millis();
  if (cur_t - last_t > 1500)
  {
    for (int i = 0; i < 5; i++)
    {
      if (infra.tx.text[i] == 'z')
        infra.tx.text[i] = 'a';
      else
        infra.tx.text[i] = infra.tx.text[i] + 1;
    }

    last_t = millis();
  }
}