#pragma once

#ifndef INFRA_PACKET_CARRIER_KHZ
#define INFRA_PACKET_CARRIER_KHZ 38
#endif

#ifndef INFRA_PACKET_UNIT_US
#define INFRA_PACKET_UNIT_US 500
#endif

// packetのサイズはj 1バイト + cycle 1バイト + message_id 4バイト + text 51バイト
#ifndef INFRA_PACKET_MAX_PAYLOAD
#define INFRA_PACKET_MAX_PAYLOAD 57
#elif INFRA_PACKET_MAX_PAYLOAD < 57
#undef INFRA_PACKET_MAX_PAYLOAD
#define INFRA_PACKET_MAX_PAYLOAD 57
#endif

#define NO_LED_SEND_FEEDBACK_CODE
#include "common/infra_packet.hpp"

//// infra ////
namespace infra
{
    struct data
    {
        uint8_t j = 0;           // 下位7ビットはID、最上位ビットは受信完了/送信の区別
        uint8_t cycle = 0;       // 0～255を繰り返す周期カウント
        uint32_t message_id = 0; // 送信データと受信完了コードを対応づける番号
        char text[50 + 1];       // 本文領域で最大51バイト
    } __attribute__((packed));

    infra_packet<data, data> packet;

    // IDには0から127の値を設定する
    uint8_t my_id = 0;
    uint8_t friends_id = 0;

    // packetの本文領域のサイズ（51バイト）
    const size_t text_size_max = sizeof(packet.tx.text);
}
