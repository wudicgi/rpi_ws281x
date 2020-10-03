#include "common.h"
#include <sys/time.h>
#include "BinaryUtil.h"
#include "Packet.h"
#include "PacketParser.h"
#include "PacketBuilder.h"
#include "TlvReader.h"
#include "FrameBuffer.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_INFO
#include "Debug.h"

// 10min = 600000000us
// #define MAX_BUFFERED_TIME_ALLOWED   (10 * 60 * 1000000)

// 2min = 120000000us
// #define MAX_BUFFERED_TIME_ALLOWED   (2 * 60 * 1000000)

// 10s
#define MAX_BUFFERED_TIME_ALLOWED   (10 * 1000000)

uint32_t _getTimeStampInUs() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (uint32_t)(tv.tv_sec*(uint64_t)1000000+tv.tv_usec);
}

uint16_t _processHostCommandPacket(uint8_t *packetBuffer, int packetLength, uint8_t *responseBuffer) {
    LOG_DEBUG("Parse packet\n");

    CommandApdu *commandApdu = PacketParser_parsePacket(packetBuffer, packetLength);
    if (commandApdu == NULL) {
        LOG_DEBUG("Parse failed\n");
        return 0;
    }

    LOG_DEBUG("Parse succeeded\n");

    switch (commandApdu->ins) {
        case INS_LOAD: {
            int32_t packetId = 0;
            uint32_t hostTime = 0;
            uint32_t displayTime = 0;
            uint8_t *ledData = NULL;
            int ledDataLength = 0;

            uint32_t deviceTime = _getTimeStampInUs();
            int32_t timeDiff = 0;

            bool succeeded = true;

            LOG_DEBUG("Parse TLV\n");

//            Debug_printHex("commandApdu->cdata", commandApdu->cdata, commandApdu->cdataLength);

            TlvReader_init(commandApdu->cdata, commandApdu->cdataLength);

            while (!TlvReader_eof()) {
                int tag = TlvReader_readTag();
                int length = TlvReader_readLength();

                switch (tag) {
                    case TAG_PACKET_ID:
                        packetId = TlvReader_readInt32Value(length);
                        LOG_DEBUG("packetId = %d\n", packetId);

                        if (packetId == 0) {
                            LOG_INFO("packetId is 0, clear buffer\n");

                            FrameBuffer_clear();
                        }

                        if (FrameBuffer_getFreeSpace() < commandApdu->cdataLength) {
                            LOG_INFO("Build failed ACK\n");

                            FrameBuffer_getUsedPercent((int *) (responseBuffer + 20));

                            return PacketBuilder_buildAck(responseBuffer, packetId, false,
                                    *((int *) (responseBuffer + 20)), deviceTime, 0);
                        }

                        break;

                    case TAG_HOST_TIME:
                        hostTime = TlvReader_readUInt32Value(length);
                        LOG_DEBUG("hostTime = %u\n", hostTime);
                        timeDiff = (int32_t) deviceTime - (int32_t) hostTime;
                        LOG_DEBUG("timeDiff = %d\n", timeDiff);
                        break;

                    case TAG_LED_DATA_SET:
                        if (ledDataLength != 0) {
                            uint32_t showTime = (uint32_t) ((int32_t) displayTime + timeDiff);
                            LOG_DEBUG("showTime = %u + %d = %u\n", displayTime, timeDiff, showTime);

                            // 此处不用针对无符号数的进位再做额外处理，减法结果即为 showTime 相对于 deviceTime 超前的 us 数
                            int32_t temp = (int32_t) showTime - (int32_t) deviceTime;
                            if ((temp > 0) && (temp <= MAX_BUFFERED_TIME_ALLOWED)) {
                                LOG_DEBUG("temp = %d <= %d, write frame\n", temp, MAX_BUFFERED_TIME_ALLOWED);

                                if (!FrameBuffer_write(showTime, ledData, ledDataLength)) {
                                }
                            } else {
                                LOG_DEBUG("temp = %d, <= 0 or > %d, skip\n", temp, MAX_BUFFERED_TIME_ALLOWED);
                            }
                        }

                        displayTime = 0;
                        ledData = NULL;
                        ledDataLength = 0;
                        LOG_DEBUG("TAG_LED_DATA_SET\n");
                        break;

                    case TAG_DISPLAY_TIME:
                        displayTime = TlvReader_readUInt32Value(length);
                        LOG_DEBUG("displayTime = %u\n", displayTime);
                        break;

                    case TAG_LED_DATA:
                        ledData = TlvReader_readValue(length);
                        ledDataLength = length;
                        LOG_DEBUG("ledDataLength = %d\n", ledDataLength);
                        break;

                    default:
                        LOG_DEBUG("unknown tag 0x%02x\n", tag);
                        break;
                }
            }

            if (ledDataLength != 0) {
                uint32_t showTime = (uint32_t) ((int32_t) displayTime + timeDiff);
                LOG_DEBUG("showTime = %u + %d = %u\n", displayTime, timeDiff, showTime);

                // 此处不用针对无符号数的进位再做额外处理，减法结果即为 showTime 相对于 deviceTime 超前的 us 数
                int32_t temp = (int32_t) showTime - (int32_t) deviceTime;
                if ((temp > 0) && (temp <= MAX_BUFFERED_TIME_ALLOWED)) {
                    LOG_DEBUG("temp = %d <= %d, write frame\n", temp, MAX_BUFFERED_TIME_ALLOWED);

                    if (!FrameBuffer_write(showTime, ledData, ledDataLength)) {
                    }
                } else {
                    LOG_DEBUG("temp = %d > %d, skip\n", temp, MAX_BUFFERED_TIME_ALLOWED);
                }
            }

            LOG_DEBUG("Process\n");

            FrameBuffer_getUsedPercent((int *) (responseBuffer + 20));

            /*
            sprintf(buffer, "LED data load %s, used = %d (host = %u, device = %u, disp = %u, time_2 = %u)",
                    (succeeded ? "succeeded" : "failed"),
                    *((int *) (buffer + 20)),
                    command_data->host_time,
                    device_time,
                    command_data->display_time,
                    display_time_2);
            return strlen(buffer);
            */

            LOG_DEBUG("Build ACK\n");

            int ackPacketLength = PacketBuilder_buildAck(responseBuffer, packetId, succeeded,
                    *((int *) (responseBuffer + 20)), deviceTime, 0);

            return ackPacketLength;
        }

        /*
        case INS_CLEAR_BUFFER: {
            int32_t packetId = 0;

            uint32_t deviceTime = system_get_time();

            bool succeeded = true;

            LOG_DEBUG("Parse TLV\n");

//            Debug_printHex("commandApdu->cdata", commandApdu->cdata, commandApdu->cdataLength);

            TlvReader_init(commandApdu->cdata, commandApdu->cdataLength);

            while (!TlvReader_eof()) {
                int tag = TlvReader_readTag();
                int length = TlvReader_readLength();

                switch (tag) {
                    case TAG_PACKET_ID:
                        packetId = TlvReader_readInt32Value(length);
                        LOG_DEBUG("packetId = %d\n", packetId);
                        break;

                    default:
                        LOG_DEBUG("unknown tag 0x%02x\n", tag);
                        break;
                }
            }

            LOG_DEBUG("Process\n");

            FrameBuffer_clear();

            FrameBuffer_getUsedPercent((int *) (responseBuffer + 20));

            LOG_DEBUG("Build ACK\n");

            int ackPacketLength = PacketBuilder_buildAck(responseBuffer, packetId, succeeded,
                    *((int *) (responseBuffer + 20)), deviceTime, 0);

            return ackPacketLength;

            break;
        }
        */

        /*
        case INS_UPDATE: {
            CommandDataUpdate *command_data = (CommandDataUpdate *)command_packet->command_data;

            i2s_update_buffer(command_data->led_data, command_data->led_data_length);

            sprintf(buffer, "LED data updated.");
            return strlen(buffer);

            break;
        }
        */

        /*
        case INS_STATUS: {
            ResponsePacket *response_packet = (ResponsePacket *)buffer;

            frame_buffer_get_status((int *) (buffer + 20));

            response_packet->signature = RESPONSE_PACKET_SIGNATURE;
            response_packet->response_type = COMMAND_TYPE_GET_BUFFER_STATUS;
            response_packet->response_data_length = sizeof(ResponseDataGetBufferStatus);
            ((ResponseDataGetBufferStatus *) response_packet->response_data)->percent_used = *((int *) (buffer + 20));

            return (sizeof(ResponsePacket) + sizeof(ResponseDataGetBufferStatus));

            break;
        }
        */

        default: {
            sprintf((char *)responseBuffer, "Unrecognized command type: 0x%02x.", (int)commandApdu->ins);

            return strlen((char *)responseBuffer);
        }
    }
}

uint16_t _processUnknownUdpPacket(uint8_t *data, unsigned short dataLength, uint8_t *responseBuffer) {
    sprintf((char *)responseBuffer, "Received %d bytes",
            (int)dataLength);
    dataLength = strlen((char *)responseBuffer);

    return dataLength;
}

unsigned short PacketProcessor_processPacket(uint8_t *data, unsigned short length, uint8_t *responseBuffer) {
    LOG_DEBUG("Compare signature\n");
    if (memcmp(data, PACKET_SIGNATURE_HOST_COMMAND, 4) == 0) {
        LOG_DEBUG("It is host command\n");
        return _processHostCommandPacket(data, length, responseBuffer);
    } else {
        LOG_DEBUG("It is not host command\n");
        return _processUnknownUdpPacket(data, length, responseBuffer);
    }
}
