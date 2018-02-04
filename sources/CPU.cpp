#include "CPU.h"
#include "_assert.h"
#include <imgui.h>
#include <stdio.h>
#include "imgui_memory_editor.h"

static MemoryEditor mem_edit;
static bool logging_state = false;

FILE* logfile;

#define CJMP \
{\
	int8_t rel8 = GetIMM8(); \
	APPEND_HEX_DBG(m_segments[CS]);\
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(uint16_t(IP + rel8)); \
	if (condition) \
	{ \
		IP += rel8; \
	} break;\
}
#define CJMP16 \
{\
	int16_t rel16 = GetIMM16(); \
	APPEND_HEX_DBG(m_segments[CS]);\
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(uint16_t(IP + rel16)); \
	if (condition) \
	{ \
		IP += rel16; \
	} break;\
}

#ifdef _DEBUG
#define UNKNOWN_OPCODE(X) \
			ASSERT(false, "Unknown opcode: 0x%s\n", n2hexstr(X).c_str());
#define APPEND_DBG_REG(X) if (Byte()) { APPEND_DBG(m_regNames[X]); } else { APPEND_DBG(m_regNames[(X) + 8]); }
#define APPEND_DBG_REGB(X) APPEND_DBG(m_regNames[X])
#define APPEND_DBG_REGW(X) APPEND_DBG(m_regNames[X + 8])
#define APPEND_DBG_SEG(X) APPEND_DBG(m_segNames[X])

#else
#define UNKNOWN_OPCODE(X) 
#define APPEND_DBG_REG(X)
#define APPEND_DBG_REGB(X) 
#define APPEND_DBG_REGW(X) 
#define APPEND_DBG_SEG(X)
#endif 

char* print_address(char* dst, uint16_t segment, uint16_t offset)
{
	char* tmp = n2hexstr(dst, segment);
	*tmp++ = ':';
	return &(*n2hexstr(tmp, offset) = '\x0');	
}


void CPU::SetRamPort(byte* ram, byte* port)
{
	m_ram = ram;
	m_port = port;
}

void CPU::SetInterruptHandler(InterruptHandler* interruptHandler)
{
	m_interruptHandler = interruptHandler;
}

CPU::byte CPU::GetRegB(byte i)
{
	byte* reg8file = (byte*)m_registers;
	byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
	return reg8file[_i];
}

CPU::word CPU::GetRegW(byte i)
{
	return m_registers[i];
}

void CPU::SetRegB(byte i, byte x)
{
	byte* reg8file = (byte*)m_registers;
	byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
	reg8file[_i] = x;
}

void CPU::SetRegW(byte i, word x)
{
	m_registers[i] = x;
}

CPU::word CPU::GetSegment(byte i)
{
	return m_segments[i];
}

void CPU::SetSegment(byte i, word v)
{
	m_segments[i] = v;
}

void CPU::INT(byte x)
{
	m_interruptHandler->Int(x);
}

inline CPU::word CPU::GetReg(byte i)
{
	if (Byte())
	{
		return GetRegB(i);
	}
	else
	{
		return m_registers[i];
	}
}

inline void CPU::SetReg(byte i, word x)
{
	if (Byte())
	{
		SetRegB(i, x & 0xFF);
	}
	else
	{
		m_registers[i] = x;
	}
}

void CPU::PrepareOperands(bool signextend)
{
	switch (ADDRESS_METHOD & DataDirectionMask)
	{
	case RM_REG:
		MODREGRM = ExtractByte();
		OPERAND_A = GetRM();
		APPEND_DBG(", ");
		OPERAND_B = GetReg();
		break;
	case REG_RM:
		MODREGRM = ExtractByte();
		OPERAND_A = GetReg();
		APPEND_DBG(", ");
		OPERAND_B = GetRM();
		break;
	case AXL_IMM:
		APPEND_DBG_REG(0);
		OPERAND_A = GetReg(0);
		APPEND_DBG(", 0x");
		if (Byte())
		{
			byte data = GetIMM8();
			OPERAND_B = data;
			APPEND_HEX_DBG(data);
		}
		else
		{
			word imm16 = GetIMM16();
			OPERAND_B = imm16;
			APPEND_HEX_DBG(imm16);
		}
		break;
	case RM_IMM:
		MODREGRM = ExtractByte();
		OPERAND_A = GetRM();
		APPEND_DBG(", 0x");
		if (Byte())
		{
			byte data = GetIMM8();
			OPERAND_B = data;
			APPEND_HEX_DBG(data);
		}
		else
		{
			if (signextend)
			{
				int8_t data = GetIMM8();
				int16_t data16 = data;
				OPERAND_B = (word)data16;
				APPEND_HEX_DBG((word)data16);
			}
			else
			{
				OPERAND_B = GetIMM16();
				APPEND_HEX_DBG((word)OPERAND_B);
			}
		}
		break;
	}
}

inline bool CPU::Byte()
{
	return (ADDRESS_METHOD & rMask) == r8;
}

inline CPU::word CPU::GetReg()
{
	byte r = (MODREGRM & REG) >> 3;
	APPEND_DBG_REG(r);
	return GetReg(r);
}

inline void CPU::SetReg(word x)
{
	SetReg((MODREGRM & REG) >> 3, x);
}

inline uint8_t& CPU::MemoryByte(uint32_t address)
{
	return *(m_ram + (address & A20));
}

inline uint16_t& CPU::MemoryWord(uint32_t address)
{
	return *reinterpret_cast<uint16_t*>(m_ram + (address & A20));
}
inline uint8_t& CPU::PortByte(uint32_t address)
{
	return *(m_port + address);
}
inline uint16_t& CPU::PortWord(uint32_t address)
{
	return *reinterpret_cast<uint16_t*>(m_port + address);
}

inline CPU::word CPU::GetRM()
{
	if ((MODREGRM & MOD) == MOD)
	{
		byte r = MODREGRM & RM;
		APPEND_DBG_REG(r);
		return GetReg(r);
	}
	else
	{
		SetRM_Address();

		if (Byte())
		{
			return MemoryByte(ADDRESS);
		}
		else
		{
			return MemoryWord(ADDRESS);
		}
	}
}

inline void CPU::SetRM(word x, bool computeAddress)
{
	if ((MODREGRM & MOD) == MOD)
	{
		byte r = MODREGRM & RM;
		if (computeAddress)
		{
			APPEND_DBG_REG(r);
		}
		SetReg(r, x);
	}
	else
	{
		if (computeAddress)
		{
			SetRM_Address();
		}

		if (Byte())
		{
			MemoryByte(ADDRESS) = (byte)x;
		}
		else
		{
			MemoryWord(ADDRESS) = x;
		}
	}
}

