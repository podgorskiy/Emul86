#include "bios.h"
#include "Disk.h"
#include <string>
#include <chrono>
#include <ctime>

#ifdef FORCE_ZERO_TIME
#define TIME_ZERO(X) 0
#else
#define TIME_ZERO(X) X
#endif



void BIOS::InitBIOSDataArea()
{
	// zero out BIOS data area(40:00 .. 40:ff)
	for (int i = 0; i < 0x100; io.MemStoreB(BIOS_DATA_OFFSET, i++, 0));

	// set all interrupts to default handler
	for (int i = 0; i < 256; ++i)
	{
		SetInterruptVector(i, BIOS_SEGMENT, DEFAULT_HANDLER);
	}
	io.MemStoreB(BIOS_SEGMENT, DEFAULT_HANDLER, IRET_INSTRUCTION); // dummy IRET

	io.MemStoreW(BIOS_DATA_OFFSET, MEMORY_SIZE_FIELD, MEMORY_SIZE_KB); //Total memory in K-bytes

	io.MemStoreW(BIOS_DATA_OFFSET, 0x0000, 0x03F8); // COM1
	io.MemStoreW(BIOS_DATA_OFFSET, 0x0002, 0x02F8); // COM2
	io.MemStoreW(BIOS_DATA_OFFSET, 0x0004, 0x03E8); // COM3
	io.MemStoreW(BIOS_DATA_OFFSET, 0x0006, 0x02E8); // COM4

	io.MemStoreB(BIOS_DATA_OFFSET, VIDEO_MOD, 0x03); // Current active video mode

	io.MemStoreW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS, 80); // Screen width in text columns
	io.MemStoreB(BIOS_DATA_OFFSET, BIOSMEM_CHAR_HEIGHT, 10);
	io.MemStoreB(BIOS_DATA_OFFSET, BIOSMEM_NB_ROWS, 24);

	io.MemStoreB(BIOS_DATA_OFFSET, 0x96, 0x10); //101/102 enhanced keyboard installed

	io.MemStoreString(BIOS_SEGMENT, 0xff00, "(c) 2018 Emul86 BIOS");
	io.MemStoreString(BIOS_SEGMENT, 0xfff5, "02/03/18");
	io.MemStoreW(BIOS_SEGMENT, 0xfffe, SYS_MODEL_ID);

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
		0x08
	};
	io.MemStore(BIOS_SEGMENT, DISKETTE_CONTROLLER_PARAMETER_TABLE, diskette_param_table, sizeof(diskette_param_table));

	uint8_t diskette_param_table2[] =
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
	io.MemStore(BIOS_SEGMENT, DISKETTE_CONTROLLER_PARAMETER_TABLE2, diskette_param_table2, sizeof(diskette_param_table2));

	SetInterruptVector(0x1E, BIOS_SEGMENT, DISKETTE_CONTROLLER_PARAMETER_TABLE2);
	SetInterruptVector(0x13, BIOS_SEGMENT, 0xe3fe);
	SetInterruptVector(0x19, BIOS_SEGMENT, 0xe6f2);
	SetInterruptVector(0x08, BIOS_SEGMENT, 0xfea5);
	SetInterruptVector(0x09, BIOS_SEGMENT, 0xe987);
	SetInterruptVector(0x10, BIOS_SEGMENT, 0xf065);

	io.MemStoreB(BIOS_SEGMENT, 0xe3fe, IRET_INSTRUCTION); // dummy IRET
	io.MemStoreB(BIOS_SEGMENT, 0xe6f2, IRET_INSTRUCTION); // dummy IRET
	io.MemStoreB(BIOS_SEGMENT, 0xfea5, IRET_INSTRUCTION); // dummy IRET
	io.MemStoreB(BIOS_SEGMENT, 0xe987, IRET_INSTRUCTION); // dummy IRET

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
	io.MemStore(BIOS_SEGMENT, BIOS_CONFIG_TABLE, bios_config, sizeof(bios_config));

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
		| 0
		| 0 << 1
		| 1 << 2
		| 0 << 3
		| 0 << 4
		| 1 << 5
		| 0 << 6
		| 0 << 7
		| 0 << 8
		| 1 << 9
		| 0 << 10
		| 0 << 11
		| 0 << 12
		| 0 << 13
		| 1 << 14
		| 0 << 15
		;
	//equpment = 1 + 2 * 0 + 4 * 1 + 8 * 0 + 16 * 0 + 32 * 0 + 64 * 0 + 128 * 0 + 256 * 0 + 512 * 1;
	io.MemStoreW(BIOS_DATA_OFFSET, EQUIPMENT_LIST_FLAGS, equpment);

	int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;

	for (int page = 0; page < 8; ++page)
	{
		for (int row = 0; row < screenHeight; ++row)
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				io.MemStoreB(0xB800, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * page + 1, 7);
			}
		}
	}
}


