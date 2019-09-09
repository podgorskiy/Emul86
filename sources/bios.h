#pragma once
#include "CPU.h"
#include "io_layer.h"
#include <stdint.h>

class BIOS: public CPU::InterruptHandler
{
public:
	typedef uint16_t word;
	typedef uint8_t byte;

	BIOS(IO& io, CPU& cpu) : io(io), m_cpu(cpu)
	{}

	void InitBIOSDataArea();

private:
	virtual void Int(byte x) override;

	void SetInterruptVector(int interrupt, word segment, word offset);
	void scrollOneLine();

	void Int10VideoServices(int function, int arg);
	void Int13DiskIOServices(int function, int arg);
	void Int14AsynchronousCommunicationsServices(int function, int arg);
	void Int15SystemBIOSServices(int function, int arg);
	void Int16KeyboardServices(int function, int arg);
	void Int17PrinterBIOSServices(int function, int arg);
	void Int1ARealTimeClockServices(int function, int arg);

	IO& io;
	CPU& m_cpu;
};
