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
#define APPEND_DBG_REGT(X, T) APPEND_DBG(GetRegName<T>(X))
#define APPEND_DBG_REGB(X) APPEND_DBG(m_regNames[X])
#define APPEND_DBG_REGW(X) APPEND_DBG(m_regNames[X + 8])
#define APPEND_DBG_SEG(X) APPEND_DBG(m_segNames[X])

#else
#define UNKNOWN_OPCODE(X) 
#define APPEND_DBG_REGT(X, T)
#define APPEND_DBG_REG(X)
#define APPEND_DBG_REGB(X) 
#define APPEND_DBG_REGW(X) 
#define APPEND_DBG_SEG(X)
#endif 

#define APPEND_DBG_JMP_DST \
	APPEND_HEX_DBG(m_segments[CS]); \
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(IP); \
	break;

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


template<>
const char* CPU::GetRegName<byte>(int i)
{
	return m_regNames[i];
}

template<>
const char* CPU::GetRegName<word>(int i)
{
	return m_regNames[i + 8];
}

template<typename T>
T CPU::GetImm()
{
	uint32_t address = select(m_segments[CS], IP);
	IP += sizeof(T);
	T data = m_io.GetMemory<T>(address);
	return data;
}


void CPU::SetInterruptHandler(InterruptHandler* interruptHandler)
{
	m_interruptHandler = interruptHandler;
}


void CPU::INT(byte x)
{
	m_interruptHandler->Int(x);
}


template<typename T>
void CPU::PrepareOperands_RM_REG()
{
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRM<T>();
	APPEND_DBG(", ");
	OPERAND_B = GetRegister<T>();
}


template<typename T>
void CPU::PrepareOperands_REG_RM()
{
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRegister<T>();
	APPEND_DBG(", ");
	OPERAND_B = GetRM<T>();
}


template<typename T>
void CPU::PrepareOperands_AXL_IMM()
{
	APPEND_DBG_REGT(0, T);
	OPERAND_A = GetRegister<T>(0);
	APPEND_DBG(", 0x");
	T data = GetImm<T>();
	OPERAND_B = data;
	APPEND_HEX_DBG(data);
}


template<typename T>
void CPU::PrepareOperands_AXL_RM_IMM()
{
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRM<T>();
	APPEND_DBG(", 0x");
	T data = GetImm<T>();
	OPERAND_B = data;
	APPEND_HEX_DBG(data);
}


void CPU::PrepareOperands_AXL_RM_IMM_signextend()
{
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRM<word>();
	APPEND_DBG(", 0x");
	sbyte data = GetImm<byte>();
	OPERAND_B = (word)data;
	APPEND_HEX_DBG(data);
}


template<typename T>
inline T CPU::GetRegister()
{
	byte r = (MODREGRM & REG) >> 3;
	APPEND_DBG_REGT(r, T);
	return GetRegister<T>(r);
}


template<typename T>
inline void CPU::SetRegister(T v)
{
	byte r = (MODREGRM & REG) >> 3;
	return SetRegister<T>(r, v);
}


template<typename T>
T CPU::GetRM()
{
	if ((MODREGRM & MOD) == MOD)
	{
		byte r = MODREGRM & RM;
		APPEND_DBG_REGT(r, T);
		return GetRegister<T>(r);
	}
	else
	{
		SetRM_Address();

		return m_io.GetMemory<T>(ADDRESS);
	}
}


template<typename T>
void CPU::SetRM(T x, bool computeAddress)
{
	if ((MODREGRM & MOD) == MOD)
	{
		byte r = MODREGRM & RM;
		if (computeAddress)
		{
			APPEND_DBG_REGT(r, T);
		}
		SetRegister<T>(r, x);
	}
	else
	{
		if (computeAddress)
		{
			SetRM_Address();
		}

		m_io.SetMemory<T>(ADDRESS, x);
	}
}