#define SET_AH(b) m_cpu.SetRegB(CPU::AH, b)
#define SET_AL(b) m_cpu.SetRegB(CPU::AL, b)
#define SET_DH(b) m_cpu.SetRegB(CPU::DH, b)
#define SET_BH(b) m_cpu.SetRegB(CPU::BH, b)
#define SET_BL(b) m_cpu.SetRegB(CPU::BL, b)
#define SET_DL(b) m_cpu.SetRegB(CPU::DL, b)
#define SET_CH(b) m_cpu.SetRegB(CPU::CH, b)
#define SET_CL(b) m_cpu.SetRegB(CPU::CL, b)
#define SET_CX(b) m_cpu.SetRegW(CPU::CX, b)
#define SET_DX(b) m_cpu.SetRegW(CPU::DX, b)
#define SET_BX(b) m_cpu.SetRegW(CPU::BX, b)
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
#define GET_DX() m_cpu.GetRegW(CPU::DX)
#define SET_CF() m_cpu.SetFlag<CPU::CF>()
#define SET_ZF() m_cpu.SetFlag<CPU::ZF>()
#define CLEAR_CF() m_cpu.ClearFlag<CPU::CF>()
#define CLEAR_ZF() m_cpu.ClearFlag<CPU::ZF>()
#define SET_DISK_RET_STATUS(status) io.MemStoreB(0x0040, 0x0074, status)

#define SET_ES(b) m_cpu.SetSegment(CPU::ES, b)
#define SET_BP(b) m_cpu.SetRegW(CPU::BP, b)
#define SET_DI(b) m_cpu.SetRegW(CPU::DI, b)


void BIOS::Int(byte x)
{
	byte function = GET_AH();
	byte arg = GET_AL();

	switch (x)
	{
	case 0x10:
		Int10VideoServices(function, arg);
		break;

	// BIOS Equipment Determination / BIOS Equipment Flags
	case 0x11:
		SET_AX(io.MemGetW(BIOS_DATA_OFFSET, EQUIPMENT_LIST_FLAGS));
		break;

	// Conventional Memory Size
	case 0x12:
		SET_AX(io.MemGetW(BIOS_DATA_OFFSET, MEMORY_SIZE_FIELD));
		break;

	case 0x13:
		Int13DiskIOServices(function, arg);
		break;

	case 0x14:
		Int14AsynchronousCommunicationsServices(function, arg);
		break;

	case 0x15:
		Int15SystemBIOSServices(function, arg);
		break;

	case 0x16:
		Int16KeyboardServices(function, arg);
		break;

	case 0x17:
		Int17PrinterBIOSServices(function, arg);
		break;

	case 0x1a:
		Int1ARealTimeClockServices(function, arg);
		break;

	// Other interrupts
	default:
		// If it is not a dummy IRET, then execute it, otherwise warn about not implemented interrupt
		if (io.MemGetW(0x0, x * 4) != DEFAULT_HANDLER && io.MemGetW(0x0, x * 4 + 2) != BIOS_SEGMENT)
		{
			m_cpu.Interrupt(x);
		}
		else
		{
			ASSERT(false, "Unknown interrupt: 0x%s\n", n2hexstr(x).c_str());
		}
	}
}


void BIOS::SetInterruptVector(int interrupt, word segment, word offset)
{
	io.MemStoreW(0x0, 4 * interrupt + 0, offset);
	io.MemStoreW(0x0, 4 * interrupt + 2, segment);
}


