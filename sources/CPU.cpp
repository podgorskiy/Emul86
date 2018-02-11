#include "CPU.h"
#include "_assert.h"
#include <imgui.h>
#include <stdio.h>
#include "imgui_memory_editor.h"


static MemoryEditor mem_edit;
static bool logging_state = false;

FILE* logfile;

#ifdef _DEBUG
#define APPEND_DBG(X) \
dbg_args_ptr = strcpy(dbg_args_ptr, X) + strlen(X);
#else
#define APPEND_DBG(X)
#endif

#ifdef _DEBUG
#define APPEND_HEX_DBG(X) \
dbg_args_ptr = n2hexstr(dbg_args_ptr, X)
#else
#define APPEND_HEX_DBG(X)
#endif

#ifdef _DEBUG
#define APPEND_HEXN_DBG(X,N) \
dbg_args_ptr = n2hexstr(dbg_args_ptr, X, N)
#else
#define APPEND_HEXN_DBG(X, N)
#endif

#ifdef _DEBUG
#define CMD_NAME(X) strcpy(dbg_cmd_name, X);
#else
#define CMD_NAME(X)
#endif

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

#define CJMP \
{\
	sbyte rel8 = GetImm<byte>(); \
	APPEND_HEX_DBG(m_segments[CS]);\
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(word(IP + rel8)); \
	if (condition) \
	{ \
		IP += rel8; \
	} break;\
}
#define CJMP16 \
{\
	int16_t rel16 = GetImm<word>(); \
	APPEND_HEX_DBG(m_segments[CS]);\
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(word(IP + rel16)); \
	if (condition) \
	{ \
		IP += rel16; \
	} break;\
}

char* print_address(char* dst, word segment, word offset)
{
	char* tmp = n2hexstr(dst, segment);
	*tmp++ = ':';
	return &(*n2hexstr(tmp, offset) = '\x0');	
}


CPU::CPU(IO& io) : m_io(io)
{
	Reset();
#ifdef _DEBUG
	logfile = fopen("log.txt", "w");
#endif
	mem_edit.GotoAddr = 0x7C00;
	m_scrollToBottom = true;
}


template<typename T>
inline T CPU::GetImm()
{
	uint32_t address = select(m_segments[CS], IP);
	IP += sizeof(T);
	return m_io.Memory<T>(address);
}


void CPU::SetInterruptHandler(InterruptHandler* interruptHandler)
{
	m_interruptHandler = interruptHandler;
}


void CPU::INT(byte x)
{
	m_interruptHandler->Int(x);
}


