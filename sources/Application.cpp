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
#include <chrono>
#include <ctime>

/*
	40:49	byte	Current video mode  (see VIDEO MODE)
	40:4A	word	Number of screen columns
	40:4C	word	Size of current video regen buffer in bytes
	40:4E	word	Offset of current video page in video regen buffer
	40:50  8 words	Cursor position of pages 1-8, high order byte=row
			low order byte=column; changing this data isn't
			reflected immediately on the display
	40:60	byte	Cursor ending (bottom) scan line (don't modify)
	40:61	byte	Cursor starting (top) scan line (don't modify)
	40:62	byte	Active display page number
	40:63	word	Base port address for active 6845 CRT controller
			3B4h = mono, 3D4h = color
	40:65	byte	6845 CRT mode control register value (port 3x8h)
			EGA/VGA values emulate those of the MDA/CGA
	40:66	byte	CGA current color palette mask setting (port 3d9h)
			EGA and VGA values emulate the CGA
*/
enum
{
	MEMORY_SIZE_KB = 0x0400,
	VIDEO_MOD = 0x0049,
	NUMBER_OF_SCREEN_COLUMNS = 0x004A,
	CURSOR_POSITION = 0x0050,
	CURSOR_ENDING = 0x0060,
	CURSOR_STARTING = 0x0061,
	ACTIVE_DISPLAY_PAGE = 0x0062,
	DISKETTE_CONTROLLER_PARAMETER_TABLE = 0xefc7,
	BIOS_CONFIG_TABLE = 0xe6f5,
	SYS_MODEL_ID = 0xFC
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

static std::chrono::time_point<std::chrono::system_clock> start;

int Application::Init()
{
	m_ram = new CPU::byte[0x200000];
	m_port = new CPU::byte[0x10000];
	memset(m_ram, 0, 0x200000);
	memset(m_port, 0, 0x10000);
	m_cpu.SetRamPort(m_ram, m_port);
	m_cpu.SetInterruptHandler(this);

	SetBIOSVars();

	m_disk.Open("Dos6.22.img");
	uint32_t loadAddress = 0x7C00;
	m_disk.Read((char*)m_ram + loadAddress, 0, 0x200);
	m_cpu.IP = loadAddress;
	m_cpu.SetSegment(CPU::CS, 0);
	m_int16_0 = false;
	start = std::chrono::system_clock::now();
	m_currentKeyCode = 0;
	
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
	static bool run = false;

	uint32_t stopAt = -1;
	if (m_cpu.IP == stopAt)
	{
		run = false;
	}

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
		doStep = 100000;
	}
	ImGui::End();

	if (m_int16_0)
	{
		doStep = 0;
	}

	if (doStep)
	{
		auto start = std::chrono::steady_clock::now();

		for (int i = 0; i < doStep && !m_int16_0; ++i)
		{
			if (i % 100)
			{
				auto current_timestamp = std::chrono::steady_clock::now();
				std::chrono::duration<float> elapsed_time = (current_timestamp - start);
				if (elapsed_time.count() > 0.03)
				{
					break;
				}
			}
			m_cpu.Step();
			if (m_cpu.IP == stopAt)
			{
				run = false;
				break;
			}
		}
	}

#ifdef _DEBUG
	m_cpu.GUI();
#endif

	ImGui::Begin("Disk");
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
	
	
	static int loadAddress = 0x7C00;
	ImGui::PushItemWidth(150);
	ImGui::InputInt("Boot address", &loadAddress, 0, 0, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal);
	if (ImGui::Button("Boot"))
	{
		memset(m_ram, 0, 0x100000);
		memset(m_port, 0, 0x10000);
		SetBIOSVars();
		m_int16_0 = false;
		m_disk.Read((char*)m_ram + (uint32_t)loadAddress, 0, 0x200);
		m_cpu.Reset();
		m_cpu.IP = loadAddress;
		m_cpu.SetSegment(CPU::CS, 0);
	}
	ImGui::Text(m_disk.GetBiosBlock().volLabel);
	ImGui::Text(m_disk.GetBiosBlock().fileSysType);

