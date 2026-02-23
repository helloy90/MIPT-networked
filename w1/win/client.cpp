#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ConnectionCheckMsg.h"
#include "socket_tools.h"

const char* PORT = "2026";

std::atomic<bool> running{true};

std::string buffered_msg;

addrinfo addr_info;
int sfd;

void receive_messages()
{
	fd_set readSet;
	FD_ZERO(&readSet);
	timeval timeout = {0, 100000}; // 100 ms
	while (running)
	{
		constexpr size_t bufferSize = 1000;
		static char buffer[bufferSize];

		FD_SET(sfd, &readSet);
		select((int)sfd + 1, &readSet, NULL, NULL, &timeout);

		if (FD_ISSET((SOCKET)sfd, &readSet))
		{
			memset(buffer, 0, bufferSize);
			sockaddr_in socketInfo;
			int socketLen = sizeof(sockaddr_in);
			int num_bytes = recvfrom((SOCKET)sfd, buffer, bufferSize - 1, 0, (sockaddr*)&socketInfo, &socketLen);

			if (std::string(buffer) == ConnectionCheck::checkMsg)
			{
				sendto((SOCKET)sfd, ConnectionCheck::checkAnswerMsg.c_str(),
					static_cast<int>(ConnectionCheck::checkAnswerMsg.size()), 0, addr_info.ai_addr,
					addr_info.ai_addrlen);
				continue;
			}

			std::cout << "From server: " << buffer << "\n";
			std::cout << "> " << buffered_msg << std::flush;
		}
	}
}

void handle_input()
{
	char ch = std::cin.get();
	bool is_printable = ch >= 32 && ch <= 126;
	bool is_backspace = ch == 127 || ch == '\b';
	bool is_enter = ch == '\n';

	if (is_printable)
	{
		buffered_msg += ch;
		std::cout << ch << std::flush;
	}
	else if (is_backspace)
	{
		if (!buffered_msg.empty())
		{
			buffered_msg.pop_back();
			std::cout << "\b \b" << std::flush;
		}
	}
	else if (is_enter)
	{
		if (buffered_msg == "/quit")
		{
			sendto((SOCKET)sfd, buffered_msg.c_str(), buffered_msg.size(), 0, addr_info.ai_addr, addr_info.ai_addrlen);
			running = false;
			std::cout << "\nExiting...\n";
		}
		else if (!buffered_msg.empty())
		{
			std::cout << "\rYou sent: " << buffered_msg << "\n";
			int res =
				sendto(sfd, buffered_msg.c_str(), buffered_msg.size(), 0, addr_info.ai_addr, addr_info.ai_addrlen);
			if (res == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				std::cout << "Error: " << err << std::endl;
			}

			buffered_msg.clear();
			std::cout << "> " << std::flush;
		}
	}
}

int main(int argc, const char** argv)
{
	std::unique_ptr<WSA> wsa = std::make_unique<WSA>();
	if (!wsa->is_initialized())
	{
		std::cout << "Failed to initialize WSA\n";
		return 1;
	}

	sfd = create_client("localhost", PORT, &addr_info);
	if (sfd == -1)
	{
		std::cout << "Failed to create a socket\n";
		return 1;
	}

	const std::string autoconnect = "/___autoconnect";
	sendto((SOCKET)sfd, autoconnect.c_str(), static_cast<int>(autoconnect.size()), 0, addr_info.ai_addr,
		addr_info.ai_addrlen);

	std::cout << "ChatClient - Type '/quit' to exit\n"
			  << "> ";

	std::thread receiver(receive_messages);
	while (running)
	{
		handle_input();
	}

	receiver.join();
	return 0;
}
