#pragma once
#include "common.h"
#include "io.h"
#include <string>
#include <imgui.h>


class CPU
{
public:
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

	CPU(IO& io);

	void Step();

	void GUI();

	void SetInterruptHandler(InterruptHandler* interruptHandler);

	void Reset();

	template <typename T>
	T GetRegister(byte i) const;

	template <typename T>
	void SetRegister(byte i, T v);

	word GetSegment(byte i) const;

	void SetSegment(byte i, word v);

	void INT(byte x);

	template<Flags F>
	bool TestFlag()
	{
		return (m_flags & (1 << F)) != 0;
	}

	template<Flags F>
	void SetFlag()
	{
		m_flags |= (1 << F);
	}

	template<Flags F>
	void SetFlag(bool x)
	{
		m_flags = m_flags & ~(uint16_t)(1 << F) | (x ? (1 << F) : 0);
	}

	template<Flags F>
	void SetFlag(uint32_t x)
	{
		SetFlag<F>(x != 0);
	}

	template<Flags F>
	void ClearFlag()
	{
		m_flags &= ~(uint16_t)(1 << F);
	}

	word GetReg(byte i);

	void SetReg(byte i, word x);

	void Interrupt(int n);

	word IP;
	
	const char* GetDebugString();
	const char* GetLastCommandAsm();
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

	void PrepareOperands(bool signextend = false);
	
	bool Byte();

	word GetReg();

	void SetReg(word x);

	word GetRM();

	void SetRM(word x, bool computeAddress = false);

	void SetRM_Address();
	
	void StoreResult();

	template<typename T>
	T GetImm();
	
	void UpdateFlags_CF();
	
	void UpdateFlags_OF();

	void UpdateFlags_OF_sub();

	void UpdateFlags_AF();

	void Flip_AF();
	
	void UpdateFlags_PF();

	void UpdateFlags_ZF();
	
	void UpdateFlags_SF();

	void UpdateFlags_CFOFAF();

	void UpdateFlags_CFOFAF_sub();

	void UpdateFlags_SFZFPF();

	void ClearFlags_CFOF();

	void UpdateFlags_OFSFZFAFPF();

	void UpdateFlags_OFSFZFAFPF_sub();

	void Push(word x);

	word Pop();

	const char* const m_segNames[4] = { "ES", "CS", "SS", "DS" };
	const char* const m_regNames[2 * 8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX","CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	const char* const m_flagNames[2 * 8] = { "CF", "", "PF", "", "AF", "", "ZF", "SF", "TF","IF", "DF", "OF", "", "", "NT", "" };

	word m_registers[8];
	word m_old_registers[8];
	word m_segments[4];
	word m_old_segments[4];

	IO& m_io;
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

	char dbg_buff[1024];

	ImGuiTextBuffer m_log;
	bool m_scrollToBottom;
	
	static const uint8_t parity[0x100];
};


inline word CPU::GetSegment(byte i) const
{
	return m_segments[i];
}


inline void CPU::SetSegment(byte i, word v)
{
	m_segments[i] = v;
}

template <>
inline byte CPU::GetRegister<byte>(byte i) const
{
	byte* reg8file = (byte*)m_registers;
	byte _i = ((i * 2) & 0x06) | ((i / 4) & 1);
	return reg8file[_i];
}

template <>
inline void CPU::SetRegister<byte>(byte i, byte v)
{
	byte* reg8file = (byte*)m_registers;
	byte _i = ((i * 2) & 0x06) | ((i / 4) & 1);
	reg8file[_i] = v;
}

template <>
inline word CPU::GetRegister<word>(byte i) const
{
	return m_registers[i];
}

template <>
inline void CPU::SetRegister<word>(byte i, word v)
{
	m_registers[i] = v;
}