	if (ImGui::Button("Load COM tests"))
	{
		m_disk.Open("../Tests/Test2/test.COM");
		m_disk.Read((char*)m_ram + (uint32_t)0x100, 0, 0x200);
		m_disk.Close();
		m_cpu.IP = 0x100;
		m_cpu.SetSegment(CPU::CS, 0);
	}

	ImGui::End();
}

void Application::DrawScreen()
{
	static SimpleText st;

	st.Begin();
	int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;
	int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);

	uint32_t p = 0xB800 << 4;
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
			st.SetTextSize((SimpleText::FontSize)m_scale);
			st.SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st.SetColor(SimpleText::BACKGROUND_COLOR, backgroundColorST, backgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st.PutSymbol(symbol, clm * 8 * m_scale, row * 16 * m_scale);
		}
	}
	st.End();
	auto now = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = now - start;
	StoreW(0x0040, 0x006E, int(elapsed_seconds.count() * 1193180 / 65536));
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

void Application::StoreString(uint16_t offset, uint16_t location, const char* x)
{
	uint32_t address = AbsAddress(offset, location);
	strcpy((char*)m_ram + address, x);
}

void Application::Store(uint16_t offset, uint16_t location, uint8_t* x, int size)
{
	uint32_t address = AbsAddress(offset, location);
	memcpy((char*)m_ram + address, x, size);
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
	// zero out BIOS data area(40:00 .. 40:ff)
	memset(m_ram + 0x400, 0, 0x100);

	// set all interrupts to default handler
	for (int i = 0; i < 256; ++i)
	{
		StoreW(0x0, 4 * i + 0, 0xff53);
		StoreW(0x0, 4 * i + 2, 0xf000);
	}

	StoreW(0x40, 0x0013, MEMORY_SIZE_KB); //Total memory in K-bytes

	StoreW(0x40, 0x0000, 0x03F8); // COM1
	StoreW(0x40, 0x0002, 0x02F8); // COM2
	StoreW(0x40, 0x0004, 0x03E8); // COM3
	StoreW(0x40, 0x0006, 0x02E8); // COM4

	StoreB(0x40, VIDEO_MOD, 0x02); // Current active video mode

	StoreW(0x40, NUMBER_OF_SCREEN_COLUMNS, 80); // Screen width in text columns
	
	StoreB(0xF000, 0xff53, 0xCF); // dummy IRET

	StoreString(0xF000, 0xff00, "(c) 2018 Emul86 BIOS");
	StoreString(0xF000, 0xfff5, "02/03/18");
	StoreW(0xF000, 0xfffe, SYS_MODEL_ID);
	
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
	Store(0xf000, DISKETTE_CONTROLLER_PARAMETER_TABLE, diskette_param_table, sizeof(diskette_param_table));

	uint8_t bios_config[] =
	{
		0x08, //Table size (bytes) -Lo
		0x00, //Table size (bytes) -Hi
		SYS_MODEL_ID,
		0x0, // SYS_SUBMODEL_ID
		0x1, // Revision
		64 + 32,
		64,
		0,
		0,
		0
	};
	Store(0xf000, BIOS_CONFIG_TABLE, bios_config, sizeof(bios_config));

	/*
	on return:
	AX contains the following bit flags:

	|F|E|D|C|B|A|9|8|7|6|5|4|3|2|1|0|  AX
	| | | | | | | | | | | | | | | `---- IPL diskette installed
	| | | | | | | | | | | | | | `----- math coprocessor
	| | | | | | | | | | | | `-------- old PC system board RAM < 256K
	| | | | | | | | | | | | | `----- pointing device installed (PS/2)
	| | | | | | | | | | | | `------ not used on PS/2
	| | | | | | | | | | `--------- initial video mode
	| | | | | | | | `------------ # of diskette drives, less 1
	| | | | | | | `------------- 0 if DMA installed
	| | | | `------------------ number of serial ports
	| | | `------------------- game adapter installed
	| | `-------------------- unused, internal modem (PS/2)
	`----------------------- number of printer ports


	- bits 3 & 2,  system board RAM if less than 256K motherboard
	00 - 16K		     01 - 32K
	10 - 16K		     11 - 64K (normal)

	- bits 5 & 4,  initial video mode
	00 - unused 	     01 - 40x25 color
	10 - 80x25 color	     11 - 80x25 monochrome


	- bits 7 & 6,  number of disk drives attached, when bit 0=1
	00 - 1 drive	     01 - 2 drives
	10 - 3 drive	     11 - 4 drives


	- returns data stored at BIOS Data Area location 40:10
	- some flags are not guaranteed to be correct on all machines
	- bit 13 is used on the PCjr to indicate serial printer

	*/
	uint16_t equpment = 0
		| 1
		| 0 << 1
		| 1 << 2
		| 1 << 3
		| 0 << 4
		| 1 << 5
		| 0 << 6
		| 0 << 7
		| 0 << 8
		| 0 << 9
		| 0 << 10
		| 0 << 11
		| 0 << 12
		| 0 << 13
		| 0 << 14
		| 0 << 15
		;
	//equpment = 1 + 2 * 0 + 4 * 1 + 8 * 0 + 16 * 0 + 32 * 0 + 64 * 0 + 128 * 0 + 256 * 0 + 512 * 1;
	StoreW(0x40, 0x0010, equpment);

	int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;

	for (int page = 0; page < 8; ++page)
	{
		for (int row = 0; row < screenHeight; ++row)
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				StoreB(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * page + 1, 7);
			}
		}
	}
}

