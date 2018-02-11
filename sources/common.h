#pragma once
#include "_assert.h"
#include <inttypes.h>
#include <string>
#include <string.h>

typedef uint16_t word;
typedef int16_t sword;
typedef uint8_t byte;
typedef int8_t sbyte;

enum
{
	MEMORY_SIZE_KB = 0x027F,

	// BIOD Data Area
	BIOS_SEGMENT = 0xF000,
	BIOS_DATA_OFFSET = 0x040,
	EQUIPMENT_LIST_FLAGS = 0x0010,
	MEMORY_SIZE_FIELD = 0x0013,
	CURSOR_POSITION = 0x0050,
	CURSOR_ENDING = 0x0060,
	CURSOR_STARTING = 0x0061,
	ACTIVE_DISPLAY_PAGE = 0x0062,
	BIOSMEM_NB_ROWS = 0x84,
	BIOSMEM_CHAR_HEIGHT = 0x85,
	SYS_MODEL_ID = 0xFC,
	VIDEO_MOD = 0x0049,
	NUMBER_OF_SCREEN_COLUMNS = 0x004A,

	// Tables locations
	DISKETTE_CONTROLLER_PARAMETER_TABLE = 0xefc7,
	DISKETTE_CONTROLLER_PARAMETER_TABLE2 = 0xefde,
	BIOS_CONFIG_TABLE = 0xe6f5,
	DEFAULT_HANDLER = 0xff53,
	
	CGA_DISPLAY_RAM = 0xB800,
	UNSUPPORTED_FUNCTION = 0x86,

	IRET_INSTRUCTION = 0xCF,
};

template <typename I>
inline std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

template <typename I>
inline char* n2hexstr(char* dst, I w, size_t hex_len = sizeof(I) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	memset(dst, '0', hex_len);
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		dst[i] = digits[(w >> j) & 0x0f];
	return &(*(dst + hex_len) = '\x0');
}

inline uint32_t select(word seg, word off)
{
	return (((uint32_t)seg) << 4) + off;
}

inline uint8_t decToBcd(uint8_t val)
{
	return ((val / 10 * 16) + (val % 10));
}
