#pragma once
#include "common.h"
#include "io.h"
#include <string>
#include <imgui.h>


class CPU
{
	enum DataDirection
	{
		RM_REG = 0 << 1,
		REG_RM = 1 << 1,
		AXL_IMM = 2 << 1,
		RM_IMM = 3 << 1,
		DataDirectionMask = RM_REG | REG_RM | AXL_IMM | RM_IMM
	};

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
	
	template<typename T>
	T GetRegister();

	template<typename T>
	void SetRegister(T v);

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
	
	enum ModRegRm
	{
		MOD = 0b11000000,
		REG = 0b00111000,
		RM  = 0b00000111
	};

	void StepInternal();
	
	template<typename T>
	void PrepareOperands_RM_REG();

	template<typename T>
	void PrepareOperands_REG_RM();

	template<typename T>
	void PrepareOperands_AXL_IMM();

	template<typename T>
	void PrepareOperands_AXL_RM_IMM();

	void PrepareOperands_AXL_RM_IMM_signextend();

	template<typename T>
	T GetRM();

	template<typename T>
	void SetRM(T x, bool computeAddress = false);

	void SetRM_Address();

	template<typename T>
	void StoreResult_RM();

	template<typename T>
	void StoreResult_REG();

	template<typename T>
	void StoreResult_AXL();

	template<typename T>
	T GetImm();

	template<typename T>
	void UpdateFlags_CF();

	template<typename T>
	void UpdateFlags_OF();

	template<typename T>
	void UpdateFlags_OF_sub();

	void UpdateFlags_AF();

	void UpdateFlags_PF();

	template<typename T>
	void UpdateFlags_ZF();

	template<typename T>
	void UpdateFlags_SF();

	template<typename T>
	void UpdateFlags_CFOFAF();

	template<typename T>
	void UpdateFlags_CFOFAF_sub();

	template<typename T>
	void UpdateFlags_SFZFPF();

	void ClearFlags_CFOF();

	template<typename T>
	void UpdateFlags_OFSFZFAFPF();

	template<typename T>
	void UpdateFlags_OFSFZFAFPF_sub();

	void Push(word x);

	word Pop();

	// Commands
	template<typename T>
	void ADD();

	template<typename T>
	void ADC();

	template<typename T>
	void AND();

	template<typename T>
	void XOR();

	template<typename T>
	void OR();

	template<typename T>
	void SBB();

	template<typename T>
	void SUB();

	template<typename T>
	void CMP();

	template<typename T>
	void MOV_rm_imm();

	template<typename T>
	void MOV_reg_imm();
	
	template<typename T>
	void MOV_a_off();

	template<typename T>
	void MOV_off_a();

	template<typename T>
	void LODS();

	template<typename T>
	void STOS();

	template<typename T>
	void SCAS();

	template<typename T>
	void CMPS();

	template<typename T>
	void MOVS();

	template<typename T>
	void XCHG();

	template<typename T>
	void TEST_reg_rm();

	template<typename T>
	void TEST_a_imm();

	void LOOP();
	void LOOPZ();
	void LOOPNZ();

	template<typename T>
	void MUL();

	template<typename T>
	void IMUL();

	template<typename T>
	void DIV();

	template<typename T>
	void IDIV();

	template<typename T>
	void IN_imm();

	template<typename T>
	void IN_dx();

	template<typename T>
	void OUT_imm();

	template<typename T>
	void OUT_dx();

	template<typename T>
	void INS();

	template<typename T>
	void OUTS();

	template<typename T>
	void Group1();

	template<typename T>
	void Group2(int times);

	template<typename T>
	void Group3();

	template<typename T>
	void Group45();

	void Group0F();

	template<typename T>
	word GetStep();

	template<typename T>
	const char* GetRegName(int i);

	const char* const m_segNames[4] = { "ES", "CS", "SS", "DS" };
	const char* const m_regNames[2 * 8] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX","CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	const char* const m_flagNames[2 * 8] = { "CF", "", "PF", "", "AF", "", "ZF", "SF", "TF","IF", "DF", "OF", "", "", "NT", "" };

	word m_registers[8];
	word m_old_registers[8];
	word m_segments[4];
	word m_old_segments[4];

	bool m_LOCK;
	bool m_REPN;
	bool m_REP;

	IO& m_io;
	word m_flags;
	word m_old_flags;

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
	byte _i = 2 * i - (i / 4) * 7;
	return reg8file[_i];
}

template <>
inline void CPU::SetRegister<byte>(byte i, byte v)
{
	byte* reg8file = (byte*)m_registers;
	byte _i = 2 * i - (i / 4) * 7;
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
