#include "Application.h"
#include "_assert.h"
#include <imgui.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <fstream>
#include <sstream>
#include <string>

#ifdef __EMSCRIPTEN__
#else
#define EM_ASM_(...)
#define EM_ASM(...)
#endif

namespace Assert
{
	extern std::function<void()> OpPause;
}

Application::Application(): m_cpu(m_io), m_bios(m_io, m_cpu)
{
	Assert::OpPause = [this](){ m_run = false; };
}

int Application::Init()
{
	m_io.Init();
	m_io.ClearMemory();
	m_cpu.SetInterruptHandler(&m_bios);
	m_io.AddCpu(m_cpu);
	m_gui.Init();

//#ifndef EMUL86_GUI
	Disk disk;	disk.Open("dos622_nc_tp_pop", true);
	m_io.AddDisk(disk);
	Boot();
//#endif

	return EXIT_SUCCESS;
}


Application::~Application()
{
}

void Application::SetScale(int scale)
{
	m_scale = scale;
}


IO& Application::GetIO()
{
	return m_io;
}

void Application::PushKey(int c, bool release, bool repeat)
{
	if (m_run)
	{
		// If interrupt 0x16 is overwritten, most likely input is handled through scancodes, 
		// so we don't need to spam CPU with repeated interrupt 9.
		if (!(repeat && m_io.IsCustomIntHandlerInstalled(0x16)))
		{
			m_io.PushKey(c, release);
		}
	}
}

int64_t g_instruction = 0;
int64_t g_frame = 0;

