#include "Application.h"
#include "_assert.h"
#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <emscripten/emscripten.h>

inline int gl3wInit()
{
	return 0;
}

inline bool gl3wIsSupported(int, int)
{
	return true;
}

#else
#include <GL/gl3w.h>
#endif

#include <imgui.h>
#include <simpletext.h>

enum
{
	MEMORY_SIZE_KB = 0x0280,
	VIDEO_MOD = 0x0049,
};

inline uint32_t AbsAddress(uint16_t offset, uint16_t location)
{
	uint32_t address = offset;
	address = address << 4;
	address += location;
	return address;
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

Application::Application()
{
}

int Application::Init()
{
	m_ram = new CPU::byte[0x100000];
	m_port = new CPU::byte[0x10000];
	memset(m_ram, 0, 0x100000);
	memset(m_port, 0, 0x10000);
	m_cpu.SetRamPort(m_ram, m_port);
	m_cpu.SetInterruptHandler(this);

	SetBIOSVars();

	m_disk.Open("Dos6.22.img");
	uint32_t loadAddress = 0x7C00;
	m_disk.Read((char*)m_ram + loadAddress, 0, 0x200);
	m_cpu.IP = loadAddress;
	m_cpu.SetSegment(CPU::CS, 0);

	return EXIT_SUCCESS;
}

Application::~Application()
{
}

void Application::LoadData(uint32_t dest, const char* source, uint32_t size)
{
	memcpy(m_ram + dest, source, size);
}

void Application::Resize(int width, int height)
{
	glViewport(0, 0, width, height);
}

void Application::SetScale(int scale)
{
	m_scale = scale;
}

void Application::Update()
{
	float doStep = false;
	ImGui::Begin("Execution control");
	if (ImGui::Button("Step"))
	{
		doStep = true;
	}
	ImGui::SameLine();
	static bool run = false;
	ImGui::Checkbox("Run", &run);
	if (run)
	{
		doStep = true;
	}
	ImGui::End();

	if (doStep)
	{
		m_cpu.Step();
	}

	m_cpu.GUI();

	ImGui::Begin("Disk");
	if (ImGui::Button("Mount Dos6.22"))
	{
		m_disk.Open("Dos6.22.img");
	}
	if (ImGui::Button("Mount GlukOS.IMA"))
	{
		m_disk.Open("GlukOS.IMA");
	}
	static int loadAddress = 0x7C00;
	ImGui::PushItemWidth(150);
	ImGui::InputInt("Boot address", &loadAddress, 0, 0, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal);
	if (ImGui::Button("Boot"))
	{
		m_disk.Read((char*)m_ram + (uint32_t)loadAddress, 0, 0x200);
		m_cpu.IP = loadAddress;
		m_cpu.SetSegment(CPU::CS, 0);
	}
	ImGui::Text(m_disk.GetBiosBlock().volLabel);
	ImGui::Text(m_disk.GetBiosBlock().fileSysType);
	ImGui::End();
}

void Application::DrawScreen()
{
	static SimpleText st;

	st.Begin();
	int screenWidth = GetW(0x40, 0x004a);

	uint32_t p = 0xB800 << 4;
	for (int row = 0; row < 25; ++row)
	{
		for (int clm = 0; clm < screenWidth; ++clm)
		{
			char symbol = m_ram[p + row * screenWidth * 2 + clm * 2];
			char attribute = m_ram[p + row * screenWidth * 2 + clm * 2 + 1];

			char forgroundColor = attribute & 0x07;
			char backgroundColor = (attribute & 0x70) >> 4;
			bool forgroundBright = (attribute & 0x08) != 0;
			bool backgroundBright = (attribute & 0x80) != 0;

			SimpleText::Color forgroundColorST = ToSimpleTextColor(forgroundColor);
			SimpleText::Color backgroundColorST = ToSimpleTextColor(backgroundColor);
			st.SetTextSize((SimpleText::FontSize)m_scale);
			st.SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st.SetColor(SimpleText::BACKGROUND_COLOR, backgroundColorST, backgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st.PutSymbol(symbol, clm * 8 * m_scale, row * 16 * m_scale);
		}
	}
	st.End();
}

void Application::StoreB(uint16_t offset, uint16_t location, uint8_t x)
{
	m_ram[AbsAddress(offset, location)] = x;
}

void Application::StoreW(uint16_t offset, uint16_t location, uint16_t x)
{
	uint32_t address = AbsAddress(offset, location);
	m_ram[address] = x & 0xFF;
	m_ram[address + 1] = (x & 0xFF00) >> 8;
}

uint8_t Application::GetB(uint16_t offset, uint16_t location)
{
	return m_ram[AbsAddress(offset, location)];
}

uint16_t Application::GetW(uint16_t offset, uint16_t location)
{
	uint32_t address = AbsAddress(offset, location);
	return ((uint16_t)m_ram[address]) | (((uint16_t)m_ram[address + 1]) << 8);
}

void Application::SetBIOSVars()
{
	StoreW(0x40, 0x0000, 0x03F8); // COM1
	StoreW(0x40, 0x0002, 0x02F8); // COM2
	StoreW(0x40, 0x0004, 0x03E8); // COM3
	StoreW(0x40, 0x0006, 0x02E8); // COM4

	StoreW(0x40, 0x0013, MEMORY_SIZE_KB); //Total memory in K-bytes
	StoreB(0x40, VIDEO_MOD, 0x0000); // Current active video mode

	StoreW(0x40, 0x004a, 80); // Screen width in text columns

	StoreW(0x0, 0x78, 0xefc7); // pointer to diskette_param_table

	uint8_t diskette_param_table[] = 
	{
		0xAF,
		0x02,// head load time 0000001, DMA used
		0x25,
		0x02,
		  18,
		0x1B,
		0xFF,
		0x6C,
		0xF6,
		0x0F,
		0x08,
		  79, // maximum track
		   0, // data transfer rate
		   4  // drive type in cmos
	};
	memcpy(m_ram + 0xefc7, diskette_param_table, sizeof(diskette_param_table));
}

#define SET_AH(b) m_cpu.SetRegB(CPU::AH, b)
#define GET_AH() m_cpu.GetRegB(CPU::AH)
#define GET_AL() m_cpu.GetRegB(CPU::AL)
#define SET_DISK_RET_STATUS(status) StoreB(0x0040, 0x0074, status)
#define SET_CF() m_cpu.SetFlag<CPU::CF>()
#define CLEAR_CF() m_cpu.ClearFlag<CPU::CF>()

void Application::Int(CPU::byte x)
{
	CPU::byte function;
	CPU::byte arg;
	CPU::byte status;
	switch (x)
	{
	// Video Services
	case 0x10:
		function = GET_AH();
		arg = GET_AL();
		switch (function)
		{
		//Set Video Mode
		case 0x00:
			StoreB(0x40, VIDEO_MOD, arg);
		case 0x0E:
			printf("%c", arg);
			break;
		default:
			ASSERT(false, "Unknown int10 function: 0x%s\n", n2hexstr(function).c_str());
			break;
		}

	// Conventional Memory Size
	case 0x12:
		m_cpu.SetRegW(CPU::AX, MEMORY_SIZE_KB);
		break;
	
	// disk I/O service
	case 0x13:
		function = GET_AH();
		switch (function)
		{
		case 0x00: /* disk controller reset */
			goto int13_success;
			break;
		case 0x01: /* read disk status */
			status = GetB(0x0040, 0x0074); 
			SET_AH(status);
			SET_DISK_RET_STATUS(0);
			/* set CF if error status read */
			if (status) SET_CF();
			else CLEAR_CF();
			return; 
		case 0x04: // verify disk sectors
		case 0x02: // read disk sectors
			/*
			AL = number of sectors to read	(1-128 dec.)
			CH = track/cylinder number  (0-1023 dec., see below)
			CL = sector number  (1-17 dec.)
			DH = head number  (0-15 dec.)
			DL = drive number (0=A:, 1=2nd floppy, 80h=drive 0, 81h=drive 1)
			ES:BX = pointer to buffer
			*/
			{
				int DiskNo = m_cpu.GetRegB(CPU::DL);
				int HeadNo = m_cpu.GetRegB(CPU::DH);
				int TrackNo = m_cpu.GetRegB(CPU::CH);
				int SectorNo = m_cpu.GetRegB(CPU::CL);
				int SectorNum = m_cpu.GetRegB(CPU::AL);
				int ADDR = (((int)m_cpu.GetSeg(CPU::ES)) << 4) + m_cpu.GetRegW(CPU::BX);

				int LBA = (TrackNo * m_disk.GetBiosBlock().numHeads + HeadNo) * m_disk.GetBiosBlock().secPerTrk + SectorNo - 1;
				ASSERT(LBA <= m_disk.GetBiosBlock().totSec16, "Error int13");
				m_disk.Read((char *)m_ram + ADDR, 512 * LBA, 512 * SectorNum);
				goto int13_success;
			}

		default:
			FAIL("Unknown int13 function: 0x%s\n", n2hexstr(function).c_str());
		}
	int13_fail:
		SET_AH(0x01); // defaults to invalid function in AH or invalid parameter
	int13_fail_noah:
		SET_DISK_RET_STATUS(GET_AH());
	int13_fail_nostatus:
		SET_CF();     // error occurred
		return;

	int13_success:
		SET_AH(0x00); // no error
	int13_success_noah:
		SET_DISK_RET_STATUS(0x00);
		CLEAR_CF(); // no error
		break;

	default:
		ASSERT(false, "Unknown interrupt: 0x%s\n", n2hexstr(x).c_str());
	}
}