void BIOS::Int10VideoServices(int function, int arg)
{
	switch (function)
	{
		//Set Video Mode
	case 0x00:
		io.MemStoreB(BIOS_DATA_OFFSET, VIDEO_MOD, arg);
		printf("Video mode: %d\n", arg);
		break;

		// Set cursor type
	case 0x01:
		arg = GET_CL();
		io.MemStoreB(BIOS_DATA_OFFSET, 0x60, arg);
		arg = GET_CH();
		io.MemStoreB(BIOS_DATA_OFFSET, 0x61, arg);
		break;

		// Set cursor position
	case 0x02:
		/*
		AH = 02
		BH = page number (0 for graphics modes)
		DH = row
		DL = column
		returns nothing
		*/
	{
		int page = GET_BH();
		int cursorpos = GET_DX();
		io.MemStoreW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page, cursorpos);
	}
	break;

	// Read cursor position
	case 0x03:
		/*
		on return:
		CH = cursor starting scan line (low order 5 bits)
		CL = cursor ending scan line (low order 5 bits)
		DH = row
		DL = column
		*/
	{
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int cursorpos = io.MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * active_page);

		arg = GET_CH();
		io.MemStoreB(BIOS_DATA_OFFSET, 0x61, arg);
		SET_DX(cursorpos);
		SET_CX(io.MemGetW(BIOS_DATA_OFFSET, 0x60));
	}
	break;

	// Select active display page
	case 0x05:
	{
		io.MemStoreB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE, arg);
	}
	break;

	// Scroll active page up
	case 0x06:
	{
		int n = GET_AL();
		int a = GET_BH();
		int y1 = GET_CH();
		int x1 = GET_CL();
		int y2 = GET_DH();
		int x2 = GET_DL();
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;
		uint32_t p = 0xB800 << 4;

		for (int row = y1; row <= y2; ++row)
		{
			if (row + n <= y2 && n != 0)
			{
				for (int col = x1; col <= x2; ++col)
				{
					int tmp = io.MemGetW(CGA_DISPLAY_RAM, (row + n) * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page);
					io.MemStoreW(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, tmp);
				}
			}
			else
			{
				for (int col = x1; col <= x2; ++col)
				{
					io.MemStoreW(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, a << 8);
				}
			}
		}
	}
	break;

	// Scroll active page down
	case 0x07:
	{
		int n = GET_AL();
		int a = GET_BH();
		int y1 = GET_CH();
		int x1 = GET_CL();
		int y2 = GET_DH();
		int x2 = GET_DL();
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;
		uint32_t p = 0xB800 << 4;

		for (int row = y2; row >= y1; --row)
		{
			if (row - n >= y1 && n != 0)
			{
				for (int col = x1; col <= x2; ++col)
				{
					int tmp = io.MemGetW(CGA_DISPLAY_RAM, (row - n) * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page);
					io.MemStoreW(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, tmp);
				}
			}
			else
			{
				for (int col = x1; col <= x2; ++col)
				{
					io.MemStoreW(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, a << 8);
				}
			}
		}
	}
	break;

	// Write text in teletype mode
	case 0x0E:
		/*
		AL = ASCII character to write
		BH = page number (text modes)
		BL = foreground pixel color (graphics modes)
		*/
	{
		char symbol = GET_AL();
		char page = GET_BH();
		int attribute = 7;
		int cursor = io.MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page);
		int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;

		if (symbol == '\b')
		{
			--xcurs;
			if (xcurs < 0)
			{
				xcurs = screenWidth;
				ycurs--;
				if (ycurs < 0)
				{
					ycurs = screenHeight;
				}
			}
		}
		else if (symbol == '\n')
		{
			xcurs = 0;
			ycurs++;
		}
		else if (symbol == '\r')
		{
			xcurs = 0;
		}
		else
		{
			io.MemStoreB(0xB800, ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * page, symbol);
			++xcurs;
			if (xcurs > screenWidth)
			{
				xcurs = 0;
				ycurs++;
			}
		}

		if (ycurs >= screenHeight)
		{
			scrollOneLine();
			ycurs = screenHeight - 1;
		}

		cursor = ycurs; cursor <<= 8; cursor += xcurs;
		io.MemStoreW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page, cursor);
	}

	break;

	// Write character and attribute at cursor
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
		int cursor = io.MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page);
		int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;

		if (ycurs >= screenHeight)
		{
			scrollOneLine();
			ycurs = screenHeight - 1;
		}

		for (int i = 0; i < count; ++i)
		{
			io.MemStoreW(CGA_DISPLAY_RAM, ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * page, symbol + (attribute << 8));
			++xcurs;
			if (xcurs >= screenWidth)
			{
				xcurs = 0;
				ycurs++;
			}
		}
	}
	break;

	// Read character and attribute at cursor
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
		int cursor = io.MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page);
		int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;
		uint16_t result = io.MemGetW(CGA_DISPLAY_RAM, ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * page);
		SET_AX(result);
	}
	break;

	// Get current video state
	case 0x0F:
		/*
		on return:
		AH = number of screen columns
		AL = mode currently set (see VIDEO MODES)
		BH = current display page
		*/
	{
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;
		int mode = io.MemGetB(BIOS_DATA_OFFSET, VIDEO_MOD);
		SET_AH(screenWidth);
		SET_AL(mode);
		SET_BH(active_page);
	}
	break;

	// Set/get palette registers (EGA/VGA)
	case 0x10:
		switch (function)
		{
		case 0x3:
			break;
		}
		break;

		// Write character at current cursor
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
		int cursor = io.MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * page);
		int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;
		int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
		int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
		int screenHeight = 25;
		for (int i = 0; i < count; ++i)
		{
			io.MemStoreW(CGA_DISPLAY_RAM, ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * page, symbol + (7 << 8));
			++xcurs;
			if (xcurs > screenWidth)
			{
				xcurs = 0;
				ycurs++;
				if (ycurs > screenHeight)
				{
					ycurs = 0;
				}
			}
		}
	}
	break;

	//Video Subsystem Configuration
	case 0x12:
	{
		arg = GET_BL();
		switch (arg)
		{
			//return video configuration information
			/*on return:
			BH = 0 if color mode in effect
			= 1 if mono mode in effect
			BL = 0 if 64k EGA memory
			= 1 if 128k EGA memory
			= 2 if 192k EGA memory
			= 3 if 256k EGA memory
			CH = feature bits
			CL = switch settings
			*/
		case 0x10:
			SET_BH(0);
			SET_BL(3);
			SET_CX(9);
			break;
		default:
			ASSERT(false, "Unknown int10 12 function: 0x%s\n", n2hexstr(arg).c_str());
		}
	}
	break;

	//Character Generator Functions
	case 0x11:
	{
		switch (arg)
		{
		case 0x30:
			/*
			30  get current character generator information
			BH = information desired:
			= 0  INT 1F pointer
			= 1  INT 44h pointer
			= 2  ROM 8x14 pointer
			= 3  ROM 8x8 double dot pointer (base)
			= 4  ROM 8x8 double dot pointer (top)
			= 5  ROM 9x14 alpha alternate pointer
			= 6  ROM 8x16 character table pointer
			= 7  ROM 9x16 alternate character table pointer
			*/
			switch (GET_BH())
			{
			case 0:
				/*
				CX = bytes per character (points)
				DL = rows (less 1)
				ES:BP = pointer to table
				*/
				//SET_ES(GetW(0x00, 0x1f * 4));
				//SET_BP(GetW(0x00, (0x1f * 4) + 2));
				SET_ES(0x1380); // fake pointer
				SET_BP(0xc000);
				// Set byte/char of on screen font
				SET_CX(io.MemGetB(BIOS_DATA_OFFSET, BIOSMEM_CHAR_HEIGHT));

				// Set Highest char row
				SET_DL(io.MemGetB(BIOS_DATA_OFFSET, BIOSMEM_NB_ROWS));

				break;
			default:
				ASSERT(false, "Unknown int10 12 function: 0x%s\n", n2hexstr(arg).c_str());
			}
			break;
		default:
			ASSERT(false, "Unknown int10 12 function: 0x%s\n", n2hexstr(arg).c_str());
		}
	}
	break;

	//Get DESQView/TopView Virtual Screen Regen Buffer
	case 0xFE:
	{
		/*
		returns:
		ES:DI = address of DESQView/TopView video buffer, DI will always
		be zero
		*/
		SET_ES(CGA_DISPLAY_RAM);
		SET_DI(0);
	}
	break;

	//Update DESQView/TopView Virtual Screen Regen Buffer
	case 0xFF:
	{
		/*
		AH = FF
		CX = number of characters changed
		ES:DI = pointer to first character in buffer to change,  ES is
		set to segment returned by INT 10,FE
		returns nothing
		*/
		// Do nothing
	}
	break;

	//Update DESQView/TopView Virtual Screen Regen Buffer
	case 0x1A:
	{
		/*
		AH = 1A
		AL = 00 get video display combination
		= 01 set video display combination
		BL = active display  (see table below)
		BH = inactive display


		on return:
		AL = 1A, if a valid function was requested in AH
		BL = active display  (AL=00, see table below)
		BH = inactive display  (AL=00)
		*/
		SET_AL(0x1A);
		SET_BL(0x08);
		SET_BH(0);
	}
	break;

	// Video7 VGA,VEGA VGA - INSTALLATION CHECK
	case 0x6F:
		SET_BX(0x5637);
		break;

	// Set Color Palette
	case 0x0B:
		break;

	default:
		ASSERT(false, "Unknown int10 function: 0x%s\n", n2hexstr(function).c_str());
		break;
	}
}


