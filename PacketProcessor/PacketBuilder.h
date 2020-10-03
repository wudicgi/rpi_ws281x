#ifndef _PACKET_BUILDER_H_
#define _PACKET_BUILDER_H_

#include "common.h"

int PacketBuilder_buildAck(uint8_t *dest, int packetId, bool succeeded, int32_t percentUsed, uint32_t deviceTime, uint32_t displayTime2);

#endif
