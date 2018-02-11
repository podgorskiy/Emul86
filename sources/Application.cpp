#include "Application.h"
#include "_assert.h"
#include "common_gl.h"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <string>


Application::Application(): m_cpu(m_io), m_bios(m_io, m_cpu)
{
}


int Application::Init()
{
	m_io.Init();
	m_cpu.SetInterruptHandler(&m_bios);
	m_io.ClearMemory();
	Disk disk;
	disk.Open("../c.img");
	m_io.AddDisk(disk);
	Boot();
	return EXIT_SUCCESS;
}


Application::~Application()
{
}


void Application::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
}


void Application::SetScale(int scale)
{
	m_scale = scale;
}


IO& Application::GetIO()
{
	return m_io;
}


void Application::Update()
{
	static bool run = false;

	uint32_t stopAt = -1;// select(0xF000, 0x00E5);
	if (select(m_cpu.GetSegment(CPU::CS), m_cpu.IP) == stopAt)
	{
		run = false;
	}

#ifdef _DEBUG
	int doStep = 0;
	ImGui::Begin("Execution control");
	if (ImGui::Button("Step"))
	{
		doStep = 1;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Run", &run);
	if (run)
	{
		doStep = 100000000;
	}

	if (m_io.ISKeyboardHalted())
	{
		doStep = 0;
	}
#else
	int doStep = 100000000;
#endif
	if (doStep)
	{
		std::chrono::time_point<std::chrono::system_clock> framestart = std::chrono::system_clock::now();

		for (int i = 0; i < doStep && !m_io.ISKeyboardHalted(); ++i)
		{
			if (i % 100000 == 0)
			{
				auto current_timestamp = std::chrono::system_clock::now();
				std::chrono::duration<float> elapsed_time = (current_timestamp - framestart);
				float seconds = elapsed_time.count();
				if (seconds > 0.018f)
				{
#ifdef _DEBUG
					ImGui::Text("IPS: %d", (int)(i / seconds));
#endif
					break;
				}
			}
			m_cpu.Step();
#ifdef _DEBUG
			if (select(m_cpu.GetSegment(CPU::CS), m_cpu.IP) == stopAt)
			{
				run = false;
				break;
			}
#endif
		}
	}

#ifdef _DEBUG
	ImGui::End();
	m_cpu.GUI();

	ImGui::Begin("Disk");
	/*
	if (ImGui::Button("Mount Dos6.22"))
	{
		m_disk.Open("Dos6.22.img");
	}
	if (ImGui::Button("Mount GlukOS.IMA"))
	{
		m_disk.Open("GlukOS.IMA");
	}
	if (ImGui::Button("Mount mikeos.dmg"))
	{
		m_disk.Open("mikeos.dmg");
	}
	if (ImGui::Button("Mount freedos722.img"))
	{
		m_disk.Open("freedos722.img");
	}
	if (ImGui::Button("Mount Dos3.3.img"))
	{
		m_disk.Open("Dos3.3.img");
	}
	if (ImGui::Button("Mount Dos4.01.img"))
	{
		m_disk.Open("Dos4.01.img");
	}
	if (ImGui::Button("Mount Dos5.0.img"))
	{
		m_disk.Open("Dos5.0.img");
	}
	if (ImGui::Button("Mount c.img"))
	{
		m_disk.Open("F:/c.img");
	}*/
	
	static int loadAddress = 0x7C00;
	ImGui::PushItemWidth(150);
	ImGui::InputInt("Boot address", &loadAddress, 0, 0, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal);
	if (ImGui::Button("Boot"))
	{
		m_io.ClearMemory();
		m_bios.InitBIOSDataArea();
		m_io.KeyCallback(0);
		int disk = m_io.GetBootableDisk();
		if (disk != -1)
		{
			m_io.ReadDisk(disk, loadAddress, 0, 0, 1, 1);
		}
		m_cpu.Reset();
		m_cpu.IP = loadAddress;
		m_cpu.SetSegment(CPU::CS, 0);
	}

	if (ImGui::Button("Load testrom"))
	{
		m_bios.InitBIOSDataArea();
		m_io.KeyCallback(0);
		FILE* f = fopen("../testrom", "rb");
		fseek(f, 0L, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0L, SEEK_SET);
		uint8_t* buff = new uint8_t[size];
		fread(buff, size, 1, f);
		m_io.MemStore(BIOS_SEGMENT, 0x0, buff, static_cast<int>(size));
		delete[] buff;
		m_cpu.Reset();
		m_cpu.IP = 0xfff0;
		m_cpu.SetSegment(CPU::CS, BIOS_SEGMENT);
	}

	if (ImGui::Button("TestCPU"))
	{
		RunCPUTest();
	}

	ImGui::End();
#endif

	m_io.DrawScreen(m_scale);
}

void Application::RunCPUTest()
{
	m_bios.InitBIOSDataArea();

	FILE* f = fopen("../testrom", "rb");
	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	uint8_t* buff = new uint8_t[size];
	fread(buff, size, 1, f);
	m_io.MemStore(BIOS_SEGMENT, 0x0, buff, static_cast<int>(size));
	delete[] buff;
	m_cpu.Reset();
	m_cpu.IP = 0xfff0;
	m_cpu.SetSegment(CPU::CS, BIOS_SEGMENT);
	m_cpu.SetRegister<word>(CPU::AX, 0);
	m_cpu.SetRegister<word>(CPU::DI, 0);
	m_cpu.SetRegister<word>(CPU::SP, 0);

	std::ifstream input("../ref_log.txt");

	std::string line;
	bool falied = false;
	while (m_cpu.IP != 0xE448)
	{
		std::getline(input, line);

		const char* debugstring = m_cpu.GetDebugString();

		if (strcmp(debugstring, line.c_str()) != 0)
		{
			std::string state = debugstring;
			printf("Error executing %s\n", m_cpu.GetLastCommandAsm());
			printf("Was expecting state:\n");
			printf("|CS||IP||AX||CX||DX||BX||SP||BP||SI||DI||ES||CS||SS||DS||F |\n"); 
			printf("%s\n", line.c_str());
			printf("Got:\n");
			printf("|CS||IP||AX||CX||DX||BX||SP||BP||SI||DI||ES||CS||SS||DS||F |\n");
			printf("%s\n", state.c_str());
			falied = true;
			break;
		}

		m_cpu.Step();
	}

	if (!falied)
	{
		printf("Passed test!:\n");
	}
}


bool Application::KeyCallback(int key)
{
	bool was_halted = m_io.KeyCallback(key);
	if (was_halted)
	{
		m_cpu.SetRegister<word>(CPU::AX, key);
		return true;
	}
	return false;
}

void Application::Boot()
{
	int loadAddress = 0x7C00;
	m_io.ClearMemory();
	m_bios.InitBIOSDataArea();
	m_io.KeyCallback(0);
	int disk = m_io.GetBootableDisk();
	if (disk != -1)
	{
		m_io.ReadDisk(disk, loadAddress, 0, 0, 1, 1);
	}
	m_cpu.Reset();
	m_cpu.IP = loadAddress;
	m_cpu.SetSegment(CPU::CS, 0);
}
