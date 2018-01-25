#include "Disk.h"
#include <cstring>

uint8_t Disk::ReadB(uint32_t location)
{
	uint8_t data;
	fseek(m_file, location, SEEK_SET);
	fread(&data, 1, 1, m_file);
	return data;
}

uint16_t Disk::ReadW(uint32_t location)
{
	uint16_t data;
	fseek(m_file, location, SEEK_SET);
	fread(&data, 2, 1, m_file);
	return data;
}

uint32_t Disk::ReadDW(uint32_t location)
{
	uint32_t data;
	fseek(m_file, location, SEEK_SET);
	fread(&data, 4, 1, m_file);
	return data;
}

void Disk::Open(const char * path)
{
	m_file = fopen(path, "rb");
	fseek(m_file, 0L, SEEK_END);
	m_size = ftell(m_file);
	fseek(m_file, 0L, SEEK_SET);

	m_biosBlock.bytsPerSec = ReadW(0x00B);
	m_biosBlock.secPerClus = ReadB(0x00D);
	m_biosBlock.totSec16 = ReadW(0x013);
	if (m_biosBlock.totSec16 == 0)
	{
		m_biosBlock.totSec16 = ReadDW(0x020);
	}
	m_biosBlock.secPerTrk = ReadW(0x018);
	m_biosBlock.numHeads = ReadW(0x01A);
	m_biosBlock.drvNum = ReadB(0x024);
	m_biosBlock.volID = ReadDW(0x027);
	memset(m_biosBlock.volLabel, 0, sizeof(m_biosBlock.volLabel));
	memset(m_biosBlock.fileSysType, 0, sizeof(m_biosBlock.fileSysType));
	for (int i = 0; i < 11; ++i)
	{
		m_biosBlock.volLabel[i] = ReadB(0x2B + i);
	}
	for (int i = 0; i < 8; ++i)
	{
		m_biosBlock.fileSysType[i] = ReadB(0x36 + i);
	}
	//printf("Disk: %d\n", m_biosBlock.volID);
	//printf("FS: %s\n", m_biosBlock.fileSysType);
}

void Disk::Close()
{
	fclose(m_file);
}

void Disk::Read(char* dst, uint32_t location, uint32_t size)
{
	fseek(m_file, location, SEEK_SET);
	fread(dst, size, 1, m_file);
}