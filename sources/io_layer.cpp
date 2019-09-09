#include "io_layer.h"
#include "CPU.h"
#include "common_gl.h"
#include <simpletext.h>
#include <string.h>
#include <imgui.h>
#include <map>

extern unsigned char palette1[64][3];
extern unsigned char palette2[64][3];
extern unsigned char palette3[256][3];

extern int64_t g_instruction;
extern int64_t g_frame;

#if defined(_DEBUG) && !defined(__EMSCRIPTEN__)
void APIENTRY DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	const char* sourceStr = "";
	switch (source)
	{
	case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "WINDOW_SYSTEM"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	 sourceStr = "SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:	 sourceStr = "THIRD_PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION:	 sourceStr = "APPLICATION"; break;
	case GL_DEBUG_SOURCE_OTHER:	 sourceStr = "OTHER"; break;
	}
	const char* severityStr = "";
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH: severityStr = "HIGH"; break;
	case GL_DEBUG_SEVERITY_MEDIUM: severityStr = "MEDIUM"; break;
	case GL_DEBUG_SEVERITY_LOW:	 severityStr = "LOW"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:	 severityStr = "NOTIFICATION"; return; break;
	}

	const char* typeStr = "";
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: typeStr = "ERROR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "UNDEFINED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_PORTABILITY:	 typeStr = "PORTABILITY"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:	 typeStr = "PERFORMANCE"; break;
	case GL_DEBUG_TYPE_MARKER:	 typeStr = "MARKER"; break;
	}

	printf("%s: severity %s, type %s, %s\n", sourceStr, severityStr, typeStr, message);
}
#endif

struct VideoMode
{
	enum Class
	{
		TEXT,
		GRAPH
	};
	enum Model
	{
		CTEXT,
		CGA,
		PLANAR1,
		PLANAR4,
		LINEAR8
	};
	Class vmclass;
	Model vmmodel;
	int bit;
	int address;
	int dac;
	int terminal_width;
	int terminal_height_m1;
	int character_height;
	int screen_width;
	int screen_height;
	int page_size;
};

//https://utcc.utoronto.ca/~pkern/stuff/cterm+/cterm/int-10
static std::map<int, VideoMode> videoModes = {
	{ 0x01,{ VideoMode::TEXT,  VideoMode::CTEXT,   4, 0xB800, 2, 40, 24, 16, 320, 400, 0x0800 } },
	{ 0x03,{ VideoMode::TEXT,  VideoMode::CTEXT,   4, 0xB800, 2, 80, 24, 16, 640, 400, 0x1000 } },
	{ 0x04,{ VideoMode::GRAPH, VideoMode::CGA,     2, 0xB800, 1, 40, 24,  8, 320, 200, 0x0800 } },
	{ 0x06,{ VideoMode::GRAPH, VideoMode::CGA,     1, 0xB800, 1, 80, 24,  8, 640, 200, 0x1000 } },
	{ 0x0d,{ VideoMode::GRAPH, VideoMode::PLANAR4, 4, 0xA000, 1, 40, 24,  8, 320, 200, 0x2000 } },
	{ 0x0e,{ VideoMode::GRAPH, VideoMode::PLANAR4, 4, 0xA000, 1, 80, 24,  8, 640, 200, 0x4000 } },
	{ 0x0f,{ VideoMode::GRAPH, VideoMode::PLANAR1, 1, 0xA000, 0, 80, 24, 14, 640, 350, 0x8000 } },
	{ 0x10,{ VideoMode::GRAPH, VideoMode::PLANAR4, 4, 0xA000, 2, 80, 24, 14, 640, 350, 0x8000 } },
	{ 0x11,{ VideoMode::GRAPH, VideoMode::PLANAR1, 1, 0xA000, 2, 80, 29, 16, 640, 480, 0x0000 } },
	{ 0x12,{ VideoMode::GRAPH, VideoMode::PLANAR4, 4, 0xA000, 2, 80, 29, 16, 640, 480, 0x0000 } },
	{ 0x13,{ VideoMode::GRAPH, VideoMode::LINEAR8, 8, 0xA000, 3, 40, 24,  8, 320, 200, 0x0000 } },
};

static unsigned char buffer[320 * 200 * 3];


SimpleText* st = nullptr;