#define SET_AH(b) m_cpu.SetRegB(CPU::AH, b)
#define SET_AL(b) m_cpu.SetRegB(CPU::AL, b)
#define SET_DH(b) m_cpu.SetRegB(CPU::DH, b)
#define SET_BH(b) m_cpu.SetRegB(CPU::BH, b)
#define SET_DL(b) m_cpu.SetRegB(CPU::DL, b)
#define SET_CH(b) m_cpu.SetRegB(CPU::CH, b)
#define SET_CL(b) m_cpu.SetRegB(CPU::CL, b)
#define SET_CX(b) m_cpu.SetRegW(CPU::CX, b)
#define SET_BX(b) m_cpu.SetRegW(CPU::BX, b)
#define SET_DX(b) m_cpu.SetRegW(CPU::DX, b)
#define SET_AX(b) m_cpu.SetRegW(CPU::AX, b)
#define GET_AH() m_cpu.GetRegB(CPU::AH)
#define GET_BH() m_cpu.GetRegB(CPU::BH)
#define GET_BL() m_cpu.GetRegB(CPU::BL)
#define GET_CH() m_cpu.GetRegB(CPU::CH)
#define GET_CL() m_cpu.GetRegB(CPU::CL)
#define GET_DL() m_cpu.GetRegB(CPU::DL)
#define GET_DH() m_cpu.GetRegB(CPU::DH)
#define GET_AL() m_cpu.GetRegB(CPU::AL)
#define GET_AX() m_cpu.GetRegW(CPU::AX)
#define GET_CX() m_cpu.GetRegW(CPU::CX)
#define SET_DISK_RET_STATUS(status) StoreB(0x0040, 0x0074, status)
#define SET_CF() m_cpu.SetFlag<CPU::CF>()
#define SET_ZF() m_cpu.SetFlag<CPU::ZF>()
#define CLEAR_CF() m_cpu.ClearFlag<CPU::CF>()
#define CLEAR_ZF() m_cpu.ClearFlag<CPU::ZF>()

