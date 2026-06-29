#include <iostream>
#include "ServerCore.h"
#include "SmokeTest.h"
#include <string_view>
#include <Windows.h>

int main(int argc, char* argv[])
{
	if (argc > 1 && std::string_view(argv[1]) == "--smoke-test")
	{
		return RunSmokeTest();
	}

	ServerCore serverCore(4, 4, 7777, 1000);
	if (not serverCore.Start())
	{
		std::cout << "Failed to start the server." << std::endl;
		return -1;
	}

	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			std::cout << "ESC key pressed. Exiting..." << std::endl;
			break;
		}

		Sleep(1000);
	}
	serverCore.Stop();

	return 0;
}
