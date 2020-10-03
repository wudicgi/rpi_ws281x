#include "common.h"
#include "Debug.h"

#ifdef DEBUG

void Debug_printHex(const char *variableName, uint8_t *buffer, int length) {
    int i;

    printf("%s =\n", variableName);

    for (i = 0; i < length; i++) {
        printf("%02X ", (int)*buffer++);

        if (i % 16 == 0) {
            printf("\n");
        }
    }

    printf("\n");
}

#endif