void BIOS::scrollOneLine()
{
	int active_page = io.MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);
	int screenWidth = io.MemGetW(BIOS_DATA_OFFSET, NUMBER_OF_SCREEN_COLUMNS);
	int screenHeight = 25;
	for (int row = 0; row < screenHeight; ++row)
	{
		if (row + 1 < screenHeight)
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				int tmp = io.MemGetB(CGA_DISPLAY_RAM, (row + 1) * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page);
				io.MemStoreB(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, tmp);
			}
		}
		else
		{
			for (int col = 0; col < screenWidth; ++col)
			{
				io.MemStoreB(CGA_DISPLAY_RAM, row * screenWidth * 2 + col * 2 + screenHeight * screenWidth * 2 * active_page, 0);
			}
		}
	}
}


void BIOS::Int13DiskIOServices(int function, int arg)
{
	int diskNo = GET_DL();

	switch (function)
	{
	case 0x00: /* disk controller reset */
		SET_AH(0x00); // no error
		SET_DISK_RET_STATUS(0x00);
		CLEAR_CF(); // no error
		break;

	case 0x01: /* read disk status */
	{
		int status = io.MemGetB(0x0040, 0x0074);
		SET_AH(status);
		SET_DISK_RET_STATUS(0);
		/* set CF if error status read */
		if (status)
			SET_CF();
		else
			CLEAR_CF();
	}
	break;

	case 0x04: // verify disk sectors
		SET_AH(0x00); // no error
		SET_DISK_RET_STATUS(0x00);
		CLEAR_CF(); // no error
		break;

	case 0x02: // read disk sectors
			   /*
			   AL = number of sectors to read	(1-128 dec.)
			   CH = track/cylinder number  (0-1023 dec., see below)
			   CL = sector number  (1-17 dec.)
			   DH = head number  (0-15 dec.)
			   DL = drive number (0=A:, 1=2nd floppy, 80h=drive 0, 81h=drive 1)
			   ES:BX = pointer to buffer
			   */
			   // We have only floppy disk, drive number 0.
		if (io.HasDisk(diskNo))
		{
			int DiskNo = m_cpu.GetRegB(CPU::DL);
			int HeadNo = m_cpu.GetRegB(CPU::DH);
			int TrackNo = m_cpu.GetRegB(CPU::CH);
			int SectorNo = m_cpu.GetRegB(CPU::CL);
			int SectorNum = m_cpu.GetRegB(CPU::AL);
			int ram_address = select(m_cpu.GetSegment(CPU::ES), m_cpu.GetRegW(CPU::BX));

			bool result = io.ReadDisk(diskNo, ram_address, TrackNo, HeadNo, SectorNo, SectorNum);

			if (result)
			{
				SET_AH(0x00); // no error
				SET_DISK_RET_STATUS(0x00);
				CLEAR_CF(); // no error
			}
			else
			{
				SET_AH(0x01);
				SET_CF(); // error
			}
		}
		else
		{
			SET_AH(0x01);
			SET_CF(); // error
		}
		break;

		// Write Disk Sectors
	case 0x03:
		/*
		AH = 03
		AL = number of sectors to write  (1-128 dec.)
		CH = track/cylinder number  (0-1023 dec.)
		CL = sector number  (1-17 dec., see below)
		DH = head number  (0-15 dec.)
		DL = drive number (0=A:, 1=2nd floppy, 80h=drive 0, 81h=drive 1)
		ES:BX = pointer to buffer
		*/
		if (io.HasDisk(diskNo))
		{
			int DiskNo = m_cpu.GetRegB(CPU::DL);
			int HeadNo = m_cpu.GetRegB(CPU::DH);
			int TrackNo = m_cpu.GetRegB(CPU::CH);
			int SectorNo = m_cpu.GetRegB(CPU::CL);
			int SectorNum = m_cpu.GetRegB(CPU::AL);
			int ram_address = select(m_cpu.GetSegment(CPU::ES), m_cpu.GetRegW(CPU::BX));

			bool result = io.WriteDisk(diskNo, ram_address, TrackNo, HeadNo, SectorNo, SectorNum);

			if (result)
			{
				SET_AH(0x00); // no error
				SET_DISK_RET_STATUS(0x00);
				CLEAR_CF(); // no error
			}
			else
			{
				SET_AH(0x01);
				SET_CF(); // error
			}
		}
		else
		{
			SET_AH(0x01);
			SET_CF(); // error
		}
		break;

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

		// We have only floppy disk, drive number 0.
		if (io.HasDisk(diskNo))
		{
			const Disk::BIOS_ParameterBlock& disk_bpb = io.GetDiskBPB(GET_DL());
			SET_DH(disk_bpb.numHeads - 1);
			SET_CL(static_cast<byte>(disk_bpb.secPerTrk));
			SET_CH((disk_bpb.totSec16 / disk_bpb.secPerTrk) / disk_bpb.numHeads - 1);
			SET_DL(1);

			SET_AX(0x00); // no error
			SET_DISK_RET_STATUS(0x00);
			CLEAR_CF(); // no error
		}
		else
		{
			SET_AX(0);
			SET_CX(0);
			SET_DX(0);
			SET_BX(0);
			SET_ES(BIOS_SEGMENT);
			SET_DI(DISKETTE_CONTROLLER_PARAMETER_TABLE2);
			CLEAR_CF();
		}
		break;

	case 0x10: /* disk controller reset */
		SET_AH(0x00); // no error
		SET_DISK_RET_STATUS(0x00);
		CLEAR_CF(); // no error
		break;

	case 0x15: /* disk controller reset */
		if (io.HasDisk(diskNo))
		{
			SET_AH(1);
		}
		else
		{
			SET_AH(0);
		}
		break;

	default:
		ASSERT(false, "Unknown int13 function: 0x%s\n", n2hexstr(function).c_str());
	}
}


