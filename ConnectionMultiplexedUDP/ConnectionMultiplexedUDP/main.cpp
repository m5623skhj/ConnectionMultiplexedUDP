#include <iostream>
#include "ServerCore.h"
#include <Windows.h>

int main()
{
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