inline uint8_t decToBcd(uint8_t val)
{
	return ((val / 10 * 16) + (val % 10));
}

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
			printf("Video mode: %d\n", arg);
			break;
		case 0x01:
			//Set cursor type
			break;
		case 0x02:
			{
				int page = GET_BH();
				int row = GET_DH();
				int column = GET_DL();
				StoreB(0x40, CURSOR_POSITION + 2 * page + 0, row);
				StoreB(0x40, CURSOR_POSITION + 2 * page + 1, column);
			}
			break;
		case 0x03:
			/*
			on return:
			CH = cursor starting scan line (low order 5 bits)
			CL = cursor ending scan line (low order 5 bits)
			DH = row
			DL = column
			*/
			{
				int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
				int row = GetB(0x40, CURSOR_POSITION + 2 * active_page + 0);
				int column = GetB(0x40, CURSOR_POSITION + 2 * active_page + 1);
				SET_DH(row);
				SET_DL(column);
				SET_CH(0);
				SET_CL(5);
				break;
			}
		case 0x05:
			// Select active display page
			{
				StoreB(0x40, ACTIVE_DISPLAY_PAGE, arg);
			}
			break;
		case 0x06:
			{
				int n = GET_AL();
				int a = GET_BH();
				int y1 = GET_CH();
				int x1 = GET_CL();
				int y2 = GET_DH();
				int x2 = GET_DL();
				int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
				int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
				int screenHeight = 25;
				uint32_t p = 0xB800 << 4;

				for (int row = y1; row <= y2; ++row)
				{
					if (row + n <= y2 && n != 0)
					{
						for (int col = x1; col <= x2; ++col)
						{
							int tmp = GetW(0xB800, (row + n) * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page);
							StoreW(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, tmp);
						}
					}
					else
					{
						for (int col = x1; col <= x2; ++col)
						{
							StoreW(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, a << 8);
						}
					}
				}
			}
			break;

		case 0x0E:
			/*
			AL = ASCII character to write
			BH = page number (text modes)
			BL = foreground pixel color (graphics modes)
			*/
			{
				char symbol = GET_AL();
				char page = 0;// GET_BH();
				int attribute = 7;
				int row = GetB(0x40, CURSOR_POSITION + 2 * page + 0);
				int column = GetB(0x40, CURSOR_POSITION + 2 * page + 1);
				int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
				int screenHeight = 25;

				if (row >= screenHeight)
				{
					scrollOneLine();
					row = screenHeight - 1;
				}

				if (symbol == '\b')
				{
					--column;
					if (column < 0)
					{
						column = screenWidth;
						row--;
						if (row < 0)
						{
							row = screenHeight;
						}
					}
				}
				else if (symbol == '\n')
				{
					column = 0;
					row++;
				}
				else if (symbol == '\r')
				{
					column = 0;
				}
				else
				{
					StoreB(0xB800, row * screenWidth * 2 + column * 2 + screenHeight * screenWidth * 2 * page, symbol);
					++column;
					if (column > screenWidth)
					{
						column = 0;
						row++;
					}
				}

				StoreB(0x40, CURSOR_POSITION + 2 * page + 0, row);
				StoreB(0x40, CURSOR_POSITION + 2 * page + 1, column);
			}

			break;

		case 0x09:
			/*
			AH = 09
			AL = ASCII character to write
			BH = display page  (or mode 13h, background pixel value)
			BL = character attribute (text) foreground color (graphics)
			CX = count of characters to write (CX >= 1)
			*/
		{
			char symbol = GET_AL();
			char page = GET_BH();
			int attribute = GET_BL();
			int count = GET_CX();
			int row = GetB(0x40, CURSOR_POSITION + 2 * page + 0);
			int column = GetB(0x40, CURSOR_POSITION + 2 * page + 1);
			int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
			int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
			int screenHeight = 25;

			if (row >= screenHeight)
			{
				scrollOneLine();
				row = screenHeight - 1;
			}

			for (int i = 0; i < count; ++i)
			{
				StoreW(0xB800, row * screenWidth * 2 + column * 2 + screenHeight * screenWidth * 2 * page, symbol + (attribute << 8));
				++column;
				if (column >= screenWidth)
				{
					column = 0;
					row++;
				}
			}
		}
		break;

		case 0x08:
			/*
			AH = 08
			BH = display page
			on return:
			AH = attribute of character (alpha modes only)
			AL = character at cursor position
			- in video mode 4 (300x200 4 color) on the EGA, MCGA and VGA
			this function works only on page zero
			*/
		{
			char page = GET_BH();
			int row = GetB(0x40, CURSOR_POSITION + 2 * page + 0);
			int column = GetB(0x40, CURSOR_POSITION + 2 * page + 1);
			int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
			int screenHeight = 25;
			uint16_t result = GetW(0xB800, row * screenWidth * 2 + column * 2 + screenHeight * screenWidth * 2 * page);
			SET_AX(result);
		}
		break;

		case 0x0F:
			/*
				on return:
				AH = number of screen columns
				AL = mode currently set (see VIDEO MODES)
				BH = current display page
			*/
		{
			int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
			int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
			int screenHeight = 25;
			int mode = GetB(0x40, VIDEO_MOD);
			SET_AH(screenWidth);
			SET_AL(mode);
			SET_BH(active_page);
		}
			break;

		case 0x10:
			switch (function)
			{
			case 0x3:
				break;
			}
			break;

		case 0x0A:
			/*
	AH = 0A
	AL = ASCII character to write
	BH = display page  (or mode 13h, background pixel value)
	BL = foreground color (graphics mode only)
	CX = count of characters to write (CX >= 1)*/
			{
				char symbol = GET_AL();
				char page = GET_BH();
				char foreground_color = GET_BL();
				int count = GET_CX();
				int row = GetB(0x40, CURSOR_POSITION + 2 * page + 0);
				int column = GetB(0x40, CURSOR_POSITION + 2 * page + 1);
				int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
				int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
				int screenHeight = 25;
				for (int i = 0; i < count; ++i)
				{
					StoreW(0xB800, row * screenWidth * 2 + column * 2 + screenHeight * screenWidth * 2 * page, symbol + (7 << 8));
					++column;
					if (column > screenWidth)
					{
						column = 0;
						row++;
						if (row > screenHeight)
						{
							row = 0;
						}
					}
				}
			}
			break;

		default:
			ASSERT(false, "Unknown int10 function: 0x%s\n", n2hexstr(function).c_str());
			break;
		}
		break;

	// Conventional Memory Size
	case 0x12:
		SET_AX(GetW(0x40, 0x0013));
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
				int ADDR = select(m_cpu.GetSegment(CPU::ES), m_cpu.GetRegW(CPU::BX));

				int LBA = (TrackNo * m_disk.GetBiosBlock().numHeads + HeadNo) * m_disk.GetBiosBlock().secPerTrk + SectorNo - 1;
				ASSERT(LBA <= m_disk.GetBiosBlock().totSec16, "Error int13");
				m_disk.Read((char *)m_ram + ADDR, 512 * LBA, 512 * SectorNum);
				goto int13_success;
			}
		case 0x08:
			/*
			AH = 08
			DL = drive number (0=A:, 1=2nd floppy, 80h=drive 0, 81h=drive 1)


			on return:
			AH = status  (see INT 13,STATUS)
			BL = CMOS drive type
			01 - 5¬  360K	     03 - 3«  720K
			02 - 5¬  1.2Mb	     04 - 3« 1.44Mb
			CH = cylinders (0-1023 dec. see below)
			CL = sectors per track	(see below)
			DH = number of sides (0 based)
			DL = number of drives attached
			ES:DI = pointer to 11 byte Disk Base Table (DBT)
			CF = 0 if successful
			= 1 if error


			Cylinder and Sectors Per Track Format

			|F|E|D|C|B|A|9|8|7|6|5|4|3|2|1|0|  CX
			| | | | | | | | | | `------------  sectors per track
			| | | | | | | | `------------	high order 2 bits of cylinder count
			`------------------------  low order 8 bits of cylinder count

			- the track/cylinder number is a 10 bit value taken from the 2 high
			order bits of CL and the 8 bits in CH (low order 8 bits of track)
			- many good programming references indicate this function is only
			available on the AT, PS/2 and later systems, but all hard disk
			systems since the XT have this function available
			- only the disk number is checked for validity
			*/
			if (GET_DL() == m_disk.GetBiosBlock().drvNum)
			{
				SET_DH(m_disk.GetBiosBlock().numHeads - 1);
				SET_CL(m_disk.GetBiosBlock().secPerTrk);
				SET_CH((m_disk.GetBiosBlock().totSec16 / m_disk.GetBiosBlock().secPerTrk) / m_disk.GetBiosBlock().numHeads - 1);
				SET_DL(1);
				goto int13_success;
			}
			else
			{
				SET_DL(1);
				goto int13_success;
				SET_AH(1);
			}
			break;
		case 0x10: /* disk controller reset */
			goto int13_success;
			break;

		case 0x15: /* disk controller reset */
			if (GET_DL() == m_disk.GetBiosBlock().drvNum)
			{
				SET_AH(1);
			}
			else
			{
				SET_AH(0);
			}
			break;

		default:
			goto int13_fail;
			ASSERT(false, "Unknown int13 function: 0x%s\n", n2hexstr(function).c_str());
		}

	case 0x14:
		function = GET_AH();
		switch (function)
		{
		case 0:
			SET_AX(0);
			break;
		case 0x01:
			SET_AX(0);
			break;
		default:
			ASSERT(false, "Unknown int14 function: 0x%s\n", n2hexstr(function).c_str());
		}
		break;

	// System BIOS Services
	case 0x15:
		function = GET_AH();
		switch (function)
		{
		case 0xC0:
			m_cpu.SetRegW(CPU::BX, 0xe6f5);
			m_cpu.SetSegment(CPU::ES, 0xf000);
			m_cpu.ClearFlag<CPU::CF>();
			m_cpu.SetRegB(CPU::AH, 0);
			break;
		case 0x41:
			break;
		case 0x88:
			m_cpu.SetRegW(CPU::AX, 0);
			m_cpu.ClearFlag<CPU::CF>();
			m_cpu.ClearFlag<CPU::CF>();
			break;

		//A20 gate
		case 0x24:
			function = GET_AL();
			switch (function)
			{
			//DISABLE A20 GATE 
			case 0x0:
				/*
				CF clear if successful
				AH = 00h
				CF set on error
				AH = status
				01h keyboard controller is in secure mode
				86h function not supported
				*/
				m_cpu.DisableA20Gate();
				SET_AH(0);
				CLEAR_CF();
				break;

			//ENABLE A20 GATE
			case 0x1:
				/*
				Return:
				CF clear if successful
				AH = 00h
				CF set on error
				AH = status
				01h keyboard controller is in secure mode
				86h function not supported
				*/
				m_cpu.EnableA20Gate();
				SET_AH(0);
				CLEAR_CF();
				break;

			case 0x02:
				/*
				Return:
				CF clear if successful
				AH = 00h
				AL = current state (00h disabled, 01h enabled)
				CX = ??? (set to 0000h-000Bh or FFFFh by AMI BIOS v1.00.03.AV0M)
				FFFFh if keyboard controller does not become ready within C000h
				read attempts
				CF set on error
				AH = status
				01h keyboard controller is in secure mode
				86h function not supported
				*/
				SET_AH(0);
				SET_AL(m_cpu.GetA20GateStatus() ? 1 : 0);
				CLEAR_CF();
				break;
			case 0x03:
				/*
				Return:
				CF clear if successful
				AH = 00h
				BX = status of A20 gate support (see #00462)
				CF set on error
				AH = status
				01h keyboard controller is in secure mode
				86h function not supported
				*/
				SET_AH(0);
				SET_BX(0);
				CLEAR_CF();
				break;

			default:
				ASSERT(false, "Unknown A20 function: 0x%s\n", n2hexstr(function).c_str());
				break;
			}
			break;

		default:
			ASSERT(false, "Unknown int15 function: 0x%s\n", n2hexstr(function).c_str());
		}
		break;

	case 0x16:
		function = GET_AH();
		switch (function)
		{
		case 0x01:
			/*
			on return:
			ZF = 0 if a key pressed (even Ctrl-Break)
			AX = 0 if no scan code is available
			AH = scan code
			AL = ASCII character or zero if special function key
			*/
			if (m_currentKeyCode != 0)
			{
				SET_AX(m_currentKeyCode);
				CLEAR_ZF();
			}
			else
			{
				SET_AX(0);
				SET_ZF();
			}
			break;
		case 0x02:
			/*
			on return:
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
			SET_AL(GetB(0x0040, 0x0017));
			break;
		case 0x00:
		case 0x10:
			m_int16_0 = true;
			break;
		default:
			ASSERT(false, "Unknown int16 function: 0x%s\n", n2hexstr(function).c_str());
		}
		break;

	case 0x17:
		function = GET_AH();
		switch (function)
		{
		case 0x01:
			SET_AX(0);
			break;
		default:
			ASSERT(false, "Unknown int17 function: 0x%s\n", n2hexstr(function).c_str());
		}
		break;

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

	case 0x11:
		SET_AL(GetB(0x0040, 0x0010));
		break;

	case 0x1a:
		function = GET_AH();
		switch (function)
		{
		case 0x00:
			/*
			AL = midnight flag, 1 if 24 hours passed since reset
			CX = high order word of tick count
			DX = low order word of tick count
			*/
			// TODO
			{
				auto now = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = now - start;
				int ticks = int(elapsed_seconds.count() * 1193180 / 65536);
				StoreW(0x0040, 0x006E, ticks & 0xFFFF);
				StoreW(0x0040, 0x006E + 2, ticks >> 16);
				SET_AL(0);
				SET_CX(ticks >> 16);
				SET_DX(ticks & 0xFFFF);
			}
			break;
		case 0x02:
		{
			/*
			CF = 0 if successful  = 1 if error, RTC not operating
			CH = hours in BCD
			CL = minutes in BCD
			DH = seconds in BCD
			DL = 1 if daylight savings time option*/
			{
				std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				auto tm = std::localtime(&now);
				SET_CH(decToBcd(tm->tm_hour));
				SET_CL(decToBcd(tm->tm_min));
				SET_DH(decToBcd(tm->tm_sec));
				SET_DL(tm->tm_isdst);
				CLEAR_CF();
			}
			break;
		}
		case 0x04:
			/*
			on return:
			CH = century in BCD (decimal 19 or 20)
			CL = year in BCD
			DH = month in BCD
			DL = day in BCD
			CF = 0 if successful
			   = 1 if error or clock not operating
			*/
			{
				std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				auto tm = std::localtime(&now);
				SET_CH(decToBcd((tm->tm_year + 1900) / 100));
				SET_CL(decToBcd(tm->tm_year % 100));
				SET_DH(decToBcd(tm->tm_mon + 1));
				SET_DL(tm->tm_mday);
				CLEAR_CF();
				break;
			}
		case 0x0a:
		{
			/*
			on return:
			CX = count of days since 1-1-1980n*/
			std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			int days = now / 3600 / 24 - 10 * 365 - 2;
			SET_CX(days);
			CLEAR_CF();
			break;
		}
		}
		break;

	default:
		//custom interrupt
		if (GetW(0x0, x * 4) != 0xff53 && GetW(0x0, x * 4 + 2) != 0xf000)
		{
			m_cpu.Interrupt(x);
		}
		else
		{
			ASSERT(false, "Unknown interrupt: 0x%s\n", n2hexstr(x).c_str());
		}
	}
}