void CPU::SetRM_Address()
{
	int32_t reg = 0;
	int32_t disp = 0;
	byte offsetreg = DS;
	byte mod = MODREGRM & MOD;
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
		if (mod != 0)
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
	if (mod == 0x40)
	{
		sbyte disp8 = GetImm<byte>();
		APPEND_DBG(" + 0x");
		APPEND_HEX_DBG(disp8);
		disp = disp8; // sign extension
	}
	else if (mod == 0x80)
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


template<typename T>
inline void CPU::StoreResult_RM()
{
	SetRM<T>(RESULT);
}

template<typename T>
inline void CPU::StoreResult_REG()
{
	SetRegister<T>(RESULT);
}

template<typename T>
inline void CPU::StoreResult_AXL()
{
	SetRegister<T>(0, RESULT);
}


template<typename T>
inline int32_t SignExtension(T x);


template<>
inline int32_t SignExtension<byte>(byte x)
{
	return (sbyte)x;
}


template<>
inline int32_t SignExtension<word>(word x)
{
	return (sword)x;
}

template<typename T>
constexpr uint32_t GetMSBMask();


template<>
inline constexpr uint32_t GetMSBMask<byte>()
{
	return 0x80;
}


template<>
inline constexpr uint32_t GetMSBMask<word>()
{
	return 0x8000;
}

template<typename T>
inline constexpr uint32_t GetShiftClippingMask();


template<>
inline constexpr uint32_t GetShiftClippingMask<byte>()
{
	return 0x7;
}

template<>
inline constexpr uint32_t GetShiftClippingMask<word>()
{
	return 0x1F;
}


template<>
inline word CPU::GetStep<byte>()
{
	return TestFlag<DF>() == 0 ? 1 : -1;
}


template<>
inline word CPU::GetStep<word>()
{
	return TestFlag<DF>() == 0 ? 2 : -2;
}


template<>
inline void CPU::UpdateFlags_CF<byte>()
{
	SetFlag<CF>((RESULT & 0x100) != 0);
}


template<>
inline void CPU::UpdateFlags_CF<word>()
{
	SetFlag<CF>((RESULT & 0x10000) != 0);
}


template<typename T>
inline void CPU::UpdateFlags_OF()
{
	SetFlag<OF>((RESULT ^ OPERAND_A) & (RESULT ^ OPERAND_B) & GetMSBMask<T>());
}


template<typename T>
inline void CPU::UpdateFlags_OF_sub()
{
	SetFlag<OF>((RESULT ^ OPERAND_A) & (OPERAND_A ^ OPERAND_B) & GetMSBMask<T>());
}


inline void CPU::UpdateFlags_AF()
{
	SetFlag<AF>((RESULT ^ OPERAND_A ^ OPERAND_B) & 0x10);
}


inline void CPU::UpdateFlags_PF()
{
	byte v = RESULT & 0xFF;
	SetFlag<PF>(bool((0x9669 >> ((v ^ (v >> 4)) & 0xF)) & 1));
}


template<>
inline void CPU::UpdateFlags_ZF<byte>()
{
	SetFlag<ZF>((RESULT & 0xFF) == 0);
}

template<>
inline void CPU::UpdateFlags_ZF<word>()
{
	SetFlag<ZF>((RESULT & 0xFFFF) == 0);
}


template<typename T>
inline void CPU::UpdateFlags_SF()
{
	SetFlag<SF>((RESULT & GetMSBMask<T>()) != 0);
}


template<typename T>
void CPU::UpdateFlags_CFOFAF()
{
	UpdateFlags_CF<T>();
	UpdateFlags_OF<T>();
	UpdateFlags_AF();
}


template<typename T>
void CPU::UpdateFlags_CFOFAF_sub()
{
	UpdateFlags_CF<T>();
	UpdateFlags_OF_sub<T>();
	UpdateFlags_AF();
}


template<typename T>
void CPU::UpdateFlags_SFZFPF()
{
	UpdateFlags_SF<T>();
	UpdateFlags_ZF<T>();
	UpdateFlags_PF();
}


void CPU::ClearFlags_CFOF()
{
	ClearFlag<CF>();
	ClearFlag<OF>();
}


template<typename T>
void CPU::UpdateFlags_OFSFZFAFPF()
{
	UpdateFlags_OF<T>();
	UpdateFlags_SF<T>();
	UpdateFlags_ZF<T>();
	UpdateFlags_AF();
	UpdateFlags_PF();
}


template<typename T>
void CPU::UpdateFlags_OFSFZFAFPF_sub()
{
	UpdateFlags_OF_sub<T>();
	UpdateFlags_SF<T>();
	UpdateFlags_ZF<T>();
	UpdateFlags_AF();
	UpdateFlags_PF();
}


void CPU::Push(word x)
{
	m_registers[SP] -= 2;
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	m_io.SetMemory<word>(address, x);
}
word CPU::Pop()
{
	uint32_t address = select(m_segments[SS], m_registers[SP]);
	word x = m_io.GetMemory<word>(address);
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


template<typename T>
void CPU::ADD()
{
	CMD_NAME("ADD");
	RESULT = OPERAND_A + OPERAND_B;
	UpdateFlags_CFOFAF<T>();
	UpdateFlags_SFZFPF<T>();
}


template<typename T>
void CPU::ADC()
{
	CMD_NAME("ADC");
	RESULT = OPERAND_A + OPERAND_B + (uint32_t)TestFlag<CF>();
	UpdateFlags_CFOFAF<T>();
	UpdateFlags_SFZFPF<T>();
}


template<typename T>
void CPU::AND()
{
	CMD_NAME("AND");
	RESULT = OPERAND_A & OPERAND_B;
	ClearFlags_CFOF();
	UpdateFlags_SFZFPF<T>();
	ClearFlag<AF>();
}


template<typename T>
void CPU::XOR()
{
	CMD_NAME("XOR");
	RESULT = OPERAND_A ^ OPERAND_B;
	ClearFlags_CFOF();
	UpdateFlags_SFZFPF<T>();
	ClearFlag<AF>();
}


template<typename T>
void CPU::OR()
{
	CMD_NAME("OR");
	RESULT = OPERAND_A | OPERAND_B;
	ClearFlags_CFOF();
	UpdateFlags_SFZFPF<T>();
	ClearFlag<AF>();
}


template<typename T>
void CPU::SBB()
{
	CMD_NAME("SBB");
	RESULT = OPERAND_A - OPERAND_B - (uint32_t)TestFlag<CF>();
	UpdateFlags_CFOFAF_sub<T>();
	UpdateFlags_SFZFPF<T>();
}


template<typename T>
void CPU::SUB()
{
	CMD_NAME("SUB");
	RESULT = OPERAND_A - OPERAND_B;
	UpdateFlags_CFOFAF_sub<T>();
	UpdateFlags_SFZFPF<T>();
}


template<typename T>
void CPU::CMP()
{
	CMD_NAME("CMP");
	RESULT = OPERAND_A - OPERAND_B;
	UpdateFlags_CFOFAF_sub<T>();
	UpdateFlags_SFZFPF<T>();
}


template<typename T>
void CPU::MOV_rm_imm()
{
	CMD_NAME("MOV");
	MODREGRM = GetImm<byte>();
	SetRM_Address();
	T data = GetImm<T>();
	SetRM<T>(data);
	APPEND_DBG(", 0x");
	APPEND_HEX_DBG(data);
}

template<typename T>
void CPU::MOV_reg_imm()
{
	CMD_NAME("MOV");
	byte reg = OPCODE1 & 0x07;
	APPEND_DBG(m_regNames[reg]);
	APPEND_DBG(", 0x");
	T data = GetImm<T>();
	SetRegister<T>(reg, data);
	APPEND_HEX_DBG(data);
}

template<typename T>
void CPU::MOV_a_off()
{
	CMD_NAME("MOV");
	APPEND_DBG_REGT(0, T);
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
	OPERAND_A = m_io.GetMemory<T>(select(src_seg, src_offset));
	SetRegister<T>(0, OPERAND_A);
}

template<typename T>
void CPU::MOV_off_a()
{
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
	APPEND_DBG_REGT(0, T);
	m_io.SetMemory<T>(select(dst_seg, dst_offset), GetRegister<T>(0));
}

template<>
void CPU::MUL<byte>()
{
	CMD_NAME("MUL");
	RESULT = OPERAND_A * OPERAND_B;
	m_registers[0] = RESULT;
	SetFlag<OF>(RESULT & 0xFF00);
	SetFlag<CF>(RESULT & 0xFF00);
	UpdateFlags_ZF<byte>();
	ClearFlag<AF>();
	UpdateFlags_PF();
	UpdateFlags_SF<byte>();
}


template<>
void CPU::MUL<word>()
{
	CMD_NAME("MUL");
	RESULT = OPERAND_A * OPERAND_B;
	m_registers[0] = RESULT;
	m_registers[DX] = RESULT >> 16;
	SetFlag<OF>(RESULT & 0xFFFF0000);
	SetFlag<CF>(RESULT & 0xFFFF0000);
	UpdateFlags_ZF<word>();
	ClearFlag<AF>();
	UpdateFlags_PF();
	UpdateFlags_SF<word>();
}

template<>
void CPU::IMUL<byte>()
{
	CMD_NAME("IMUL");
	RESULT = OPERAND_A * OPERAND_B;
	m_registers[0] = RESULT;
	uint32_t of = RESULT & 0xFF80;
	SetFlag<OF>(of != 0 && of != 0xFF80);
	SetFlag<CF>(of != 0 && of != 0xFF80);
	UpdateFlags_ZF<byte>();
	ClearFlag<AF>();
	UpdateFlags_PF();
	UpdateFlags_SF<byte>();
}


template<>
void CPU::IMUL<word>()
{
	CMD_NAME("IMUL");
	RESULT = OPERAND_A * OPERAND_B;
	m_registers[AX] = RESULT;
	m_registers[DX] = RESULT >> 16;
	uint32_t of = RESULT & 0xFFFF8000;
	SetFlag<OF>(of != 0 && of != 0xFFFF8000);
	SetFlag<CF>(of != 0 && of != 0xFFFF8000);
	UpdateFlags_ZF<word>();
	ClearFlag<AF>();
	UpdateFlags_PF();
	UpdateFlags_SF<word>();
}


template<>
void CPU::DIV<byte>()
{
	CMD_NAME("DIV");
	APPEND_DBG_REGW(0);
	OPERAND_A = (word)GetRegister<word>(AX);
	APPEND_DBG(", ");
	OPERAND_B = (word)GetRM<byte>();
	RESULT = OPERAND_A / OPERAND_B;
	SetRegister<byte>(AL, RESULT);
	SetRegister<byte>(AH, (OPERAND_A % OPERAND_B));
}


template<>
void CPU::DIV<word>()
{
	CMD_NAME("DIV");
	APPEND_DBG_REGW(DX);
	APPEND_DBG(":");
	APPEND_DBG_REGW(0);
	OPERAND_A = (uint32_t)GetRegister<word>(0) + ((uint32_t)GetRegister<word>(DX) << 16);
	APPEND_DBG(", ");
	OPERAND_B = (word)GetRM<word>();
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


template<>
void CPU::IDIV<byte>()
{
	CMD_NAME("IDIV");
	APPEND_DBG_REGW(0);
	OPERAND_A = (int16_t)GetRegister<word>(0);
	APPEND_DBG(", ");
	OPERAND_B = (sbyte)GetRM<byte>();
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


template<>
void CPU::IDIV<word>()
{
	CMD_NAME("IDIV");
	APPEND_DBG_REGW(DX);
	APPEND_DBG(":");
	APPEND_DBG_REGW(0);
	OPERAND_A = (int32_t)(GetRegister<word>(0) + ((uint32_t)GetRegister<word>(DX) << 16));
	APPEND_DBG(", ");
	OPERAND_B = (int16_t)GetRM<word>();
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


template<typename T>
void CPU::TEST_reg_rm()
{
	CMD_NAME("TEST");
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRegister<T>();
	APPEND_DBG(", ");
	OPERAND_B = GetRM<T>();
	RESULT = OPERAND_A & OPERAND_B;
	UpdateFlags_SFZFPF<T>();
	SetFlag<CF>(false);
	SetFlag<OF>(false);
	ClearFlag<AF>();
}


template<typename T>
void CPU::TEST_a_imm()
{
	CMD_NAME("TEST");
	APPEND_DBG_REGT(0, T);
	OPERAND_A = GetRegister<T>(0);
	APPEND_DBG(", 0x");
	T data = GetImm<T>();
	APPEND_HEX_DBG(data);
	OPERAND_B = data;
	RESULT = OPERAND_A & OPERAND_B;
	UpdateFlags_SFZFPF<T>();
	SetFlag<CF>(false);
	SetFlag<OF>(false);
	ClearFlag<AF>();
}


template<typename T>
void CPU::XCHG()
{
	CMD_NAME("XCHG");
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRegister<T>();
	APPEND_DBG(", ");
	OPERAND_B = GetRM<T>();
	SetRegister<T>(OPERAND_B);
	SetRM<T>(OPERAND_A);
}


void CPU::LOOP()
{
	CMD_NAME("LOOP");
	--m_registers[CX];
	sbyte rel8 = GetImm<byte>();
	if (m_registers[CX] != 0)
	{
		IP += rel8;
	}
}


void CPU::LOOPZ()
{
	CMD_NAME("LOOPZ");
	--m_registers[CX];
	sbyte rel8 = GetImm<byte>();
	APPEND_HEX_DBG(m_segments[CS]);
	APPEND_DBG(":");
	APPEND_HEX_DBG(IP + rel8);
	if (m_registers[CX] != 0 && TestFlag<ZF>())
	{
		IP += rel8;
	}
}


void CPU::LOOPNZ()
{
	CMD_NAME("LOOPNZ");
	--m_registers[CX];
	sbyte rel8 = GetImm<byte>();
	APPEND_HEX_DBG(m_segments[CS]);
	APPEND_DBG(":");
	APPEND_HEX_DBG(IP + rel8);
	if (m_registers[CX] != 0 && !TestFlag<ZF>())
	{
		IP += rel8;
	}
}


template<typename T>
void CPU::LODS()
{
	int times = 1;
	if (m_REP)
	{
		times = m_registers[CX];
	}
	CMD_NAME("LODS");

	byte offsetreg = DS;
	if (m_segmentOverride != NONE)
	{
		offsetreg = m_segmentOverride;
	}

	int segment = GetSegment(offsetreg) << 4;
	APPEND_DBG(" [");
	APPEND_DBG_SEG(offsetreg);
	APPEND_DBG(":SI], ");

#ifdef _DEBUG
	GetRegister<T>(0);
#endif

	for (int i = 0; i < times; ++i)
	{
		ADDRESS = segment + m_registers[SI];
		OPERAND_A = m_io.GetMemory<T>(ADDRESS);
		SetRegister<T>(0, OPERAND_A);
		m_registers[SI] += GetStep<T>();

#ifdef _DEBUG
		if (i != times - 1)
		{
			*(dbg_args_ptr = dbg_args) = 0;
		}
#endif
		if (m_REP)
		{
			m_registers[CX]--;
		}
	}
}


template<typename T>
void CPU::STOS()
{
	int times = 1;
	if (m_REP)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
	for (int i = 0; i < times; ++i)
	{
		CMD_NAME("STOS");
		APPEND_DBG_REGT(0, T);
		OPERAND_A = GetRegister<T>(0);
		APPEND_DBG(" [ES:DI]");
		ASSERT(m_segmentOverride == NONE, "Override not permited");
		ADDRESS = GetSegment(ES) << 4;
		ADDRESS += m_registers[DI];

		m_io.SetMemory<T>(ADDRESS, OPERAND_A);
		m_registers[DI] += step;

#ifdef _DEBUG
		if (i != times - 1)
		{
			*(dbg_args_ptr = dbg_args) = 0;
		}
#endif
		if (m_REP)
		{
			m_registers[CX]--;
		}
	}
}


template<typename T>
void CPU::SCAS()
{
	int times = 1;
	if (m_REP || m_REPN)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
	for (int i = 0; i < times; ++i)
	{
		CMD_NAME("SCAS");
		APPEND_DBG_REGT(0, T);
		OPERAND_A = GetRegister<T>(0);
		APPEND_DBG(" [ES:DI]");
		ASSERT(m_segmentOverride == NONE, "Override not permited");
		ADDRESS = GetSegment(ES) << 4;
		ADDRESS += m_registers[DI];

		OPERAND_B = m_io.GetMemory<T>(ADDRESS);
		m_registers[DI] += step;

		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_CFOFAF_sub<T>();
		UpdateFlags_SFZFPF<T>();

#ifdef _DEBUG
		if (i != times - 1)
		{
			*(dbg_args_ptr = dbg_args) = 0;
		}
#endif

		if (m_REP || m_REPN)
		{
			m_registers[CX]--;
		}
		if (m_REP && !TestFlag<ZF>())
		{
			break;
		}
		if (m_REPN && TestFlag<ZF>())
		{
			break;
		}
	}
}


template<typename T>
void CPU::CMPS()
{
	int times = 1;
	if (m_REP || m_REPN)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
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
		OPERAND_A = m_io.GetMemory<T>(select(src_seg, src_offset));
		OPERAND_B = m_io.GetMemory<T>(select(dst_seg, dst_offset));
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_CFOFAF_sub<T>();
		UpdateFlags_SFZFPF<T>();
		m_registers[SI] += step;
		m_registers[DI] += step;

#ifdef _DEBUG
		if (i != times - 1)
		{
			*(dbg_args_ptr = dbg_args) = 0;
		}
#endif
		if (m_REP || m_REPN)
		{
			m_registers[CX]--;
		}
		if (m_REP && !TestFlag<ZF>())
		{
			break;
		}
		if (m_REPN && TestFlag<ZF>())
		{
			break;
		}
	}
}


template<typename T>
void CPU::MOVS()
{
	int times = 1;
	if (m_REP || m_REPN)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
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
		T w = m_io.GetMemory<T>(select(src_seg, src_offset));
		m_io.SetMemory<T>(select(dst_seg, dst_offset), w);

		m_registers[SI] += step;
		m_registers[DI] += step;

#ifdef _DEBUG
		if (i != times - 1)
		{
			*(dbg_args_ptr = dbg_args) = 0;
		}
#endif
		if (m_REP || m_REPN)
		{
			m_registers[CX]--;
		}
	}
}


template<typename T>
void CPU::IN_imm()
{
	CMD_NAME("IN");
	ADDRESS = GetImm<byte>();
	SetRegister<T>(0, m_io.Port<T>(ADDRESS));
}


template<typename T>
void CPU::IN_dx()
{
	CMD_NAME("IN");
	APPEND_DBG_REGW(DX);
	ADDRESS = GetRegister<word>(DX);
	APPEND_DBG_REGT(0, T);
	SetRegister<T>(0, m_io.Port<T>(ADDRESS));
}


template<typename T>
void CPU::OUT_imm()
{
	CMD_NAME("OUT");
	ADDRESS = GetImm<byte>();
	APPEND_DBG_REGB(0);
	m_io.Port<T>(ADDRESS) = GetRegister<T>(AL);
}


template<typename T>
void CPU::OUT_dx()
{
	CMD_NAME("OUT");
	ADDRESS = GetRegister<word>(DX);
	APPEND_DBG_REGW(DX);
	APPEND_DBG_REGT(0, T);
	m_io.Port<T>(ADDRESS) = GetRegister<T>(0);
}

template<typename T>
void CPU::INS()
{
	CMD_NAME("INS");
	int times = 1;
	if (m_REP)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
	for (int i = 0; i < times; ++i)
	{
		m_io.SetMemory<T>(select(GetSegment(ES), GetRegister<word>(DI)), m_io.Port<T>(GetRegister<word>(DX)));

		m_registers[SI] += step;
		m_registers[DI] += step;

		if (m_REP)
		{
			m_registers[CX]--;
		}
	}
}


template<typename T>
void CPU::OUTS()
{
	CMD_NAME("OUTS");
	int times = 1;
	if (m_REP)
	{
		times = m_registers[CX];
	}
	word step = GetStep<T>();
	for (int i = 0; i < times; ++i)
	{
		byte offsetreg = DS;
		if (m_segmentOverride != NONE)
		{
			offsetreg = m_segmentOverride;
		}
		m_io.Port<T>(GetRegister<word>(DX)) = m_io.GetMemory<T>(select(GetSegment(offsetreg), GetRegister<word>(SI)));

		m_registers[SI] += step;
		m_registers[DI] += step;

		if (m_REP)
		{
			m_registers[CX]--;
		}
	}
}


void CPU::Step()
{
#ifdef _DEBUG
	dbg_args_ptr = dbg_args;
	dbg_args[0] = 0;

	memcpy(m_old_registers, m_registers, sizeof(m_registers));
	memcpy(m_old_segments, m_segments, sizeof(m_segments));
	memcpy(&m_old_flags, &m_flags, sizeof(m_flags));
	print_address(dbg_cmd_address, m_segments[CS], (word)(IP));
	mem_edit.HighlightMin = (((uint32_t)m_segments[CS]) << 4) + IP;
	mem_edit.HighlightMax = mem_edit.HighlightMin + 1;

	if (logging_state)
	{
		fprintf(logfile, GetDebugString());
		fprintf(logfile, "\n");
		fflush(logfile);
	}
#endif
	m_LOCK = false;
	m_REPN = false;
	m_REP = false;
	m_segmentOverride = NONE;

	StepInternal();

#ifdef _DEBUG
	if (logging_state)
	{
		m_log.appendf("%s %s %s\n\r", dbg_cmd_address, dbg_cmd_name, dbg_args);
		printf("%s %s %s\n", dbg_cmd_address, dbg_cmd_name, dbg_args);
		m_scrollToBottom = true;
	}
#endif
}

void CPU::StepInternal()
{
	byte seg = 0;
	byte data = 0;
	bool condition = false;
	int16_t offset = 0;
	int times = 1;

	bool sizeOverride = false;
	bool addressOverride = false;

	OPCODE1 = GetImm<byte>();
	
	switch (OPCODE1)
	{
	case 0x00 + 0x00: PrepareOperands_RM_REG<byte>(); ADD<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x00: PrepareOperands_RM_REG<word>(); ADD<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x00: PrepareOperands_REG_RM<byte>(); ADD<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x00: PrepareOperands_REG_RM<word>(); ADD<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x00: PrepareOperands_AXL_IMM<byte>(); ADD<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x00: PrepareOperands_AXL_IMM<word>(); ADD<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x10: PrepareOperands_RM_REG<byte>(); ADC<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x10: PrepareOperands_RM_REG<word>(); ADC<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x10: PrepareOperands_REG_RM<byte>(); ADC<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x10: PrepareOperands_REG_RM<word>(); ADC<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x10: PrepareOperands_AXL_IMM<byte>(); ADC<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x10: PrepareOperands_AXL_IMM<word>(); ADC<word>(); StoreResult_AXL<word>(); break;
	
	case 0x00 + 0x20: PrepareOperands_RM_REG<byte>(); AND<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x20: PrepareOperands_RM_REG<word>(); AND<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x20: PrepareOperands_REG_RM<byte>(); AND<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x20: PrepareOperands_REG_RM<word>(); AND<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x20: PrepareOperands_AXL_IMM<byte>(); AND<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x20: PrepareOperands_AXL_IMM<word>(); AND<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x30: PrepareOperands_RM_REG<byte>(); XOR<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x30: PrepareOperands_RM_REG<word>(); XOR<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x30: PrepareOperands_REG_RM<byte>(); XOR<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x30: PrepareOperands_REG_RM<word>(); XOR<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x30: PrepareOperands_AXL_IMM<byte>(); XOR<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x30: PrepareOperands_AXL_IMM<word>(); XOR<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x08: PrepareOperands_RM_REG<byte>(); OR<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x08: PrepareOperands_RM_REG<word>(); OR<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x08: PrepareOperands_REG_RM<byte>(); OR<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x08: PrepareOperands_REG_RM<word>(); OR<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x08: PrepareOperands_AXL_IMM<byte>(); OR<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x08: PrepareOperands_AXL_IMM<word>(); OR<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x18: PrepareOperands_RM_REG<byte>(); SBB<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x18: PrepareOperands_RM_REG<word>(); SBB<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x18: PrepareOperands_REG_RM<byte>(); SBB<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x18: PrepareOperands_REG_RM<word>(); SBB<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x18: PrepareOperands_AXL_IMM<byte>(); SBB<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x18: PrepareOperands_AXL_IMM<word>(); SBB<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x28: PrepareOperands_RM_REG<byte>(); SUB<byte>(); StoreResult_RM<byte>(); break;
	case 0x01 + 0x28: PrepareOperands_RM_REG<word>(); SUB<word>(); StoreResult_RM<word>(); break;
	case 0x02 + 0x28: PrepareOperands_REG_RM<byte>(); SUB<byte>(); StoreResult_REG<byte>(); break;
	case 0x03 + 0x28: PrepareOperands_REG_RM<word>(); SUB<word>(); StoreResult_REG<word>(); break;
	case 0x04 + 0x28: PrepareOperands_AXL_IMM<byte>(); SUB<byte>(); StoreResult_AXL<byte>(); break;
	case 0x05 + 0x28: PrepareOperands_AXL_IMM<word>(); SUB<word>(); StoreResult_AXL<word>(); break;

	case 0x00 + 0x38: PrepareOperands_RM_REG<byte>(); CMP<byte>(); break;
	case 0x01 + 0x38: PrepareOperands_RM_REG<word>(); CMP<word>(); break;
	case 0x02 + 0x38: PrepareOperands_REG_RM<byte>(); CMP<byte>(); break;
	case 0x03 + 0x38: PrepareOperands_REG_RM<word>(); CMP<word>(); break;
	case 0x04 + 0x38: PrepareOperands_AXL_IMM<byte>(); CMP<byte>(); break;
	case 0x05 + 0x38: PrepareOperands_AXL_IMM<word>(); CMP<word>(); break;

	case 0x06: CMD_NAME("PUSH ES"); Push(m_segments[ES]); break;
	case 0x07: CMD_NAME("POP ES"); m_segments[ES] = Pop(); break;
	case 0x0E: CMD_NAME("PUSH CS"); Push(m_segments[CS]); break;
	case 0x0F: Group0F(); break;
	case 0x16: CMD_NAME("PUSH SS"); Push(m_segments[SS]); break;
	case 0x17: CMD_NAME("POP SS"); m_segments[SS] = Pop(); break;
	case 0x1E: CMD_NAME("PUSH DS"); Push(m_segments[DS]); break;
	case 0x1F: CMD_NAME("POP DS"); m_segments[DS] = Pop(); break;

	case 0x26: m_segmentOverride = ES; StepInternal(); break;

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
			UpdateFlags_SFZFPF<byte>();
		}
		break;

	case 0x2E: m_segmentOverride = CS; StepInternal(); break;

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
			UpdateFlags_SFZFPF<byte>();
		}
		break;

	case 0x36: m_segmentOverride = SS; StepInternal(); break;

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

	case 0x3E: m_segmentOverride = DS; StepInternal(); break;

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
		
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		CMD_NAME("INC");
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF<word>();
		SetRegister<word>(OPCODE1 & 0x07, RESULT);
		break;

	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		CMD_NAME("DEC");
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_OFSFZFAFPF_sub<word>();
		SetRegister<word>(OPCODE1 & 0x07, RESULT);
		break;

	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		CMD_NAME("PUSH");
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		Push(OPERAND_A);
		break;

	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		CMD_NAME("POP");
		OPERAND_A = Pop();
		APPEND_DBG_REGW(OPCODE1 & 0x07);
		SetRegister<word>(OPCODE1 & 0x07, OPERAND_A);
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

	case 0x66: sizeOverride = true; StepInternal(); break;
	case 0x67: addressOverride = true; StepInternal(); break;

	case 0x68:
		CMD_NAME("PUSH");
		OPERAND_A = GetImm<word>();
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x69:
		MODREGRM = GetImm<byte>();
		CMD_NAME("IMUL");
		GetRegister<word>();
		APPEND_DBG(", ");
		OPERAND_A = (int16_t)GetRM<word>();
		APPEND_DBG(", ");
		OPERAND_B = (int16_t)GetImm<word>();
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetRegister<word>(RESULT);
		UpdateFlags_ZF<word>();
		ClearFlag<AF>();
		UpdateFlags_PF();
		UpdateFlags_SF<word>();
		RESULT = RESULT & 0xFFFF8000;
		SetFlag<OF>(RESULT != 0 && RESULT != 0xFFFF8000);
		SetFlag<CF>(RESULT != 0 && RESULT != 0xFFFF8000);
		break;

	case 0x6A:
		CMD_NAME("PUSH");
		OPERAND_A = (sbyte)GetImm<byte>(); // sign extend
		APPEND_HEX_DBG(OPERAND_A);
		Push(OPERAND_A);
		break;

	case 0x6B:
		MODREGRM = GetImm<byte>();
		CMD_NAME("IMUL");
		GetRegister<word>();
		APPEND_DBG(", ");
		OPERAND_A = (int16_t)GetRM<word>(); // sign extend
		APPEND_DBG(", ");
		OPERAND_B = (sbyte)GetImm<byte>(); // sign extend
		APPEND_HEX_DBG(OPERAND_B);
		RESULT = OPERAND_A * OPERAND_B;
		SetRegister<word>(RESULT);
		UpdateFlags_ZF<word>();
		ClearFlag<AF>();
		UpdateFlags_PF();
		UpdateFlags_SF<word>();
		RESULT = RESULT & 0xFFFF8000;
		SetFlag<OF>(RESULT != 0 && RESULT != 0xFFFF8000);
		SetFlag<CF>(RESULT != 0 && RESULT != 0xFFFF8000);
		break;

	case 0x6C: INS<byte>(); break;
	case 0x6D: INS<word>(); break;
	case 0x6E: OUTS<byte>(); break;
	case 0x6F: OUTS<word>(); break;
		
	case 0x70: CMD_NAME("JO"); condition = TestFlag<OF>(); CJMP
	case 0x71: CMD_NAME("JNO"); condition = !TestFlag<OF>(); CJMP
	case 0x72: CMD_NAME("JB"); condition = TestFlag<CF>(); CJMP
	case 0x73: CMD_NAME("JNB"); condition = !TestFlag<CF>(); CJMP
	case 0x74: CMD_NAME("JE"); condition = TestFlag<ZF>(); CJMP
	case 0x75: CMD_NAME("JNE"); condition = !TestFlag<ZF>(); CJMP
	case 0x76: CMD_NAME("JBE"); condition = TestFlag<CF>() || TestFlag<ZF>(); CJMP
	case 0x77: CMD_NAME("JNBE"); condition = !TestFlag<CF>() && !TestFlag<ZF>(); CJMP
	case 0x78: CMD_NAME("JS"); condition = TestFlag<SF>(); CJMP
	case 0x79: CMD_NAME("JNS"); condition = !TestFlag<SF>(); CJMP
	case 0x7A: CMD_NAME("JP"); condition = TestFlag<PF>(); CJMP
	case 0x7B: CMD_NAME("JNP"); condition = !TestFlag<PF>(); CJMP
	case 0x7C: CMD_NAME("JL"); condition = TestFlag<SF>() != TestFlag<OF>(); CJMP
	case 0x7D: CMD_NAME("JNL"); condition = TestFlag<SF>() == TestFlag<OF>(); CJMP
	case 0x7E: CMD_NAME("JLE"); condition = TestFlag<ZF>() || (TestFlag<SF>() != TestFlag<OF>()); CJMP
	case 0x7F: CMD_NAME("JNLE"); condition = !TestFlag<ZF>() && (TestFlag<SF>() == TestFlag<OF>()); CJMP

	case 0x80: PrepareOperands_AXL_RM_IMM<byte>(); Group1<byte>(); break;
	case 0x81: PrepareOperands_AXL_RM_IMM<word>(); Group1<word>(); break;
	case 0x82: PrepareOperands_AXL_RM_IMM<byte>(); Group1<byte>(); break;
	case 0x83: PrepareOperands_AXL_RM_IMM_signextend(); Group1<word>(); break;

	case 0x84: TEST_reg_rm<byte>(); break;
	case 0x85: TEST_reg_rm<word>(); break;

	case 0x86: XCHG<byte>(); break;
	case 0x87: XCHG<word>(); break;

	case 0x88: CMD_NAME("MOV"); PrepareOperands_RM_REG<byte>(); RESULT = OPERAND_B; StoreResult_RM<byte>(); break;
	case 0x89: CMD_NAME("MOV"); PrepareOperands_RM_REG<word>(); RESULT = OPERAND_B; StoreResult_RM<word>(); break;
	case 0x8A: CMD_NAME("MOV"); PrepareOperands_REG_RM<byte>(); RESULT = OPERAND_B; StoreResult_REG<byte>(); break;
	case 0x8B: CMD_NAME("MOV"); PrepareOperands_REG_RM<word>(); RESULT = OPERAND_B; StoreResult_REG<word>(); break;
	
	case 0x8C:
		CMD_NAME("MOV");
		MODREGRM = GetImm<byte>();
		seg = (MODREGRM & REG) >> 3;
		SetRM<word>(m_segments[seg], true);
		APPEND_DBG(", ");
		APPEND_DBG (m_segNames[seg]);
		break;

	case 0x8D:
		CMD_NAME("LEA");
		MODREGRM = GetImm<byte>();
		GetRegister<word>();
		APPEND_DBG(", ");
		SetRM_Address();
		SetRegister<word>(EFFECTIVE_ADDRESS);
		break;

	case 0x8E:
		CMD_NAME("MOV");
		MODREGRM = GetImm<byte>();
		seg = (MODREGRM & REG) >> 3;
		APPEND_DBG(m_segNames[seg]);
		APPEND_DBG(", ");
		m_segments[seg] = GetRM<word>();
		break;

	case 0x8F:
		CMD_NAME("POP");
		MODREGRM = GetImm<byte>();
		SetRM<word>(Pop(), true);
		break;

	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		CMD_NAME("XCHG");
		OPERAND_A = GetRegister<word>(OPCODE1 & 0x07);
		APPEND_DBG_REGW((OPCODE1 & 0x07));
		APPEND_DBG(", ");
		OPERAND_B = GetRegister<word>(0);
		APPEND_DBG_REGW(0);
		SetRegister<word>(OPCODE1 & 0x07, OPERAND_B);
		SetRegister<word>(0, OPERAND_A);
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

	case 0x9C:
		CMD_NAME("PUSHF");
		Push(m_flags);
		break;

	case 0x9D:
		CMD_NAME("POPF");
		m_flags = Pop() | 0x2;
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

	case 0xA0: MOV_a_off<byte>(); break;
	case 0xA1: MOV_a_off<word>(); break;
	case 0xA2: MOV_off_a<byte>(); break;
	case 0xA3: MOV_off_a<word>(); break;
	case 0xA4: MOVS<byte>(); break;
	case 0xA5: MOVS<word>(); break;
	case 0xA6: CMPS<byte>(); break;
	case 0xA7: CMPS<word>(); break;
	case 0xA8: TEST_a_imm<byte>(); break;
	case 0xA9: TEST_a_imm<word>(); break;
	case 0xAA: STOS<byte>(); break;
	case 0xAB: STOS<word>(); break;
	case 0xAC: LODS<byte>(); break;
	case 0xAD: LODS<word>(); break;
	case 0xAE: SCAS<byte>(); break;
	case 0xAF: SCAS<word>(); break;

	case 0xB0:case 0xB1:case 0xB2:case 0xB3:case 0xB4:case 0xB5:case 0xB6:case 0xB7: MOV_reg_imm<byte>(); break;
	case 0xB8:case 0xB9:case 0xBA:case 0xBB:case 0xBC:case 0xBD:case 0xBE:case 0xBF: MOV_reg_imm<word>(); break;

	case 0xC0: MODREGRM = GetImm<byte>(); times = GetImm<byte>(); Group2<byte>(times); break;
	case 0xC1: MODREGRM = GetImm<byte>(); times = GetImm<byte>(); Group2<word>(times); break;
	case 0xC2: CMD_NAME("RET"); times = GetImm<word>(); IP = Pop(); m_registers[SP] += times; break;
	case 0xC3: CMD_NAME("RET"); IP = Pop(); break;

	case 0xC4:
		CMD_NAME("LES");
		MODREGRM = GetImm<byte>();
		GetRegister<word>();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = m_io.GetMemory<word>(ADDRESS);
		OPERAND_B = m_io.GetMemory<word>(ADDRESS + 2);
		SetRegister<word>(OPERAND_A);
		SetSegment(ES, OPERAND_B);
		break;

	case 0xC5:
		CMD_NAME("LDS");
		MODREGRM = GetImm<byte>();
		GetRegister<word>();
		APPEND_DBG(", ");
		SetRM_Address();
		OPERAND_A = m_io.GetMemory<word>(ADDRESS);
		OPERAND_B = m_io.GetMemory<word>(ADDRESS + 2);
		SetRegister<word>(OPERAND_A);
		SetSegment(DS, OPERAND_B);
		break;

	case 0xC6: MOV_rm_imm<byte>(); break;
	case 0xC7: MOV_rm_imm<word>(); break;

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
				word temp16 = m_io.GetMemory<word>(select(GetSegment(SS), bp));
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
		word value = m_io.GetMemory<word>(select(GetSegment(SS), bp));
		SetRegister<word>((byte)SP, bp + 2);
		SetRegister<word>(BP, value + 0);
	}
	break;

	case 0xCA: CMD_NAME("RET"); times = GetImm<word>(); IP = Pop(); m_segments[CS] = Pop(); m_registers[SP] += times; break;
	case 0xCB: CMD_NAME("RET"); IP = Pop(); m_segments[CS] = Pop(); break;
	case 0xCC: CMD_NAME("INT 3"); INT(3);break;
	case 0xCD: CMD_NAME("INT"); data = GetImm<byte>(); APPEND_HEX_DBG(data); INT(data); break;
	case 0xCE: CMD_NAME("INTO"); INT(4); break;
	case 0xCF: CMD_NAME("IRET"); IP = Pop(); m_segments[CS] = Pop(); m_flags = Pop() | 0x2; break;

	case 0xD0: MODREGRM = GetImm<byte>(); Group2<byte>(1); break;
	case 0xD1: MODREGRM = GetImm<byte>(); Group2<word>(1); break;
	case 0xD2: MODREGRM = GetImm<byte>(); Group2<byte>(GetRegister<byte>(CL)); break;
	case 0xD3: MODREGRM = GetImm<byte>(); Group2<word>(GetRegister<byte>(CL)); break;

	case 0xD4:
		CMD_NAME("AAM");
		{
			unsigned int tempL = GetRegister<byte>(AL);
			unsigned int imm8 = GetImm<byte>();
			SetRegister<byte>(AH, tempL / imm8);
			SetRegister<byte>(AL, tempL % imm8);
			RESULT = GetRegister<byte>(AL);
			UpdateFlags_SFZFPF<byte>();
		}
		break;

	case 0xD5:
		CMD_NAME("AAD");
		{
			int tempL = GetRegister<byte>(AL);
			int tempH = GetRegister<byte>(AH);
			SetRegister<byte>(AL, tempL + (tempH * GetImm<byte>()));
			SetRegister<byte>(AH, 0);
			RESULT = GetRegister<byte>(AL);
			UpdateFlags_SFZFPF<byte>();
		}
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
			SetRegister<byte>(AL, m_io.GetMemory<byte>(select(seg, GetRegister<word>(BX) + word(GetRegister<byte>(AL)))));
		}
		break;

	case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF: GetImm<byte>();	break;

	case 0xE0: LOOPNZ();  break;
	case 0xE1: LOOPZ();	break;
	case 0xE2: LOOP(); break;
	case 0xE3: CMD_NAME("JCXZ"); condition = m_registers[CX] == 0; CJMP;
	case 0xE4: IN_imm<byte>(); break;
	case 0xE5: IN_imm<word>(); break;
	case 0xE6: OUT_imm<byte>(); break;
	case 0xE7: OUT_imm<word>(); break; 
	case 0xE8: CMD_NAME("CALL"); offset = GetImm<word>(); Push(IP); IP += offset; APPEND_DBG_JMP_DST;
	case 0xE9: CMD_NAME("JMP"); IP += GetImm<word>(); APPEND_DBG_JMP_DST;
	case 0xEA: CMD_NAME("JMP"); { word new_IP = GetImm<word>(); word new_CS = GetImm<word>(); IP = new_IP; m_segments[CS] = new_CS; } APPEND_DBG_JMP_DST;
	case 0xEB: CMD_NAME("JMP"); IP += (sbyte)GetImm<byte>(); APPEND_DBG_JMP_DST;
	case 0xEC: IN_dx<byte>(); break;
	case 0xED: IN_dx<word>(); break;
	case 0xEE: OUT_dx<byte>(); break;
	case 0xEF: OUT_dx<word>(); break;

	case 0xF0: m_LOCK = true; StepInternal(); break;
	case 0xF2: m_REPN = true; StepInternal(); break;
	case 0xF3: m_REP = true; StepInternal(); break;
	case 0xF4: CMD_NAME("HLT"); ASSERT(false, "HALT!"); break;
	case 0xF5: CMD_NAME("CMC"); SetFlag<CF>(!TestFlag<CF>()); break;
	case 0xF6: Group3<byte>(); break;
	case 0xF7: Group3<word>(); break;
	case 0xF8: CMD_NAME("CLC"); ClearFlag<CF>(); break;
	case 0xF9: CMD_NAME("STC"); SetFlag<CF>(); break;
	case 0xFA: CMD_NAME("CLI"); ClearFlag<IF>(); break;
	case 0xFB: CMD_NAME("STI"); SetFlag<IF>(); break;
	case 0xFC: CMD_NAME("CLD"); ClearFlag<DF>(); break;
	case 0xFD: CMD_NAME("STD"); SetFlag<DF>(); break;
	case 0xFE: Group45<byte>(); break;
	case 0xFF: Group45<word>(); break;

	default:
		UNKNOWN_OPCODE(OPCODE1);
	}
}

