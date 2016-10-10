#pragma once
#include "CPU.h"
#include <vector>
#include <memory>

class Application
{
public:
	Application();
	~Application();

	int Init();
	void Test();
	
private:
	CPU m_cpu;
};