void Application::Update()
{
	g_frame++;
	bool singleStep = false;

#ifdef EMUL86_GUI
	static word stopAt_segment = 0;
	static word stopAt_offset = 0;
	uint32_t stopAt = select(stopAt_segment, stopAt_offset);

	ImGui::Begin("Execution control");
	if (ImGui::Button("Step"))
	{
		singleStep = true;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Run", &m_run);

	static word loadAddress = 0x7C00;
	ImGui::PushItemWidth(75 * ImGui::GetIO().FontGlobalScale);
	m_gui.InputWord("Boot addr.", &loadAddress);

	if (ImGui::Button("Boot", ImVec2(60 * ImGui::GetIO().FontGlobalScale, 30 * ImGui::GetIO().FontGlobalScale)))
	{
		Boot(loadAddress);
	}

	m_gui.InputSegmentOffset("Breakpoint", &stopAt_segment, &stopAt_offset);

	if (select(m_cpu.GetSegment(CPU::CS), m_cpu.IP) == stopAt)
	{
		m_run = false;
	}
#endif

	if (m_run)
	{
		m_io.UpdateKeyState();
	}

	static int64_t instructions = 0;
	static float seconds = 0;
	static int64_t IPS = 1;
	static int64_t current_pit_clock_count = 0;
	static int64_t last_pit_clock_count = 0;

	std::chrono::time_point<std::chrono::steady_clock> framestart = std::chrono::steady_clock::now();

#ifdef EMUL86_GUI
	if (singleStep)
	{
		m_cpu.Step();
		m_run = false;
	}
#endif

	for (int i = 0; m_run && !m_io.IsKeyboardHalted(); ++i)
	{
		// Clock related stuff is updated once per 400 cycles
		if (i % 400 == 0)
		{
			auto current_timestamp = std::chrono::steady_clock::now();
			std::chrono::duration<float> elapsed_time_from_frame = (current_timestamp - framestart);
			std::chrono::duration<double> elapsed_time_from_start = (current_timestamp - m_start);

			last_pit_clock_count = current_pit_clock_count;
			current_pit_clock_count = int64_t(elapsed_time_from_start.count() * 1193182.0);
			m_io.PITUpdate((int)(current_pit_clock_count - last_pit_clock_count));
			m_io.MemStoreD(BIOS_DATA_OFFSET, 0x006C, int(current_pit_clock_count >> 16));
			
			float elapsed = elapsed_time_from_frame.count();
			if (elapsed > 0.018f)
			{
				seconds += elapsed;

				if (seconds > 1.0f)
				{
					IPS = (int64_t)(instructions / seconds);
					instructions = 0;
					seconds = 0;
					printf("IPS: %d\n", (int)IPS);

					EM_ASM_({
						UpdateIPS($0);
					}, (int)IPS);

				}
				break;
			}
		}
		m_cpu.Step();
		++instructions; 
		++g_instruction;
#ifdef EMUL86_GUI
		if (select(m_cpu.GetSegment(CPU::CS), m_cpu.IP) == stopAt || singleStep) 
		{
			m_run = false;
			singleStep = false;
			break;
		}
#endif
	}

#ifdef EMUL86_GUI
	ImGui::Text("IPS: %d", (int)IPS);

	ImGui::End();
	m_cpu.GUI();
	DiskGUI();
#endif

	EM_ASM({
		HDDLight($0);
	}, (int)m_io.HDDLight());

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

void Application::Boot(int address)
{
	m_io.ClearMemory();
	m_bios.InitBIOSDataArea();
	m_io.SetCurrentKey(0);
	int disk = m_io.GetBootableDisk();
	if (disk != -1)
	{
		m_io.ReadDisk(disk, address, 0, 0, 1, 1);
	}
	m_cpu.Reset();
	m_cpu.IP = address;
	m_cpu.SetSegment(CPU::CS, 0);
	m_start = std::chrono::steady_clock::now();
	m_run = true;
}

void Application::Boot()
{
	Boot(0x7C00);
}

void Application::SetRunning(bool enabled)
{
	m_run = enabled;
}

void Application::DiskGUI()
{
	ImGui::Begin("Disk");

	static std::vector<std::string> disks = { "dos622_nc_tp_pop", "Dos6.22", "mikeos", "freedos722", "Dos3.3", "Dos4.01", "Dos5.0" };

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	for (int i = 0, count = disks.size(); i < count; ++i)
	{
		if (ImGui::Button(disks[i].c_str()))
		{
			Disk disk;	
			disk.Open(disks[i].c_str(), true);
			m_io.AddDisk(disk);
		}
		if (i + 1 < count)
		{
			float last_button_x2 = ImGui::GetItemRectMax().x;
			float next_button_x2 = last_button_x2 + style.ItemSpacing.x + ImGui::CalcTextSize(disks[i + 1].c_str()).x + style.FramePadding.x * 2.0f; // Expected position if next button was on same line

			if (next_button_x2 < window_visible_x2)
			{
				ImGui::SameLine();
			}
		}
	}

	if (ImGui::Button("Unmount all"))
	{
		m_io.ClearDisks();
	}

	m_io.GUI();

	/*
#ifdef _DEBUG
	if (ImGui::Button("Load testrom"))
	{
		m_bios.InitBIOSDataArea();
		m_io.SetCurrentKey(0);
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
		m_start = std::chrono::steady_clock::now();
	}

	if (ImGui::Button("TestCPU"))
	{
		RunCPUTest();
	}
#endif
	*/
	ImGui::End();
}

bool Application::GuiHelper::InputWord(const char* label, word* v)
{
	ImGuiStyle& style = ImGui::GetStyle();
	uint32_t x = *v;
	ImGui::PushItemWidth(m_glyphWidth * 5 + +style.FramePadding.x * 2.0f);
	bool r = ImGui::InputScalar(label, ImGuiDataType_U32, (void*)&x, 0, 0, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();
	*v = x;
	return r;
}

bool Application::GuiHelper::InputSegmentOffset(const char* label, word* segment, word* offset)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushID(label);
	bool r = InputWord("##segment", segment);
	ImGui::SameLine();
	ImGui::Text(":");
	ImGui::SameLine();
	r = r || InputWord("##offset", offset);
	ImGui::SameLine(0, style.ItemInnerSpacing.x);
	ImGui::TextUnformatted(label);
	ImGui::PopID();
	return r;
}

void Application::GuiHelper::Init()
{
	// We assume the font is mono-space
	m_glyphWidth = ImGui::CalcTextSize("F").x + 1;
}