void BIOS::Int14AsynchronousCommunicationsServices(int function, int arg)
{
	function = GET_AH();
	switch (function)
	{
		//  Initialize Communications Port Parameters
	case 0:
		// Pretend, we have only COM0
		if (GET_DX() == 0)
		{
			SET_AX(0x6030);
		}
		break;

		// Send Character to Communications Port
	case 0x01:
		SET_AX(0);
		break;
	default:
		ASSERT(false, "Unknown int14 function: 0x%s\n", n2hexstr(function).c_str());
	}
}

void BIOS::Int15SystemBIOSServices(int function, int arg)
{
	switch (function)
	{
		//Return system configuration parameters (PS/2 only)
		/*
		on return:
		CF = 0 if successful
		= 1 if error
		AH = when CF set, 80h for PC & PCjr, 86h for XT
		(BIOS after 11/8/82) and AT (BIOS after 1/10/84)

		ES:BX = pointer to system descriptor table in ROM of the format:
		*/
	case 0xC0:
		SET_BX(BIOS_CONFIG_TABLE);
		SET_ES(BIOS_SEGMENT);
		CLEAR_CF();
		SET_AH(0);
		break;

		//Wait on external event (convertible only)
	case 0x41:
		SET_CF(); // error
		SET_AH(UNSUPPORTED_FUNCTION);
		break;

		// Extended memory size determination
	case 0x88:
		/*
		on return:
		CF = 80h for PC, PCjr
		= 86h for XT and Model 30
		= other machines, set for error, clear for success
		AX = number of contiguous 1k blocks of memory starting
		at address 1024k (100000h)
		*/
		SET_AX(0);
		CLEAR_CF();
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
			io.DisableA20Gate();
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
			io.EnableA20Gate();
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
			SET_AL(io.GetA20GateStatus() ? 1 : 0);
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
			SET_CF(); // error
			SET_AH(UNSUPPORTED_FUNCTION);
			ASSERT(false, "Unknown A20 function: 0x%s\n", n2hexstr(function).c_str());
			break;
		}
		break;

	default:
		ASSERT(false, "Unknown int15 function: 0x%s\n", n2hexstr(function).c_str());
	}
}

