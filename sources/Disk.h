#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <memory>
#include <string>

struct DiskImpl;

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
		char volLabel[12 + 512];
		char fileSysType[9 + 512];
	};
	Disk();

	void Open(const char* path, bool inmemory);

	void Close();

	bool Read(char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount);

	bool Write(const char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount);

	bool IsBootlable() const;

	bool IsFloppyDrive() const;

	const BIOS_ParameterBlock& GetBiosBlock();

	size_t size() const;

	std::string GetPath() const;
private:
	uint32_t ToLBA(uint16_t cylinder, uint8_t head, uint8_t sector);

	void ReadBIOS_ParameterBlock(uint32_t offset);

	std::shared_ptr<DiskImpl> m_impl;
};
