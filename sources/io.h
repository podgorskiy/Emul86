#pragma once
#include "Disk.h"
#include "common.h"
#include <vector>
#include <list>
#include <chrono>

class IO
{
public:
	void Init();
	void DrawScreen(int m_displayScale);

	// Keyboard
	std::pair<bool, word> UpdateKeyState();
	void KeyboardHalt();
	bool ISKeyboardHalted() const;
	bool SetCurrentKey(int c);
	void PushKey(int c);
	bool HasKeyToPop() const;
	int PopKey();
	void ClearCurrentCode();
	void SetCurrentScanCode(int c);
	void SetKeyFlags(bool rightShift, bool leftShift, bool CTRL, bool alt);
	int GetCurrentKeyCode() const;

	// Disk
	void AddDisk(Disk disk);
	int GetDiskCount() const;
	bool HasDisk(int diskNo) const;
	bool ReadDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount);
	bool WriteDisk(int diskNo, uint32_t ram_address, word cylinder, byte head, byte sector, uint32_t sectorCount);
	const Disk::BIOS_ParameterBlock& GetDiskBPB(int diskNo) const;
	int GetBootableDisk() const;

	// Memory/Ports
	void ClearMemory();
	void MemStore(word offset, word location, byte* x, int size);
	void MemStoreB(word offset, word location, byte x);
	void MemStoreW(word offset, word location, word x);
	void MemStoreD(word offset, word location, uint32_t x);
	void MemStoreString(word offset, word location, const char* x);

	byte MemGetB(word offset, word location) const;
	word MemGetW(word offset, word location) const;
	uint32_t MemGetD(word offset, word location) const;

	template<typename T>
	void SetMemory(uint32_t address, T x);

	template<typename T>
	T GetMemory(uint32_t address) const;

	template<typename T>
	inline T& Port(uint32_t address);

	byte* GetRamRawPointer();
	
	// A20 gate
	void EnableA20Gate();
	void DisableA20Gate();
	bool GetA20GateStatus() const;

private:
	std::vector<Disk> m_floppyDrives;
	std::vector<Disk> m_hardDrives;
	std::list<int> keyBuffer;

	byte* m_ram;
	byte* m_port;
	int A20;
	int m_currentKeyCode;
	int m_keyFlags;
	bool m_int16_halt;

	std::chrono::time_point<std::chrono::system_clock> m_start;
};

#ifdef ENABLE_A20
#define A20_GATE(X) (X & A20)
#else
#define A20_GATE(X) X
#endif


template<>
inline void IO::SetMemory<byte>(uint32_t address, byte x)
{
	m_ram[A20_GATE(address)] = x;
}

template<>
inline byte IO::GetMemory<byte>(uint32_t address) const
{
	return m_ram[A20_GATE(address)];
}

template<>
inline void IO::SetMemory<word>(uint32_t address, word x)
{
	m_ram[A20_GATE(address)] = x & 0xFF;
	m_ram[A20_GATE(address) + 1] = (x & 0xFF00) >> 8;
}

template<>
inline word IO::GetMemory<word>(uint32_t address) const
{
	return ((word)m_ram[A20_GATE(address)]) | (((word)m_ram[A20_GATE(address) + 1]) << 8);
}

template<>
inline byte& IO::Port<byte>(uint32_t address)
{
	return *reinterpret_cast<byte*>(m_ram + A20_GATE(address));
}

template<>
inline word& IO::Port<word>(uint32_t address)
{
	return *reinterpret_cast<word*>(m_ram + A20_GATE(address));
}
