#include "common.h"
#include "PacketBuilder.h"
#include "Packet.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_INFO
#include "Debug.h"

static uint8_t *_ptr = NULL;

static inline void _appendTag(int tag) {
    if ((tag & 0x1F) == 0x1F) {
        // 若 tag 的低 5 位都为 1, 则 tag 长度为 2 字节
        *_ptr++ = (tag >> 8) & 0xFF;
        *_ptr++ = tag & 0xFF;
    } else {
        // tag 长度为 1 字节
        *_ptr++ = tag & 0xFF;
    }
}

static inline void _appendLength(int length) {
    if (length > 0x7F) {
        *_ptr++ = 0x82;
        *_ptr++ = (length >> 8) & 0xFF;
        *_ptr++ = length & 0xFF;
    } else {
        *_ptr++ = length & 0xFF;
    }
}

static void _appendTlvUInt32(int tag, uint32_t value) {
    _appendTag(tag);

    _appendLength(4);

    *_ptr++ = (value >> 24) & 0xFF;
    *_ptr++ = (value >> 16) & 0xFF;
    *_ptr++ = (value >> 8) & 0xFF;
    *_ptr++ = value & 0xFF;
}

static void _appendTlvInt32(int tag, int32_t value) {
    _appendTag(tag);

    _appendLength(4);

    *_ptr++ = (value >> 24) & 0xFF;
    *_ptr++ = (value >> 16) & 0xFF;
    *_ptr++ = (value >> 8) & 0xFF;
    *_ptr++ = value & 0xFF;
}

void PacketBuilder_init(uint8_t *buffer) {
    _ptr = buffer;

    memcpy(_ptr + PACKET_OFFSET_SIGNATURE, PACKET_SIGNATURE_DEVICE_COMMAND, 4);

    memset(_ptr + PACKET_OFFSET_CLA, 0, (PACKET_OFFSET_EXT_CDATA - PACKET_OFFSET_CLA));

    _ptr += PACKET_OFFSET_EXT_CDATA;
}

int PacketBuilder_buildAck(uint8_t *dest, int packetId, bool succeeded, int32_t percentUsed, uint32_t deviceTime, uint32_t displayTime2) {
    _ptr = dest;

    memcpy(_ptr, PACKET_SIGNATURE_DEVICE_COMMAND, 4);
    _ptr += 4;

    memset(_ptr, 0, 4);
    _ptr += 4;

    uint8_t *ptrApduLength = _ptr;

    memset(_ptr, 0, 4);
    _ptr += 4;

    *_ptr++ = CLA_80;
    *_ptr++ = INS_ACK;
    *_ptr++ = 0x00;
    *_ptr++ = 0x00;

    uint8_t *ptrLc = _ptr;

    *_ptr++ = 0x00;
    *_ptr++ = 0x00;
    *_ptr++ = 0x00;

    uint8_t *ptrCdata = _ptr;

    _appendTlvInt32(TAG_PACKET_ID, packetId);
    _appendTlvInt32(TAG_SUCCEEDED, succeeded ? 1 : 0);
    _appendTlvInt32(TAG_PERCENT_USED, percentUsed);
    _appendTlvUInt32(TAG_DEVICE_TIME, deviceTime);
    _appendTlvUInt32(TAG_DISPLAY_TIME_2, displayTime2);

    int cdataLength = (_ptr - ptrCdata);
    int apduLength = 7 + cdataLength;

    *(ptrLc + 1) = (cdataLength >> 8) & 0xFF;
    *(ptrLc + 2) = cdataLength & 0xFF;

    *(ptrApduLength + 2) = (apduLength >> 8) & 0xFF;
    *(ptrApduLength + 3) = apduLength & 0xFF;

    return (_ptr - dest);
}