inline void CPU::SetRM_Address()
{
	int32_t reg = 0;
	int32_t disp = 0;
	byte offsetreg = DS;
	APPEND_DBG("[");
	APPEND_DBG_SEG(offsetreg);
	APPEND_DBG(":");
	switch (MODREGRM & RM)
	{
	case 0: reg = (uint32_t)m_registers[BX] + m_registers[SI]; APPEND_DBG("BX + SI"); break;
	case 1: reg = (uint32_t)m_registers[BX] + m_registers[DI]; APPEND_DBG("BX + DI"); break;
	case 2: reg = (uint32_t)m_registers[BP] + m_registers[SI]; offsetreg = SS;  APPEND_DBG("BP + SI"); break;
	case 3: reg = (uint32_t)m_registers[BP] + m_registers[DI]; offsetreg = SS;  APPEND_DBG("BP + DI"); break;
	case 4: reg = (uint32_t)m_registers[SI]; APPEND_DBG("SI"); break;
	case 5: reg = (uint32_t)m_registers[DI]; APPEND_DBG("DI"); break;
	case 6: 
		if ((MODREGRM & MOD) != 0) 
		{ 
			reg = m_registers[BP]; 
			offsetreg = SS; 
			APPEND_DBG("BP") 
		}
		else 
		{
			reg = GetIMM16(); 
			APPEND_HEXN_DBG(reg, 4); 
		} 
		break;
	case 7: reg = m_registers[BX]; APPEND_DBG("BX") break;
	}
	if ((MODREGRM & MOD) == 0x40)
	{
		int8_t disp8 = GetIMM8();
		APPEND_DBG(" + 0x");
		APPEND_HEX_DBG(disp8);
		disp = disp8; // sign extension
	}
	else if ((MODREGRM & MOD) == 0x80)
	{
		int16_t disp16 = GetIMM16();
		APPEND_DBG(" + 0x");
		APPEND_HEX_DBG(disp16);
		disp = disp16; // sign extension
	}
	APPEND_DBG("]");
	EFFECTIVE_ADDRESS = reg + disp;
	if (m_segmentOverride != NONE)
	{
		offsetreg = m_segmentOverride;
	}
	ADDRESS = select(m_segments[offsetreg], reg + disp) & A20;
}

inline void CPU::StoreResult()
{
	switch (ADDRESS_METHOD & DataDirectionMask)
	{
	case RM_IMM:
	case RM_REG:
		SetRM(RESULT);
		break;
	case REG_RM:
		SetReg((MODREGRM & REG) >> 3, RESULT);
		break;
	case AXL_IMM:
		SetReg(0, RESULT);
		break;
	}
}

inline CPU::byte CPU::GetIMM8()
{
	return ExtractByte();
}

inline CPU::word CPU::GetIMM16()
{
	word result = ExtractByte();
	result |= ExtractByte() << 8;
	return result;
}

inline CPU::byte CPU::ExtractByte()
{
	uint32_t address = m_segments[CS];
	address = (address << 4) + IP++;
	return m_ram[(address & 0xFFFFF)];
}

inline void CPU::UpdateFlags_CF()
{
	if (Byte())
	{
		SetFlag<CF>((RESULT & 0x100) != 0);
	}
	else
	{
		SetFlag<CF>((RESULT & 0x10000) != 0);
	}
}

inline void CPU::UpdateFlags_OF()
{
	bool a_positive;
	bool b_positive;
	bool r_positive;
	if (Byte())
	{
		a_positive = (OPERAND_A & 0x80) == 0;
		b_positive = (OPERAND_B & 0x80) == 0;
		r_positive = (RESULT & 0x80) == 0;
	}
	else
	{
		a_positive = (OPERAND_A & 0x8000) == 0;
		b_positive = (OPERAND_B & 0x8000) == 0;
		r_positive = (RESULT & 0x8000) == 0;
	}
	SetFlag<OF>(
		(!a_positive && !b_positive && r_positive)
		|| (a_positive && b_positive && !r_positive)
		);
}

inline void CPU::UpdateFlags_AF()
{
	byte a = (OPERAND_A & 0xf);
	byte b = (OPERAND_B & 0xf);
	a += b;
	SetFlag<AF>((a & 0x10) != 0);
}

inline void CPU::UpdateFlags_PF()
{
	//byte b = RESULT;
	//b ^= b >> 4;
	//b ^= b >> 2;
	//b ^= b >> 1;
	SetFlag<PF>(parity[RESULT & 0xFF] == 1);
}

inline void CPU::UpdateFlags_ZF()
{
	if (Byte())
	{
		SetFlag<ZF>((RESULT & 0xFF) == 0);
	}
	else
	{
		SetFlag<ZF>((RESULT & 0xFFFF) == 0);
	}
}

inline void CPU::UpdateFlags_SF()
{
	if (Byte())
	{
		SetFlag<SF>((RESULT & 0x80) != 0);
	}
	else
	{
		SetFlag<SF>((RESULT & 0x8000) != 0);
	}
}
inline void CPU::UpdateFlags_CFOFAF()
{
	UpdateFlags_CF();
	UpdateFlags_OF();
	UpdateFlags_AF();
}
inline void CPU::UpdateFlags_SFZFPF()
{
	UpdateFlags_SF();
	UpdateFlags_ZF();
	UpdateFlags_PF();
}
inline void CPU::ClearFlags_CFOF()
{
	ClearFlag<CF>();
	ClearFlag<OF>();
}
inline void CPU::UpdateFlags_OFSFZFAFPF()
{
	UpdateFlags_OF();
	UpdateFlags_SF();
	UpdateFlags_ZF();
	UpdateFlags_AF();
	UpdateFlags_PF();
}
inline void CPU::Push(word x)
{
	m_registers[SP] -= 2;
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	MemoryWord(address) = x;
}
inline CPU::word CPU::Pop()
{
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	word x = MemoryWord(address);
	m_registers[SP] += 2;
	return x;
}

CPU::CPU()
{
	Reset();
#ifdef _DEBUG
	logfile = fopen("log.txt", "w");
#endif
	mem_edit.GotoAddr = 0x7C00;
	m_scrollToBottom = true;
}

void CPU::Reset()
{
	memset(m_registers, 0, sizeof(word) * 8);
	memset(m_segments, 0, sizeof(word) * 4);
	m_registers[SP] = 0xFFD6;
	IP = 0;
	m_segments[CS] = 0x10;
	m_flags = 0;
	dbg_cmd_address[0] = 0;
	dbg_cmd_name[0] = 0;
	dbg_args[0] = 0;
	dbg_args_ptr = dbg_args;
	DisableA20Gate();
}