void Application::scrollOneLine()
{
	int active_page = GetB(0x40, ACTIVE_DISPLAY_PAGE);
	int screenWidth = GetW(0x40, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;
	for (int row = 0; row < screenHeight; ++row)
	{
		if (row + 1 < screenHeight)
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				int tmp = GetB(0xB800, (row + 1) * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page);
				StoreB(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, tmp);
			}
		}
		else
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				StoreB(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, 0);
			}
		}
	}
}

bool Application::KeyCallback(int c)
{
	m_currentKeyCode = c;
	StoreB(0x40, 0xBA, c>>8);
	if (m_int16_0)
	{
		m_cpu.SetRegW(CPU::AX, c);
		m_int16_0 = false;
		return true;
	}
	return false;
}

void Application::PushKey(int c)
{
	int tail = GetW(0x40, 0x1C);
	StoreW(0x40, tail, c);
	tail += 2;
	if ((0x1E + 32) <= tail)
	{
		tail = 0x1E;
	}
	StoreW(0x40, 0x1C, tail);
}

int Application::PopKey()
{
	int head = GetW(0x40, 0x1A);
	int c = GetW(0x40, head);
	head += 2;
	if ((0x1E + 32) <= head)
	{
		head = 0x1E;
	}
	StoreW(0x40, 0x1A, head);
	return c;
}

bool Application::HasKeyToPop()
{
	int head = GetW(0x40, 0x1A);
	int tail = GetW(0x40, 0x1C);
	return head != tail;
}
