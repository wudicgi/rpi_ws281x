#include "common.h"
#include "BinaryUtil.h"
#include "PacketParser.h"
#include "Packet.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_INFO
#include "Debug.h"

static CommandApdu _sharedCommandApdu = {0};

CommandApdu *PacketParser_parsePacket(uint8_t *buffer, int length) {
    if (length < PACKET_MIN_LENGTH) {
        Debug_printf("error 1\n");
        return NULL;
    }

    if (memcmp(buffer + PACKET_OFFSET_SIGNATURE, PACKET_SIGNATURE_HOST_COMMAND, 4) != 0) {
        Debug_printf("error 2\n");
        return NULL;
    }

    /*int packetId = */BinaryUtil_getInt32Value(buffer + PACKET_OFFSET_PACKET_ID);

    int apduLength = BinaryUtil_getInt32Value(buffer + PACKET_OFFSET_APDU_LENGTH);

    return PacketParser_parseCommandApdu(buffer + PACKET_OFFSET_CLA, apduLength);
}

CommandApdu *PacketParser_parseCommandApdu(uint8_t *buffer, int length) {
    if (length < 4) {
        Debug_printf("error 3\n");
        return NULL;
    }

    uint8_t *ptr = buffer;
    uint8_t *ptrEnd = buffer + length;

    CommandApdu *obj = &_sharedCommandApdu;

    obj->cla = *ptr++;;
    obj->ins = *ptr++;;
    obj->p1 = *ptr++;;
    obj->p2 = *ptr++;;

    obj->cdata = NULL;
    obj->cdataLength = 0;
    obj->le = 0;

    if (ptr == ptrEnd) {
        // case 1: CLA INS P1 P2
        return obj;
    }

    int lcOrLe = 0;
    if (*ptr == 0x00) {
        // to be noticed
        if ((ptr + 1) == ptrEnd) {
            // case 2: CLA INS P1 P2 Le
            obj->le = 0;
            return obj;
        }

        if ((ptr + 3) >= ptrEnd) {
            Debug_printf("error 4\n");
            return NULL;
        }

        lcOrLe = (*(ptr + 1) << 8) + *(ptr + 2);
        ptr += 3;
    } else {
        lcOrLe = *ptr;
        ptr++;
    }

    if (ptr == ptrEnd) {
        // case 2: CLA INS P1 P2 Le
        obj->le = lcOrLe;
        return obj;
    }

    if ((ptr + lcOrLe) > ptrEnd) {
        Debug_printf("error 5\n");
        return NULL;
    }

    obj->cdata = ptr;
    obj->cdataLength = lcOrLe;
    ptr += lcOrLe;

    if (ptr == ptrEnd) {
        // case 3: CLA INS P1 P2 Lc cdata
        return obj;
    }

    int le = 0;
    if (*ptr == 0x00) {
        if ((ptr + 1) == ptrEnd) {
            le = *ptr;
            ptr++;
        } else {
            if ((ptr + 3) >= ptrEnd) {
                Debug_printf("error 6\n");
                return NULL;
            }

            le = (*(ptr + 1) << 8) + *(ptr + 2);
            ptr += 3;
        }
    } else {
        le = *ptr;
        ptr++;
    }

    if (ptr == ptrEnd) {
        // case 4: CLA INS P1 P2 Lc cdata Le
        obj->le = le;
        return obj;
    } else {
        Debug_printf("error 7\n");
        return NULL;
    }
}
