#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <memory>

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
		char volLabel[12];
		char fileSysType[9];
	};
	Disk();

	void Open(const char* path);

	void Close();

	bool Read(char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount);

	bool Write(const char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount);

	bool IsBootlable() const;

	bool IsFloppyDrive() const;

	const BIOS_ParameterBlock& GetBiosBlock();

	size_t size() const;

private:
	uint32_t ToLBA(uint16_t cylinder, uint8_t head, uint8_t sector);
	void Read(char* dst, uint32_t location, uint32_t size);
	void Write(const char* dst, uint32_t location, uint32_t size);

	void ReadBIOS_ParameterBlock(uint32_t offset);

	uint8_t ReadB(uint32_t location);
	uint16_t ReadW(uint32_t location);
	uint32_t ReadDW(uint32_t location);
	
	std::shared_ptr<DiskImpl> m_impl;
};
