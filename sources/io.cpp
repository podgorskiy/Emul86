#include "IO.h"
#include "common_gl.h"
#include <simpletext.h>
#include <string.h>


SimpleText* st = nullptr;

void IO::Init()
{
	m_ram = new byte[0x100000];
	m_port = new byte[0x10000];
	ClearMemory();
	m_currentKeyCode = 0;
	m_start = std::chrono::system_clock::now();
	m_int16_halt = false;
	EnableA20Gate();
}


inline SimpleText::Color ToSimpleTextColor(char x)
{
	SimpleText::Color color = SimpleText::WHITE;
	switch (x)
	{
	case 0x0: color = SimpleText::BLACK; break;
	case 0x1: color = SimpleText::BLUE; break;
	case 0x2: color = SimpleText::GREEN; break;
	case 0x3: color = SimpleText::CYAN; break;
	case 0x4: color = SimpleText::RED; break;
	case 0x5: color = SimpleText::MAGENTA; break;
	case 0x6: color = SimpleText::YELLOW; break;
	case 0x7: color = SimpleText::WHITE; break;
	}
	return color;
}


void IO::DrawScreen(int displayScale)
{
	if (st == nullptr)
	{
		st = new SimpleText;
	}
	st->Begin();
	int screenWidth = MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;
	int active_page = MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);

	uint32_t p = CGA_DISPLAY_RAM << 4;
	for (int row = 0; row < screenHeight; ++row)
	{
		for (int clm = 0; clm < screenWidth; ++clm)
		{
			char symbol = m_ram[p + row * screenWidth * 2 + clm * 2 + screenHeight * screenWidth * 2 * active_page];
			char attribute = m_ram[p + row * screenWidth * 2 + clm * 2 + screenHeight * screenWidth * 2 * active_page + 1];

			char forgroundColor = attribute & 0x07;
			char backgroundColor = (attribute & 0x70) >> 4;
			bool forgroundBright = (attribute & 0x08) != 0;
			bool backgroundBright = (attribute & 0x80) != 0;

			SimpleText::Color forgroundColorST = ToSimpleTextColor(forgroundColor);
			SimpleText::Color backgroundColorST = ToSimpleTextColor(backgroundColor);
			st->SetTextSize((SimpleText::FontSize)displayScale);
			st->SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st->SetColor(SimpleText::BACKGROUND_COLOR, backgroundColorST, backgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st->PutSymbol(symbol, clm * 8 * displayScale, row * 16 * displayScale);
		}
	}

	int top = MemGetB(BIOS_DATA_OFFSET, CURSOR_ENDING);
	int bottom = MemGetB(BIOS_DATA_OFFSET, CURSOR_STARTING);

	int cursor = MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * active_page);
	int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;

	auto now = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = now - m_start;

	st->End();
	if (!(bottom & 0x20) && ((int)(5 * elapsed_seconds.count())) % 2)
	{
		st->EnableBlending(true);
		st->Begin();
		char attribute = m_ram[p + ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * active_page + 1];
		char forgroundColor = attribute & 0x07;
		bool forgroundBright = (attribute & 0x08) != 0;
		SimpleText::Color forgroundColorST = ToSimpleTextColor(forgroundColor);
		st->SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
		st->SetColor(SimpleText::BACKGROUND_COLOR, 0, 0, 0, 0);
		st->PutSymbol('_', xcurs * 8 * displayScale, ycurs * 16 * displayScale);
		st->End();
		st->EnableBlending(false);
	}

	MemStoreD(BIOS_DATA_OFFSET, 0x006C, int(elapsed_seconds.count() * 1193180 / 65536));
}


void IO::MemStoreB(word offset, word location, byte x)
{
	m_ram[select(offset, location)] = x;
}


void IO::MemStoreW(word offset, word location, word x)
{
	uint32_t address = select(offset, location);
	m_ram[address] = x & 0xFF;
	m_ram[address + 1] = (x & 0xFF00) >> 8;
}


void IO::MemStoreD(word offset, word location, uint32_t x)
{
	uint32_t address = select(offset, location);
	m_ram[address] = x & 0xFF;
	m_ram[address + 1] = (x & 0xFF00) >> 8;
	m_ram[address + 2] = (x & 0xFF0000) >> 16;
	m_ram[address + 3] = (x & 0xFF000000) >> 24;
}


void IO::MemStoreString(word offset, word location, const char* x)
{
	uint32_t address = select(offset, location);
	strcpy((char*)m_ram + address, x);
}


void IO::MemStore(word offset, word location, byte* x, int size)
{
	uint32_t address = select(offset, location);
	memcpy((char*)m_ram + address, x, size);
}

byte IO::MemGetB(word offset, word location) const
{
	return m_ram[select(offset, location)];
}


word IO::MemGetW(word offset, word location) const
{
	uint32_t address = select(offset, location);
	return ((word)m_ram[address]) | (((word)m_ram[address + 1]) << 8);
}


uint32_t IO::MemGetD(word offset, word location) const
{
	uint32_t address = select(offset, location);
	return 0 
		| ((word)m_ram[address]) 
		| (((word)m_ram[address + 1]) << 8) 
		| (((word)m_ram[address + 2]) << 16) 
		| (((word)m_ram[address + 3]) << 24);
}


void IO::ClearMemory()
{
	memset(m_ram, 0, 0x100000);
	memset(m_port, 0, 0x10000);
}

