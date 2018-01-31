#include "CPU.h"
#include "_assert.h"
#include <imgui.h>
#include "imgui_memory_editor.h"

static MemoryEditor mem_edit;

#define CJMP \
{\
	int8_t rel8 = GetIMM8(); \
	APPEND_HEX_DBG(m_segments[CS]);\
	APPEND_DBG(":"); \
	APPEND_HEX_DBG(IP); \
	if (condition) \
	{ \
		IP += rel8; \
	} break;\
}

#define UNKNOWN_OPCODE(X) \
			ASSERT(false, "Unknown opcode: 0x%s\n", n2hexstr(X).c_str());

char* print_address(char* dst, uint16_t segment, uint16_t offset)
{
	char* tmp = n2hexstr(dst, segment);
	*tmp++ = ':';
	return &(*n2hexstr(tmp, offset) = '\x0');	
}

CPU::CPU()
{
	memset(m_segments, 0, sizeof(m_segments));
	memset(m_registers, 0, sizeof(m_registers));
	IP = 0;
	m_segments[CS] = 0x10;
	m_flags = 0;
	dbg_cmd_address[0] = 0;
	dbg_cmd_name[0] = 0;
	dbg_args[0] = 0;
	dbg_args_ptr = dbg_args;
	m_scrollToBottom = true;
	mem_edit.GotoAddr = 0x7C00;
}

void CPU::Step()
{
#ifdef _DEBUG
	print_address(dbg_cmd_address, m_segments[CS], IP);
#endif
	dbg_args_ptr = dbg_args;
	dbg_args[0] = 0;
	
	memcpy(m_old_registers, m_registers, sizeof(m_registers));
	memcpy(m_old_segments, m_segments, sizeof(m_segments));
	memcpy(&m_old_flags, &m_flags, sizeof(m_flags));

	LOCK = false;
	REPN = false;
	REP = false;
	m_segmentOverride = NONE;
	int times = 1;

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
			//case 0x66: /* operand SIZE override */ break;
			//case 0x67: /* address SIZE override */ break;
			//case 0x0F: /* SIMD stream */ break;
		default:
			OPCODE1 = data;
			goto opcode_;
		}
	}