void IO::Init()
{
	m_ram = new byte[0x200000];
	m_port = new byte[0x10000];
	ClearMemory();
	m_currentKeyCode = 0;
	m_start = std::chrono::system_clock::now();
	m_int16_halt = false;
	EnableA20Gate();
	m_texture = 0;
	m_fbo = 0;
	m_currentScanCode = 0;
	m_pending_pit_interrupt = 0;
	InitGL();
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


void IO::DrawScreen(int displayScale)
{
	auto now = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = now - m_start;

	int mode = MemGetB(BIOS_DATA_OFFSET, BIOSMEM_CURRENT_MODE);

	//mode = 0x3;

	if (mode == 0x03)
	{
		if (st == nullptr)
		{
			st = new SimpleText;
		}
		st->Begin();
		int screenWidth = MemGetW(BIOS_DATA_OFFSET, BIOSMEM_NB_COLS);
		int screenHeight = MemGetB(BIOS_DATA_OFFSET, BIOSMEM_NB_ROWS) + 1;
		int active_page = MemGetB(BIOS_DATA_OFFSET, ACTIVE_DISPLAY_PAGE);

		uint32_t p = CGA_DISPLAY_RAM << 4;
		for (int row = 0; row < screenHeight; ++row)
		{
			for (int clm = 0; clm < screenWidth; ++clm)
			{
				char symbol = GetMemory<byte>(p + row * screenWidth * 2 + clm * 2 + screenHeight * screenWidth * 2 * active_page);
				char attribute = GetMemory<byte>(p + row * screenWidth * 2 + clm * 2 + screenHeight * screenWidth * 2 * active_page + 1);

				char forgroundColor = attribute & 0x07;
				char backgroundColor = (attribute & 0x70) >> 4;
				bool forgroundBright = (attribute & 0x08) != 0;
				bool backgroundBright = (attribute & 0x80) != 0;

				SimpleText::Color forgroundColorST = ToSimpleTextColor(forgroundColor);
				SimpleText::Color backgroundColorST = ToSimpleTextColor(backgroundColor);
				st->SetTextSize((SimpleText::FontSize)displayScale);
				st->SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
				st->SetColor(SimpleText::BACKGROUND_COLOR, backgroundColorST, backgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
				st->PutSymbol(symbol, clm * 8 * displayScale, row * 16 * displayScale);
			}
		}

		int top = MemGetB(BIOS_DATA_OFFSET, CURSOR_ENDING);
		int bottom = MemGetB(BIOS_DATA_OFFSET, CURSOR_STARTING);

		int cursor = MemGetW(BIOS_DATA_OFFSET, CURSOR_POSITION + 2 * active_page);
		int xcurs = cursor & 0x00ff; int ycurs = (cursor & 0xff00) >> 8;


		st->End();
		if (!(bottom & 0x20) && ((int)(5 * elapsed_seconds.count())) % 2)
		{
			st->EnableBlending(true);
			st->Begin();
			char attribute = m_ram[p + ycurs * screenWidth * 2 + xcurs * 2 + screenHeight * screenWidth * 2 * active_page + 1];
			char forgroundColor = attribute & 0x07;
			bool forgroundBright = (attribute & 0x08) != 0;
			SimpleText::Color forgroundColorST = ToSimpleTextColor(forgroundColor);
			st->SetColor(SimpleText::TEXT_COLOR, forgroundColorST, forgroundBright ? SimpleText::BOLD : SimpleText::NORMAL);
			st->SetColor(SimpleText::BACKGROUND_COLOR, 0, 0, 0, 0);
			st->PutSymbol('_', xcurs * 8 * displayScale, ycurs * 16 * displayScale);
			st->End();
			st->EnableBlending(false);
		}
	}
	else if (mode == 0x13)
	{
		int width = 320;
		int height = 200;
		uint32_t p = VGA_DISPLAY_RAM << 4;

		for (int i = 0; i < height; ++i)
		{
			for (int j = 0; j < width; ++j)
			{
				buffer[(i * width + j) * 3 + 0] = (m_dac.data[m_ram[p + i * width + j]].r & m_dac.mask) * 4;
				buffer[(i * width + j) * 3 + 1] = (m_dac.data[m_ram[p + i * width + j]].g & m_dac.mask) * 4;
				buffer[(i * width + j) * 3 + 2] = (m_dac.data[m_ram[p + i * width + j]].b & m_dac.mask) * 4;
			}
		}
		BlitData(displayScale);
	}
	else if (mode == 0x4)
	{
		int width = 320;
		int height = 200;
		uint32_t p0 = 0xB800 << 4;
		uint32_t p1 = 0xBA00 << 4;
		uint32_t p2 = 0xBA00 << 4;
		uint32_t p3 = 0xBB00 << 4;
		
		for (int i = 0; i < height; ++i)
		{
			for (int j = 0; j < width; ++j)
			{
				/*
				int plane0 = m_ram[p0 + (i * width + j) / 8];
				int plane1 = m_ram[p1 + (i * width + j) / 8];

				int b0 = (plane0 >> j) & 1;
				int b1 = (plane1 >> j) & 1;

				int index = b0 + b1 * 2;
				*/
				int val = m_ram[(i % 2 ? p1 : p0) + (i / 2 * width + j) / 4];
				int indices[4];
				indices[3] = (val >> 0) & 3;
				indices[2] = (val >> 2) & 3;
				indices[1] = (val >> 4) & 3;
				indices[0] = (val >> 6) & 3;

				int index = indices[(i * width + j) % 4];

				buffer[(i * width + j) * 3 + 0] = (m_dac.data[index].r & m_dac.mask) * 4;
				buffer[(i * width + j) * 3 + 1] = (m_dac.data[index].g & m_dac.mask) * 4;
				buffer[(i * width + j) * 3 + 2] = (m_dac.data[index].b & m_dac.mask) * 4;
			}
		}
		BlitData(displayScale);
	}
}

void IO::ActivateVideoMode(int mode)
{
	auto it = videoModes.find(mode);
	if (it == videoModes.end())
	{
		return;
	}
	const VideoMode& vm = it->second;

	MemStoreB(BIOS_DATA_OFFSET, BIOSMEM_CURRENT_MODE, mode);
	MemStoreW(BIOS_DATA_OFFSET, BIOSMEM_NB_COLS, vm.terminal_width);
	MemStoreW(BIOS_DATA_OFFSET, BIOSMEM_PAGE_SIZE, vm.page_size);
	MemStoreB(BIOS_DATA_OFFSET, BIOSMEM_CHAR_HEIGHT, vm.character_height);
	MemStoreB(BIOS_DATA_OFFSET, BIOSMEM_NB_ROWS, vm.terminal_height_m1);

	glDeleteFramebuffers(1, &m_fbo);
	glDeleteTextures(1, &m_texture);

	m_dac.mask = 0xff;

	if (mode == 0x13)
	{
		memcpy(m_dac.data, palette3, 256 * 3);
	}
	else if (mode == 0x4)
	{
		memcpy(m_dac.data, palette1, 64 * 3);
	}

	if (vm.vmclass == VideoMode::GRAPH)
	{
		glGenTextures(1, &m_texture);

		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void IO::BlitData(int displayScale)
{
	int framebufferBackUp;
	int viewportBackUp[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBackUp);
	glGetIntegerv(GL_VIEWPORT, viewportBackUp);
	GLint dims[4] = { 0 };
	glGetIntegerv(GL_VIEWPORT, dims);
	GLint fbWidth = dims[2];
	GLint fbHeight = dims[3];

	int mode = MemGetB(BIOS_DATA_OFFSET, BIOSMEM_CURRENT_MODE);
	const VideoMode& vm = videoModes[mode];
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vm.screen_width, vm.screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(m_shaderprogram);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));

	int i = 0;
	glUniform1iv(u_text, 1, &i);
	float val[4];
	val[0] = 0;
	val[1] = 1.0f - vm.screen_height * displayScale * 2.0f / float(fbHeight);
	val[2] = vm.screen_width * displayScale * 2.0f / float(fbWidth);
	val[3] = vm.screen_height * displayScale * 2.0f / float(fbHeight);

	glUniform4fv(u_posoffset, 1, val);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void IO::InitGL()
{
#if defined(_DEBUG) && !defined(__EMSCRIPTEN__)
	glDebugMessageCallback(&DebugMessageCallback, nullptr);
	glEnable(GL_DEBUG_OUTPUT);
#endif
	const GLchar* vShaderCode =
		"precision highp float; \n"
		"attribute vec2 in_position; \n"
		"attribute vec2 in_coord; \n"
		"varying vec2 out_coord; \n"
		"uniform vec4 posoffset; \n"
		"void main() {\n"
		"	gl_Position = vec4(vec2(posoffset.xy + posoffset.zw * in_position) * 2.0 - vec2(1.0), 0.0, 1.0); \n"
		"	out_coord = in_coord; \n"
		"}\n";

	const GLchar* fShaderCode =
		"precision highp float; \n"
		"varying vec2 out_coord; \n"
		"uniform sampler2D text; \n"
		"void main() {\n"
		"	vec4 c = texture2D(text, out_coord); \n"
		"	gl_FragColor = c; \n"
		"}\n";

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	printf("m_vertexShader %d\n", vertexShader);
	glShaderSource(vertexShader, 1, &vShaderCode, 0);
	glCompileShader(vertexShader);

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	printf("m_fragmentShader %d\n", fragmentShader);
	glShaderSource(fragmentShader, 1, &fShaderCode, 0);
	glCompileShader(fragmentShader);

	m_shaderprogram = glCreateProgram();
	glAttachShader(m_shaderprogram, vertexShader);
	glAttachShader(m_shaderprogram, fragmentShader);
	glLinkProgram(m_shaderprogram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	u_posoffset = glGetUniformLocation(m_shaderprogram, "posoffset");
	u_text = glGetUniformLocation(m_shaderprogram, "text");

	glBindAttribLocation(m_shaderprogram, 0, "in_position");
	glBindAttribLocation(m_shaderprogram, 1, "in_coord");

	const GLfloat vertices[6][4] = {
		{ 0.0, 0.0, 0.0, 1.0 },
		{ 0.0, 1.0, 0.0, 0.0 },
		{ 1.0, 1.0, 1.0, 0.0 },
		{ 0.0, 0.0, 0.0, 1.0 },
		{ 1.0, 1.0, 1.0, 0.0 },
		{ 1.0, 0.0, 1.0, 1.0 } };

	glGenBuffers(1, &m_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void IO::MemStoreB(word offset, word location, byte x)
{
	m_ram[select(offset, location)] = x;
}


void IO::MemStoreW(word offset, word location, word x)
{
	uint32_t address = select(offset, location);
	m_ram[address] = x & 0xFF;
	m_ram[address + 1] = (x & 0xFF00) >> 8;
}


void IO::MemStoreD(word offset, word location, uint32_t x)
{
	uint32_t address = select(offset, location);
	m_ram[address] = x & 0xFF;
	m_ram[address + 1] = (x & 0xFF00) >> 8;
	m_ram[address + 2] = (x & 0xFF0000) >> 16;
	m_ram[address + 3] = (x & 0xFF000000) >> 24;
}


void IO::MemStoreString(word offset, word location, const char* x)
{
	uint32_t address = select(offset, location);
	strcpy((char*)m_ram + address, x);
}


void IO::MemStore(word offset, word location, byte* x, int size)
{
	uint32_t address = select(offset, location);
	memcpy((char*)m_ram + address, x, size);
}

byte IO::MemGetB(word offset, word location) const
{
	return m_ram[select(offset, location)];
}


word IO::MemGetW(word offset, word location) const
{
	uint32_t address = select(offset, location);
	return ((word)m_ram[address]) | (((word)m_ram[address + 1]) << 8);
}


uint32_t IO::MemGetD(word offset, word location) const
{
	uint32_t address = select(offset, location);
	return 0 
		| ((word)m_ram[address]) 
		| (((word)m_ram[address + 1]) << 8) 
		| (((word)m_ram[address + 2]) << 16) 
		| (((word)m_ram[address + 3]) << 24);
}


void IO::ClearMemory()
{
	memset(m_ram, 0, 0x200000);
	memset(m_port, 0, 0x10000);
	m_int16_halt = false;
}

byte* IO::GetRamRawPointer()
{
	return m_ram;
}

void IO::EnableA20Gate()
{
	A20 = 0x1FFFFF;
}


void IO::DisableA20Gate()
{
	A20 = 0x0FFFFF;
}


bool IO::GetA20GateStatus() const
{
	return A20 == 0x1FFFFF;
}


bool IO::UpdateKeyState()
{
	if (keyBuffer.size() > 0)
	{
		if (keyBuffer.front() != 0)
		{
			int key = keyBuffer.front();
			bool was_halted = SetCurrentKey(key);
			if (was_halted)
			{
				m_cpu->SetRegister(CPU::AX, (word)key);

				keyBuffer.pop_front();
				if (keyBuffer.size() == 0)
				{
					ClearCurrentCode();
				}
				PopKey();
			}
		}
		else
		{
			keyBuffer.pop_front();
		}
	}
	else
	{
		ClearCurrentCode();
	}
	if (scanCodeBuffer.size() > 0)
	{
		int scancode = scanCodeBuffer.front();
		// if release key
		if (scancode & 0x80 && (m_lastReadScanCode == (scancode & 0x7F)))
		{
			std::chrono::duration<float> elapsed_from_key_press = (std::chrono::steady_clock::now() - m_lastKeyPress);
			// Allow at least 100ms, before sending release key. In some games there is risk of key event being omitted.
			if (elapsed_from_key_press.count() < 0.1f)
			{
				return m_int16_halt;
			}
		}
		if (m_cpu->TestFlag<CPU::IF>())
		{
			m_currentScanCode = scancode;
			scanCodeBuffer.pop_front();
			m_cpu->Interrupt(9);
			// If key went down
			if ((m_currentScanCode & 0x80) == 0)
			{
				m_lastKeyPress = std::chrono::steady_clock::now();
			}
		}
	}
	return m_int16_halt;
}


void IO::KeyboardHalt()
{
	m_int16_halt = true;
}


bool IO::IsKeyboardHalted() const
{
	return m_int16_halt;
}


int IO::GetCurrentKeyCode() const
{
	return m_currentKeyCode;
}


void IO::ClearCurrentCode()
{
	m_currentKeyCode = 0;
}


void IO::SetKeyFlags(bool rightShift, bool leftShift, bool CTRL, bool alt)
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
	MemStoreB(0x0040, 0x0017, m_keyFlags);
}


void IO::AddDisk(Disk disk)
{
	if (disk.IsFloppyDrive())
	{
		m_floppyDrives.push_back(disk);
	}
	else
	{
		m_hardDrives.push_back(disk);
	}
}

void IO::ClearDisks()
{
	m_floppyDrives.clear();
	m_hardDrives.clear();
}


int IO::GetDiskCount() const
{
	return static_cast<int>(m_floppyDrives.size() + m_hardDrives.size());
}


bool IO::HasDisk(int diskNo) const
{
	int disk = diskNo & 0x7F;
	if (diskNo & 0x80)
	{
		return m_hardDrives.size() > disk;
	}
	else
	{
		return m_floppyDrives.size() > disk;
	}
}


bool IO::ReadDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount)
{
	m_hddLight |= READ;
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.Read((char*)m_ram + ram_address, cylinder, head, sector, sectorCount);
}


bool IO::WriteDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount)
{
	m_hddLight |= WRITE;
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.Write((const char*)m_ram + ram_address, cylinder, head, sector, sectorCount);
}


const Disk::BIOS_ParameterBlock& IO::GetDiskBPB(int diskNo) const
{
	Disk disk = diskNo & 0x80 ? m_hardDrives[diskNo & 0x7F] : m_floppyDrives[diskNo & 0x7F];
	return disk.GetBiosBlock();
}


int IO::GetBootableDisk() const
{
	for (int i = 0; i < m_floppyDrives.size(); ++i)
	{
		if (m_floppyDrives[i].IsBootlable())
		{
			return i;
		}
	}
	for (int i = 0; i < m_hardDrives.size(); ++i)
	{
		if (m_hardDrives[i].IsBootlable())
		{
			return i + 0x80;
		}
	}
	return -1;
}

IO::HddLightStatus IO::HDDLight()
{
	HddLightStatus ret = m_hddLight;
	m_hddLight = OFF;
	return ret;
}

bool IO::SetCurrentKey(int c)
{
	m_currentKeyCode = c;
	//MemStoreB(BIOS_DATA_OFFSET, 0xBA, (c >> 8));
	if (m_int16_halt && c != 0)
	{
		m_int16_halt = false;
		return true;
	}
	return false;
}

void IO::PushKey(int c, bool release)
{
	if (!release)
	{
		int tail = MemGetW(BIOS_DATA_OFFSET, 0x1C);
		if (tail < 0x1E)
		{
			tail = 0x1E;
		}
		MemStoreW(BIOS_DATA_OFFSET, tail, c);
		tail += 2;
		if ((0x1E + 32) < tail)
		{
			tail = 0x1E;
		}
		MemStoreW(BIOS_DATA_OFFSET, 0x1C, tail);

		keyBuffer.push_back(c);
		byte scanCode = c >> 8;
		scanCodeBuffer.push_back(scanCode);
		MemStoreB(BIOS_DATA_OFFSET, 0xBA, scanCode);
	}
	else
	{
		byte scanCode = (c >> 8) | 0x80;
		scanCodeBuffer.push_back(scanCode);
		MemStoreB(BIOS_DATA_OFFSET, 0xBA, scanCode);
	}
	while (scanCodeBuffer.size() > 20)
	{
		scanCodeBuffer.pop_back();
	}
}


int IO::PopKey()
{
	int head = MemGetW(BIOS_DATA_OFFSET, 0x1A);
	int c = MemGetW(BIOS_DATA_OFFSET, head);
	head += 2;
	if (head < 0x1E)
	{
		head = 0x1E;
	}
	if ((0x1E + 32) <= head)
	{
		head = 0x1E;
	}
	MemStoreW(BIOS_DATA_OFFSET, 0x1A, head);
	return c;
}


bool IO::HasKeyToPop() const
{
	int head = MemGetW(BIOS_DATA_OFFSET, 0x1A);
	int tail = MemGetW(BIOS_DATA_OFFSET, 0x1C);
	return head != tail;
}


void LogPortAccess(int address)
{
	// 0000..000F
	if (0
		|| (address >= 0x0000 && address <= 0x000F)
		|| (address >= 0x0080 && address <= 0x008F)
		|| (address >= 0x00C0 && address <= 0x00DE)
		) {
		//WARN("DMA controller 0x%04x", address);
	}
	else if (0
		|| (address == 0x0020)
		|| (address == 0x0021)
		|| (address == 0x00A0)
		|| (address == 0x00A1)
		) {
		//WARN("8259 PIC 0x%04x", address);
	}
	else if (0
		|| (address >= 0x0040 && address <= 0x0043)
		|| (address == 0x0061)
		) {
		//WARN("8254 PIT 0x%04x", address);
	}
	else if (0
		|| (address == 0x0060)
		|| (address == 0x0064)
		) {
		//WARN("8254 PIT 0x%04x", address);
	}
	else if (0
		|| (address >= 0x03B4 && address <= 0x03B5)
		|| (address == 0x03BA)
		|| (address >= 0x03C0 && address <= 0x03CF)
		|| (address >= 0x03D4 && address <= 0x03D5)
		|| (address == 0x03DA)
		) {
		WARN("vga video 0x%04x", address);
	}
}


word IO::GetPort(uint32_t address, IN_OUT_SIZE size)
{
	if (size == WORD)
	{
		return GetPort(address, BYTE) | (GetPort(address + 1, BYTE) << 8);
	}

	if (0
		|| (address >= 0x0000 && address <= 0x000F)
		|| (address >= 0x0080 && address <= 0x008F)
		|| (address >= 0x00C0 && address <= 0x00DE)
		) {
		//WARN("DMA controller 0x%04x", address);
		// Ignore DMA...
		return 0xff;
	}

	switch (address) 
	{

	//https://wiki.osdev.org/PIT
	case 0x40: case 0x41: case 0x42:
	{
		int channel = address & 0x3;
		PIT& p = m_pit[channel];
		int count = p.current_count;
		if (p.latch_acive)
		{
			count = p.current_count_latched;
		}
		if (p.access_mode == 1)
		{
			p.latch_acive = false;
			p.access_cycle = 0;
			return count & 0xff;
		}
		else if (p.access_mode == 2)
		{
			p.latch_acive = false;
			p.access_cycle = 0;
			return (count >> 8) & 0xff;
		}
		else if (p.access_mode == 3)
		{
			if (p.access_cycle == 0)
			{
				p.access_cycle = 1;
				return count & 0xff;
			}
			else
			{
				p.latch_acive = false;
				p.access_cycle = 0;
				return (count >> 8) & 0xff;
			}
		}
		break;
	}

	case 0x61:
		// Ignore sound;
		break;

	case 0x43:
		return 0;

		// https://wiki.osdev.org/%228042%22_PS/2_Controller
	case 0x60:
	{
		int val = m_currentScanCode;
		m_lastReadScanCode = m_currentScanCode;
		m_currentScanCode = 0;
		return val;
	}


	// VGA
	// Just ignoring most of vga controls... 
	case 0x03c0: case 0x03c1: case 0x03c4: case 0x03c5: case 0x03ce: case 0x03cf:
		return 0;

	case 0x03ba:
	case 0x03ca:
	case 0x03da:
		return  0x08; //always in retrace

	case 0x03c6:
		return m_dac.mask;

	case 0x03c7:
		return m_dac.state;

	case 0x03c8:
		return m_dac.wreg;

	case 0x03c9:
		if (m_dac.state == 0x03) 
		{
			int ret = 0;
			switch (m_dac.rcycle) {
			case 0:
				ret = m_dac.data[m_dac.rreg].r;
				break;
			case 1:
				ret = m_dac.data[m_dac.rreg].g;
				break;
			case 2:
				ret = m_dac.data[m_dac.rreg].b;
				break;
			}
			if (++m_dac.rcycle >= 3)
			{
				m_dac.rcycle = 0;
				++m_dac.rreg;
			}
			return ret;
		}
		else {
			return 0x3f;
		}
		break;
	}

	LogPortAccess(address);
	return *reinterpret_cast<byte*>(m_port + A20_GATE(address));
}

void IO::SetPort(uint32_t address, word wx, IN_OUT_SIZE size)
{
	if (size == WORD)
	{
		SetPort(address, wx & 0xFF, BYTE);
		SetPort(address + 1, (wx >> 8) & 0xFF, BYTE);
	}

	byte x = (byte)wx;

	if (0
		|| (address >= 0x0000 && address <= 0x000F)
		|| (address >= 0x0080 && address <= 0x008F)
		|| (address >= 0x00C0 && address <= 0x00DE)
		) {
		//WARN("DMA controller 0x%04x", address);
		// Ignore DMA...
		return;
	}

	switch (address)
	{
	//https://wiki.osdev.org/PIT
	case 0x40: case 0x41: case 0x42:
	{
		int channel = address & 0x3;
		PIT& p = m_pit[channel];
		if (p.access_mode == 1)
		{
			p.reload_val = x;
			p.access_cycle = 0;
			p.changing_reload = false;
			p.current_count = p.reload_val;
		}
		else if (p.access_mode == 2)
		{
			p.reload_val = x << 8;
			p.access_cycle = 0;
			p.changing_reload = false;
			p.current_count = p.reload_val;
		}
		else if (p.access_mode == 3)
		{
			if (p.access_cycle == 0)
			{
				p.access_cycle = 1;
				p.reload_val = x;
			}
			else
			{
				p.reload_val |= x << 8;
				p.changing_reload = false;
				p.access_cycle = 0;
				p.current_count = p.reload_val;
			}
		}
		break;
	}
	case 0x43:
	{
		int channel = x >> 6;
		PIT& p = m_pit[channel];
		byte mode = (x >> 4) & 0b11;
		if (mode != 0)
		{
			p.access_mode = mode;
		}
		else
		{
			p.latch_acive = true;
			p.current_count_latched = p.current_count;
		}
		p.operating_mode = (x >> 1) & 0b111;
		p.changing_reload = true;
		p.access_cycle = 0;

		break;
	}

	case 0x61:
		// Ignore sound;
		break;

	// VGA
	// Just ignoring most of vga controls...
	case 0x03c0: case 0x03c1: case 0x03c4: case 0x03c5: case 0x03ce: case 0x03cf:
		break;

	case 0x03c6:
		m_dac.mask = x;
		break;

	case 0x03c7:
		m_dac.rreg = x;
		m_dac.rcycle = 0;
		m_dac.state = 0x03;
		break;
	case 0x03c8:
		m_dac.wreg = x;
		m_dac.wcycle = 0;
		m_dac.state = 0x00;
		break;
	case 0x03c9:
		switch (m_dac.wcycle)
		{
		case 0:
			m_dac.data[m_dac.wreg].r = x;
			break;
		case 1:
			m_dac.data[m_dac.wreg].g = x;
			break;
		case 2:
			m_dac.data[m_dac.wreg].b = x;
			break;
		}

		if (++m_dac.wcycle >= 3)
		{
			m_dac.wcycle = 0;
			++m_dac.wreg;
		}
		break;
	default:
		LogPortAccess(address);
		*reinterpret_cast<byte*>(m_port + A20_GATE(address)) = x;
	}
}

void IO::PITUpdate(int cycles)
{
	for (int i = 0; i < 3; ++i)
	{
		if (!m_pit[i].changing_reload)
		{
			if (m_pit[i].current_count > cycles)
			{
				m_pit[i].current_count -= cycles;
			}
			else
			{
				cycles -= m_pit[i].current_count;

				if (m_pit[i].operating_mode == 2)
				{
					m_pit[i].current_count = m_pit[i].reload_val;
				}
				if (i == 0)
				{
					++m_pending_pit_interrupt;
				}

				m_pit[i].current_count -= cycles;
			}
		}
	}
	if (m_pending_pit_interrupt > 0 && m_cpu->TestFlag<CPU::IF>())
	{
		m_cpu->Interrupt(8);
		--m_pending_pit_interrupt;
	}
}

void IO::AddCpu(CPU & cpu)
{
	m_cpu = &cpu;
}

void IO::GUI()
{
	ImGui::Columns(2, "mycolumns");
	ImGui::Separator();
	ImGui::Text("Floppy"); ImGui::NextColumn();
	ImGui::Text("Hard"); ImGui::NextColumn();
	ImGui::Separator();

	int bootdisk = GetBootableDisk();

	for (int i = 0, j = 0; i < m_floppyDrives.size() || j < m_hardDrives.size(); ++i, ++j)
	{
		if (i < m_floppyDrives.size())
		{
			if (i == bootdisk)
			{
				ImGui::Text("B:");
				ImGui::SameLine();
			}
			ImGui::Text("%s", m_floppyDrives[i].GetPath().c_str());
			ImGui::Text("totSec16: %d", m_floppyDrives[i].GetBiosBlock().totSec16);
			//ImGui::Text("Label: %s", m_floppyDrives[i].GetBiosBlock().volLabel);
			//ImGui::Text("FS: %s", m_floppyDrives[i].GetBiosBlock().fileSysType);
		}
		ImGui::NextColumn();
		if (j < m_hardDrives.size())
		{
			if ((j | 0x80) == bootdisk)
			{
				ImGui::Text("B:");
				ImGui::SameLine();
			}
			ImGui::Text("%s", m_hardDrives[j].GetPath().c_str());
			ImGui::Text("totSec16: %d", m_hardDrives[i].GetBiosBlock().totSec16);
			//ImGui::Text("Label: %s", m_hardDrives[i].GetBiosBlock().volLabel);
			//ImGui::Text("FS: %s", m_hardDrives[i].GetBiosBlock().fileSysType);
		}
		ImGui::NextColumn();
	}
	ImGui::Separator();
	ImGui::Columns(1);
}

bool IO::IsCustomIntHandlerInstalled(int x)
{
	return (MemGetW(0x0, x * 4) != DEFAULT_HANDLER || MemGetW(0x0, x * 4 + 2) != BIOS_SEGMENT);
}

unsigned char palette1[64][3] =
{
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f
};

unsigned char palette2[64][3] =
{
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x2a,0x00, 0x2a,0x2a,0x2a,
	0x00,0x00,0x15, 0x00,0x00,0x3f, 0x00,0x2a,0x15, 0x00,0x2a,0x3f, 0x2a,0x00,0x15, 0x2a,0x00,0x3f, 0x2a,0x2a,0x15, 0x2a,0x2a,0x3f,
	0x00,0x15,0x00, 0x00,0x15,0x2a, 0x00,0x3f,0x00, 0x00,0x3f,0x2a, 0x2a,0x15,0x00, 0x2a,0x15,0x2a, 0x2a,0x3f,0x00, 0x2a,0x3f,0x2a,
	0x00,0x15,0x15, 0x00,0x15,0x3f, 0x00,0x3f,0x15, 0x00,0x3f,0x3f, 0x2a,0x15,0x15, 0x2a,0x15,0x3f, 0x2a,0x3f,0x15, 0x2a,0x3f,0x3f,
	0x15,0x00,0x00, 0x15,0x00,0x2a, 0x15,0x2a,0x00, 0x15,0x2a,0x2a, 0x3f,0x00,0x00, 0x3f,0x00,0x2a, 0x3f,0x2a,0x00, 0x3f,0x2a,0x2a,
	0x15,0x00,0x15, 0x15,0x00,0x3f, 0x15,0x2a,0x15, 0x15,0x2a,0x3f, 0x3f,0x00,0x15, 0x3f,0x00,0x3f, 0x3f,0x2a,0x15, 0x3f,0x2a,0x3f,
	0x15,0x15,0x00, 0x15,0x15,0x2a, 0x15,0x3f,0x00, 0x15,0x3f,0x2a, 0x3f,0x15,0x00, 0x3f,0x15,0x2a, 0x3f,0x3f,0x00, 0x3f,0x3f,0x2a,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f
};

unsigned char palette3[256][3] =
{
	0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
	0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
	0x00,0x00,0x00, 0x05,0x05,0x05, 0x08,0x08,0x08, 0x0b,0x0b,0x0b, 0x0e,0x0e,0x0e, 0x11,0x11,0x11, 0x14,0x14,0x14, 0x18,0x18,0x18,
	0x1c,0x1c,0x1c, 0x20,0x20,0x20, 0x24,0x24,0x24, 0x28,0x28,0x28, 0x2d,0x2d,0x2d, 0x32,0x32,0x32, 0x38,0x38,0x38, 0x3f,0x3f,0x3f,
	0x00,0x00,0x3f, 0x10,0x00,0x3f, 0x1f,0x00,0x3f, 0x2f,0x00,0x3f, 0x3f,0x00,0x3f, 0x3f,0x00,0x2f, 0x3f,0x00,0x1f, 0x3f,0x00,0x10,
	0x3f,0x00,0x00, 0x3f,0x10,0x00, 0x3f,0x1f,0x00, 0x3f,0x2f,0x00, 0x3f,0x3f,0x00, 0x2f,0x3f,0x00, 0x1f,0x3f,0x00, 0x10,0x3f,0x00,
	0x00,0x3f,0x00, 0x00,0x3f,0x10, 0x00,0x3f,0x1f, 0x00,0x3f,0x2f, 0x00,0x3f,0x3f, 0x00,0x2f,0x3f, 0x00,0x1f,0x3f, 0x00,0x10,0x3f,
	0x1f,0x1f,0x3f, 0x27,0x1f,0x3f, 0x2f,0x1f,0x3f, 0x37,0x1f,0x3f, 0x3f,0x1f,0x3f, 0x3f,0x1f,0x37, 0x3f,0x1f,0x2f, 0x3f,0x1f,0x27,

	0x3f,0x1f,0x1f, 0x3f,0x27,0x1f, 0x3f,0x2f,0x1f, 0x3f,0x37,0x1f, 0x3f,0x3f,0x1f, 0x37,0x3f,0x1f, 0x2f,0x3f,0x1f, 0x27,0x3f,0x1f,
	0x1f,0x3f,0x1f, 0x1f,0x3f,0x27, 0x1f,0x3f,0x2f, 0x1f,0x3f,0x37, 0x1f,0x3f,0x3f, 0x1f,0x37,0x3f, 0x1f,0x2f,0x3f, 0x1f,0x27,0x3f,
	0x2d,0x2d,0x3f, 0x31,0x2d,0x3f, 0x36,0x2d,0x3f, 0x3a,0x2d,0x3f, 0x3f,0x2d,0x3f, 0x3f,0x2d,0x3a, 0x3f,0x2d,0x36, 0x3f,0x2d,0x31,
	0x3f,0x2d,0x2d, 0x3f,0x31,0x2d, 0x3f,0x36,0x2d, 0x3f,0x3a,0x2d, 0x3f,0x3f,0x2d, 0x3a,0x3f,0x2d, 0x36,0x3f,0x2d, 0x31,0x3f,0x2d,
	0x2d,0x3f,0x2d, 0x2d,0x3f,0x31, 0x2d,0x3f,0x36, 0x2d,0x3f,0x3a, 0x2d,0x3f,0x3f, 0x2d,0x3a,0x3f, 0x2d,0x36,0x3f, 0x2d,0x31,0x3f,
	0x00,0x00,0x1c, 0x07,0x00,0x1c, 0x0e,0x00,0x1c, 0x15,0x00,0x1c, 0x1c,0x00,0x1c, 0x1c,0x00,0x15, 0x1c,0x00,0x0e, 0x1c,0x00,0x07,
	0x1c,0x00,0x00, 0x1c,0x07,0x00, 0x1c,0x0e,0x00, 0x1c,0x15,0x00, 0x1c,0x1c,0x00, 0x15,0x1c,0x00, 0x0e,0x1c,0x00, 0x07,0x1c,0x00,
	0x00,0x1c,0x00, 0x00,0x1c,0x07, 0x00,0x1c,0x0e, 0x00,0x1c,0x15, 0x00,0x1c,0x1c, 0x00,0x15,0x1c, 0x00,0x0e,0x1c, 0x00,0x07,0x1c,

	0x0e,0x0e,0x1c, 0x11,0x0e,0x1c, 0x15,0x0e,0x1c, 0x18,0x0e,0x1c, 0x1c,0x0e,0x1c, 0x1c,0x0e,0x18, 0x1c,0x0e,0x15, 0x1c,0x0e,0x11,
	0x1c,0x0e,0x0e, 0x1c,0x11,0x0e, 0x1c,0x15,0x0e, 0x1c,0x18,0x0e, 0x1c,0x1c,0x0e, 0x18,0x1c,0x0e, 0x15,0x1c,0x0e, 0x11,0x1c,0x0e,
	0x0e,0x1c,0x0e, 0x0e,0x1c,0x11, 0x0e,0x1c,0x15, 0x0e,0x1c,0x18, 0x0e,0x1c,0x1c, 0x0e,0x18,0x1c, 0x0e,0x15,0x1c, 0x0e,0x11,0x1c,
	0x14,0x14,0x1c, 0x16,0x14,0x1c, 0x18,0x14,0x1c, 0x1a,0x14,0x1c, 0x1c,0x14,0x1c, 0x1c,0x14,0x1a, 0x1c,0x14,0x18, 0x1c,0x14,0x16,
	0x1c,0x14,0x14, 0x1c,0x16,0x14, 0x1c,0x18,0x14, 0x1c,0x1a,0x14, 0x1c,0x1c,0x14, 0x1a,0x1c,0x14, 0x18,0x1c,0x14, 0x16,0x1c,0x14,
	0x14,0x1c,0x14, 0x14,0x1c,0x16, 0x14,0x1c,0x18, 0x14,0x1c,0x1a, 0x14,0x1c,0x1c, 0x14,0x1a,0x1c, 0x14,0x18,0x1c, 0x14,0x16,0x1c,
	0x00,0x00,0x10, 0x04,0x00,0x10, 0x08,0x00,0x10, 0x0c,0x00,0x10, 0x10,0x00,0x10, 0x10,0x00,0x0c, 0x10,0x00,0x08, 0x10,0x00,0x04,
	0x10,0x00,0x00, 0x10,0x04,0x00, 0x10,0x08,0x00, 0x10,0x0c,0x00, 0x10,0x10,0x00, 0x0c,0x10,0x00, 0x08,0x10,0x00, 0x04,0x10,0x00,

	0x00,0x10,0x00, 0x00,0x10,0x04, 0x00,0x10,0x08, 0x00,0x10,0x0c, 0x00,0x10,0x10, 0x00,0x0c,0x10, 0x00,0x08,0x10, 0x00,0x04,0x10,
	0x08,0x08,0x10, 0x0a,0x08,0x10, 0x0c,0x08,0x10, 0x0e,0x08,0x10, 0x10,0x08,0x10, 0x10,0x08,0x0e, 0x10,0x08,0x0c, 0x10,0x08,0x0a,
	0x10,0x08,0x08, 0x10,0x0a,0x08, 0x10,0x0c,0x08, 0x10,0x0e,0x08, 0x10,0x10,0x08, 0x0e,0x10,0x08, 0x0c,0x10,0x08, 0x0a,0x10,0x08,
	0x08,0x10,0x08, 0x08,0x10,0x0a, 0x08,0x10,0x0c, 0x08,0x10,0x0e, 0x08,0x10,0x10, 0x08,0x0e,0x10, 0x08,0x0c,0x10, 0x08,0x0a,0x10,
	0x0b,0x0b,0x10, 0x0c,0x0b,0x10, 0x0d,0x0b,0x10, 0x0f,0x0b,0x10, 0x10,0x0b,0x10, 0x10,0x0b,0x0f, 0x10,0x0b,0x0d, 0x10,0x0b,0x0c,
	0x10,0x0b,0x0b, 0x10,0x0c,0x0b, 0x10,0x0d,0x0b, 0x10,0x0f,0x0b, 0x10,0x10,0x0b, 0x0f,0x10,0x0b, 0x0d,0x10,0x0b, 0x0c,0x10,0x0b,
	0x0b,0x10,0x0b, 0x0b,0x10,0x0c, 0x0b,0x10,0x0d, 0x0b,0x10,0x0f, 0x0b,0x10,0x10, 0x0b,0x0f,0x10, 0x0b,0x0d,0x10, 0x0b,0x0c,0x10,
	0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00
};