void BIOS::Int16KeyboardServices(int function, int arg)
{
	switch (function)
	{
		// Get keystroke status  (AT,PS/2 enhanced keyboards)
	case 0x11:
		// Get keystroke status
	case 0x01:
		/*
		on return:
		ZF = 0 if a key pressed (even Ctrl-Break)
		AX = 0 if no scan code is available
		AH = scan code
		AL = ASCII character or zero if special function key
		*/
		if (io.GetCurrentKeyCode() != 0)
		{
			SET_AX(io.GetCurrentKeyCode());
			CLEAR_ZF();
		}
		else
		{
			SET_AX(0);
			SET_ZF();
		}
		break;

		// Get shift status
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
		SET_AL(io.MemGetB(0x0040, 0x0017));
		break;

		// Wait for keystroke and read. halts program until key with a scancode is pressed
	case 0x00:
		// Wait for keystroke and read  (AT,PS/2 enhanced keyboards). 
		// halts program until key with a scancode is pressed
	case 0x10:
		/*
		on return:
		AH = scan code
		AL = ASCII character or zero if special function key
		*/
		io.KeyboardHalt();
		break;

	default:
		ASSERT(false, "Unknown int16 function: 0x%s\n", n2hexstr(function).c_str());
	}
}

void BIOS::Int17PrinterBIOSServices(int function, int arg)
{
	switch (function)
	{
		// Initialize printer port
	case 0x01:
		// Lets say, we have LPT0
		if (GET_AL() == 0)
		{
			SET_AX(0x1000);
		}
		break;
	default:
		ASSERT(false, "Unknown int17 function: 0x%s\n", n2hexstr(function).c_str());
	}
}
void BIOS::Int1ARealTimeClockServices(int function, int arg)
{
	switch (function)
	{
		// Read system clock counter
	case 0x00:
		/*
		AL = midnight flag, 1 if 24 hours passed since reset
		CX = high order word of tick count
		DX = low order word of tick count
		*/
		// TODO
	{
		int ticks = io.MemGetW(BIOS_DATA_OFFSET, 0x6c) + io.MemGetW(BIOS_DATA_OFFSET, 0x6e) * 0x10000;
		SET_AL(0);
		SET_CX(TIME_ZERO(ticks >> 16));
		SET_DX(TIME_ZERO(ticks & 0xFFFF));
	}
	break;

	// Read real time clock time (AT,PS/2)
	case 0x02:
	{
		/*
		CF = 0 if successful  = 1 if error, RTC not operating
		CH = hours in BCD
		CL = minutes in BCD
		DH = seconds in BCD
		DL = 1 if daylight savings time option*/
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		auto tm = std::localtime(&now);
		SET_CH(TIME_ZERO(decToBcd(tm->tm_hour)));
		SET_CL(TIME_ZERO(decToBcd(tm->tm_min)));
		SET_DH(TIME_ZERO(decToBcd(tm->tm_sec)));
		SET_DL(TIME_ZERO(tm->tm_isdst));
		CLEAR_CF();
		break;
	}

	// Read real time clock date (AT,PS/2)
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
		SET_CH(TIME_ZERO(decToBcd((tm->tm_year + 1900) / 100)));
		SET_CL(TIME_ZERO(decToBcd(tm->tm_year % 100)));
		SET_DH(TIME_ZERO(decToBcd(tm->tm_mon + 1)));
		SET_DL(TIME_ZERO(tm->tm_mday));
		CLEAR_CF();
		break;
	}

	// Read system day counter (PS/2)
	case 0x0a:
	{
		/*
		on return:
		CX = count of days since 1-1-1980n*/
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		int days = static_cast<int>(now / 3600 / 24 - 10 * 365 - 2);
		SET_CX(TIME_ZERO(days));
		CLEAR_CF();
		break;
	}
	}
}
