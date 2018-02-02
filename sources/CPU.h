#pragma once
#include <inttypes.h>
#include <string>
#include <imgui.h>

extern const uint8_t parity[0x100];

template <typename I> 
inline std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

template <typename I>
inline char* n2hexstr(char* dst, I w, size_t hex_len = sizeof(I) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	memset(dst, '0', hex_len);
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		dst[i] = digits[(w >> j) & 0x0f];
	return &(*(dst + hex_len) = '\x0');
}

inline uint32_t _(uint16_t seg, uint16_t off)
{
	return (((uint32_t)seg) << 4) + off;
}

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
#define APPEND_HEXN_DBG(X)
#endif

#ifdef _DEBUG
#define CMD_NAME(X) strcpy(dbg_cmd_name, X);
#else
#define CMD_NAME(X)
#endif

class CPU
{
public:
	typedef uint16_t word;
	typedef uint8_t byte;

	class InterruptHandler
	{
	public:
		virtual void Int(byte x) = 0;
	};

	enum Registers
	{
		AL = 0, CL, DL, BL, AH, CH, DH, BH,
		AX = 0, CX, DX, BX, SP, BP, SI, DI
	};
	enum Segments
	{
		ES = 0, CS = 1, SS = 2, DS = 3, NONE = 4
	};

	CPU();

	void Step();

	void GUI();

	void SetRamPort(byte* ram, byte* port)
	{
		m_ram = ram;
		m_port = port;
	}

	void SetInterruptHandler(InterruptHandler* interruptHandler)
	{
		m_interruptHandler = interruptHandler;
	}

	inline byte GetRegB(byte i)
	{
		byte* reg8file = (byte*)m_registers;
		byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
		return reg8file[_i];
	}
	inline word GetRegW(byte i)
	{
		return m_registers[i];
	}
	inline void SetRegB(byte i, byte x)
	{
		byte* reg8file = (byte*)m_registers;
		byte _i = (i & 0x3) << 1 | ((i & 0x4) >> 2);
		reg8file[_i] = x;
	}
	inline void SetRegW(byte i, word x)
	{
		m_registers[i] = x;
	}
	inline word GetSeg(byte i)
	{
		return m_segments[i];
	}
	inline void SetSegment(byte i, word v)
	{
		m_segments[i] = v;
	}
	
	void INT(byte x)
	{
		m_interruptHandler->Int(x);
	}

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

	inline word GetReg(byte i)
	{
		if (Byte())
		{
			APPEND_DBG(m_regNames[i]);
			return GetRegB(i);
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
			SetRegB(i, x & 0xFF);
		}
		else
		{
			m_registers[i] = x;
		}
	}

	void Interrupt(int n);

	word IP;

private:
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
		MOD = 0b11000000,
		REG = 0b00111000,
		RM  = 0b00000111
	};

	
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
				APPEND_HEX_DBG(data);
			}
			else
			{
				OPERAND_B = GetIMM16();
				APPEND_HEX_DBG(OPERAND_B);
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
					OPERAND_B = data16;
					APPEND_HEX_DBG(data);
				}
				else
				{
					OPERAND_B = GetIMM16();
					APPEND_HEX_DBG(OPERAND_B);
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

	inline uint8_t& MemoryByte(uint32_t address)
	{
		return *(m_ram + address);
	}

	inline uint16_t& MemoryWord(uint32_t address)
	{
		return *reinterpret_cast<uint16_t*>(m_ram + address);
	}
	inline uint8_t& PortByte(uint32_t address)
	{
		return *(m_port + address);
	}
	inline uint16_t& PortWord(uint32_t address)
	{
		return *reinterpret_cast<uint16_t*>(m_port + address);
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
				return MemoryByte(ADDRESS);
			}
			else
			{
				return MemoryWord(ADDRESS);
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
				MemoryByte(ADDRESS) = x;
			}
			else
			{
				MemoryWord(ADDRESS) = x;
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
			offsetreg = m_segmentOverride;
		}
		APPEND_DBG(m_segNames[offsetreg]);
		APPEND_DBG(": ");
		switch (MODREGRM & RM)
		{
			case 0: reg = (int32_t)m_registers[BX] + m_registers[SI]; APPEND_DBG("BX + SI"); break;
			case 1: reg = (int32_t)m_registers[BX] + m_registers[DI]; APPEND_DBG("BX + DI"); break;
			case 2: reg = (int32_t)m_registers[BP] + m_registers[SI]; offsetreg = BP;  APPEND_DBG("BP + SI"); break;
			case 3: reg = (int32_t)m_registers[BP] + m_registers[DI]; offsetreg = BP;  APPEND_DBG("BP + DI"); break;
			case 4: reg = (int32_t)m_registers[SI]; APPEND_DBG("SI"); break;
			case 5: reg = (int32_t)m_registers[DI]; APPEND_DBG("DI"); break;
			case 6: if ((MODREGRM & MOD) != 0) { reg = m_registers[BP]; offsetreg = BP; APPEND_DBG("BP")  } else { reg = GetIMM16(); APPEND_HEXN_DBG(reg, 4); } break;
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
		ADDRESS = _(m_segments[offsetreg], reg + disp);
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
			break;
		case AXL_IMM:
			SetReg(0, RESULT);
			break;
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

	inline void UpdateFlags_AF()
	{
		byte a = (OPERAND_A & 0xf);
		byte b = (OPERAND_B & 0xf);
		a += b;
		SetFlag<AF>((a & 0x10) != 0);
	}
	
	inline void UpdateFlags_PF()
	{
		//byte b = RESULT;
		//b ^= b >> 4;
		//b ^= b >> 2;
		//b ^= b >> 1;
		SetFlag<PF>(parity[RESULT & 0xFF]);
	}

	inline void UpdateFlags_ZF()
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
	inline void UpdateFlags_OFSFZFAFPF()
	{
		UpdateFlags_OF();
		UpdateFlags_SF();
		UpdateFlags_ZF();
		UpdateFlags_AF();
		UpdateFlags_PF();
	}
	inline void Push(word x)
	{
		m_registers[SP] -= 2;
		uint32_t address = _(m_segments[SS], m_registers[SP]);
		MemoryWord(address) = x;
	}
	inline word Pop()
	{
		uint32_t address = _(m_segments[SS], m_registers[SP]);
		word x = MemoryWord(address);
		m_registers[SP] += 2;
		return x;
	}

	const char* const m_segNames[4] = { "ES", "CS", "SS", "DS" };
	const char* const m_regNames[2 * 8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX","CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	const char* const m_flagNames[2 * 8] = { "CF", "", "PF", "", "AF", "", "ZF", "SF", "TF","IF", "DF", "OF", "", "", "NT", "" };

	word m_registers[8];
	word m_old_registers[8];
	word m_segments[4];
	word m_old_segments[4];

	byte* m_ram;
	byte* m_port;
	word m_flags;
	word m_old_flags;

	byte ADDRESS_METHOD;
	byte MODREGRM;
	int32_t ADDRESS;

	byte OPCODE1;
	byte OPCODE2;

	int32_t OPERAND_A, OPERAND_B;
	uint32_t RESULT;

	Segments m_segmentOverride;

	bool LOCK;
	bool REPN;
	bool REP;

	InterruptHandler* m_interruptHandler;

	char dbg_cmd_address[128];
	char dbg_cmd_name[128];
	char* dbg_args_ptr;
	char dbg_args[128];

	ImGuiTextBuffer m_log;
	bool m_scrollToBottom;
};
