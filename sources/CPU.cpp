#include "CPU.h"

#define CJMP if (condition) { int8_t rel8 = GetIMM8(); int16_t rel16 = rel8; IP += rel16; } break;

CPU::CPU()
{
	m_ram = new byte[0x100000];
	m_port = new byte[0x10000];
	memset(m_ram, 0, 0x100000);
	memset(m_port, 0, 0x10000);
	memset(m_segments, 0, sizeof(m_segments));
	memset(m_registers, 0, sizeof(m_registers));
	IP = 0;
	m_segments[CS] = 0x10;
}

void CPU::Step()
{
#ifdef _DEBUG
	m_disasm = "";
	std::string cmd = n2hexstr(m_segments[CS]) + ':' + n2hexstr(IP) + ": ";
#endif

	LOCK = false;
	REPN = false;
	REP = false;
	m_segmentOverride = NONE;

	while (true)
	{
		byte data;
		data = ExtractByte();
		switch (data)
		{
		//case 0xF0: LOCK = true; break;
		//case 0xF2: REPN = true; break;
		//case 0xF3: REP = true; break;
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

	byte seg = 0;
	byte data = 0;
	byte reg = 0;
	bool condition = false;

	switch (OPCODE1)
	{
	case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:
		PREPEND_DBG("ADD ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:
		PREPEND_DBG("ADC ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		OPERAND_B += TestFlag<CF>() ? 1 : 0;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x20:case 0x21:case 0x22:case 0x23:case 0x24:case 0x25:
		PREPEND_DBG("AND ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A & OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
	case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35:
		PREPEND_DBG("XOR ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A ^ OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x08:case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:
		PREPEND_DBG("OR ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		RESULT = OPERAND_A | OPERAND_B;
		ClearFlags_CFOF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x18:case 0x19:case 0x1A:case 0x1B:case 0x1C:case 0x1D:
		PREPEND_DBG("SBB ");
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
		PREPEND_DBG("SUB ");
		ADDRESS_METHOD = OPCODE1 & 7;
		PrepareOperands();
		OPERAND_B = -OPERAND_B;
		RESULT = OPERAND_A + OPERAND_B;
		UpdateFlags_CFOFAF();
		UpdateFlags_SFZFPF();
		StoreResult();
		break;
	case 0x38:case 0x39:case 0x3A:case 0x3B:case 0x3C:case 0x3D:
		PREPEND_DBG("CMP ");
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
			PREPEND_DBG("ADD ");
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x01:
			PREPEND_DBG("OR ");
			RESULT = OPERAND_A | OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x02:
			PREPEND_DBG("ADC ");
			OPERAND_B += TestFlag<CF>() ? 1 : 0;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x03:
			PREPEND_DBG("SBB ");
			OPERAND_B += TestFlag<CF>() ? 1 : 0;
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x04:
			PREPEND_DBG("AND ");
			RESULT = OPERAND_A & OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x05:
			PREPEND_DBG("SUB ");
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x06:
			PREPEND_DBG("XOR ");
			RESULT = OPERAND_A ^ OPERAND_B;
			ClearFlags_CFOF();
			UpdateFlags_SFZFPF();
			StoreResult();
			break;
		case 0x07:
			PREPEND_DBG("CMP ");
			OPERAND_B = -OPERAND_B;
			RESULT = OPERAND_A + OPERAND_B;
			UpdateFlags_CFOFAF();
			break;
		}
		break;

	case 0x88:case 0x89:case 0x8A:case 0x8B:
		PREPEND_DBG("MOV ");
		ADDRESS_METHOD = OPCODE1 & 0x03;
		PrepareOperands();
		RESULT = OPERAND_B;
		StoreResult();
		break;

	case 0x8C:
		PREPEND_DBG("MOV ");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		seg = (MODREGRM & REG) >> 3;
		SetRM(m_segments[seg], true);
		APPEND_DBG(", " + std::string(m_segNames[seg]));
		break;
	
	case 0x8E:
		PREPEND_DBG("MOV ");
		ADDRESS_METHOD = r16;
		MODREGRM = ExtractByte();
		seg = (MODREGRM & REG) >> 3;
		APPEND_DBG(std::string(m_segNames[seg]) + ", ");
		m_segments[seg] = GetRM();
		break;

	case 0xB0:case 0xB1:case 0xB2:case 0xB3:case 0xB4:case 0xB5:case 0xB6:case 0xB7:
	case 0xB8:case 0xB9:case 0xBA:case 0xBB:case 0xBC:case 0xBD:case 0xBE:case 0xBF:
		PREPEND_DBG("MOV ");
		ADDRESS_METHOD = ((OPCODE1 & 0x08) == 0x0) ? r8 : r16;
		reg = OPCODE1 & 0x07;
		if (Byte())
		{
			APPEND_DBG(std::string(m_regNames[reg]) + ",  0x");
			byte data = GetIMM8();
			SetReg(reg, data);
			APPEND_DBG(n2hexstr(data));
		}
		else
		{
			APPEND_DBG(std::string(m_regNames[reg + 8]) + ",  0x");
			word data = GetIMM16();
			SetReg(reg, data);
			APPEND_DBG(n2hexstr(data));
		}
		break;

	case 0xC6:case 0xC7:
		PREPEND_DBG("MOV ");
		ADDRESS_METHOD = OPCODE1 & 1;
		MODREGRM = ExtractByte();
		SetRM_Address();
		if (Byte())
		{
			byte data = GetIMM8();
			SetRM(data);
			APPEND_DBG(",  0x" + n2hexstr(data));
		}
		else
		{
			word data = GetIMM16();
			SetRM(data);
			APPEND_DBG(",  0x" + n2hexstr(data));
		}
		break;

	case 0xCC:
		PREPEND_DBG("INT 3");
		INT(3);
		break;

	case 0xCD:
		data = GetIMM8();
		PREPEND_DBG("INT ");
		APPEND_DBG(n2hexstr(data));
		INT(data);
		break;

	case 0xCE:
		PREPEND_DBG("INTO");
		INT(4);
		break;

	case 0xE2:
		PREPEND_DBG("LOOP");
		--m_registers[CX];
		if (m_registers[CX] != 0)
		{
			int8_t rel8 = GetIMM8();
			int16_t rel16 = rel8;
			IP += rel16;
		}
		break;

	case 0xE1:
		PREPEND_DBG("LOOPZ");
		--m_registers[CX];
		if (m_registers[CX] != 0 && TestFlag<ZF>())
		{
			int8_t rel8 = GetIMM8();
			int16_t rel16 = rel8;
			IP += rel16;
		}
		break;

	case 0xE0:
		PREPEND_DBG("LOOPNZ");
		--m_registers[CX];
		if (m_registers[CX] != 0 && !TestFlag<ZF>())
		{
			int8_t rel8 = GetIMM8();
			int16_t rel16 = rel8;
			IP += rel16;
		}
		break;

	case 0xF8:
		PREPEND_DBG("CLC");
		ClearFlag<CF>();
		break;
	case 0xF9:
		PREPEND_DBG("STC");
		SetFlag<CF>();
		break;
	case 0xFC:
		PREPEND_DBG("CLD");
		ClearFlag<DF>();
		break;
	case 0xFD:
		PREPEND_DBG("STD");
		SetFlag<DF>();
		break;
	case 0xFA:
		PREPEND_DBG("CLI");
		ClearFlag<IF>();
		break;
	case 0xFB:
		PREPEND_DBG("STI");
		SetFlag<IF>();
		break;
	case 0xF5:
		PREPEND_DBG("CMC");
		SetFlag<CF>(!TestFlag<CF>());
		break;

	case 0x70:
		PREPEND_DBG("JO"); condition = TestFlag<OF>(); CJMP
	case 0x71:
		PREPEND_DBG("JNO"); condition = !TestFlag<OF>(); CJMP
	case 0x72:
		PREPEND_DBG("JB"); condition = TestFlag<CF>(); CJMP
	case 0x73:
		PREPEND_DBG("JNB"); condition = !TestFlag<CF>(); CJMP
	case 0x74:
		PREPEND_DBG("JE"); condition = TestFlag<ZF>(); CJMP
	case 0x75:
		PREPEND_DBG("JNE"); condition = !TestFlag<ZF>(); CJMP
	case 0x76:
		PREPEND_DBG("JBE"); condition = TestFlag<CF>() || TestFlag<ZF>(); CJMP
	case 0x77:
		PREPEND_DBG("JNBE"); condition = !TestFlag<CF>() && !TestFlag<ZF>(); CJMP
	case 0x78:
		PREPEND_DBG("JS"); condition = TestFlag<SF>(); CJMP
	case 0x79:
		PREPEND_DBG("JNS"); condition = !TestFlag<SF>(); CJMP
	case 0x7A:
		PREPEND_DBG("JP"); condition = TestFlag<PF>(); CJMP
	case 0x7B:
		PREPEND_DBG("JNP"); condition = !TestFlag<PF>(); CJMP
	case 0x7C:
		PREPEND_DBG("JL"); condition = TestFlag<SF>() != TestFlag<OF>(); CJMP
	case 0x7D:
		PREPEND_DBG("JNL"); condition = TestFlag<SF>() == TestFlag<OF>(); CJMP
	case 0x7E:
		PREPEND_DBG("JLE"); condition = TestFlag<ZF>() || (TestFlag<SF>() != TestFlag<OF>()); CJMP
	case 0x7F:
		PREPEND_DBG("JNLE"); condition = !TestFlag<ZF>() && (TestFlag<SF>() == TestFlag<OF>()); CJMP

	default:
		printf("Unknown opcode: 0x%s\n", n2hexstr(OPCODE1).c_str());
	}

	PREPEND_DBG(cmd);

#ifdef _DEBUG
	printf("%s\n", m_disasm.c_str());

	for (int i = 0; i < 8; ++i)
	{
		printf("%s=%s ", m_regNames[i + 8], n2hexstr(m_registers[i]).c_str());
	}
	printf("\n");
	for (int i = 0; i < 4; ++i)
	{
		printf("%s=%s ", m_segNames[i], n2hexstr(m_segments[i]).c_str());
	}
	printf("IP=%s ", n2hexstr(IP).c_str());
	printf("\n");

	printf("CF=%d ", TestFlag<CF>() ? 1 : 0);
	printf("PF=%d ", TestFlag<PF>() ? 1 : 0);
	printf("AF=%d ", TestFlag<AF>() ? 1 : 0);
	printf("ZF=%d ", TestFlag<ZF>() ? 1 : 0);
	printf("SF=%d ", TestFlag<SF>() ? 1 : 0);
	printf("TF=%d ", TestFlag<TF>() ? 1 : 0);
	printf("IF=%d ", TestFlag<IF>() ? 1 : 0);
	printf("DF=%d ", TestFlag<DF>() ? 1 : 0);
	printf("OF=%d ", TestFlag<OF>() ? 1 : 0);

	printf("\n\n");
#endif
}

void CPU::LoadData(uint32_t dest, const char* source, uint32_t size)
{
	memcpy(m_ram + dest, source, size);
}

void CPU::INT(byte n)
{
	byte* reg8file = (byte*)m_registers;
	switch (n)
	{
	case 3:
		return;
	case 0x10:
		byte function = reg8file[1];
		switch (function)
		{
		case 0x0: 
			byte videoMode = reg8file[0];
			return;
		}
	}
}