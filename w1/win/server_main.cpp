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
