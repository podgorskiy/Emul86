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
private:
	virtual void Int(CPU::byte x);

	void SetBIOSVars();
	void StoreB(uint16_t offset, uint16_t location, uint8_t x);
	void StoreW(uint16_t offset, uint16_t location, uint16_t x);
	uint8_t GetB(uint16_t offset, uint16_t location);
	uint16_t GetW(uint16_t offset, uint16_t location);
	CPU m_cpu;
	Disk m_disk;
	CPU::byte* m_ram;
	CPU::byte* m_port;

	int m_scale;
};
