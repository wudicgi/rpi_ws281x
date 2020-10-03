#ifndef PACKET_H_
#define PACKET_H_

#include "common.h"

#define PACKET_OFFSET_SIGNATURE     0
#define PACKET_OFFSET_PACKET_ID     (PACKET_OFFSET_SIGNATURE + 4)
#define PACKET_OFFSET_APDU_LENGTH   (PACKET_OFFSET_PACKET_ID + 4)

#define PACKET_OFFSET_CLA           (PACKET_OFFSET_APDU_LENGTH + 4)
#define PACKET_OFFSET_INS           (PACKET_OFFSET_CLA + 1)
#define PACKET_OFFSET_P1            (PACKET_OFFSET_INS + 1)
#define PACKET_OFFSET_P2            (PACKET_OFFSET_P1 + 1)
#define PACKET_OFFSET_LC            (PACKET_OFFSET_P2 + 1)
#define PACKET_OFFSET_EXT_CDATA     (PACKET_OFFSET_LC + 3)

#define PACKET_MIN_LENGTH           (PACKET_OFFSET_LC + 1)

#define PACKET_SIGNATURE_HOST_COMMAND       "BSHP"
#define PACKET_SIGNATURE_DEVICE_COMMAND     "BSDP"

#define CLA_00                  0x00
#define CLA_80                  0x80
    
#define INS_LOAD                0x01
#define INS_UPDATE              0x02
#define INS_CLEAR_BUFFER        0x06
#define INS_STATUS              0x11
#define INS_ACK                 0x21
#define INS_CRYPTO_AUTH         0xC0
#define INS_CRYPTO_EXEC_CMD     0xF0

#define TAG_PACKET_ID           0x41
#define TAG_HOST_TIME           0x42
#define TAG_DISPLAY_TIME        0x43
#define TAG_LED_DATA            0x51
    
#define TAG_LED_DATA_SET        0x71
    
#define TAG_SUCCEEDED           0x5A
#define TAG_PERCENT_USED        0x5B
#define TAG_DEVICE_TIME         0x5C
#define TAG_DISPLAY_TIME_2      0x5D

typedef struct {
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t *cdata;
    int cdataLength;
    int le;
} CommandApdu;

#endif
