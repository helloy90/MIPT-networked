#include <cstring>
#include <iostream>
#include <ws2tcpip.h>

#include "Server.h"

int main(int argc, const char** argv)
{
	Server server = Server();

	if (server.isValid())
	{
		server.run();
	}

	return 0;
}