inline word CPU::GetReg(byte i)
{
	if (Byte())
	{
		return GetRegister<byte>(i);
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
		SetRegister<byte>(i, x & 0xFF);
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
		MODREGRM = GetImm<byte>();
		OPERAND_A = GetRM();
		APPEND_DBG(", ");
		OPERAND_B = GetReg();
		break;
	case REG_RM:
		MODREGRM = GetImm<byte>();
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
			byte data = GetImm<byte>();
			OPERAND_B = data;
			APPEND_HEX_DBG(data);
		}
		else
		{
			word imm16 = GetImm<word>();
			OPERAND_B = imm16;
			APPEND_HEX_DBG(imm16);
		}
		break;
	case RM_IMM:
		MODREGRM = GetImm<byte>();
		OPERAND_A = GetRM();
		APPEND_DBG(", 0x");
		if (Byte())
		{
			byte data = GetImm<byte>();
			OPERAND_B = data;
			APPEND_HEX_DBG(data);
		}
		else
		{
			if (signextend)
			{
				sbyte data = GetImm<byte>();
				int16_t data16 = data;
				OPERAND_B = (word)data16;
				APPEND_HEX_DBG((word)data16);
			}
			else
			{
				OPERAND_B = GetImm<word>();
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


inline word CPU::GetReg()
{
	byte r = (MODREGRM & REG) >> 3;
	APPEND_DBG_REG(r);
	return GetReg(r);
}


inline void CPU::SetReg(word x)
{
	SetReg((MODREGRM & REG) >> 3, x);
}


inline word CPU::GetRM()
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
			return m_io.Memory<byte>(ADDRESS);
		}
		else
		{
			return m_io.Memory<word>(ADDRESS);
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
			m_io.Memory<byte>(ADDRESS) = (byte)x;
		}
		else
		{
			m_io.Memory<word>(ADDRESS) = x;
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
			reg = GetImm<word>(); 
			APPEND_HEXN_DBG(reg, 4); 
		} 
		break;
	case 7: reg = m_registers[BX]; APPEND_DBG("BX") break;
	}
	if ((MODREGRM & MOD) == 0x40)
	{
		sbyte disp8 = GetImm<byte>();
		APPEND_DBG(" + 0x");
		APPEND_HEX_DBG(disp8);
		disp = disp8; // sign extension
	}
	else if ((MODREGRM & MOD) == 0x80)
	{
		int16_t disp16 = GetImm<word>();
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
	ADDRESS = select(m_segments[offsetreg], reg + disp);
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
	uint32_t mask = (0x80 + (ADDRESS_METHOD & rMask) * (0x8000 - 0x80));
	SetFlag<OF>((RESULT ^ OPERAND_A) & (RESULT ^ OPERAND_B) & mask);
}


inline void CPU::UpdateFlags_OF_sub()
{
	uint32_t mask = (0x80 + (ADDRESS_METHOD & rMask) * (0x8000 - 0x80));
	SetFlag<OF>((RESULT ^ OPERAND_A) & (OPERAND_A ^ OPERAND_B) & mask);
}


inline void CPU::UpdateFlags_AF()
{
	SetFlag<AF>((RESULT ^ OPERAND_A ^ OPERAND_B) & 0x10);
}


inline void CPU::Flip_AF()
{
	SetFlag<AF>(!TestFlag<AF>());
}


inline void CPU::UpdateFlags_PF()
{
	byte v = RESULT & 0xFF;
	SetFlag<PF>(bool((0x9669 >> ((v ^ (v >> 4)) & 0xF)) & 1));
}


inline void CPU::UpdateFlags_ZF()
{
	SetFlag<ZF>((RESULT & (0xFF + (ADDRESS_METHOD & rMask) * 0xFF00)) == 0);
}


inline void CPU::UpdateFlags_SF()
{
	SetFlag<SF>((RESULT & (0x80 + (ADDRESS_METHOD & rMask) * (0x8000 - 0x80))) != 0);
}


inline void CPU::UpdateFlags_CFOFAF()
{
	UpdateFlags_CF();
	UpdateFlags_OF();
	UpdateFlags_AF();
}


inline void CPU::UpdateFlags_CFOFAF_sub()
{
	UpdateFlags_CF();
	UpdateFlags_OF_sub();
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


inline void CPU::UpdateFlags_OFSFZFAFPF_sub()
{
	UpdateFlags_OF_sub();
	UpdateFlags_SF();
	UpdateFlags_ZF();
	UpdateFlags_AF();
	UpdateFlags_PF();
}

inline void CPU::Push(word x)
{
	m_registers[SP] -= 2;
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	m_io.Memory<word>(address) = x;
}
inline word CPU::Pop()
{
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	word x = m_io.Memory<word>(address);
	m_registers[SP] += 2;
	return x;
}


void CPU::Reset()
{
	memset(m_registers, 0, sizeof(word) * 8);
	memset(m_segments, 0, sizeof(word) * 4);
	m_registers[SP] = 0xFFD6;
	m_registers[AX] = 0xAA55;
	m_registers[DI] = 0xFFAC;
	IP = 0;
	m_segments[CS] = 0x10;
	m_flags = 2;
	dbg_cmd_address[0] = 0;
	dbg_cmd_name[0] = 0;
	dbg_args[0] = 0;
	dbg_args_ptr = dbg_args;
	dbg_buff[0] = 0;
}


const char* CPU::GetDebugString()
{
	char* tmp = dbg_buff;
	tmp = n2hexstr(tmp, m_segments[CS]);
	tmp = n2hexstr(tmp, (word)(IP));
	for (int i = 0; i < 8; ++i)
	{
		tmp = n2hexstr(tmp, m_registers[i]);
	}
	for (int i = 0; i < 4; ++i)
	{
		tmp = n2hexstr(tmp, m_segments[i]);
	}
	n2hexstr(tmp, m_flags);
	return dbg_buff;
}


const char* CPU::GetLastCommandAsm()
{
	sprintf(dbg_buff, "%s %s %s", dbg_cmd_address, dbg_cmd_name, dbg_args);
	return dbg_buff;
}


void CPU::Step()
{
#ifdef _DEBUG
	dbg_args_ptr = dbg_args;
	dbg_args[0] = 0;
	
	memcpy(m_old_registers, m_registers, sizeof(m_registers));
	memcpy(m_old_segments, m_segments, sizeof(m_segments));
	memcpy(&m_old_flags, &m_flags, sizeof(m_flags));
#endif

	bool LOCK = false;
	bool REPN = false;
	bool REP = false;

#ifdef _DEBUG
	if (logging_state)
	{
		fprintf(logfile, GetDebugString());
		fprintf(logfile, "\n");
		fflush(logfile);
	}
#endif

	m_segmentOverride = NONE;
	int times = 1;
	bool sizeOverride = false;
	bool addressOverride = false;
	// Read Instruction Prefixes. Up to four prefixes of 1 - byte each (optional
	while (true)
	{
		byte data;
		data = GetImm<byte>();
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
			goto opcode;
		}
	}
	opcode:

#ifdef _DEBUG
	print_address(dbg_cmd_address, m_segments[CS], (word)(IP-1));
#endif

#ifdef _DEBUG
	mem_edit.HighlightMin = (((uint32_t)m_segments[CS]) << 4) + IP - 1;
	mem_edit.HighlightMax = mem_edit.HighlightMin + 1;
#endif

	byte seg = 0;
	byte data = 0;
	byte reg = 0;
	bool condition = false;
	word MSB_MASK = 0;
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
		RESULT = OPERAND_A + OPERAND_B + (uint32_t)TestFlag<CF>();
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
		ClearFlag<AF>();
		break;
	case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:
		CMD_NAME("XOR");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A ^ OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		ClearFlag<AF>();
		break;
	case 0x08:case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:
		CMD_NAME("OR");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A | OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		ClearFlag<AF>();
		break;
	case 0x18:case 0x19:case 0x1A:case 0x1B:case 0x1C:case 0x1D:
		CMD_NAME("SBB");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A - OPERAND_B - (uint32_t)TestFlag<CF>();
		UpdateFlags_CFOFAF_sub();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x28:case 0x29:case 0x2A:case 0x2B:case 0x2C:case 0x2D:
		CMD_NAME("SUB");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_CFOFAF_sub();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x38:case 0x39:case 0x3A:case 0x3B:case 0x3C:case 0x3D:
		CMD_NAME("CMP");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_CFOFAF_sub();
		UpdateFlags_SFZFPF();
		break;

	case 0x80:case 0x81:case 0x82:case 0x83:
		ADDRESS_METHOD = RM_IMM | OPCODE1 & 1;
		PrepareOperands(OPCODE1 == 0x83); // sign extend opcodes starting with 0x83
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
			ClearFlag<AF>();
			break;
		case 0x02:
			CMD_NAME("ADC");
			RESULT = OPERAND_A + OPERAND_B + (uint32_t)TestFlag<CF>();
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x03:
			CMD_NAME("SBB");
			RESULT = OPERAND_A - OPERAND_B - (uint32_t)TestFlag<CF>();
			UpdateFlags_CFOFAF_sub();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x04:
			CMD_NAME("AND");
			RESULT = OPERAND_A & OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			ClearFlag<AF>();
			break;
		case 0x05:
			CMD_NAME("SUB");
			RESULT = OPERAND_A - OPERAND_B;
			UpdateFlags_CFOFAF_sub();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x06:
			CMD_NAME("XOR");
			RESULT = OPERAND_A ^ OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			ClearFlag<AF>();
			break;
		case 0x07:
			CMD_NAME("CMP");
			RESULT = OPERAND_A - OPERAND_B;
			UpdateFlags_CFOFAF_sub();
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
		MODREGRM = GetImm<byte>();
		seg = (MODREGRM & REG) >> 3;
		SetRM(m_segments[seg], true);
		APPEND_DBG(", ");
		APPEND_DBG (m_segNames[seg]);
		break;

	case 0x8E:
		CMD_NAME("MOV");
		ADDRESS_METHOD = r16;
		MODREGRM = GetImm<byte>();
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
			byte data = GetImm<byte>();
			SetReg(reg, data);
			APPEND_HEX_DBG(data);
		}
		else
		{
			APPEND_DBG(m_regNames[reg + 8]);
			APPEND_DBG(", 0x");
			word data = GetImm<word>();
			SetReg(reg, data);
			APPEND_HEX_DBG(data);
		}
		break;

	case 0xC6:case 0xC7:
		CMD_NAME("MOV");
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = GetImm<byte>();
		SetRM_Address();
		if (Byte())
		{
			byte data = GetImm<byte>();
			SetRM(data);
			APPEND_DBG(", 0x");
			APPEND_HEX_DBG(data);
		}
		else
		{
			word data = GetImm<word>();
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
		data = GetImm<byte>();
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
			sbyte rel8 = GetImm<byte>();
			if (m_registers[CX] != 0)
			{
				IP += rel8;
			}}
		break;

	case 0xE1:
		CMD_NAME("LOOPZ");
		--m_registers[CX];
		{
			sbyte rel8 = GetImm<byte>();
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
			sbyte rel8 = GetImm<byte>();
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
			word new_IP = GetImm<word>();
			word new_CS = GetImm<word>();
			IP = new_IP;
			m_segments[CS] = new_CS;
			APPEND_HEX_DBG(new_CS);
			APPEND_DBG(":");
			APPEND_HEX_DBG(new_IP);
		}
		break;
	case 0xEB:
		CMD_NAME("JMP");
		IP += (sbyte)GetImm<byte>();
		APPEND_HEX_DBG(m_segments[CS]);
		APPEND_DBG(":");
		APPEND_HEX_DBG(IP);
		break;
	case 0xE9:
		CMD_NAME("JMP");
		IP += GetImm<word>();
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
			int16_t offset = GetImm<word>();
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
			word new_IP = GetImm<word>();
			word new_CS = GetImm<word>();
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
		ADDRESS = GetImm<byte>();
		if (Byte())
		{
			SetReg(0, m_io.Port<byte>(ADDRESS));
		}
		else
		{
			SetReg(0, m_io.Port<word>(ADDRESS));
		}
		break;

	case 0xE6: case 0xE7:
		CMD_NAME("OUT");
		ADDRESS_METHOD = OPCODE1 & 1;
		ADDRESS = GetImm<byte>();
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			m_io.Port<byte>(ADDRESS) = GetRegister<byte>(AL);
		}
		else
		{
			APPEND_DBG_REGW(0);
			m_io.Port<word>(ADDRESS) = GetRegister<word>(AX);
		}
		break;

	case 0xEC: case 0xED:
		CMD_NAME("IN");
		ADDRESS_METHOD = OPCODE1 & 1;
		APPEND_DBG_REGW(DX);
		ADDRESS = GetRegister<word>(DX);
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			SetRegister<byte>(0, m_io.Port<byte>(ADDRESS));
		}
		else
		{
			APPEND_DBG_REGW(0);
			SetRegister<word>(0, m_io.Port<word>(ADDRESS));
		}
		break;

	case 0xEE: case 0xEF:
		CMD_NAME("OUT");
		ADDRESS_METHOD = OPCODE1 & 1;
		ADDRESS = GetRegister<word>(DX);
		APPEND_DBG_REGW(DX);
		if (Byte())
		{
			APPEND_DBG_REGB(0);
			m_io.Port<byte>(ADDRESS) = GetRegister<byte>(0);
		}
		else
		{
			APPEND_DBG_REGW(0);
			m_io.Port<word>(ADDRESS) = GetRegister<word>(0);
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
			m_io.Port<byte>(GetRegister<word>(DX)) = m_io.Memory<byte>(select(GetSegment(offsetreg), GetRegister<word>(SI)));

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
			m_io.Port<word>(GetRegister<word>(DX)) = m_io.Memory<word>(select(GetSegment(offsetreg), GetRegister<word>(SI)));

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
			m_io.Memory<byte>(select(GetSegment(ES), GetRegister<word>(DI))) = m_io.Port<byte>(GetRegister<word>(DX));
			
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
			m_io.Memory<word>(select(GetSegment(ES), GetRegister<word>(DI))) = m_io.Port<word>(GetRegister<word>(DX));
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
		MODREGRM = GetImm<byte>();
		OPERAND_A = GetRM();
		OPERAND_B = m_io.Memory<word>(ADDRESS + 2);

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
			OPERAND_B = 1;
			RESULT = OPERAND_A - OPERAND_B;
			UpdateFlags_OFSFZFAFPF_sub();
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
		OPERAND_A = (sbyte)GetRegister<byte>(0);
		SetRegister<word>(0, OPERAND_A);
		break;

	case 0x99:
		CMD_NAME("CWD");
		OPERAND_A = GetRegister<word>(0);
		SetRegister<word>(DX, ((OPERAND_A & 0x8000) != 0) * 0xFFFF);
		break;

	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		CMD_NAME("INC");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		CMD_NAME("DEC");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_OFSFZFAFPF_sub();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		CMD_NAME("PUSH");
		ADDRESS_METHOD = r16;
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		Push(OPERAND_A);
		break;

	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		CMD_NAME("POP");
		{
			int reg = OPCODE1 & 0x07;
			ADDRESS_METHOD = r16;
			OPERAND_A = Pop();
			APPEND_DBG_REGW(reg);
			SetRegister<word>(reg, OPERAND_A);
		}
		break;

	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		CMD_NAME("XCHG");
		ADDRESS_METHOD = r16;
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		APPEND_DBG(", ");
		OPERAND_B = GetRegister<word>(0);
		APPEND_DBG_REGW(0);
		SetRegister<word>(OPCODE1 & 0x07, OPERAND_B);
		SetRegister<word>(0, OPERAND_A);
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
		m_flags = Pop() | 0x2;
		break;

	case 0x0F:
		OPCODE2 = GetImm<byte>();
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

		case 0xAC:
			CMD_NAME("SHRD");
			ADDRESS_METHOD = r16;
			MODREGRM = GetImm<byte>();
			OPCODE2 = (MODREGRM & REG) >> 3;

			times = GetImm<byte>();
			OPERAND_A = GetRM() | GetRegister<word>((MODREGRM & REG) >> 3) * 0x10000;
			RESULT = OPERAND_A;

			for (int i = 0; i < times; ++i)
			{
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2;
				SetFlag<OF>((MSB_MASK & OPERAND_A) != 0);
				UpdateFlags_SFZFPF();
				ClearFlag<AF>();
			}
			SetRM(RESULT);
			break;

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
		OPERAND_A = (sbyte)GetImm<byte>(); // sign extend
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x68:
		CMD_NAME("PUSH");
		OPERAND_A = GetImm<word>();
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x60:
		{
			CMD_NAME("PUSHA");
			word originalSP = m_registers[SP];
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
			word originalSP = Pop();
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
		MODREGRM = GetImm<byte>();
		SetRM(Pop(), true);
		break;

	case 0xF6: case 0xF7:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = GetImm<byte>();
		OPCODE2 = (MODREGRM & REG) >> 3;

		switch (OPCODE2)
		{
		case 0x00:
			CMD_NAME("TEST");
			OPERAND_A = GetRM();
			APPEND_DBG(", ");
			if (Byte())
			{
				OPERAND_B = GetImm<byte>();
			}
			else
			{
				OPERAND_B = GetImm<word>();
			}
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			ClearFlag<AF>();
			break;
		case 0x04:
			CMD_NAME("MUL");
			APPEND_DBG_REG(0);
			OPERAND_A = (word)GetReg(0);
			APPEND_DBG(", ");
			OPERAND_B = (word)GetRM();
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
			UpdateFlags_ZF();
			ClearFlag<AF>();
			UpdateFlags_PF();
			UpdateFlags_SF();
			break;
		case 0x05:
			CMD_NAME("IMUL");
			if (Byte())
			{
				APPEND_DBG_REGB(0);
				OPERAND_A = (sbyte)GetRegister<byte>(0);
				APPEND_DBG(", ");
				OPERAND_B = (sbyte)GetRM();
				RESULT = OPERAND_A * OPERAND_B;
				m_registers[0] = RESULT;
				uint32_t of = RESULT & 0xFF80;
				SetFlag<OF>(of != 0 && of != 0xFF80);
				SetFlag<CF>(of != 0 && of != 0xFF80);
			}
			else
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (int16_t)GetRegister<word>(0);
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();
				RESULT = OPERAND_A * OPERAND_B;
				m_registers[AX] = RESULT;
				m_registers[DX] = RESULT >> 16;
				uint32_t of = RESULT & 0xFFFF8000;
				SetFlag<OF>(of != 0 && of != 0xFFFF8000);
				SetFlag<CF>(of != 0 && of != 0xFFFF8000);
			}
			UpdateFlags_ZF();
			ClearFlag<AF>();
			UpdateFlags_PF();
			UpdateFlags_SF();
			break;

		case 0x06:
			CMD_NAME("DIV");
			if (Byte())
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (word)GetRegister<word>(AX);
				APPEND_DBG(", ");
				OPERAND_B = (word)GetRM();
				RESULT = OPERAND_A / OPERAND_B;
				SetRegister<byte>(AL, RESULT);
				SetRegister<byte>(AH, (OPERAND_A % OPERAND_B));
			}
			else
			{
				APPEND_DBG_REGW(DX);
				APPEND_DBG(":");
				APPEND_DBG_REGW(0);
				OPERAND_A = (uint32_t)GetRegister<word>(0) + ((uint32_t)GetRegister<word>(DX) << 16);
				APPEND_DBG(", ");
				OPERAND_B = (word)GetRM();
				RESULT = OPERAND_A / OPERAND_B;
				if (OPERAND_B == 0)
				{
					m_registers[AX] = 0;
					m_registers[DX] = 0;
				}
				else
				{
					m_registers[AX] = RESULT;
					m_registers[DX] = OPERAND_A % OPERAND_B;
				}
			}
			break;

		case 0x07:
			CMD_NAME("IDIV");
			if (Byte())
			{
				APPEND_DBG_REGW(0);
				OPERAND_A = (int16_t)GetRegister<word>(0);
				APPEND_DBG(", ");
				OPERAND_B = (sbyte)GetRM();
				if (OPERAND_B == 0)
				{
					m_registers[AX] = 0;
					m_registers[DX] = 0;
				}
				else
				{
					RESULT = OPERAND_A / OPERAND_B;
					SetRegister<byte>(AL, RESULT);
					SetRegister<byte>(AH, (OPERAND_A % OPERAND_B));
				}
			}
			else
			{
				APPEND_DBG_REGW(DX);
				APPEND_DBG(":");
				APPEND_DBG_REGW(0);
				OPERAND_A = (int32_t)(GetRegister<word>(0) + ((uint32_t)GetRegister<word>(DX) << 16));
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();				
				if (OPERAND_B == 0)
				{
					m_registers[0] = 0;
					m_registers[DX] = 0;
				}
				else
				{
					RESULT = OPERAND_A / OPERAND_B;
					m_registers[0] = RESULT;
					m_registers[DX] = OPERAND_A % OPERAND_B;
				}
			}
			break;

		case 0x03:
			CMD_NAME("NEG");
			if (Byte())
			{
				OPERAND_B = 0;
				OPERAND_A = (sbyte)GetRM();
				RESULT = -OPERAND_A;
				SetRM(RESULT);
				SetFlag<CF>(RESULT != 0);
				UpdateFlags_OFSFZFAFPF_sub();
			}
			else
			{
				OPERAND_B = 0;
				OPERAND_A = (int16_t)GetRM();
				RESULT = -OPERAND_A;
				SetRM(RESULT);
				SetFlag<CF>(RESULT != 0);
				UpdateFlags_OFSFZFAFPF_sub();
			}
			break;

		case 0x02:
			CMD_NAME("NOT");
			OPERAND_A = GetRM();
			RESULT = ~OPERAND_A;
			SetRM(RESULT);
			break;
		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;

	case 0x6B:
		ADDRESS_METHOD = r16;
		MODREGRM = GetImm<byte>();
		CMD_NAME("IMUL");
		GetReg();
		APPEND_DBG(", ");
		OPERAND_A = (int16_t)GetRM();
		APPEND_DBG(", ");
		OPERAND_B = (sbyte)GetImm<byte>(); // sign extend
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetReg(RESULT);
		UpdateFlags_ZF();
		ClearFlag<AF>();
		UpdateFlags_PF();
		UpdateFlags_SF();
		RESULT = RESULT & 0xFFFF8000;
		SetFlag<OF>(RESULT != 0 && RESULT != 0xFFFF8000);
		SetFlag<CF>(RESULT != 0 && RESULT != 0xFFFF8000);
		break;

	case 0x69:
		ADDRESS_METHOD = r16;
		MODREGRM = GetImm<byte>();
		CMD_NAME("IMUL");
		GetReg();
		APPEND_DBG(", ");
		OPERAND_A = (int16_t)GetRM();
		APPEND_DBG(", ");
		OPERAND_B = (int16_t)GetImm<word>();
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetReg(RESULT);
		UpdateFlags_ZF();
		ClearFlag<AF>();
		UpdateFlags_PF();
		UpdateFlags_SF();
		RESULT = RESULT & 0xFFFF8000;
		SetFlag<OF>(RESULT != 0 && RESULT != 0xFFFF8000);
		SetFlag<CF>(RESULT != 0 && RESULT != 0xFFFF8000);
		break;

	case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = GetImm<byte>();
		OPCODE2 = (MODREGRM & REG) >> 3;

		if (OPCODE1 & 0x02)
		{
			times = GetRegister<byte>(CL) & 0x1F;
		}
		if (Byte())
		{
			times &= 0x7;
		}
		else
		{
			times &= 0x1f;
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
				ClearFlag<AF>();
				break;
			case 0x05:
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2;
				SetFlag<OF>((MSB_MASK & OPERAND_A) != 0);
				UpdateFlags_SFZFPF();
				ClearFlag<AF>();
				break;
			case 0x06:
				SetFlag<CF>(MSB_MASK & RESULT);
				RESULT = RESULT * 2;
				SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
				UpdateFlags_SFZFPF();
				ClearFlag<AF>();
				break;
			case 0x07:
			{
				bool negative = (MSB_MASK & RESULT) != 0;
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2 + negative * MSB_MASK;
				ClearFlag<OF>();
				UpdateFlags_SFZFPF();
				ClearFlag<AF>();
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
		MODREGRM = GetImm<byte>();
		OPCODE2 = (MODREGRM & REG) >> 3;

		times = GetImm<byte>();

		if (Byte())
		{
			times &= 0x7;
		}
		else
		{
			times &= 0x1f;
		}

		MSB_MASK = (OPCODE1 & 1) ? 0x8000 : 0x80;

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
			ASSERT(m_segmentOverride == NONE, "Override not permited");
			ADDRESS = GetSegment(ES) << 4;
			ADDRESS += m_registers[DI];

			if (Byte())
			{
				m_io.Memory<byte>(ADDRESS) = OPERAND_A;
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				m_io.Memory<word>(ADDRESS) = OPERAND_A;
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
			ASSERT(m_segmentOverride == NONE, "Override not permited");
			ADDRESS = GetSegment(ES) << 4;
			ADDRESS += m_registers[DI];

			if (Byte())
			{
				OPERAND_B = m_io.Memory<byte>(ADDRESS);
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				OPERAND_B = m_io.Memory<word>(ADDRESS);
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}

			RESULT = OPERAND_A - OPERAND_B;
			UpdateFlags_CFOFAF_sub();
			UpdateFlags_SFZFPF();

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
				OPERAND_A = m_io.Memory<byte>(ADDRESS);
				SetReg(0, OPERAND_A);
				m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			else
			{
				OPERAND_A = m_io.Memory<word>(ADDRESS);
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
			int countToPop = GetImm<word>();
			IP = Pop();
			m_registers[SP] += countToPop;
		}
		break;

	case 0xCA:
		CMD_NAME("RET");
		{
			int countToPop = GetImm<word>();
			IP = Pop();
			m_segments[CS] = Pop();
			m_registers[SP] += countToPop;
		}
		break;

	case 0xCF:
		CMD_NAME("IRET");
		IP = Pop();
		m_segments[CS] = Pop();
		m_flags = Pop() | 0x2;
		break;

	case 0xC5:
		ADDRESS_METHOD = r16;
		CMD_NAME("LDS");
		MODREGRM = GetImm<byte>();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = m_io.Memory<word>(ADDRESS);
		OPERAND_B = m_io.Memory<word>(ADDRESS + 2);
		SetReg(OPERAND_A);
		SetSegment(DS, OPERAND_B);
		break;

	case 0xC4:
		ADDRESS_METHOD = r16;
		CMD_NAME("LES");
		MODREGRM = GetImm<byte>();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = m_io.Memory<word>(ADDRESS);
		OPERAND_B = m_io.Memory<word>(ADDRESS + 2);
		SetReg(OPERAND_A);
		SetSegment(ES, OPERAND_B);
		break;

	case 0x8D:
		ADDRESS_METHOD = r16;
		CMD_NAME("LEA");
		MODREGRM = GetImm<byte>();
		GetReg();
		APPEND_DBG(", ");
		SetRM_Address();
		SetReg(EFFECTIVE_ADDRESS);
		break;

	case 0xA4: case 0xA5:
		ADDRESS_METHOD = OPCODE1 & 1;
		if (REP || REPN)
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
			word src_offset = GetRegister<word>(SI);
			APPEND_DBG(", [");
			APPEND_DBG_SEG(ES);
			word dst_seg = GetSegment(ES);
			APPEND_DBG(":");
			APPEND_DBG_REGW(DI);
			APPEND_DBG("]");
			word dst_offset = GetRegister<word>(DI);
			if (OPCODE1 & 1)
			{
				word w = m_io.Memory<word>(select(src_seg, src_offset));
				m_io.Memory<word>(select(dst_seg, dst_offset)) = w;
				m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}
			else
			{
				byte b = m_io.Memory<byte>(select(src_seg, src_offset));
				m_io.Memory<byte>(select(dst_seg, dst_offset)) = b;
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
			word src_offset = GetImm<word>();
			APPEND_DBG_SEG(src_offsetreg);
			APPEND_DBG(":");
			APPEND_HEX_DBG(src_offset);
			APPEND_DBG("]");
			if (OPCODE1 & 1)
			{
				OPERAND_A = m_io.Memory<word>(select(src_seg, src_offset));
				SetReg(AX, OPERAND_A);
			}
			else
			{
				OPERAND_A = m_io.Memory<byte>(select(src_seg, src_offset));
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
			word dst_offset = GetImm<word>();
			APPEND_DBG_SEG(dst_offsetreg);
			APPEND_DBG(":");
			APPEND_HEX_DBG(dst_offset);
			APPEND_DBG("], ");

			APPEND_DBG(OPCODE1 & 1 ? "AX" : "AL");

			if (OPCODE1 & 1)
			{
				m_io.Memory<word>(select(dst_seg, dst_offset)) = GetReg(AX);
			}
			else
			{
				m_io.Memory<byte>(select(dst_seg, dst_offset)) = GetReg(AL) & 0xFF;
			}
		}
		break;

		case 0x86: case 0x87:
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("XCHG");
			MODREGRM = GetImm<byte>();
			OPERAND_A = GetReg();
			APPEND_DBG(", ");
			OPERAND_B = GetRM();
			SetReg(OPERAND_B);
			SetRM(OPERAND_A);
		break;

		case 0x84: case 0x85:
			ADDRESS_METHOD = OPCODE1 & 1;
			CMD_NAME("TEST");
			MODREGRM = GetImm<byte>();
			OPERAND_A = GetReg();
			APPEND_DBG(", ");
			OPERAND_B = GetRM();
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			ClearFlag<AF>();
			break;

		case 0xA6: case 0xA7:
			ADDRESS_METHOD = OPCODE1 & 1;
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
				word src_offset = GetRegister<word>(SI);
				APPEND_DBG(", ");
				APPEND_DBG_SEG(ES);
				word dst_seg = GetSegment(ES);
				APPEND_DBG(":");
				APPEND_DBG_REGW(DI);
				word dst_offset = GetRegister<word>(DI);
				if (Byte())
				{
					OPERAND_A = m_io.Memory<byte>(select(src_seg, src_offset));
					OPERAND_B = m_io.Memory<byte>(select(dst_seg, dst_offset));
					RESULT = OPERAND_A - OPERAND_B;
					UpdateFlags_CFOFAF_sub();
					UpdateFlags_SFZFPF();
					m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
					m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
				}
				else
				{
					OPERAND_A = m_io.Memory<word>(select(src_seg, src_offset));
					OPERAND_B = m_io.Memory<word>(select(dst_seg, dst_offset));
					RESULT = OPERAND_A - OPERAND_B;
					UpdateFlags_CFOFAF_sub();
					UpdateFlags_SFZFPF();
					m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
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
				byte data = GetImm<byte>();
				APPEND_HEX_DBG(data);
				OPERAND_B = data;
			}
			else
			{
				word data = GetImm<word>();
				APPEND_HEX_DBG(data);
				OPERAND_B = data;
			}
			RESULT = OPERAND_A & OPERAND_B;
			UpdateFlags_SFZFPF();
			SetFlag<CF>(false);
			SetFlag<OF>(false);
			ClearFlag<AF>();
			break;

		case 0x9E:
			CMD_NAME("SAHF");
			{
				byte mask = 0x80 | 0x40 | 0x10 | 0x04 | 0x01;
				m_flags = (m_flags & 0xFF00) | (GetRegister<byte>(AH) & mask) | 0x2;
			}
			break;

		case 0x9F:
			CMD_NAME("LAHF");
			SetRegister<byte>(AH, (m_flags & 0xFF) | 0x02);
			break;

		case 0xD5:
			CMD_NAME("AAD");
			{
				int tempL = GetRegister<byte>(AL);
				int tempH = GetRegister<byte>(AH);
				SetRegister<byte>(AL, tempL + (tempH * GetImm<byte>()));
				SetRegister<byte>(AH, 0);
				RESULT = GetRegister<byte>(AL);
				UpdateFlags_SFZFPF();
				break;
			}

		case 0xD4:
			CMD_NAME("AAM");
			{
				unsigned int tempL = GetRegister<byte>(AL);
				unsigned int imm8 = GetImm<byte>();
				SetRegister<byte>(AH, tempL / imm8);
				SetRegister<byte>(AL, tempL % imm8);
				RESULT = GetRegister<byte>(AL);
				UpdateFlags_SFZFPF();
				break;
			}

		case 0x37:
			CMD_NAME("AAA");
			{
				int tempL = GetRegister<byte>(AL);
				int tempH = GetRegister<byte>(AH);
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegister<byte>(AL, tempL + 6);
					SetRegister<byte>(AH, tempH + 1);
					SetFlag<AF>();
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<AF>();
					ClearFlag<CF>();
				}
				tempL = GetRegister<byte>(AL);
				SetRegister<byte>(AL, tempL & 0xF);
			}
			break;

		case 0x3F:
			CMD_NAME("AAS");
			{
				int tempL = GetRegister<byte>(AL);
				int tempH = GetRegister<byte>(AH);
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegister<byte>(AL, tempL - 6);
					SetRegister<byte>(AH, tempH - 1);
					SetFlag<AF>();
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<AF>();
					ClearFlag<CF>();
				}
				tempL = GetRegister<byte>(AL);
				SetRegister<byte>(AL, tempL & 0xF);
			}
			break;

		case 0x27:
			CMD_NAME("DAA");
			{
				int tempL = GetRegister<byte>(AL);
				int tempH = GetRegister<byte>(AH);
				bool oldCF = TestFlag<CF>();
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegister<byte>(AL, tempL + 6);
					SetFlag<AF>();
					SetFlag<CF>(TestFlag<CF>() || (0x100 & (tempL + 6)));
				}
				else
				{
					ClearFlag<AF>();
				}
				if (tempL > 0x99 || oldCF)
				{
					SetRegister<byte>(AL, GetRegister<byte>(AL) + 0x60);
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<CF>();
				}
				RESULT = GetRegister<byte>(AL);
				UpdateFlags_SFZFPF();
			}
			break;

		case 0x2F:
			CMD_NAME("DAS");
			{
				int tempL = GetRegister<byte>(AL);
				int tempH = GetRegister<byte>(AH);
				bool oldCF = TestFlag<CF>();
				if (((tempL & 0x0F) > 9) || TestFlag<AF>())
				{
					SetRegister<byte>(AL, tempL - 6);
					SetFlag<AF>();
					SetFlag<CF>(TestFlag<CF>() || (0x100 & (tempL - 6)));
				}
				else
				{
					ClearFlag<AF>();
				}
				if (tempL > 0x99 || oldCF)
				{
					SetRegister<byte>(AL, GetRegister<byte>(AL) - 0x60);
					SetFlag<CF>();
				}
				else
				{
					ClearFlag<CF>();
				}
				RESULT = GetRegister<byte>(AL);
				UpdateFlags_SFZFPF();
			}
			break;

		case 0xF4:
			CMD_NAME("HLT");
			ASSERT(false, "HALT!");
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
				SetRegister<byte>(AL, m_io.Memory<byte>(select(seg, GetRegister<word>(BX) + word(GetRegister<byte>(AL)))));
			}
			break;

		case 0xC8:
		{
			CMD_NAME("ENTER");
			word imm16 = GetImm<word>();
			byte level = GetImm<byte>();
			level &= 0x1F;

			word bp = GetRegister<word>(BP);

			Push(bp);
			word frame_ptr16 = GetRegister<word>(SP);

			if (level > 0)
			{
				while (--level)
				{
					bp -= 2;
					word temp16 = m_io.Memory<word>(select(GetSegment(SS), bp));
					Push(temp16);
				}

				Push(frame_ptr16);
			}

			SetRegister<word>(BP, frame_ptr16 + 0);
			SetRegister<word>(SP, GetRegister<word>(SP) - imm16);
		}
		break;

		case 0xC9:
		{
			CMD_NAME("LEAVE");
			word bp = GetRegister<word>(BP);
			word value = m_io.Memory<word>(select(GetSegment(SS), bp));
			SetRegister<word>((byte)SP, bp + 2);
			SetRegister<word>(BP, value + 0);
		}
		break;

		// escaping FPU commands
		case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
			GetImm<byte>();
			break;

	default:
		UNKNOWN_OPCODE(OPCODE1);
	}

#ifdef _DEBUG
	if (logging_state)
	{
		m_log.appendf("%s %s %s\n\r", dbg_cmd_address, dbg_cmd_name, dbg_args);
		printf("%s %s %s\n", dbg_cmd_address, dbg_cmd_name, dbg_args);
		m_scrollToBottom = true;
	}
#endif
}

void CPU::Interrupt(int n)
{
	Push(m_flags);
	Push(m_segments[CS]);
	Push(IP);
	IP = m_io.Memory<word>(n * 4);
	m_segments[CS] = m_io.Memory<word>(n * 4 + 2);
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
	ImGui::SameLine();
	n2hexstr(buff, m_flags);
	ImGui::Text("F=%s", buff);

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

	mem_edit.DrawWindow("Memory Editor", m_io.GetRamRawPointer(), 0x100000);
}