template<typename T>
void CPU::Group1()
{
	OPCODE2 = (MODREGRM & REG) >> 3;
	switch (OPCODE2)
	{
	case 0x00: ADD<T>(); StoreResult_RM<T>(); break;
	case 0x01: OR<T>();  StoreResult_RM<T>(); break;
	case 0x02: ADC<T>(); StoreResult_RM<T>(); break;
	case 0x03: SBB<T>(); StoreResult_RM<T>(); break;
	case 0x04: AND<T>(); StoreResult_RM<T>(); break;
	case 0x05: SUB<T>(); StoreResult_RM<T>(); break;
	case 0x06: XOR<T>(); StoreResult_RM<T>(); break;
	case 0x07: CMP<T>(); break;
	}
}


template<typename T>
void CPU::Group2(int times)
{
	OPCODE2 = (MODREGRM & REG) >> 3;
	times &= GetShiftClippingMask<T>();
	constexpr int32_t MSB_MASK = GetMSBMask<T>();

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
	OPERAND_A = GetRM<T>();
	RESULT = OPERAND_A;
	bool tempCF = false;

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
			UpdateFlags_SFZFPF<T>();
			ClearFlag<AF>();
			break;
		case 0x05:
			SetFlag<CF>(1 & RESULT);
			RESULT = RESULT / 2;
			SetFlag<OF>((MSB_MASK & OPERAND_A) != 0);
			UpdateFlags_SFZFPF<T>();
			ClearFlag<AF>();
			break;
		case 0x06:
			SetFlag<CF>(MSB_MASK & RESULT);
			RESULT = RESULT * 2;
			SetFlag<OF>(((MSB_MASK & RESULT) != 0) != TestFlag<CF>());
			UpdateFlags_SFZFPF<T>();
			ClearFlag<AF>();
			break;
		case 0x07:
		{
			bool negative = (MSB_MASK & RESULT) != 0;
			SetFlag<CF>(1 & RESULT);
			RESULT = RESULT / 2 + negative * MSB_MASK;
			ClearFlag<OF>();
			UpdateFlags_SFZFPF<T>();
			ClearFlag<AF>();
		}
		break;

		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
	}
	SetRM<T>(RESULT);
}


