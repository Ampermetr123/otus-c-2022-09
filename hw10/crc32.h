#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>
#include <stddef.h>

void crc32_start(uint32_t* crc32);
void crc32_proc(const uint8_t* data, size_t _SIZE_T_, uint32_t* crc32);
void crc32_end(uint32_t* crc32);

#endif // __CRC32_H__