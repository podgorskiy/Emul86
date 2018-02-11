#include "Disk.h"
#include <cstring>

enum
{
	SectorSize = 512
};

struct DiskImpl
{
	DiskImpl();
	~DiskImpl();
	void Close();

	Disk::BIOS_ParameterBlock m_biosBlock;
	size_t m_size;

	bool m_bootable;
	bool m_floppy;

	uint16_t m_physical_secPerTrk;
	uint16_t m_physical_numHeads;
	bool m_doTranslation;
	FILE* m_file;
};


Disk::Disk(): m_impl(new DiskImpl())
{}


DiskImpl::DiskImpl() : m_bootable(false), m_floppy(false)
{}


DiskImpl::~DiskImpl()
{
	Close();
}


void DiskImpl::Close()
{
	fclose(m_file);
}


uint8_t Disk::ReadB(uint32_t location)
{
	uint8_t data;
	fseek(m_impl->m_file, location, SEEK_SET);
	fread(&data, 1, 1, m_impl->m_file);
	return data;
}


uint16_t Disk::ReadW(uint32_t location)
{
	uint16_t data;
	fseek(m_impl->m_file, location, SEEK_SET);
	fread(&data, 2, 1, m_impl->m_file);
	return data;
}


uint32_t Disk::ReadDW(uint32_t location)
{
	uint32_t data;
	fseek(m_impl->m_file, location, SEEK_SET);
	fread(&data, 4, 1, m_impl->m_file);
	return data;
}


void Disk::ReadBIOS_ParameterBlock(uint32_t offset)
{
	BIOS_ParameterBlock& block = m_impl->m_biosBlock;
	block.bytsPerSec = ReadW(0x00B + offset);
	block.secPerClus = ReadB(0x00D + offset);
	block.totSec16 = ReadW(0x013 + offset);
	if (block.totSec16 == 0)
	{
		block.totSec16 = ReadDW(0x020 + offset);
	}
	block.secPerTrk = ReadW(0x018 + offset);
	block.numHeads = ReadW(0x01A + offset);
	block.drvNum = ReadB(0x024 + offset);
	block.volID = ReadDW(0x027 + offset);
	memset(block.volLabel, 0, sizeof(block.volLabel));
	memset(block.fileSysType, 0, sizeof(block.fileSysType));
	for (int i = 0; i < 11; ++i)
	{
		block.volLabel[i] = ReadB(0x2B + i);
	}
	for (int i = 0; i < 8; ++i)
	{
		block.fileSysType[i] = ReadB(0x36 + i);
	}
}


void Disk::Open(const char * path)
{
	BIOS_ParameterBlock& block = m_impl->m_biosBlock;
	m_impl->m_file = fopen(path, "rb+");
	fseek(m_impl->m_file, 0L, SEEK_END);
	m_impl->m_size = ftell(m_impl->m_file);
	fseek(m_impl->m_file, 0L, SEEK_SET);

	// Check for boot sector signature
	m_impl->m_bootable = ReadW(0x1FE) == 0xAA55;

	// Check if it is a floppy drive image
	ReadBIOS_ParameterBlock(0);
	if (block.bytsPerSec == SectorSize)
	{
		// Floppy iamge
		m_impl->m_floppy = true;
		m_impl->m_doTranslation = false;
	}
	else
	{
		// Hard disk image

		// Assume 63/16/x geometry for all images
		m_impl->m_physical_secPerTrk = 63;
		m_impl->m_physical_numHeads = 16;
		block.bytsPerSec = SectorSize;
		int cylinders = static_cast<int>(m_impl->m_size / (m_impl->m_physical_numHeads * m_impl->m_physical_secPerTrk * SectorSize));
		int heads = m_impl->m_physical_numHeads;

		while (cylinders > 1024) {
			cylinders >>= 1;
			heads <<= 1;

			// If we max out the head count
			if (heads > 127) break;
		}
		
		// clip to 1024 cylinders in lchs
		if (cylinders > 1024) cylinders = 1024;
		
		block.numHeads = heads;
		block.secPerTrk = 63;
		
		m_impl->m_size = ((size_t)cylinders * block.numHeads * block.secPerTrk * SectorSize);
		block.totSec16 = static_cast<int>(m_impl->m_size / SectorSize);

		m_impl->m_doTranslation = block.numHeads != m_impl->m_physical_numHeads;
	}
}


uint32_t Disk::ToLBA(uint16_t cylinder, uint8_t head, uint8_t sector)
{
	BIOS_ParameterBlock& block = m_impl->m_biosBlock;
	return (cylinder * block.numHeads + head) * block.secPerTrk + sector - 1;
}


bool Disk::Read(char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount)
{
	uint32_t location = SectorSize * ToLBA(cylinder, head, sector);
	if (location + SectorSize * sectorCount < m_impl->m_size)
	{
		Read(dst, location, sectorCount * SectorSize);
		return true;
	}
	return false;
}


bool Disk::Write(const char* dst, uint16_t cylinder, uint8_t head, uint8_t sector, uint32_t sectorCount)
{
	uint32_t location = SectorSize * ToLBA(cylinder, head, sector);
	if (location + SectorSize * sectorCount < m_impl->m_size)
	{
		Write(dst, location, sectorCount * SectorSize);
		return true;
	}
	return false;
}


bool Disk::IsBootlable() const
{
	return m_impl->m_bootable;
}


bool Disk::IsFloppyDrive() const
{
	return m_impl->m_floppy;
}

void Disk::Close()
{
	m_impl->Close();
}


void Disk::Read(char* dst, uint32_t location, uint32_t size)
{
	fseek(m_impl->m_file, location, SEEK_SET);
	fread(dst, size, 1, m_impl->m_file);
}


void Disk::Write(const char* dst, uint32_t location, uint32_t size)
{
	fseek(m_impl->m_file, location, SEEK_SET);
	fwrite(dst, size, 1, m_impl->m_file);
}


const Disk::BIOS_ParameterBlock& Disk::GetBiosBlock()
{
	return m_impl->m_biosBlock;
}


size_t Disk::size() const
{
	return m_impl->m_size;
}
