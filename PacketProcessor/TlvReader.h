#ifndef _TLV_READER_H_
#define _TLV_READER_H_

#include "common.h"

void TlvReader_init(uint8_t *buffer, int length);
bool TlvReader_eof(void);
int TlvReader_readTag(void);
int TlvReader_readLength(void);
uint8_t *TlvReader_readValue(int length);
// void TlvReader_readValue(int length, uint8_t *dest);
uint32_t TlvReader_readUInt32Value(int length);
int32_t TlvReader_readInt32Value(int length);
bool TlvReader_isCompositeTlvTag(int tag);

#endif