void CPU::Step()
{
	dbg_args_ptr = dbg_args;
	dbg_args[0] = 0;
	
	memcpy(m_old_registers, m_registers, sizeof(m_registers));
	memcpy(m_old_segments, m_segments, sizeof(m_segments));
	memcpy(&m_old_flags, &m_flags, sizeof(m_flags));

	bool LOCK = false;
	bool REPN = false;
	bool REP = false;

	m_segmentOverride = NONE;
	int times = 1;
	bool sizeOverride = false;
	bool addressOverride = false;
	// Read Instruction Prefixes. Up to four prefixes of 1 - byte each (optional
	while (true)
	{
		byte data;
		data = ExtractByte();
		switch (data)
		{
		case 0xF0: LOCK = true; break;
		case 0xF2: REPN = true; break;
		case 0xF3: REP = true; break;
		case 0x2E: m_segmentOverride = CS; break;
		case 0x36: m_segmentOverride = SS; break;
		case 0x3E: m_segmentOverride = DS; break;
		case 0x26: m_segmentOverride = ES; break;
		case 0x66: sizeOverride = true; break;
		case 0x67: addressOverride = true; break;
			//case 0x66: /* operand SIZE override */ break;
			//case 0x67: /* address SIZE override */ break;
			//case 0x0F: /* SIMD stream */ break;
		default:
			OPCODE1 = data;
			goto opcode_;
		}
	}

opcode_:
#ifdef _DEBUG
	print_address(dbg_cmd_address, m_segments[CS], (uint16_t)(IP-1));
	char buff[1024];
	n2hexstr(buff, m_segments[CS]); fprintf(logfile, buff);
	n2hexstr(buff, (uint16_t)(IP - 1)); fprintf(logfile, buff);
#endif

	mem_edit.HighlightMin = (((uint32_t)m_segments[CS]) << 4) + IP - 1;
	mem_edit.HighlightMax = mem_edit.HighlightMin + 1;

	byte seg = 0;
	byte data = 0;
	byte reg = 0;
	bool condition = false;
	uint16_t MSB_MASK = 0;
	bool tempCF = false;

	switch (OPCODE1)
	{
	case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:
		CMD_NAME("ADD");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:
		CMD_NAME("ADC");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A + OPERAND_B + (TestFlag<CF>() ? 1 : 0);
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x20:case 0x21:case 0x22:case 0x23:case 0x24:case 0x25:
		CMD_NAME("AND");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A & OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:
		CMD_NAME("XOR");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A ^ OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x08:case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:
		CMD_NAME("OR");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A | OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x18:case 0x19:case 0x1A:case 0x1B:case 0x1C:case 0x1D:
		CMD_NAME("SBB");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		OPERAND_B += TestFlag<CF>() ? 1 : 0;
		OPERAND_B = -OPERAND_B;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x28:case 0x29:case 0x2A:case 0x2B:case 0x2C:case 0x2D:
		CMD_NAME("SUB");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		OPERAND_B = -OPERAND_B;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x38:case 0x39:case 0x3A:case 0x3B:case 0x3C:case 0x3D:
		CMD_NAME("CMP");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		OPERAND_B = -OPERAND_B;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		break;

	case 0x80:case 0x81:case 0x82:case 0x83:
		ADDRESS_METHOD = RM_IMM | OPCODE1 & 1;
		PrepareOperands((OPCODE1 & 0x03) == 0x03);
		OPCODE2 = (MODREGRM & REG) >> 3;

		switch (OPCODE2)
		{
		case 0x00:
			CMD_NAME("ADD");
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x01:
			CMD_NAME("OR");
			RESULT = OPERAND_A | OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x02:
			CMD_NAME("ADC");
			RESULT = OPERAND_A + OPERAND_B + (TestFlag<CF>() ? 1 : 0);
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x03:
			CMD_NAME("SBB");
			OPERAND_B += TestFlag<CF>() ? 1 : 0;
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x04:
			CMD_NAME("AND");
			RESULT = OPERAND_A & OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x05:
			CMD_NAME("SUB");
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x06:
			CMD_NAME("XOR");
			RESULT = OPERAND_A ^ OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x07:
			CMD_NAME("CMP");
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			break;
		}
		break;

	case 0x88:case 0x89:case 0x8A:case 0x8B:
		CMD_NAME("MOV");
		ADDRESS_METHOD = OPCODE1 & 0x03;
		PrepareOperands();
		RESULT = OPERAND_B;
		StoreResult();
		break;

	case 0x8C:
		CMD_NAME("MOV");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		seg = (MODREGRM & REG) >> 3;
		SetRM(m_segments[seg], true);
		APPEND_DBG(", ");
		APPEND_DBG (m_segNames[seg]);
		break;

	case 0x8E:
		CMD_NAME("MOV");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		seg = (MODREGRM & REG) >> 3;
		APPEND_DBG(m_segNames[seg]);
		APPEND_DBG(", ");
		m_segments[seg] = GetRM();
		break;

	case 0xB0:case 0xB1:case 0xB2:case 0xB3:case 0xB4:case 0xB5:case 0xB6:case 0xB7:
	case 0xB8:case 0xB9:case 0xBA:case 0xBB:case 0xBC:case 0xBD:case 0xBE:case 0xBF:
		CMD_NAME("MOV");
		ADDRESS_METHOD = ((OPCODE1 & 0x08) == 0x0) ? r8 : r16;
		reg = OPCODE1 & 0x07;
		if (Byte())
		{
			APPEND_DBG(m_regNames[reg]);
			APPEND_DBG(", 0x");
			byte data = GetIMM8();
			SetReg(reg, data);
			APPEND_HEX_DBG(data);
		}
		else
		{
			APPEND_DBG(m_regNames[reg + 8]);
			APPEND_DBG(", 0x");
			word data = GetIMM16();
			SetReg(reg, data);
			APPEND_HEX_DBG(data);
		}
		break;

	case 0xC6:case 0xC7:
		CMD_NAME("MOV");
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		SetRM_Address();
		if (Byte())
		{
			byte data = GetIMM8();
			SetRM(data);
			APPEND_DBG(", 0x");
			APPEND_HEX_DBG(data);
		}
		else
		{
			word data = GetIMM16();
			SetRM(data);
			APPEND_DBG(", 0x");
			APPEND_HEX_DBG(data);
		}
		break;

	case 0xCC:
		CMD_NAME("INT 3");
		INT(3);
		break;

	case 0xCD:
		data = GetIMM8();
		CMD_NAME("INT");
		APPEND_HEX_DBG(data);
		INT(data);
		break;

	case 0xCE:
		CMD_NAME("INTO");
		INT(4);
		break;

	case 0xE2:
		CMD_NAME("LOOP");
		--m_registers[CX];
		{
			int8_t rel8 = GetIMM8();
			if (m_registers[CX] != 0)
			{
				IP += rel8;
			}}
		break;

	case 0xE1:
		CMD_NAME("LOOPZ");
		--m_registers[CX];
		{
			int8_t rel8 = GetIMM8();
			APPEND_HEX_DBG(m_segments[CS]);
			APPEND_DBG(":");
			APPEND_HEX_DBG(IP + rel8);
			if (m_registers[CX] != 0 && TestFlag<ZF>())
			{
				IP += rel8;
			}
		}
		break;

	case 0xE0:
		CMD_NAME("LOOPNZ");
		--m_registers[CX];
		{
			int8_t rel8 = GetIMM8();
			APPEND_HEX_DBG(m_segments[CS]);
			APPEND_DBG(":");
			APPEND_HEX_DBG(IP + rel8);
			if (m_registers[CX] != 0 && !TestFlag<ZF>())
			{
				IP += rel8;
			}
		}
		break;

	case 0xF8:
		CMD_NAME("CLC");
		ClearFlag<CF>();
		break;
	case 0xF9:
		CMD_NAME("STC");
		SetFlag<CF>();
		break;
	case 0xFC:
		CMD_NAME("CLD");
		ClearFlag<DF>();
		break;
	case 0xFD:
		CMD_NAME("STD");
		SetFlag<DF>();
		break;
	case 0xFA:
		CMD_NAME("CLI");
		ClearFlag<IF>();
		break;
	case 0xFB:
		CMD_NAME("STI");
		SetFlag<IF>();
		break;
	case 0xF5:
		CMD_NAME("CMC");
		SetFlag<CF>(!TestFlag<CF>());
		break;

	case 0x70:
		CMD_NAME("JO"); condition = TestFlag<OF>(); CJMP
	case 0x71:
		CMD_NAME("JNO"); condition = !TestFlag<OF>(); CJMP
	case 0x72:
		CMD_NAME("JB"); condition = TestFlag<CF>(); CJMP
	case 0x73:
		CMD_NAME("JNB"); condition = !TestFlag<CF>(); CJMP
	case 0x74:
		CMD_NAME("JE"); condition = TestFlag<ZF>(); CJMP
	case 0x75:
		CMD_NAME("JNE"); condition = !TestFlag<ZF>(); CJMP
	case 0x76:
		CMD_NAME("JBE"); condition = TestFlag<CF>() || TestFlag<ZF>(); CJMP
	case 0x77:
		CMD_NAME("JNBE"); condition = !TestFlag<CF>() && !TestFlag<ZF>(); CJMP
	case 0x78:
		CMD_NAME("JS"); condition = TestFlag<SF>(); CJMP
	case 0x79:
		CMD_NAME("JNS"); condition = !TestFlag<SF>(); CJMP
	case 0x7A:
		CMD_NAME("JP"); condition = TestFlag<PF>(); CJMP
	case 0x7B:
		CMD_NAME("JNP"); condition = !TestFlag<PF>(); CJMP
	case 0x7C:
		CMD_NAME("JL"); condition = TestFlag<SF>() != TestFlag<OF>(); CJMP
	case 0x7D:
		CMD_NAME("JNL"); condition = TestFlag<SF>() == TestFlag<OF>(); CJMP
	case 0x7E:
		CMD_NAME("JLE"); condition = TestFlag<ZF>() || (TestFlag<SF>() != TestFlag<OF>()); CJMP
	case 0x7F:
		CMD_NAME("JNLE"); condition = !TestFlag<ZF>() && (TestFlag<SF>() == TestFlag<OF>()); CJMP

	case 0xEA:
		CMD_NAME("JMP");
		{
			uint16_t new_IP = GetIMM16();
			uint16_t new_CS = GetIMM16();
			IP = new_IP;
			m_segments[CS] = new_CS;
			APPEND_HEX_DBG(new_CS);
			APPEND_DBG(":");
			APPEND_HEX_DBG(new_IP);
		}
		break;
	case 0xEB:
		CMD_NAME("JMP");
		IP += (int8_t)GetIMM8();
		APPEND_HEX_DBG(m_segments[CS]);
		APPEND_DBG(":");
		APPEND_HEX_DBG(IP);
		break;
	case 0xE9:
		CMD_NAME("JMP");
		IP += GetIMM16();
		APPEND_HEX_DBG(m_segments[CS]);
		APPEND_DBG(":");
		APPEND_HEX_DBG(IP);
		break;
	case 0xE3:
		CMD_NAME("JCXZ");
		condition = m_registers[CX] == 0; 
		CJMP
		break;
	case 0xE8:
		CMD_NAME("CALL");
		{
			int16_t offset = GetIMM16();
			Push(IP);
			IP += offset;
			APPEND_HEX_DBG(m_segments[CS]);
			APPEND_DBG(":");
			APPEND_HEX_DBG(IP);
		}
		break;

	case 0x9A:
		CMD_NAME("CALL");
		{
			uint16_t new_IP = GetIMM16();
			uint16_t new_CS = GetIMM16();
			Push(m_segments[CS]);
			Push(IP);
			IP = new_IP;
			m_segments[CS] = new_CS;
			APPEND_HEX_DBG(new_CS);
			APPEND_DBG(":");
			APPEND_HEX_DBG(new_IP);
		}
		break;

	case 0xE4: case 0xE5:
		CMD_NAME("IN");
		ADDRESS_METHOD = OPCODE1 & 1;
		ADDRESS = GetIMM8();
		if (Byte())
		{
			SetReg(0, PortByte(ADDRESS));
		}
		else
		{
			SetReg(0, PortWord(ADDRESS));
		}
		break;

	case 0xE6: case 0xE7:
		CMD_NAME("OUT");
		ADDRESS_METHOD = OPCODE1 & 1;
		ADDRESS = GetIMM8();
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			PortByte(ADDRESS) = GetRegB(0);
		}
		else
		{
			APPEND_DBG_REGW(0);
			PortWord(ADDRESS) = GetRegW(0);
		}
		break;

	case 0xEC: case 0xED:
		CMD_NAME("IN");
		ADDRESS_METHOD = OPCODE1 & 1;
		APPEND_DBG_REGW(DX);
		ADDRESS = GetRegW(DX);
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			SetRegB(0, PortByte(ADDRESS));
		}
		else
		{
			APPEND_DBG_REGW(0);
			SetRegW(0, PortWord(ADDRESS));
		}
		break;

	case 0xEE: case 0xEF:
		CMD_NAME("OUT");
		ADDRESS_METHOD = OPCODE1 & 1;
		ADDRESS = GetRegW(DX);
		APPEND_DBG_REGW(DX);
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			PortByte(ADDRESS) = GetRegB(0);
		}
		else
		{
			APPEND_DBG_REGW(0);
			PortWord(ADDRESS) = GetRegW(0);
		}
		break;

	case 0x6E:
		CMD_NAME("OUTS");

		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			byte offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				offsetreg = m_segmentOverride;
			}
			PortByte(GetRegW(DX)) = MemoryByte(select(GetSegment(offsetreg), GetRegW(SI)));

			m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
			m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;

			if (REP)
			{
				m_registers[CX]--;
			}
		}
		break;

	case 0x6F: 
		CMD_NAME("OUTS");

		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			byte offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				offsetreg = m_segmentOverride;
			}
			PortWord(GetRegW(DX)) = MemoryWord(select(GetSegment(offsetreg), GetRegW(SI)));

			m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
			m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;

			if (REP)
			{
				m_registers[CX]--;
			}
		}
		break;

	case 0x6C:
		CMD_NAME("INS");

		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			MemoryByte(select(GetSegment(ES), GetRegW(DI))) = PortByte(GetRegW(DX));
			
			m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
			m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;

			if (REP)
			{
				m_registers[CX]--;
			}
		}
		break;

	case 0x6D:
		CMD_NAME("INS");
		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			MemoryWord(select(GetSegment(ES), GetRegW(DI))) = PortWord(GetRegW(DX));
			m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
			m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;

			if (REP)
			{
				m_registers[CX]--;
			}
		}
		break;

	case 0xFE: case 0xFF:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPERAND_A = GetRM();
		OPERAND_B = MemoryWord(ADDRESS + 2);

		OPCODE2 = (MODREGRM & REG) >> 3;

		switch (OPCODE2)
		{
		case 0x00:
			CMD_NAME("INC");
			OPERAND_B = 1;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_OFSFZFAFPF();
			SetRM(RESULT);
			break;
		case 0x01:
			CMD_NAME("DEC");
			OPERAND_B = -1;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_OFSFZFAFPF();
			SetRM(RESULT);
			break;
		case 0x02:
			CMD_NAME("CALL");
			Push(IP);
			IP = OPERAND_A;
			APPEND_DBG("CS:");
			APPEND_HEX_DBG(IP);
			break;
		case 0x03:
			CMD_NAME("CALL");
			Push(m_segments[CS]);
			Push(IP);
			IP = OPERAND_A;
			m_segments[CS] = OPERAND_B;
			APPEND_DBG("CS:");
			APPEND_HEX_DBG(IP);
			break;
		case 0x04:
			CMD_NAME("JMP");
			IP = OPERAND_A;
			APPEND_DBG("CS:");
			APPEND_HEX_DBG(IP);
			break;
		case 0x05:
			CMD_NAME("JMP");
			IP = OPERAND_A;
			m_segments[CS] = OPERAND_B;
			APPEND_DBG("CS:");
			APPEND_HEX_DBG(IP);
			break;
		case 0x06:
			CMD_NAME("PUSH");
			Push(OPERAND_A);
			break;
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;

	case 0x98:
		CMD_NAME("CBW");
		OPERAND_A = (int8_t)GetRegB(0);
		SetRegW(0, OPERAND_A);
		break;

	case 0x99:
		CMD_NAME("CWD");
		OPERAND_A = GetRegW(0);
		if (OPERAND_A & 0x8000)
		{
			SetRegW(DX, 0xFFFF);
		}
		else
		{
			SetRegW(DX, 0x0000);
		}
		break;

	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		CMD_NAME("INC");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegW(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		CMD_NAME("DEC");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegW(OPCODE1 & 0x07);
		OPERAND_B = -1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		CMD_NAME("PUSH");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegW(OPCODE1 & 0x07);
		Push(OPERAND_A);
		break;

	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		CMD_NAME("POP");
		{
			int reg = OPCODE1 & 0x07;
			ADDRESS_METHOD = r16;
			OPERAND_A = Pop();
			APPEND_DBG_REGW(reg);
			SetRegW(reg, OPERAND_A);
		}
		break;

	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		CMD_NAME("XCHG");
		ADDRESS_METHOD = r16;
		OPERAND_A = GetRegW(OPCODE1 & 0x07);
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		APPEND_DBG(", ");
		OPERAND_B = GetRegW(0);
		APPEND_DBG_REGW(0);
		SetRegW(OPCODE1 & 0x07, OPERAND_B);
		SetRegW(0, OPERAND_A);
		break;

	case 0x06:
		CMD_NAME("PUSH ES");
		Push(m_segments[ES]);
		break;

	case 0x16:
		CMD_NAME("PUSH SS");
		Push(m_segments[SS]);
		break;

	case 0x07:
		CMD_NAME("POP ES");
		m_segments[ES] = Pop();
		break;

	case 0x17:
		CMD_NAME("POP SS");
		m_segments[SS] = Pop();
		break;

	case 0x0E:
		CMD_NAME("PUSH CS");
		Push(m_segments[CS]);
		break;

	case 0x1E:
		CMD_NAME("PUSH DS");
		Push(m_segments[DS]);
		break;

	case 0x9C:
		CMD_NAME("PUSHF");
		Push(m_flags);
		break;

	case 0x9D:
		CMD_NAME("POPF");
		m_flags = Pop();
		break;

	case 0x0F:
		OPCODE2 = GetIMM8();
		switch (OPCODE2)
		{
		case 0x87:
			CMD_NAME("JA"); condition = !TestFlag<CF>() && !TestFlag<ZF>(); CJMP16
		case 0x83:
			CMD_NAME("JAE"); condition = !TestFlag<CF>(); CJMP16
		case 0x82:
			CMD_NAME("JB"); condition = TestFlag<CF>(); CJMP16
		case 0x86:
			CMD_NAME("JBE"); condition = TestFlag<CF>() || TestFlag<ZF>(); CJMP16
		case 0x84:
			CMD_NAME("JZ"); condition = TestFlag<ZF>(); CJMP16
		case 0x8F:
			CMD_NAME("JG"); condition = !TestFlag<ZF>() && (TestFlag<SF>() == TestFlag<OF>()); CJMP16
		case 0x8D:
			CMD_NAME("JGE"); condition = (TestFlag<SF>() == TestFlag<OF>()); CJMP16
		case 0x8C:
			CMD_NAME("JL"); condition = (TestFlag<SF>() != TestFlag<OF>()); CJMP16
		case 0x8E:
			CMD_NAME("JLE"); condition = TestFlag<ZF>() || (TestFlag<SF>() != TestFlag<OF>()); CJMP16
		case 0x85:
			CMD_NAME("JNE"); condition = !TestFlag<ZF>(); CJMP16
		case 0x81:
			CMD_NAME("JNO"); condition = !TestFlag<OF>(); CJMP16
		case 0x8B:
			CMD_NAME("JNP"); condition = !TestFlag<PF>(); CJMP16
		case 0x89:
			CMD_NAME("JNS"); condition = !TestFlag<SF>(); CJMP16
		case 0x80:
			CMD_NAME("JO"); condition = TestFlag<OF>(); CJMP16
		case 0x8A:
			CMD_NAME("JP"); condition = TestFlag<PF>(); CJMP16
		case 0x88:
			CMD_NAME("JS"); condition = TestFlag<SF>(); CJMP16
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;

	case 0x1F:
		CMD_NAME("POP DS");
		m_segments[DS] = Pop();
		break;

	case 0x6A:
		CMD_NAME("PUSH");
		OPERAND_A = GetIMM8();
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x68:
		CMD_NAME("PUSH");
		OPERAND_A = GetIMM16();
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x60:
		{
			CMD_NAME("PUSHA");
			uint16_t originalSP = m_registers[SP];
			Push(m_registers[AX]);
			Push(m_registers[CX]);
			Push(m_registers[DX]);
			Push(m_registers[BX]);
			Push(originalSP);
			Push(m_registers[BP]);
			Push(m_registers[SI]);
			Push(m_registers[DI]);
		}
		break;

	case 0x61:
		{
			CMD_NAME("POPA");
			m_registers[DI] = Pop();
			m_registers[SI] = Pop();
			m_registers[BP] = Pop();
			uint16_t originalSP = Pop();
			m_registers[BX] = Pop();
			m_registers[DX] = Pop();
			m_registers[CX] = Pop();
			m_registers[AX] = Pop();
			//m_registers[SP] = originalSP;
		}
		break;

	case 0x8F:
		CMD_NAME("POP");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		SetRM(Pop(), true);
		break;

	case 0xF6: case 0xF7:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPCODE2 = (MODREGRM & REG) >> 3;

		switch (OPCODE2)
		{
		case 0x00:
		case 0x01:
			CMD_NAME("TEST");
			OPERAND_A = GetRM();
			APPEND_DBG(", ");
			if (Byte())
			{
				OPERAND_B = GetIMM8();
			}
			else
			{
				OPERAND_B = GetIMM16();
			}
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			break;
		case 0x04:
			CMD_NAME("MUL");
			APPEND_DBG_REG(0);
			OPERAND_A = (uint16_t)GetReg(0);
			APPEND_DBG(", ");
			OPERAND_B = (uint16_t)GetRM();
			RESULT = OPERAND_A * OPERAND_B;
			if (Byte())
			{
				m_registers[0] = RESULT;
				SetFlag<OF>(RESULT & 0xFF00);
				SetFlag<CF>(RESULT & 0xFF00);
			}
			else
			{
				m_registers[0] = RESULT;
				m_registers[DX] = RESULT >> 16;
				SetFlag<OF>(RESULT & 0xFFFF0000);
				SetFlag<CF>(RESULT & 0xFFFF0000);
			}
			break;
		case 0x05:
			CMD_NAME("IMUL");
			if (Byte())
			{
				APPEND_DBG_REGB(0);
				OPERAND_A = (int8_t)GetRegB(0);
				APPEND_DBG(", ");
				OPERAND_B = (int8_t)GetRM();
				RESULT = OPERAND_A * OPERAND_B;
				m_registers[0] = RESULT;
				SetFlag<OF>(RESULT & 0xFF00);
				SetFlag<CF>(RESULT & 0xFF00);
			}
			else
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (int16_t)GetRegW(0);
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();
				RESULT = OPERAND_A * OPERAND_B;
				m_registers[0] = RESULT;
				m_registers[DX] = RESULT >> 16;
				SetFlag<OF>(RESULT & 0xFFFF0000);
				SetFlag<CF>(RESULT & 0xFFFF0000);
			}
			break;

		case 0x06:
			CMD_NAME("DIV");
			if (Byte())
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (uint16_t)GetRegW(0);
				APPEND_DBG(", ");
				OPERAND_B = (uint16_t)GetRM();
				SetRegB(AL, OPERAND_A / OPERAND_B);
				SetRegB(AH, (OPERAND_A % OPERAND_B));
			}
			else
			{
				APPEND_DBG_REGW(DX);
				APPEND_DBG(":");
				APPEND_DBG_REGW(0);
				OPERAND_A = (uint32_t)GetRegW(0) + ((uint32_t)GetRegW(DX) << 16);
				APPEND_DBG(", ");
				OPERAND_B = (uint16_t)GetRM();
				if (OPERAND_B == 0)
				{
					m_registers[0] = 0;
					m_registers[DX] = 0;
				}
				else
				{
					m_registers[0] = OPERAND_A / OPERAND_B;
					m_registers[DX] = OPERAND_A % OPERAND_B;
				}
			}
			break;
		case 0x07:
			CMD_NAME("IDIV");
			if (Byte())
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (int16_t)GetRegW(0);
				APPEND_DBG(", ");
				OPERAND_B = (int8_t)GetRM();
				SetRegB(AL, OPERAND_A / OPERAND_B);
				SetRegB(AH, (OPERAND_A % OPERAND_B));
			}
			else
			{
				APPEND_DBG_REGW(DX);
				APPEND_DBG(":");
				APPEND_DBG_REGW(0);
				OPERAND_A = (int32_t)(GetRegW(0) + ((uint32_t)GetRegW(DX) << 16));
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();
				m_registers[0] = OPERAND_A / OPERAND_B;
				m_registers[DX] = OPERAND_A % OPERAND_B;
			}
			break;

		case 0x03:
			CMD_NAME("NEG");
			if (Byte())
			{
				OPERAND_A = (int8_t)GetRM();
				OPERAND_A = -OPERAND_A;
				SetRM(OPERAND_A);
				SetFlag<CF>(OPERAND_A != 0);
			}
			else
			{
				OPERAND_A = (int16_t)GetRM();
				OPERAND_A = -OPERAND_A;
				SetRM(OPERAND_A);
				SetFlag<CF>(OPERAND_A != 0);
			}
			break;

		case 0x02:
			CMD_NAME("NOT");
			OPERAND_A = GetRM();
			OPERAND_A = ~OPERAND_A;
			SetRM(OPERAND_A);
			break;
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;

	case 0x6B:
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		CMD_NAME("IMUL");
		OPERAND_A = (int16_t)GetReg();
		APPEND_DBG(", ");
		OPERAND_B = (int8_t)GetIMM8();
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetFlag<OF>(RESULT & 0xFFFF0000);
		SetFlag<CF>(RESULT & 0xFFFF0000);
		SetReg(RESULT);
		break;

	case 0x69:
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		CMD_NAME("IMUL");
		GetReg();
		APPEND_DBG(", ");
		OPERAND_A = (int16_t)GetRM();
		APPEND_DBG(", ");
		OPERAND_B = (int16_t)GetIMM16();
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetFlag<OF>(RESULT & 0xFFFF0000);
		SetFlag<CF>(RESULT & 0xFFFF0000);
		SetReg(RESULT);
		break;

	case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPCODE2 = (MODREGRM & REG) >> 3;

		if (OPCODE1 & 0x02)
		{
			times = GetRegB(CL) & 0x1F;
		}

		MSB_MASK = (OPCODE1 & 1 )? 0x8000 : 0x80;

		switch (OPCODE2)
		{
		case 0x00:
			CMD_NAME("ROL");
			break;
		case 0x01:
			CMD_NAME("ROR");
			break;
		case 0x02:
			CMD_NAME("RCL");
			break;
		case 0x03:
			CMD_NAME("RCR");
			break;
		case 0x04:
			CMD_NAME("SHL");
			break;
		case 0x05:
			CMD_NAME("SHR");
			break;
		case 0x06:
			CMD_NAME("SAL");
			break;
		case 0x07:
			CMD_NAME("SAR");
			break;
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		OPERAND_A = GetRM();
		RESULT = OPERAND_A;
		
		for (int i = 0; i < times; ++i)
		{
			switch (OPCODE2)
			{
			case 0x00:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2 + TestFlag<CF>();
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				break;
			case 0x01:
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2 + TestFlag<CF>() * MSB_MASK;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != (((MSB_MASK >> 1) & RESULT) != 0));
				break;
			case 0x02:
				tempCF = (MSB_MASK & RESULT) != 0;
				RESULT = RESULT * 2 + TestFlag<CF>();
				SetFlag<CF>(tempCF);
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				break;
			case 0x03:
				tempCF = 1 & RESULT;
				RESULT = RESULT / 2 + TestFlag<CF>() * MSB_MASK;
				SetFlag<CF>(tempCF);
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != (((MSB_MASK >> 1) & RESULT) != 0));
				break;
			case 0x04:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				UpdateFlags_SFZFPF();
				break;
			case 0x05:
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2;
				SetFlag<OF>((MSB_MASK & OPERAND_A) != 0);
				UpdateFlags_SFZFPF();
				break;
			case 0x06:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				UpdateFlags_SFZFPF();
				break;
			case 0x07:
			{
				bool negative = (MSB_MASK & RESULT) != 0;
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2 + negative * MSB_MASK;
				ClearFlag<OF>();
				UpdateFlags_SFZFPF();
			}
				break;

			default:
				UNKNOWN_OPCODE(OPCODE2);
			}
		}
		SetRM(RESULT);
		break;

	case 0xC0: case 0xC1:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPCODE2 = (MODREGRM & REG) >> 3;

		times = GetIMM8();

		MSB_MASK = (OPCODE1 & 1) ? 0x8000 : 0x80;

		switch (OPCODE2)
		{
		case 0x04:
			CMD_NAME("SHL");
			break;
		case 0x05:
			CMD_NAME("SHR");
			break;
		case 0x06:
			CMD_NAME("SAL");
			break;
		case 0x07:
			CMD_NAME("SAR");
			break;
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		OPERAND_A = GetRM();
		RESULT = OPERAND_A;

		for (int i = 0; i < times; ++i)
		{
			switch (OPCODE2)
			{
			case 0x04:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				UpdateFlags_SFZFPF();
				break;
			case 0x05:
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2;
				SetFlag<OF>((MSB_MASK & OPERAND_A) != 0);
				UpdateFlags_SFZFPF();
				break;
			case 0x06:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				UpdateFlags_SFZFPF();
				break;
			case 0x07:
			{
				bool negative = (MSB_MASK & RESULT) != 0;
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2 + negative * MSB_MASK;
				ClearFlag<OF>();
				UpdateFlags_SFZFPF();
				break;
			}

			default:
				UNKNOWN_OPCODE(OPCODE2);
			}
		}
		SetRM(RESULT);
		break;

	case 0xAA: case 0xAB:
		ADDRESS_METHOD = OPCODE1 & 1;
		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			CMD_NAME("STOS");
			APPEND_DBG_REG(0);
			OPERAND_A = GetReg(0);
			APPEND_DBG(" [ES:DI]");
			ADDRESS = GetSegment(ES) << 4;
			ADDRESS += m_registers[DI];

			if (Byte())
			{
				MemoryByte(ADDRESS) = OPERAND_A;
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				MemoryWord(ADDRESS) = OPERAND_A;
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}

			if (i != times - 1)
			{
#ifdef _DEBUG
				*(dbg_args_ptr = dbg_args) = 0;
#endif
			}
			if (REP)
			{
				m_registers[CX]--;
			}
		}

		break;

	case 0xAE: case 0xAF:
		ADDRESS_METHOD = OPCODE1 & 1;
		if (REP || REPN)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			CMD_NAME("SCAS");
			APPEND_DBG_REG(0);
			OPERAND_A = GetReg(0);
			APPEND_DBG(" [ES:DI]");
			ADDRESS = GetSegment(ES) << 4;
			ADDRESS += m_registers[DI];

			if (Byte())
			{
				OPERAND_B = MemoryByte(ADDRESS);
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				OPERAND_B = MemoryWord(ADDRESS);
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}

			if (i != times - 1)
			{
#ifdef _DEBUG
				*(dbg_args_ptr = dbg_args) = 0;
#endif
			}
			if (REP || REPN)
			{
				m_registers[CX]--;
			}
			if (REP && (OPERAND_B != OPERAND_A))
			{
				break;
			}
			if (REPN && (OPERAND_B == OPERAND_A))
			{
				break;
			}
		}

		break;

	case 0xAC: case 0xAD:
		ADDRESS_METHOD = OPCODE1 & 1;
		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			byte offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				offsetreg = m_segmentOverride;
			}

			CMD_NAME("LODS");
			APPEND_DBG(" [");
			APPEND_DBG_SEG(offsetreg);
			ADDRESS = GetSegment(offsetreg) << 4;
			APPEND_DBG(":");
			ADDRESS += m_registers[SI];
			APPEND_DBG("SI], ");

			GetReg(0);

			if (Byte())
			{
				OPERAND_A = MemoryByte(ADDRESS);
				SetReg(0, OPERAND_A);
				m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				OPERAND_A = MemoryWord(ADDRESS);
				SetReg(0, OPERAND_A);
				m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
			}

			if (i != times - 1)
			{
#ifdef _DEBUG
				*(dbg_args_ptr = dbg_args) = 0;
#endif
			}
			if (REP)
			{
				m_registers[CX]--;
			}
		}

		break;

	case 0xC3:
		CMD_NAME("RET");
		IP = Pop();
		break;

	case 0xCB:
		CMD_NAME("RET");
		IP = Pop();
		m_segments[CS] = Pop();
		break;

	case 0xC2:
		CMD_NAME("RET");
		{
			int countToPop = GetIMM16();
			IP = Pop();
			m_registers[SP] += countToPop;
		}
		break;

	case 0xCA:
		CMD_NAME("RET");
		{
			int countToPop = GetIMM16();
			IP = Pop();
			m_segments[CS] = Pop();
			m_registers[SP] += countToPop;
		}
		break;

	case 0xCF:
		CMD_NAME("IRET");
		IP = Pop();
		m_segments[CS] = Pop();
		m_flags = Pop();
		break;

	case 0xC5:
		ADDRESS_METHOD = r16;
		CMD_NAME("LDS");
		MODREGRM = ExtractByte();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = MemoryWord(ADDRESS);
		OPERAND_B = MemoryWord(ADDRESS + 2);
		SetReg(OPERAND_A);
		SetSegment(DS, OPERAND_B);
		break;

	case 0xC4:
		ADDRESS_METHOD = r16;
		CMD_NAME("LES");
		MODREGRM = ExtractByte();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = MemoryWord(ADDRESS);
		OPERAND_B = MemoryWord(ADDRESS + 2);
		SetReg(OPERAND_A);
		SetSegment(ES, OPERAND_B);
		break;

	case 0x8D:
		ADDRESS_METHOD = r16;
		CMD_NAME("LEA");
		MODREGRM = ExtractByte();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		SetReg(EFFECTIVE_ADDRESS);
		break;

	case 0xA4: case 0xA5:
		if (REP)
		{
			times = m_registers[CX];
		}
		for (int i = 0; i < times; ++i)
		{
			CMD_NAME("MOVS");
			byte src_offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				src_offsetreg = m_segmentOverride;
			}
			word src_seg = GetSegment(src_offsetreg);
			APPEND_DBG(":");
			APPEND_DBG_REGW(SI);
			word src_offset = GetRegW(SI);
			APPEND_DBG(", [");
			APPEND_DBG_SEG(ES);
			word dst_seg = GetSegment(ES);
			APPEND_DBG(":");
			APPEND_DBG_REGW(DI);
			APPEND_DBG("]");
			word dst_offset = GetRegW(DI);
			if (OPCODE1 & 1)
			{
				MemoryWord(select(dst_seg, dst_offset)) = MemoryWord(select(src_seg, src_offset));
				m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}
			else
			{
				MemoryByte(select(dst_seg, dst_offset)) = MemoryByte(select(src_seg, src_offset));
				m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			if (i != times - 1)
			{
	#ifdef _DEBUG
				*(dbg_args_ptr = dbg_args) = 0;
	#endif
			}
			if (REP)
			{
				m_registers[CX]--;
			}
		}
		break;

		case 0xA0: case 0xA1:
		{
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("MOV");
			APPEND_DBG(OPCODE1 & 1 ? "AX" : "AL");
			APPEND_DBG(", [");

			byte src_offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				src_offsetreg = m_segmentOverride;
			}
			word src_seg = GetSegment(src_offsetreg);
			word src_offset = GetIMM16();
			APPEND_DBG_SEG(src_offsetreg);
			APPEND_DBG(":");
			APPEND_HEX_DBG(src_offset);
			APPEND_DBG("]");
			if (OPCODE1 & 1)
			{
				OPERAND_A = MemoryWord(select(src_seg, src_offset));
				SetReg(AX, OPERAND_A);
			}
			else
			{
				OPERAND_A = MemoryByte(select(src_seg, src_offset));
				SetReg(AL, OPERAND_A);
			}
		}
		break;

		case 0xA2: case 0xA3:
		{
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("MOV");
			APPEND_DBG("[");
			byte dst_offsetreg = DS;
			if (m_segmentOverride != NONE)
			{
				dst_offsetreg = m_segmentOverride;
			}
			word dst_seg = GetSegment(dst_offsetreg);
			word dst_offset = GetIMM16();
			APPEND_DBG_SEG(dst_offsetreg);
			APPEND_DBG(":");
			APPEND_HEX_DBG(dst_offset);
			APPEND_DBG("], ");

			APPEND_DBG(OPCODE1 & 1 ? "AX" : "AL");

			if (OPCODE1 & 1)
			{
				MemoryWord(select(dst_seg, dst_offset)) = GetReg(AX);
			}
			else
			{
				MemoryByte(select(dst_seg, dst_offset)) = GetReg(AL) & 0xFF;
			}
		}
		break;

		case 0x86: case 0x87:
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("XCHG");
			MODREGRM = ExtractByte();
			OPERAND_A = GetReg();
			APPEND_DBG(", ");
			OPERAND_B = GetRM();
			SetReg(OPERAND_B);
			SetRM(OPERAND_A);
		break;

		case 0x84: case 0x85:
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("TEST");
			MODREGRM = ExtractByte();
			OPERAND_A = GetReg();
			APPEND_DBG(", ");
			OPERAND_B = GetRM();
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			break;

		case 0xA6: case 0xA7:
			if (REP || REPN)
			{
				times = m_registers[CX];
			}
			for (int i = 0; i < times; ++i)
			{
				CMD_NAME("CMPS");
				byte src_offsetreg = DS;
				if (m_segmentOverride != NONE)
				{
					src_offsetreg = m_segmentOverride;
				}
				word src_seg = GetSegment(src_offsetreg);
				APPEND_DBG(":");
				APPEND_DBG_REGW(SI);
				word src_offset = GetRegW(SI);
				APPEND_DBG(", ");
				APPEND_DBG_SEG(ES);
				word dst_seg = GetSegment(ES);
				APPEND_DBG(":");
				APPEND_DBG_REGW(DI);
				word dst_offset = GetRegW(DI);
				if (OPCODE1 & 1)
				{
					OPERAND_A = MemoryWord(select(src_seg, src_offset));
					OPERAND_B = MemoryWord(select(dst_seg, dst_offset));
					OPERAND_B = -OPERAND_B;
					RESULT = OPERAND_A + OPERAND_B;
					UpdateFlags_CFOFAF();
					UpdateFlags_SFZFPF();
					m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
					m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
				}
				else
				{
					OPERAND_A = MemoryByte(select(src_seg, src_offset));
					OPERAND_B = MemoryByte(select(dst_seg, dst_offset));
					OPERAND_B = -OPERAND_B;
					RESULT = OPERAND_A + OPERAND_B;
					UpdateFlags_CFOFAF();
					UpdateFlags_SFZFPF();
					m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
					m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
				}
				if (i != times - 1)
				{
#ifdef _DEBUG
					*(dbg_args_ptr = dbg_args) = 0;
#endif
				}
				if (REP || REPN)
				{
					m_registers[CX]--;
				}
				if (REP && !TestFlag<ZF>())
				{
					break;
				}
				if (REPN && TestFlag<ZF>())
				{
					break;
				}
			}
			break;

		case 0xA8: case 0xA9:
			CMD_NAME("TEST");
			ADDRESS_METHOD = OPCODE1 & 1;
			APPEND_DBG_REG(0);
			OPERAND_A = GetReg(0);
			APPEND_DBG(", 0x");
			if (Byte())
			{
				byte data = GetIMM8();
				APPEND_HEX_DBG(data);
				OPERAND_B = data;
			}
			else
			{
				word data = GetIMM16();
				APPEND_HEX_DBG(data);
				OPERAND_B = data;
			}
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			break;

		case 0x9E:
			CMD_NAME("SAHF");
			m_flags = (m_flags & 0xFF00) | GetRegB(AH);
			break;

		case 0x9F:
			CMD_NAME("LAHF");
			SetRegB(AH, m_flags & 0xFF);
			break;

		case 0xD5:
			CMD_NAME("AAD");
			{
				int tempL = GetRegB(AL);
				int tempH = GetRegB(AH);
				SetRegB(AL, tempL + (tempH * GetIMM8()));
				SetRegB(AH, 0);
				RESULT = GetRegB(AL);
				UpdateFlags_SFZFPF();
				break;
			}

		case 0xD4:
			CMD_NAME("AAM");
			{
				unsigned int tempL = GetRegB(AL);
				unsigned int imm8 = GetIMM8();
				SetRegB(AH, tempL / imm8);
				SetRegB(AL, tempL % imm8);
				RESULT = GetRegB(AL);
				UpdateFlags_SFZFPF();
				break;
			}

		case 0x37:
			CMD_NAME("AAA");
			{
				int tempL = GetRegB(AL);
				int tempH = GetRegB(AH);
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegB(AL, tempL + 6);
					SetRegB(AH, tempH + 1);
					SetFlag<AF>();
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<AF>();
					ClearFlag<CF>();
				}
				tempL = GetRegB(AL);
				SetRegB(AL, tempL & 0xF);
			}
			break;

		case 0x3F:
			CMD_NAME("AAS");
			{
				int tempL = GetRegB(AL);
				int tempH = GetRegB(AH);
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegB(AL, tempL - 6);
					SetRegB(AH, tempH - 1);
					SetFlag<AF>();
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<AF>();
					ClearFlag<CF>();
				}
				tempL = GetRegB(AL);
				SetRegB(AL, tempL & 0xF);
			}
			break;

		case 0x27:
			CMD_NAME("DAA");
			{
				int tempL = GetRegB(AL);
				int tempH = GetRegB(AH);
				bool oldCF = TestFlag<CF>();
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegB(AL, tempL + 6);
					SetFlag<AF>();
					SetFlag<CF>(TestFlag<CF>() || (0x100 & (tempL + 6)));
				}
				else
				{
					ClearFlag<AF>();
				}
				if (tempL > 0x99 || oldCF)
				{
					SetRegB(AL, GetRegB(AL) + 0x60);
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<CF>();
				}
				RESULT = GetRegB(AL);
				UpdateFlags_SFZFPF();
			}
			break;

		case 0x2F:
			CMD_NAME("DAS");
			{
				int tempL = GetRegB(AL);
				int tempH = GetRegB(AH);
				bool oldCF = TestFlag<CF>();
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegB(AL, tempL - 6);
					SetFlag<AF>();
					SetFlag<CF>(TestFlag<CF>() || (0x100 & (tempL - 6)));
				}
				else
				{
					ClearFlag<AF>();
				}
				if (tempL > 0x99 || oldCF)
				{
					SetRegB(AL, GetRegB(AL) - 0x60);
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<CF>();
				}
				RESULT = GetRegB(AL);
				UpdateFlags_SFZFPF();
			}
			break;

		case 0xF4:
			CMD_NAME("HLT");
			ASSERT(false, "HALT!");
			break;

		case 0xD9:
			CMD_NAME("F2XM1");
			GetIMM8();
			ASSERT(false, "No FPU");
			break;

		case 0x9B:
			CMD_NAME("WAIT");
			ASSERT(false, "No WAIT implementation");
			break;

		case 0xD7:
			CMD_NAME("XLAT");
			{
				byte offsetreg = DS; 
				if (m_segmentOverride != NONE)
				{
					offsetreg = m_segmentOverride;
				}
				word seg = GetSegment(offsetreg);
				SetRegB(AL, MemoryByte(select(seg, GetRegW(BX) + uint16_t(GetRegB(AL)))));
			}
			break;


	default:
		UNKNOWN_OPCODE(OPCODE1);
	}

