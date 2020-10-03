#ifndef _PACKET_PARSER_H_
#define _PACKET_PARSER_H_

#include "common.h"
#include "Packet.h"

CommandApdu *PacketParser_parsePacket(uint8_t *buffer, int length);
CommandApdu *PacketParser_parseCommandApdu(uint8_t *buffer, int length);

#endif
