#include "common.h"
#include "BinaryUtil.h"
#include "TlvReader.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_INFO
#include "Debug.h"

static uint8_t *_ptr = NULL;
static uint8_t *_ptrEnd = NULL;

void TlvReader_init(uint8_t *buffer, int length) {
    _ptr = buffer;
    _ptrEnd = buffer + length;
}

bool TlvReader_eof(void) {
    return (_ptr >= _ptrEnd);
}

int TlvReader_readTag(void) {
    int tag;

    uint8_t byte1 = *_ptr++;
    if ((byte1 & 0x1F) == 0x1F) {
        // 若 tag 的低 5 位都为 1, 则 tag 长度为 2 字节
        uint8_t byte2 = *_ptr++;
        tag = (byte1 << 8) + byte2;
    } else {
        // tag 长度为 1 字节
        tag = byte1;
    }

    return tag;
}

int TlvReader_readLength(void) {
    int length;

    uint8_t byte1 = *_ptr++;

    if ((byte1 & 0x80) != 0) {
        int i;

        int byteCount = byte1 - 0x80;

        if (byteCount < 1 || byteCount > 4)
        {
            // byte1 is 0x80 or 0x85 - 0x8F, invalid value

            // to be noticed
            return -1;
        }

        // byte1 is 0x81 - 0x84

        length = 0;
        for (i = 0; i < byteCount; i++) {
            length = (length << 8) + *_ptr++;
        }
    } else {
        // byte1 is 0x00 - 0x7F

        length = byte1;
    }

    return length;
}

/*
void TlvReader_readValue(int length, uint8_t *dest) {
    memcpy(dest, _ptr, length);
    _ptr += length;
}
*/

uint8_t *TlvReader_readValue(int length) {
    uint8_t *value = _ptr;
    _ptr += length;

    return value;
}

uint32_t TlvReader_readUInt32Value(int length) {
    uint32_t value = BinaryUtil_getUInt32Value(_ptr);
    _ptr += 4;

    return value;
}

int32_t TlvReader_readInt32Value(int length) {
    int32_t value = BinaryUtil_getInt32Value(_ptr);
    _ptr += 4;

    return value;
}

bool TlvReader_isCompositeTlvTag(int tag)
{
    if (tag > 0xFF)
    {
        if (((tag >> 8) & 0x20) != 0)
        {
            return true;
        }
    }
    else
    {
        if ((tag & 0x20) != 0)
        {
            return true;
        }
    }

    return false;
}


