#pragma once
#include <inttypes.h>
#include <string>

template <typename I> 
inline std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

#ifdef _DEBUG
#define APPEND_DBG(X) m_disasm += X;
#else
#define APPEND_DBG(X)
#endif

#ifdef _DEBUG
#define PREPEND_DBG(X) m_disasm = X + m_disasm;
#else
#define PREPEND_DBG(X)
#endif

class CPU
{
public:
	CPU();
	void Step();
	void LoadData(uint32_t dest, const char* source, uint32_t size);
private:
	typedef uint16_t word;
	typedef uint8_t byte;

	enum Segments
	{
		ES = 0, CS = 1, SS = 2, DS = 3, NONE = 4
	};
	enum Registers
	{
		AL = 0, CL, DL, BL, AH, CH, DH, BH,
		AX = 0, CX, DX, BX, SP, BP, SI, DI
	};

	enum DataSize
	{
		r8 = 0,
		r16 = 1,
		rMask = r8 | r16
	};

	enum DataDirection
	{
		RM_REG = 0 << 1,
		REG_RM = 1 << 1,
		AXL_IMM = 2 << 1,
		RM_IMM = 3 << 1,
		DataDirectionMask = RM_REG | REG_RM | AXL_IMM | RM_IMM
	};

	enum ModRegRm
	{
		MOD = 0xC0,
		REG = 0x38,
		RM = 0x07
	};

	enum Flags
	{
		CF = 0,
		PF = 2,
		AF = 4,
		ZF = 6,
		SF = 7,
		TF = 8,
		IF = 9,
		DF = 10,
		OF = 11,
		IOPL_1 = 12,
		IOPL_2 = 13,
		NT = 14,
		RESERVERD = 15
	};

	template<Flags F>
	inline bool TestFlag()
	{
		return (m_flags & (1 << F)) != 0;
	}

	template<Flags F>
	inline void SetFlag()
	{
		m_flags |= (1 << F);
	}

	template<Flags F>
	inline void SetFlag(bool x)
	{
		m_flags = m_flags & ~(1 << F) | (x ? (1 << F) : 0);
	}

	template<Flags F>
	inline void ClearFlag()
	{
		m_flags &= ~(1 << F);
	}
	
