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
	void StoreB(uint16_t offset, uint16_t location, uint8_t x);
	void StoreW(uint16_t offset, uint16_t location, uint16_t x);
	uint8_t GetB(uint16_t offset, uint16_t location);
	uint16_t GetW(uint16_t offset, uint16_t location);
	CPU m_cpu;
	Disk m_disk;
	CPU::byte* m_ram;
	CPU::byte* m_port;
	video m_video;
	int m_scale;

	bool m_int16_0;
};
