#pragma once

#define INFRA_PACKET_MAX_PAYLOAD (52 + 2) // 送るデータのバイト数に2を足したものを代入
#define NO_LED_SEND_FEEDBACK_CODE         // ledデバックを消す
#include "common/infra_packet.hpp"

// infra
struct packet // 50文字まで送る
{
    uint8_t count = 0;
    char text[50 + 1];
} __attribute__((packed));

infra_packet<packet, packet> infra;

// サイズ
const size_t text_size = sizeof(infra.tx.text);