#pragma once
#include <inttypes.h>
#include <stdio.h>

class Disk
{
public:
	struct BIOS_ParameterBlock
	{
		uint16_t bytsPerSec;
		uint8_t secPerClus;
		uint32_t totSec16;
		uint16_t secPerTrk;
		uint16_t numHeads;
		uint8_t drvNum;
		uint32_t volID;
		char volLabel[12];
		char fileSysType[9];
	};

	void Open(const char* path);

	void Close();

	void Read(char* dst, uint32_t location, uint32_t size);

	void Write(const char* dst, uint32_t location, uint32_t size);
	
	const BIOS_ParameterBlock& GetBiosBlock()
	{
		return m_biosBlock;
	}

	size_t size() const
	{
		return m_size;
	}
private:
	uint8_t ReadB(uint32_t location);
	uint16_t ReadW(uint32_t location);
	uint32_t ReadDW(uint32_t location);

	BIOS_ParameterBlock m_biosBlock;
	FILE* m_file;
	size_t m_size;
};