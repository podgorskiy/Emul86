#pragma once
#include "CPU.h"
#include "Disk.h"
#include <vector>
#include <memory>

class Application: CPU::InterruptHandler
{
public:
	Application();
	~Application();

	int Init();
	void Update();
	void DrawScreen();
	void LoadData(uint32_t dest, const char* source, uint32_t size);
	void Resize(int width, int height);
	void SetScale(int scale);
	bool KeyCallback(int c);
	void PushKey(int c);
	bool HasKeyToPop();
	int PopKey();
	void ClearCurrentCode()
	{
		m_currentKeyCode = 0;
	}
	void SetKeyFlags(bool rightShift, bool leftShift, bool CTRL, bool alt)
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
		StoreB(0x0040, 0x0017, m_keyFlags);
	}
private:
	virtual void Int(CPU::byte x);
	void scrollOneLine();
	struct video
	{
		struct CurPos
		{
			CPU::byte row;
			CPU::byte colum;

		};
		CPU::byte cur_size_first_raw;
		CPU::byte cur_size_last_raw;
		CPU::byte mode;
		CurPos cur_position[8];
		CPU::byte active_page;
	};

	void SetBIOSVars();
	void Store(uint16_t offset, uint16_t location, uint8_t* x, int size);
	void StoreB(uint16_t offset, uint16_t location, uint8_t x);
	void StoreW(uint16_t offset, uint16_t location, uint16_t x);
	void StoreString(uint16_t offset, uint16_t location, const char* x);

	uint8_t GetB(uint16_t offset, uint16_t location);
	uint16_t GetW(uint16_t offset, uint16_t location);
	CPU m_cpu;
	Disk m_disk;
	CPU::byte* m_ram;
	CPU::byte* m_port;
	video m_video;
	int m_scale;
	int m_currentKeyCode;
	int m_keyFlags;

	bool m_int16_0;
};
