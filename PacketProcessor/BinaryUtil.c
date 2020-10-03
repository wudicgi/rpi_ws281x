#include "common.h"
#include "BinaryUtil.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_INFO
#include "Debug.h"

int32_t BinaryUtil_getInt32Value(uint8_t *ptr) {
    int32_t value = (*ptr << 24)
            | (*(ptr + 1) << 16)
            | (*(ptr + 2) << 8)
            | (*(ptr + 3));

    return value;
}

uint32_t BinaryUtil_getUInt32Value(uint8_t *ptr) {
    return (uint32_t)BinaryUtil_getInt32Value(ptr);
}
