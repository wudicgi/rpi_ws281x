#ifndef _PACKET_PROCESSOR_H_
#define _PACKET_PROCESSOR_H_

#include "common.h"

unsigned short PacketProcessor_processPacket(uint8_t *data, unsigned short length, uint8_t *responseBuffer);

#endif
