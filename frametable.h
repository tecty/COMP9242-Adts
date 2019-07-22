#if !defined(FRAME_TABLE_H)
#define FRAME_TABLE_H

#include <stdlib.h>
#include <stdint.h>

typedef void (* frame_table_callback_t)(int64_t err, void * private_data);

void FrameTable__init();
size_t FrameTable__allocFrame();
size_t FrameTable__allocCspace();
size_t FrameTable__copyCap(size_t src, size_t dest);
void FrameTable__delCap(size_t cap);

void FrameTable__unMapCap(size_t cap);
void * FrameTable__getFrameVaddr(size_t frame_id);


/**
 * Passthrough the async request, Just let the upper layer know there's an
 * async event there
 */

void FrameTable__swapOutFrame(
    size_t frame_id, size_t disk_id, frame_table_callback_t cb, void * data
);
void FrameTable__swapInFrame(
    size_t frame_id, size_t disk_id ,frame_table_callback_t cb, void * data
);

#endif // FRAME_TABLE_H