template<typename T>
void CPU::Group3()
{
	MODREGRM = GetImm<byte>();
	OPCODE2 = (MODREGRM & REG) >> 3;

	switch (OPCODE2)
	{
	case 0x00:
		CMD_NAME("TEST");
		OPERAND_A = GetRM<T>();
		APPEND_DBG(", ");
		OPERAND_B = GetImm<T>();
		RESULT = OPERAND_A & OPERAND_B;
		UpdateFlags_SFZFPF<T>();
		SetFlag<CF>(false);
		SetFlag<OF>(false);
		ClearFlag<AF>();
		break;

	case 0x04:
		APPEND_DBG_REGT(0, T);
		OPERAND_A = (word)GetRegister<T>(0);
		APPEND_DBG(", ");
		OPERAND_B = (word)GetRM<T>();
		MUL<T>();
		break;

	case 0x05:
		APPEND_DBG_REGT(0, T);
		OPERAND_A = SignExtension<T>(GetRegister<T>(0));
		APPEND_DBG(", ");
		OPERAND_B = SignExtension<T>(GetRM<T>());
		IMUL<T>();
		break;

	case 0x06: DIV<T>(); break;
	case 0x07: IDIV<T>(); break;

	case 0x03:
		CMD_NAME("NEG");
		OPERAND_B = 0;
		OPERAND_A = SignExtension<T>(GetRM<T>());
		RESULT = -OPERAND_A;
		SetRM<T>(RESULT);
		SetFlag<CF>(RESULT != 0);
		UpdateFlags_OFSFZFAFPF_sub<T>();
		break;

	case 0x02:
		CMD_NAME("NOT");
		OPERAND_A = GetRM<T>();
		RESULT = ~OPERAND_A;
		SetRM<T>(RESULT);
		break;
	default:
		UNKNOWN_OPCODE(OPCODE2);
	}
}


