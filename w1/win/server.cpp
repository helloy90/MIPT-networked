#include <cstring>
#include <iostream>
#include <memory>
#include <ws2tcpip.h>

#include "socket_tools.h"


const char* PORT = "2026";

int main(int argc, const char** argv)
{
	std::unique_ptr<WSA> wsa = std::make_unique<WSA>();
	if (!wsa->is_initialized())
	{
		std::cout << "Failed to initialize WSA\n";
		return 1;
	}

	int sfd = create_server(PORT);
	if (sfd == -1)
	{
		std::cout << "Failed to create a socket\n";
		return 1;
	}

	std::cout << "ChatServer - Listening!\n";

	fd_set read_set;
	FD_ZERO(&read_set);
	timeval timeout = {0, 100000}; // 100 ms
	while (true)
	{
		FD_SET(sfd, &read_set);
		select((int)sfd + 1, &read_set, NULL, NULL, &timeout);

		if (FD_ISSET((SOCKET)sfd, &read_set))
		{
			constexpr size_t buf_size = 1000;
			static char buffer[buf_size];
			memset(buffer, 0, buf_size);

			sockaddr_in socket_in;
			int socket_len = sizeof(sockaddr_in);
			int num_bytes = recvfrom((SOCKET)sfd, buffer, buf_size - 1, 0, (sockaddr*)&socket_in, &socket_len);
			if (num_bytes > 0)
			{
				std::cout << "(" << inet_ntoa(socket_in.sin_addr) << ":" << socket_in.sin_port << "): " << buffer
						  << std::endl;
			}
		}
	}
	return 0;
}
