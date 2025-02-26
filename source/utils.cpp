#include "utils.hpp"

void write_le16(uint8_t* data, size_t pos, uint16_t to_write) {
	data[pos] = to_write & 0xFF;
	data[pos + 1] = (to_write >> 8) & 0xFF;
}

uint16_t read_le16(const uint8_t* data, size_t pos) {
	return data[pos] | (data[pos + 1] << 8);
}

void write_le32(uint8_t* data, size_t pos, uint32_t to_write) {
	data[pos] = to_write & 0xFF;
	data[pos + 1] = (to_write >> 8) & 0xFF;
	data[pos + 2] = (to_write >> 16) & 0xFF;
	data[pos + 3] = (to_write >> 24) & 0xFF;
}

uint32_t read_le32(const uint8_t* data, size_t pos) {
	return data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
}

void write_be16(uint8_t* data, size_t pos, uint16_t to_write) {
	data[pos + 1] = to_write & 0xFF;
	data[pos] = (to_write >> 8) & 0xFF;
}

uint16_t read_be16(const uint8_t* data, size_t pos) {
	return data[pos + 1] | (data[pos] << 8);
}

void write_be32(uint8_t* data, size_t pos, uint32_t to_write) {
	data[pos + 3] = to_write & 0xFF;
	data[pos + 2] = (to_write >> 8) & 0xFF;
	data[pos + 1] = (to_write >> 16) & 0xFF;
	data[pos] = (to_write >> 24) & 0xFF;
}

uint32_t read_be32(const uint8_t* data, size_t pos) {
	return data[pos + 3] | (data[pos + 2] << 8) | (data[pos + 1] << 16) | (data[pos] << 24);
}
