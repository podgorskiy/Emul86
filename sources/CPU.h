#pragma once
#include <inttypes.h>
#include <string>
#include <imgui.h>

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

inline uint32_t select(uint16_t seg, uint16_t off)
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
#define APPEND_HEXN_DBG(X, N)
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

	CPU();

	void Step();

	void GUI();

	void SetRamPort(byte* ram, byte* port);

	void SetInterruptHandler(InterruptHandler* interruptHandler);

	void Reset();

	byte GetRegB(byte i);

	word GetRegW(byte i);

	void SetRegB(byte i, byte x);

	void SetRegW(byte i, word x);

	word GetSegment(byte i);

	void SetSegment(byte i, word v);

	void INT(byte x);

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
		m_flags = m_flags & ~(uint16_t)(1 << F) | (x ? (1 << F) : 0);
	}

	template<Flags F>
	inline void SetFlag(uint32_t x)
	{
		SetFlag<F>(x != 0);
	}

	template<Flags F>
	inline void ClearFlag()
	{
		m_flags &= ~(uint16_t)(1 << F);
	}

	word GetReg(byte i);

	void SetReg(byte i, word x);

	void Interrupt(int n);

	word IP;

	void EnableA20Gate();
	void DisableA20Gate();
	bool GetA20GateStatus();

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

	inline void PrepareOperands(bool signextend = false);
	
	inline bool Byte();

	inline word GetReg();

	inline void SetReg(word x);

	inline uint8_t& MemoryByte(uint32_t address);

	inline uint16_t& MemoryWord(uint32_t address);

	inline uint8_t& PortByte(uint32_t address);

	inline uint16_t& PortWord(uint32_t address);

	inline word GetRM();

	inline void SetRM(word x, bool computeAddress = false);

	inline void SetRM_Address();
	
	inline void StoreResult();

	inline byte GetIMM8();
	
	inline word GetIMM16();
	
	inline byte ExtractByte();

	inline void UpdateFlags_CF();
	
	inline void UpdateFlags_OF();

	inline void UpdateFlags_OF_sub();

	inline void UpdateFlags_AF();

	inline void Flip_AF();
	
	inline void UpdateFlags_PF();

	inline void UpdateFlags_ZF();
	
	inline void UpdateFlags_SF();

	inline void UpdateFlags_CFOFAF();

	inline void UpdateFlags_CFOFAF_sub();

	inline void UpdateFlags_SFZFPF();

	inline void ClearFlags_CFOF();

	inline void UpdateFlags_OFSFZFAFPF();

	inline void UpdateFlags_OFSFZFAFPF_sub();

	inline void Push(word x);

	inline word Pop();

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
	int32_t EFFECTIVE_ADDRESS;

	byte OPCODE1;
	byte OPCODE2;

	int32_t OPERAND_A, OPERAND_B;
	uint32_t RESULT;

	Segments m_segmentOverride;
	
	InterruptHandler* m_interruptHandler;

	char dbg_cmd_address[128];
	char dbg_cmd_name[128];
	char* dbg_args_ptr;
	char dbg_args[128];

	ImGuiTextBuffer m_log;
	bool m_scrollToBottom;

	int A20;

	static const uint8_t parity[0x100];
};