byte * IO::GetRamRawPointer()
{
	return m_ram;
}

void IO::EnableA20Gate()
{
	A20 = 0xFFFFFF;
}


void IO::DisableA20Gate()
{
	A20 = 0x0FFFFF;
}


bool IO::GetA20GateStatus() const
{
	return A20 == 0xFFFFFF;
}


std::pair<bool, word> IO::UpdateKeyState()
{
	bool write_to_ax = false;
	word ax_value_to_write = 0;

	if (keyBuffer.size() > 0)
	{
		if (keyBuffer.front() != 0)
		{
			int key = keyBuffer.front();
			bool was_halted = SetCurrentKey(key);
			if (was_halted)
			{
				write_to_ax = true;
				ax_value_to_write = key;
			}

			if (was_halted)
			{
				keyBuffer.pop_front();
				if (keyBuffer.size() != 0)
				{
					SetCurrentScanCode(keyBuffer.front());
				}
				else
				{
					ClearCurrentCode();
				}
				PopKey();
			}
		}
		else
		{
			keyBuffer.pop_front();
		}
	}
	else
	{
		ClearCurrentCode();
	}
	return std::make_pair(write_to_ax, ax_value_to_write);
}


void IO::KeyboardHalt()
{
	m_int16_halt = true;
}


bool IO::ISKeyboardHalted() const
{
	return m_int16_halt;
}


int IO::GetCurrentKeyCode() const
{
	return m_currentKeyCode;
}


void IO::ClearCurrentCode()
{
	m_currentKeyCode = 0;
}


void IO::SetCurrentScanCode(int c)
{
	m_currentKeyCode = c;
}


void IO::SetKeyFlags(bool rightShift, bool leftShift, bool CTRL, bool alt)
{
	/*
	AL = BIOS keyboard flags (located in BIOS Data Area 40:17)

	|7|6|5|4|3|2|1|0|  AL or BIOS Data Area 40:17
	| | | | | | | `---- right shift key depressed
	| | | | | | `----- left shift key depressed
	| | | | | `------ CTRL key depressed
	| | | | `------- ALT key depressed
	| | | `-------- scroll-lock is active
	| | `--------- num-lock is active
	| `---------- caps-lock is active
	`----------- insert is active
	*/
	m_keyFlags = rightShift * 1 | leftShift * 2 | CTRL * 4 | alt * 8;
	MemStoreB(0x0040, 0x0017, m_keyFlags);
}


void IO::AddDisk(Disk disk)
{
	if (disk.IsFloppyDrive())
	{
		m_floppyDrives.push_back(disk);
	}
	else
	{
		m_hardDrives.push_back(disk);
	}
}


int IO::GetDiskCount() const
{
	return static_cast<int>(m_floppyDrives.size() + m_hardDrives.size());
}


bool IO::HasDisk(int diskNo) const
{
	int disk = diskNo & 0x7F;
	if (diskNo & 0x80)
	{
		return m_hardDrives.size() > disk;
	}
	else
	{
		return m_floppyDrives.size() > disk;
	}
}


bool IO::ReadDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount)
{
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.Read((char*)m_ram + ram_address, cylinder, head, sector, sectorCount);
}


bool IO::WriteDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount)
{
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.Write((const char*)m_ram + ram_address, cylinder, head, sector, sectorCount);
}


const Disk::BIOS_ParameterBlock& IO::GetDiskBPB(int diskNo) const
{
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.GetBiosBlock();
}


int IO::GetBootableDisk() const
{
	for (int i = 0; i < m_floppyDrives.size(); ++i)
	{
		if (m_floppyDrives[i].IsBootlable())
		{
			return i;
		}
	}
	for (int i = 0; i < m_hardDrives.size(); ++i)
	{
		if (m_hardDrives[i].IsBootlable())
		{
			return i + 0x80;
		}
	}
	return -1;
}


bool IO::SetCurrentKey(int c)
{
	m_currentKeyCode = c;
	MemStoreB(BIOS_DATA_OFFSET, 0xBA, c >> 8);
	if (m_int16_halt)
	{
		m_int16_halt = false;
		return true;
	}
	return false;
}


void IO::PushKey(int c)
{
	int tail = MemGetW(BIOS_DATA_OFFSET, 0x1C);
	if (tail < 0x1E)
	{
		tail = 0x1E;
	}
	MemStoreW(BIOS_DATA_OFFSET, tail, c);
	tail += 2;
	if ((0x1E + 32) < tail)
	{
		tail = 0x1E;
	}
	MemStoreW(BIOS_DATA_OFFSET, 0x1C, tail);

	keyBuffer.push_back(c);
}


int IO::PopKey()
{
	int head = MemGetW(BIOS_DATA_OFFSET, 0x1A);
	int c = MemGetW(BIOS_DATA_OFFSET, head);
	head += 2;
	if (head < 0x1E)
	{
		head = 0x1E;
	}
	if ((0x1E + 32) <= head)
	{
		head = 0x1E;
	}
	MemStoreW(BIOS_DATA_OFFSET, 0x1A, head);
	return c;
}


bool IO::HasKeyToPop() const
{
	int head = MemGetW(BIOS_DATA_OFFSET, 0x1A);
	int tail = MemGetW(BIOS_DATA_OFFSET, 0x1C);
	return head != tail;
}
