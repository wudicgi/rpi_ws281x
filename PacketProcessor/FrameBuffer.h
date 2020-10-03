#ifndef FRAME_BUFFER_H_
#define FRAME_BUFFER_H_

#include "common.h"

void FrameBuffer_getUsedPercent(int *percentUsed);

int FrameBuffer_getFreeSpace(void);

void FrameBuffer_clear(void);

bool FrameBuffer_write(uint32_t showTime, uint8_t *data, int dataLength);

bool FrameBuffer_canRead(uint32_t time);

bool FrameBuffer_read(uint8_t **data, int *dataLength);

#endif