	inline void PrepareOperands(bool signextend = false)
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
			OPERAND_A = GetReg(0);
			APPEND_DBG(", 0x");
			if (Byte())
			{
				byte data = GetIMM8();
				OPERAND_B = data;
				APPEND_DBG(n2hexstr(data));
			}
			else
			{
				OPERAND_B = GetIMM16();
				APPEND_DBG(n2hexstr(OPERAND_B));
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
				APPEND_DBG(n2hexstr(data));
			}
			else
			{
				if (signextend)
				{
					int8_t data = GetIMM8();
					int16_t data16 = data;
					OPERAND_B = data16;
					APPEND_DBG(n2hexstr(data));
				}
				else
				{
					OPERAND_B = GetIMM16();
					APPEND_DBG(n2hexstr(OPERAND_B));
				}
			}
			break;
		}
	}

	inline word GetSegment(byte i)
	{
		APPEND_DBG(m_segNames[i]);
		return m_segments[i];
	}

	inline void SetSegment(byte i, word v)
	{
		m_segments[i] = v;
	}

	inline bool Byte()
	{
		return (ADDRESS_METHOD & rMask) == r8;
	}

	inline word GetReg()
	{
		return GetReg((MODREGRM & REG) >> 3);
	}

	inline void SetReg(word x)
	{
		SetReg((MODREGRM & REG) >> 3, x);
	}

	inline word GetReg(byte i)
	{
		if (Byte())
		{
			APPEND_DBG(m_regNames[i]);
			byte* reg8file = (byte*)m_registers;
			byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
			return reg8file[_i];
		}
		else
		{
			APPEND_DBG(m_regNames[i + 8]);
			return m_registers[i];
		}
	}

	inline void SetReg(byte i, word x)
	{
		if (Byte())
		{
			byte* reg8file = (byte*)m_registers;
			byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
			reg8file[_i] = x & 0xFF;
		}
		else
		{
			m_registers[i] = x;
		}
	}

	inline word GetRM()
	{
		if ((MODREGRM & MOD) == MOD)
		{
			return GetReg(MODREGRM & RM);
		}
		else
		{
			SetRM_Address();

			if (Byte())
			{
				return m_ram[ADDRESS];
			}
			else
			{
				return m_ram[ADDRESS] | (m_ram[ADDRESS + 1] << 8);
			}
		}
	}

	inline void SetRM(word x, bool computeAddress = false)
	{
		if ((MODREGRM & MOD) == MOD)
		{
			SetReg(MODREGRM & RM, x);
		}
		else
		{
			if (computeAddress)
			{
				SetRM_Address();
			}

			if (Byte())
			{
				m_ram[ADDRESS] = x & 0xFF;
			}
			else
			{
				m_ram[ADDRESS] = x & 0xFF;
				m_ram[ADDRESS + 1] = (x & 0xFF00) >> 8;
			}
		}
	}

	inline void SetRM_Address()
	{
		int32_t reg = 0;
		int32_t disp = 0;
		byte offsetreg = DS;
		APPEND_DBG("[");
		if (m_segmentOverride != NONE)
		{
			APPEND_DBG(std::string(m_segNames[m_segmentOverride]) + ": ");
		}
		switch (MODREGRM & RM)
		{
			case 0: reg = (int32_t)m_registers[BX] + m_registers[SI]; APPEND_DBG("BX + SI"); break;
			case 1: reg = (int32_t)m_registers[BX] + m_registers[DI]; APPEND_DBG("BX + DI"); break;
			case 2: reg = (int32_t)m_registers[BP] + m_registers[SI]; offsetreg = BP;  APPEND_DBG("BP + SI"); break;
			case 3: reg = (int32_t)m_registers[BP] + m_registers[DI]; offsetreg = BP;  APPEND_DBG("BP + DI"); break;
			case 4: reg = (int32_t)m_registers[SI]; APPEND_DBG("SI"); break;
			case 5: reg = (int32_t)m_registers[DI]; APPEND_DBG("DI"); break;
			case 6: if ((MODREGRM & MOD) != 0) { reg = m_registers[BP]; offsetreg = BP; APPEND_DBG("BP")  } else { reg = GetIMM16(); APPEND_DBG(n2hexstr(reg, 4)); } break;
			case 7: reg = m_registers[BX]; APPEND_DBG("BX") break;
		}
		if ((MODREGRM & MOD) == 0x40)
		{
			int8_t disp8 = GetIMM8();
			APPEND_DBG(" + 0x" + n2hexstr(disp8));
			disp = disp8; // sign extension
		}
		else if ((MODREGRM & MOD) == 0x80)
		{
			disp = GetIMM16();
			APPEND_DBG(" + 0x" + n2hexstr(disp));
		}
		APPEND_DBG("]");
		if (m_segmentOverride != NONE)
		{
			offsetreg = m_segmentOverride;
		}
		ADDRESS = reg + disp + (int32_t)m_segments[offsetreg];
	}
	
	inline void StoreResult()
	{
		switch (ADDRESS_METHOD & DataDirectionMask)
		{
		case RM_IMM:
		case RM_REG:
			SetRM(RESULT);
			break;
		case REG_RM:
			SetReg((MODREGRM & REG) >> 3, RESULT);
		case AXL_IMM:
			SetReg(0, RESULT);
		}
	}

	inline byte GetIMM8()
	{
		return ExtractByte();
	}
	inline word GetIMM16()
	{
		word result = ExtractByte();
		result |= ExtractByte() << 8;
		return result;
	}
	inline byte ExtractByte()
	{
		uint32_t address = m_segments[CS];
		address = (address << 4) + IP++;
		return m_ram[address];
	}
	inline void UpdateFlags_CF()
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
	inline void UpdateFlags_OF()
	{
		if (Byte())
		{
			bool a = (OPERAND_A & 0x80) != 0;
			bool b = (OPERAND_B & 0x80) != 0;
			bool r = (RESULT & 0x80) != 0;
			SetFlag<OF>((!a && !b && r) || (a && b && !r));
		}
		else
		{
			bool a = (OPERAND_A & 0x8000) != 0;
			bool b = (OPERAND_B & 0x8000) != 0;
			bool r = (RESULT & 0x8000) != 0;
			SetFlag<OF>((!a && !b && r) || (a && b && !r));
		}
	}
	inline void UpdateFlags_AF()
	{
		byte a = (OPERAND_A & 0xf);
		byte b = (OPERAND_B & 0xf);
		a += b;
		SetFlag<AF>((a & 0x10) != 0);
	}
	inline void UpdateFlags_PF()
	{
		byte b = RESULT;
		b ^= b >> 4;
		b ^= b >> 2;
		b ^= b >> 1;
		SetFlag<PF>((~b) & 1);
	}
	inline void UpdateFlags_ZF()
	{
		SetFlag<ZF>(RESULT == 0);
	}
	inline void UpdateFlags_SF()
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
	inline void UpdateFlags_CFOFAF()
	{
		UpdateFlags_CF();
		UpdateFlags_OF();
		UpdateFlags_AF();
	}
	inline void UpdateFlags_SFZFPF()
	{
		UpdateFlags_SF();
		UpdateFlags_ZF();
		UpdateFlags_PF();
	}
	inline void ClearFlags_CFOF()
	{
		ClearFlag<CF>();
		ClearFlag<OF>();
	}

	void INT(byte n);

	const char* const m_segNames[4] = { "ES", "CS", "SS", "DS" };
	const char* const m_regNames[2 * 8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX","CX", "DX", "BX", "SP", "BP", "SI", "DI" };

	word m_registers[8];
	word m_segments[4];

	byte* m_ram;
	byte* m_port;
	word m_flags;

	byte ADDRESS_METHOD;
	byte MODREGRM;
	int32_t ADDRESS;
	word IP;

	byte OPCODE1;
	byte OPCODE2;

	int32_t OPERAND_A, OPERAND_B;
	uint32_t RESULT;

	Segments m_segmentOverride;

	bool LOCK;
	bool REPN;
	bool REP;

	std::string m_disasm;
};