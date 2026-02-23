#include "Server.h"

#include <iostream>

enum class DisplayLog : uint8_t
{
	Info,
	Warning,
	Error,
};

static const std::unordered_map<DisplayLog, const std::string> display = {
	{DisplayLog::Info, "<<INFO>> "},
	{DisplayLog::Warning, "<<WARNING>> "},
	{DisplayLog::Error, "<<ERROR>> "},
};

static __forceinline std::string cutFirstWordStr(const std::string& string)
{
	std::string::size_type index = string.find_first_of(" \t");
	return (index != std::string::npos) ? string.substr(index + 1) : "";
}

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

	lastGlobalCheck = Clock::now();
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

				auto now = Clock::now();
				ClientInfo client = {
					.socketInfo = socketInfo,
					.ip = inet_ntoa(socketInfo.sin_addr),
					.port = clientPort,
					.lastCheck = now,
				};

				std::string requestBuffer = buffer;
				processRequest(client, requestBuffer, clientPort);
			}
		}

		sendConnectionChecks();
		checkConnections();
	}
}

void Server::processRequest(const ClientInfo& client, const std::string& request_buffer, uint32_t client_port)
{
	Server::RequestType type = getRequestTypeFromBuffer(request_buffer);
	std::string portAndMessage = "";
	std::string port = "";

	switch (type)
	{
		case RequestType::Connect:
			if (!clientInfos.contains(client_port))
			{
				std::cout << "Client with address " << client.getAddress() << " connected." << std::endl;
			}
			clientInfos[client_port] = client;
			break;
		case RequestType::Broadcast:
			broadcast(cutFirstWordStr(request_buffer), client_port);
			break;
		case RequestType::DirectMessage:
			portAndMessage = cutFirstWordStr(request_buffer);
			port = portAndMessage.substr(0, portAndMessage.find_first_of(" \t"));
			directMessage(cutFirstWordStr(portAndMessage), port, client_port);
			break;
		case RequestType::ConnectionCheck:
			clientInfos[client_port].lastCheck = Clock::now();
			break;
		case RequestType::Disconnect:
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

void Server::sendConnectionChecks()
{
	const auto now = Clock::now();
	if (now - lastGlobalCheck < timeBetweenChecks)
	{
		return;
	}

	for (const auto& [port, client] : clientInfos)
	{
		sendMessage(ConnectionCheck::checkMsg, client.socketInfo);
	}

	lastGlobalCheck = now;
}

void Server::checkConnections()
{
	const auto now = Clock::now();
	std::vector<uint32_t> portsTodisconnect;

	for (const auto& [port, client] : clientInfos)
	{
		if (now - client.lastCheck > timeBeforeDisconnect)
		{
			std::cout << client.getAddress() << " has disconnected (timeout)." << std::endl;
			portsTodisconnect.emplace_back(port);
		}
	}

	for (const uint32_t port : portsTodisconnect)
	{
		clientInfos.erase(port);
	}
}

void Server::broadcast(const std::string& message, uint32_t client_self_port)
{
	for (const auto& [port, client] : clientInfos)
	{
		if (port != client_self_port)
		{
			sendMessage(message, client.socketInfo);
		}
	}
}

void Server::directMessage(const std::string& message, const std::string& receiver_port, uint32_t client_self_port)
{
	uint32_t port = 0;
	try
	{
		port = std::stoul(receiver_port);
	}
	catch (...)
	{
		std::cout << display.at(DisplayLog::Error) << "Unvalid port in direct message." << std::endl;
		return;
	}

	if (port == 0 || port > 65536)
	{
		std::cout << display.at(DisplayLog::Error) << "Port is out of possible range." << std::endl;
		return;
	}

	if (std::to_string(port).size() != receiver_port.size())
	{
		std::cout << display.at(DisplayLog::Error) << "Port contains incorrect characters, only digits are possible."
				  << std::endl;
		return;
	}

	if (port == client_self_port)
	{
		std::cout << display.at(DisplayLog::Warning)
				  << "Sending message to self is prohibited, so no action will be taken." << std::endl;
		return;
	}

	if (!clientInfos.contains(port))
	{
		static const std::string errorString = display.at(DisplayLog::Error) + "No user is connected to this port.";
		sendMessage(errorString, clientInfos[client_self_port].socketInfo);
		return;
	}

	sendMessage(message, clientInfos[port].socketInfo);
}

void Server::sendMessage(const std::string& message, const sockaddr_in& address_info)
{
	sendto((SOCKET)fd, message.c_str(), static_cast<int>(message.size()), 0, (sockaddr*)&address_info,
		sizeof(address_info));
}