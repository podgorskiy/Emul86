#pragma once
#include "CPU.h"
#include "io_layer.h"
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
	void SetScale(int scale);
	void Boot();
	void Boot(int address);
	void PushKey(int c, bool release, bool repeat);

	IO& GetIO();

	void DiskGUI();

	void SetRunning(bool enabled);

	struct GuiHelper
	{
		bool InputWord(const char* label, word* v);
		bool InputSegmentOffset(const char* label, word* segment, word* offset);

		void Init();
		int scale;
		int m_glyphWidth;
	};
private:
	void RunCPUTest();

	CPU m_cpu;
	IO m_io;
	BIOS m_bios;
	int m_scale; 
	std::chrono::time_point<std::chrono::steady_clock> m_start;
	GuiHelper m_gui;

	// Exec contorls
	bool m_run = false;
};
