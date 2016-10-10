#include "Application.h"

int main(int argc, char **argv)
{
	Application app;
	if (app.Init() != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	app.Test();
	
	return 0;
}