opcode_:
	mem_edit.HighlightMin = (((uint32_t)m_segments[CS]) << 4) + IP - 1;
	mem_edit.HighlightMax = mem_edit.HighlightMin + 1;

	byte seg = 0;
	byte data = 0;
	byte reg = 0;
	bool condition = false;

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
		OPERAND_B += TestFlag<CF>() ? 1 : 0;
		RESULT = OPERAND_A + OPERAND_B;
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
			OPERAND_B += TestFlag<CF>() ? 1 : 0;
			RESULT = OPERAND_A + OPERAND_B;
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
		APPEND_DBG(",");
		APPEND_DBG (m_segNames[seg]);
		break;

	case 0x8E:
		CMD_NAME("MOV");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		seg = (MODREGRM & REG) >> 3;
		APPEND_DBG(m_segNames[seg]);
		APPEND_DBG(",");
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
	case 0xE8:
		CMD_NAME("CALL");
		{
			uint16_t offset = GetIMM16();
			Push(IP);
			IP += offset;
			APPEND_HEX_DBG(m_segments[CS]);
			APPEND_DBG(":");
			APPEND_HEX_DBG(IP);
		}
		break;

	case 0xFE: case 0xFF:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPERAND_A = GetRM();

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

		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;

	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		CMD_NAME("INC");
		ADDRESS_METHOD = r16;
		OPERAND_A = GetReg(OPCODE1 & 0x07);
		OPERAND_B = 1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		CMD_NAME("DEC");
		ADDRESS_METHOD = r16;
		OPERAND_A = GetReg(OPCODE1 & 0x07);
		OPERAND_B = -1;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_OFSFZFAFPF();
		SetReg(OPCODE1 & 0x07, RESULT);
		break;

	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		CMD_NAME("PUSH");
		ADDRESS_METHOD = r16;
		OPERAND_A = GetReg(OPCODE1 & 0x07);
		Push(OPERAND_A);
		break;

	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		CMD_NAME("POP");
		ADDRESS_METHOD = r16;
		OPERAND_A = Pop();
		SetReg(OPCODE1 & 0x07, OPERAND_A);
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

	case 0x0F:
		CMD_NAME("POP CS");
		m_segments[CS] = Pop();
		break;

	case 0x1F:
		CMD_NAME("POP DS");
		m_segments[DS] = Pop();
		break;

	case 0xF6: case 0xF7:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPCODE2 = (MODREGRM & REG) >> 3;

		switch (OPCODE2)
		{
		case 0x04:
			CMD_NAME("MUL");
			OPERAND_A = (uint16_t)GetReg(0);
			APPEND_DBG(", ");
			OPERAND_B = (uint16_t)GetRM();
			RESULT = OPERAND_A * OPERAND_B;
			if (Byte())
			{
				m_registers[0] = RESULT;
			}
			else
			{
				m_registers[0] = RESULT;
				m_registers[DX] = RESULT >> 16;
			}
			UpdateFlags_CF();
			SetFlag<OF>(TestFlag<CF>());
			break;
		case 0x05:
			CMD_NAME("IMUL");
			OPERAND_A = (int16_t)GetReg(0);
			APPEND_DBG(", ");
			OPERAND_B = (int16_t)GetRM();
			RESULT = OPERAND_A * OPERAND_B;
			if (Byte())
			{
				m_registers[0] = RESULT;
			}
			else
			{
				m_registers[0] = RESULT;
				m_registers[DX] = RESULT >> 16;
			}
			UpdateFlags_CF();
			SetFlag<OF>(TestFlag<CF>());
			break;

		case 0x06:
			CMD_NAME("DIV");
			if (Byte())
			{
				OPERAND_A = (uint16_t)GetReg(0);
				APPEND_DBG(", ");
				OPERAND_B = (uint16_t)GetRM();
				m_registers[0] = OPERAND_A / OPERAND_B + ((OPERAND_A % OPERAND_B) << 8);
			}
			else
			{
				OPERAND_A = (uint16_t)GetReg(0) + ((uint16_t)GetReg(DX) << 16);
				APPEND_DBG(", ");
				OPERAND_B = (uint16_t)GetRM();
				m_registers[0] = OPERAND_A / OPERAND_B;
				m_registers[DX] = OPERAND_A % OPERAND_B;
			}
			break;
		case 0x07:
			CMD_NAME("IDIV");
			if (Byte())
			{
				OPERAND_A = (int16_t)GetReg(0);
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();
				m_registers[0] = OPERAND_A / OPERAND_B + ((OPERAND_A % OPERAND_B) << 8);
			}
			else
			{
				OPERAND_A = (int16_t)GetReg(0) + ((int16_t)GetReg(DX) << 16);
				APPEND_DBG(", ");
				OPERAND_B = (int16_t)GetRM();
				m_registers[0] = OPERAND_A / OPERAND_B;
				m_registers[DX] = OPERAND_A % OPERAND_B;
			}
			break;

		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
		break;


	case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		OPCODE2 = (MODREGRM & REG) >> 3;

		if (OPCODE2 & 0x02)
		{
			times = GetRegB(CL) & 0x1F;
		}

		switch (OPCODE2)
		{
		case 0x04:
			CMD_NAME("SHL");
			OPERAND_A = GetRM();
			RESULT = OPERAND_A << times;
			SetRM(RESULT);
			UpdateFlags_CF();
			break;
		case 0x05:
			CMD_NAME("SHR");
			OPERAND_A = GetRM();
			RESULT = OPERAND_A >> times;
			SetRM(RESULT);
			UpdateFlags_CF();
			break;
		case 0x06:
			CMD_NAME("SAL");
			OPERAND_A = (int16_t)GetRM();
			RESULT = OPERAND_A << times;
			SetRM(RESULT);
			UpdateFlags_CF();
			break;
		case 0x07:
			CMD_NAME("SAR");
			OPERAND_A = (int16_t)GetRM();
			RESULT = OPERAND_A >> times;
			SetRM(RESULT);
			UpdateFlags_CF();
			break;

		default:
			UNKNOWN_OPCODE(OPCODE2);
		}
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
			OPERAND_A = GetReg(0);
			APPEND_DBG(" [");
			ADDRESS = GetSegment(ES) << 4;
			APPEND_DBG(":");
			ADDRESS += m_registers[DI];
			APPEND_DBG("DI]");

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
			m_registers[CX]--;
		}

		break;

	case 0xC3:
		CMD_NAME("RET");
		IP = Pop();
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
			word src_offset = GetReg(SI);
			APPEND_DBG(", ");
			word dst_seg = GetSegment(ES);
			APPEND_DBG(":");
			word dst_offset = GetReg(DI);
			if (OPCODE1 & 1)
			{
				MemoryWord(_(dst_seg, dst_offset)) = MemoryWord(_(src_seg, src_offset));
				m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
				m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
			}
			else
			{
				MemoryByte(_(dst_seg, dst_offset)) = MemoryByte(_(src_seg, src_offset));
				m_registers[SI] += TestFlag<DF>() == 0 ? 1 : -1;
				m_registers[DI] += TestFlag<DF>() == 0 ? 1 : -1;
			}
			if (i != times - 1)
			{
	#ifdef _DEBUG
				*(dbg_args_ptr = dbg_args) = 0;
	#endif
			}
			m_registers[CX]--;
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
			APPEND_DBG(":");
			APPEND_HEX_DBG(src_offset);
			APPEND_DBG("]");
			if (OPCODE1 & 1)
			{
				OPERAND_A = MemoryWord(_(src_seg, src_offset));
				SetReg(AX, OPERAND_A);
			}
			else
			{
				OPERAND_A = MemoryByte(_(src_seg, src_offset));
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
			APPEND_DBG(":");
			APPEND_HEX_DBG(dst_offset);
			APPEND_DBG("], ");

			APPEND_DBG(OPCODE1 & 1 ? "AX" : "AL");

			if (OPCODE1 & 1)
			{
				MemoryWord(_(dst_seg, dst_offset)) = GetReg(AX);
			}
			else
			{
				MemoryByte(_(dst_seg, dst_offset)) = GetReg(AL);
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

		case 0xA6: case 0xA7:
			if (REP)
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
				word src_offset = GetReg(SI);
				APPEND_DBG(", ");
				word dst_seg = GetSegment(ES);
				APPEND_DBG(":");
				word dst_offset = GetReg(DI);
				if (OPCODE1 & 1)
				{
					OPERAND_A = MemoryWord(_(src_seg, src_offset));
					OPERAND_B = MemoryWord(_(dst_seg, dst_offset));
					RESULT = OPERAND_A - OPERAND_B;
					UpdateFlags_CFOFAF();
					UpdateFlags_SFZFPF();
					m_registers[SI] += TestFlag<DF>() == 0 ? 2 : -2;
					m_registers[DI] += TestFlag<DF>() == 0 ? 2 : -2;
				}
				else
				{
					OPERAND_A = MemoryByte(_(src_seg, src_offset));
					OPERAND_B = MemoryByte(_(dst_seg, dst_offset));
					RESULT = OPERAND_A - OPERAND_B;
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
				m_registers[CX]--;
			}
			break;
	default:
		UNKNOWN_OPCODE(OPCODE1);
	}

#ifdef _DEBUG
	m_log.appendf("%s %s %s\n", dbg_cmd_address, dbg_cmd_name, dbg_args);
#endif
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
			bool v = m_flags & (1 << i);
			ImGui::Checkbox(m_flagNames[i], &v);
			ImGui::SameLine();
			m_flags = (m_flags & ~(1 << i)) | (v ? (1 << i) : 0);
		}
	}

	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("log");
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
