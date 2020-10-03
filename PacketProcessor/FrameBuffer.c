#include "common.h"
#include "FrameBuffer.h"

#define DEBUG_LOG_LEVEL     DEBUG_LOG_LEVEL_VERBOSE
#include "Debug.h"

typedef struct {
    uint32_t    showTime;
    int         dataLength;
    uint8_t     data[];
} Frame;

#define BUFFER_SIZE             8192

#define DATA_MAX_LENGTH         (3 * 100)

static char _buffer[BUFFER_SIZE] = {0};

static int _bufferLength = BUFFER_SIZE;
static int _readOffset = 0;
static int _writeOffset = 0;

void FrameBuffer_getUsedPercent(int *percentUsed) {
    int usedLength = _writeOffset - _readOffset;
    if (usedLength < 0) {
        usedLength += _bufferLength;
    }

    *percentUsed = ((usedLength * 100) / _bufferLength);
}

#define GAP_SIZE    4

int FrameBuffer_getFreeSpace(void) {
    int usedLength = _writeOffset - _readOffset;
    if (usedLength < 0) {
        usedLength += _bufferLength;
    }

    LOG_VERBOSE("used space: %d bytes\r\n", usedLength);

    return (BUFFER_SIZE - usedLength);
}

void FrameBuffer_clear(void) {
    _readOffset = 0;
    _writeOffset = 0;
    _bufferLength = BUFFER_SIZE;
}

/**
 * 向 frame buffer 结尾写入一个 frame
 *
 * @showTime    显示时间
 * @data        数据
 * @dataLength  数据的长度
 */
bool FrameBuffer_write(uint32_t showTime, uint8_t *data, int dataLength) {
    int frameLength = (((sizeof(Frame) + dataLength) + 3) / 4) * 4;

    int writeEndOffset = (_writeOffset + frameLength + GAP_SIZE);

    if ((_writeOffset < _readOffset) && (writeEndOffset > _readOffset)) {
        // _data_buffer 已满
        return false;
    }

    if (writeEndOffset > BUFFER_SIZE) {
        if ((0 + frameLength + GAP_SIZE) > _readOffset) {
            // _data_buffer 已满
            return false;
        }

        _bufferLength = _writeOffset;
        _writeOffset = 0;
    }

    Frame *frame = (Frame *) (_buffer + _writeOffset);

    LOG_VERBOSE("write at _writeOffset = %d\r\n",
            _writeOffset);

    frame->showTime = showTime;
    frame->dataLength = dataLength;
    memcpy(frame->data, data, dataLength);

    _writeOffset += frameLength;

    LOG_VERBOSE("wrote %d bytes\r\n", frameLength);

    return true;
}

/**
 * 检查 frame buffer 中是否有 frame 可读
 *
 * @time
 *
 * @return      有 frame 可读返回 true, 否则返回 false
 */
bool FrameBuffer_canRead(uint32_t time) {
    if (_readOffset == _writeOffset) {
        // _data_buffer 为空
        LOG_VERBOSE("empty -> false\r\n");
        return false;
    }

    LOG_VERBOSE("read at _readOffset = %d\r\n",
            _readOffset);

    if (time >= ((Frame *) (_buffer + _readOffset))->showTime) {
        LOG_VERBOSE("%u >= %u -> true\r\n",
                time, ((Frame *) (_buffer + _readOffset))->showTime);

        return true;
    } else {
        LOG_VERBOSE("%u < %u -> false\r\n",
                time, ((Frame *) (_buffer + _readOffset))->showTime);

        return false;
    }
}

/**
 * 从 frame buffer 开头读取一个 frame
 *
 * @data        指向 char* 型变量的指针，函数执行后该 char* 变量为指向 frame 数据的指针
 * @dataLength  指向 int 型变量的指针，函数执行后该 int 变量的值为 frame 数据的长度
 *
 * @return      执行成功返回 true, 否则返回 false
 */
bool FrameBuffer_read(uint8_t **data, int *dataLength) {
    if (_readOffset == _writeOffset) {
        // _data_buffer 为空
        return false;
    }

    if (_readOffset >= _bufferLength) {
        _readOffset -= _bufferLength;
    }

    Frame *frame = (Frame *) (_buffer + _readOffset);

    LOG_VERBOSE("read at _readOffset = %d\r\n",
            _readOffset);

//    memcpy(dest, frame->data, frame->data_length);
    *data = frame->data;
    *dataLength = frame->dataLength;

    int frameLength = (((sizeof(Frame) + frame->dataLength) + 3) / 4) * 4;

    _readOffset += frameLength;
    if (_readOffset >= _bufferLength) {
        _readOffset -= _bufferLength;
    }

    return true;
}
