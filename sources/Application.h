#pragma once
#include "CPU.h"
#include "io.h"
#include "bios.h"
#include <vector>


class Application
{
public:
	Application();
	~Application();

	int Init();
	void Update();
	void LoadData(uint32_t dest, const char* source, uint32_t size);
	void Resize(int width, int height);
	void SetScale(int scale);
	void Boot();

	IO& GetIO();
private:
	void RunCPUTest();

	CPU m_cpu;
	IO m_io;
	BIOS m_bios;
	int m_scale; 
	std::chrono::time_point<std::chrono::system_clock> m_start;
};
