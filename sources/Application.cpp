#include "Application.h"

Application::Application()
{
}

int Application::Init()
{
	FILE* f = fopen("test.COM", "rb");
	fseek(f, 0L, SEEK_END);
	size_t s = ftell(f);
	fseek(f, 0L, SEEK_SET);
	char* buff = new char[s];
	fread(buff, 1, s, f);
	m_cpu.LoadData(0x100, buff, s);
	delete buff;
	fclose(f);
	return EXIT_SUCCESS;
}

Application::~Application()
{
}

void Application::Test()
{
	while (true)
	{
		m_cpu.Step();
		getchar();
	}
}
