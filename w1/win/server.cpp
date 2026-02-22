#include "Server.h"

#include <iostream>
#include <sstream>


Server::Server()
	: fd(-1)
	, valid(false)
{
	wsa = std::make_unique<WSA>();
	if (!wsa->is_initialized())
	{
		std::cout << display.at(DisplayLog::Error) << "Failed to initialize WSA\n";
		return;
	}

	const char* PORT = "2026";

	fd = create_server(PORT);
	if (fd == -1)
	{
		std::cout << "Failed to create a socket\n";
		return;
	}

	std::cout << "ChatServer - Listening!\n";

	valid = true;
}

void Server::run()
{
	fd_set readSet;
	FD_ZERO(&readSet);
	timeval timeout = {0, 100000}; // 100 ms
	while (true)
	{
		FD_SET(fd, &readSet);
		select((int)fd + 1, &readSet, NULL, NULL, &timeout);

		if (FD_ISSET((SOCKET)fd, &readSet))
		{
			constexpr size_t bufferSize = 1000;
			static char buffer[bufferSize];
			memset(buffer, 0, bufferSize);

			sockaddr_in socketInfo;
			int socketLen = sizeof(sockaddr_in);
			int num_bytes = recvfrom((SOCKET)fd, buffer, bufferSize - 1, 0, (sockaddr*)&socketInfo, &socketLen);

			if (num_bytes > 0)
			{
				uint16_t clientPort = ntohs(socketInfo.sin_port);
				ClientInfo client = {
					.socketInfo = socketInfo, .ip = inet_ntoa(socketInfo.sin_addr), .port = clientPort};

				if (!clientInfos.contains(clientPort))
				{
					std::cout << "Client with address " << client.getAddress() << " connected." << std::endl;
				}
				clientInfos[clientPort] = client;


				std::string requestBuffer = buffer;
				processRequest(clientPort, requestBuffer);
			}
		}
	}
}

void Server::processRequest(uint32_t client_port, std::string& request_buffer)
{
	Server::RequestType type = getRequestTypeFromBuffer(request_buffer);
	// std::istringstream stream(request_buffer);
	std::string _notNeeded = "";
	std::string port = "";

	switch (type)
	{
		case RequestType::Broadcast:
			// stream >> _notNeeded;
			broadcast(request_buffer.substr(request_buffer.find_first_of(" \t") + 1));
			break;
		case RequestType::DirectMessage:
			request_buffer = request_buffer.substr(request_buffer.find_first_of(" \t") + 1);
			port = request_buffer.substr(0, request_buffer.find_first_of(" \t") + 1);
			directMessage(request_buffer.substr(request_buffer.find_first_of(" \t") + 1), port);
			break;
		case RequestType::Quitting:
			std::cout << clientInfos[client_port].getAddress() << " has disconnected." << std::endl;
			clientInfos.erase(client_port);
			break;
		case RequestType::None:
		default:
			std::cout << "<" << clientInfos[client_port].getAddress() << "> " << request_buffer << std::endl;
			break;
	}
}

Server::RequestType Server::getRequestTypeFromBuffer(const std::string& request_buffer)
{
	for (const auto& [prefix, type] : Server::requestPrefixes)
	{
		if (request_buffer.starts_with(prefix))
		{
			return type;
		}
	}
	if (request_buffer.starts_with('/'))
	{
		std::cout << display.at(DisplayLog::Warning) << "Unknown command encountered. Treaing it as regular message."
				  << std::endl;
	}
	return RequestType::None;
}

std::string Server::getMessageWithoutRequest(const std::string& request_buffer, RequestType request_type)
{
	std::istringstream stream(request_buffer);
	std::string notNeeded = "";

	switch (request_type)
	{
		case RequestType::Broadcast:
			stream >> notNeeded;
			return stream.str();
		case RequestType::DirectMessage:
			stream >> notNeeded;
			stream >> notNeeded;
			return stream.str();
		case RequestType::Quitting:
			stream >> notNeeded;
			if (!stream.str().empty())
			{
				std::cout << display.at(DisplayLog::Warning) << "Skipping characters after /quit command." << std::endl;
			}
			return "";
		default:
			return request_buffer;
	}
}

void Server::broadcast(const std::string& message)
{
	for (const auto& [port, client] : clientInfos)
	{
		sendMessage(message, client.socketInfo);
	}
}

void Server::directMessage(const std::string& message, const std::string& receiver_port)
{
	int32_t port = -1;
	try
	{
		port = std::stoi(receiver_port);
	}
	catch (...)
	{
		std::cout << display.at(DisplayLog::Error) << "Unvalid port in direct message." << std::endl;
		return;
	}

	if (port < 0 || port > 65536)
	{
		std::cout << display.at(DisplayLog::Error) << "Unvalid port in direct message." << std::endl;
		return;
	}

	sendMessage(message, clientInfos[port].socketInfo);
}

void Server::sendMessage(const std::string& message, const sockaddr_in& address_info)
{
	sendto(
		(SOCKET)fd,
		message.c_str(),
		static_cast<int>(message.size()),
		0,
		(sockaddr*)&address_info,
		sizeof(address_info));
}