template<typename T>
void CPU::Group45()
{
	MODREGRM = GetImm<byte>();
	OPERAND_A = GetRM<T>();
	OPERAND_B = m_io.GetMemory<word>(ADDRESS + 2);

	OPCODE2 = (MODREGRM & REG) >> 3;

	switch (OPCODE2)
	{
	case 0x00:
		CMD_NAME("INC");
		OPERAND_B = 1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF<T>();
		SetRM<T>(RESULT);
		break;
	case 0x01:
		CMD_NAME("DEC");
		OPERAND_B = 1;
		RESULT = OPERAND_A - OPERAND_B;
		UpdateFlags_OFSFZFAFPF_sub<T>();
		SetRM<T>(RESULT);
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
}


void CPU::Group0F()
{
	bool condition = false;

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
		MODREGRM = GetImm<byte>();
		OPCODE2 = (MODREGRM & REG) >> 3;
		{
			int times = GetImm<byte>();
			OPERAND_A = GetRM<word>() | GetRegister<word>((MODREGRM & REG) >> 3) * 0x10000;
			RESULT = OPERAND_A;

			for (int i = 0; i < times; ++i)
			{
				SetFlag<CF>(1 & RESULT);
				RESULT = RESULT / 2;
				SetFlag<OF>((GetMSBMask<word>() & OPERAND_A) != 0);
				UpdateFlags_SFZFPF<word>();
				ClearFlag<AF>();
			}
			SetRM<word>(RESULT);
		}
		break;

	default:
		UNKNOWN_OPCODE(OPCODE2);
	}
}


void CPU::Interrupt(int n)
{
	Push(m_flags);
	Push(m_segments[CS]);
	Push(IP);
	IP = m_io.GetMemory<word>(n * 4);
	m_segments[CS] = m_io.GetMemory<word>(n * 4 + 2);
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
