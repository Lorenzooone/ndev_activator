#ifndef __UTILS_HPP
#define __UTILS_HPP

#include <cstdint>
#ifndef _WIN32
#include <unistd.h>
#endif

#if _MSC_VER && !__INTEL_COMPILER
#define PACKED
#define ALIGNED(x) alignas(x)
#define STDCALL __stdcall
#else
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define STDCALL
#endif

void write_le16(uint8_t* data, size_t pos, uint16_t to_write);
uint16_t read_le16(const uint8_t* data, size_t pos);
void write_le32(uint8_t* data, size_t pos, uint32_t to_write);
uint32_t read_le32(const uint8_t* data, size_t pos);
void write_be16(uint8_t* data, size_t pos, uint16_t to_write);
uint16_t read_be16(const uint8_t* data, size_t pos);
void write_be32(uint8_t* data, size_t pos, uint32_t to_write);
uint32_t read_be32(const uint8_t* data, size_t pos);

#endif