#ifdef _DEBUG
	if (logging_state)
	{
		m_log.appendf("%s %s %s\n", dbg_cmd_address, dbg_cmd_name, dbg_args);
		printf("%s %s %s\n", dbg_cmd_address, dbg_cmd_name, dbg_args);
		m_scrollToBottom = true;
	}

	for (int i = 0; i < 8; ++i)
	{
		n2hexstr(buff, m_registers[i]); fprintf(logfile, buff);
	}
	for (int i = 0; i < 4; ++i)
	{
		n2hexstr(buff, m_segments[i]); fprintf(logfile, buff);
	}
	fprintf(logfile, "\n");
#endif
}

void CPU::Interrupt(int n)
{
	Push(m_flags);
	Push(m_segments[CS]);
	Push(IP);
	IP = MemoryWord(n * 4);
	m_segments[CS] = MemoryWord(n * 4 + 2);
}

void CPU::EnableA20Gate()
{
	A20 = 0xFFFFFF;
}

void CPU::DisableA20Gate()
{
	A20 = 0x0FFFFF;
}

bool CPU::GetA20GateStatus()
{
	return A20 == 0xFFFFFF;
}

void CPU::GUI()
{
	char buff[128];
	ImGui::Begin("CPU Status");

	ImGui::Text("%s %s %s", dbg_cmd_address, dbg_cmd_name, dbg_args);
	ImGui::NewLine();
	for (int i = 0; i < 8; ++i)
	{
		ImVec4 color = m_registers[i] == m_old_registers[i] ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		n2hexstr(buff, m_registers[i]);
		ImGui::TextColored(color, "%s=%s", m_regNames[i + 8], buff);
		ImGui::SameLine();
	}
	ImGui::NewLine();

	for (int i = 0; i < 4; ++i)
	{
		ImVec4 color = m_segments[i] == m_old_segments[i] ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		n2hexstr(buff, m_segments[i]);
		ImGui::TextColored(color, "%s=%s", m_segNames[i], buff);
		ImGui::SameLine();
	}
	ImGui::NewLine();

	n2hexstr(buff, IP);
	ImGui::Text("IP=%s", buff);

	for (int i = 0; i < 16; ++i)
	{
		if (*m_flagNames[i] != 0)
		{
			ImVec4 color = (m_flags & (1 << i)) == (m_old_flags & (1 << i)) ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			bool v = (m_flags & (1 << i)) != 0;
			ImGui::Checkbox(m_flagNames[i], &v);
			ImGui::SameLine();
			m_flags = (m_flags & ~(1 << i)) | (v ? (1 << i) : 0);
		}
	}

	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("log");
	if (ImGui::Checkbox("Do logging", &logging_state)) {
		m_log.clear();
	};
	ImGui::SameLine();
	if (ImGui::Button("Close")) {
		fclose(logfile);
	};
	if (ImGui::Button("Clear")) {
		m_log.clear();
	};
	ImGui::SameLine();
	bool copy = ImGui::Button("Copy");
	ImGui::SameLine();
	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (copy) ImGui::LogToClipboard();

	ImGui::TextUnformatted(m_log.begin());

	if (m_scrollToBottom)
		ImGui::SetScrollHere(1.0f);
	m_scrollToBottom = false;
	ImGui::EndChild();
	ImGui::End();

	mem_edit.DrawWindow("Memory Editor", m_ram, 0x100000);
}

const uint8_t CPU::parity[0x100] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};
