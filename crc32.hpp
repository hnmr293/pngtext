#include <vector>
#include <cstdint>

uint32_t crc32(const std::vector<uint8_t> &buffer);
uint32_t crc32(uint32_t crc, const std::vector<uint8_t> &buffer);
uint32_t crc32(uint32_t crc, uint8_t c);
