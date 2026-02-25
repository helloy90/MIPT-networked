#include "Server.h"

#include <iostream>

#include "ConnectionCheckMsg.h"
#include "DisplayLog.h"

static const std::unordered_map<std::string, Server::RequestType> simpleRequests = {
	{"/___autoconnect", Server::RequestType::Connect},
	{"/duel", Server::RequestType::DuelStart},
	{ConnectionCheck::checkAnswerMsg, Server::RequestType::ConnectionCheck},
	{"/quit", Server::RequestType::Disconnect},
};

static const std::unordered_map<std::string, Server::RequestType> compositeRequests = {
	{"/all ", Server::RequestType::Broadcast},
	{"/w ", Server::RequestType::DirectMessage},
	{"/answer ", Server::RequestType::DuelAnswer},
};

static const Server::TimeDuration timeBeforeDisconnect = Server::TimeDuration(5);
static const Server::TimeDuration timeBetweenChecks = Server::TimeDuration(1);
static Server::TimePoint lastGlobalCheck;

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
		std::cout << Log::msg(Log::Type::Error) << "Failed to initialize WSA\n";
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
				uint32_t clientPort = ntohs(socketInfo.sin_port);

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
	std::string truncatedMessage = "";
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
			truncatedMessage = cutFirstWordStr(request_buffer);
			port = truncatedMessage.substr(0, truncatedMessage.find_first_of(" \t"));
			directMessage(cutFirstWordStr(truncatedMessage), port, client_port);
			break;
		case RequestType::DuelStart:
			processDuelStart(client_port);
			break;
		case RequestType::DuelAnswer:
			truncatedMessage = cutFirstWordStr(request_buffer);
			processDuelAnswer(client_port, truncatedMessage.substr(0, truncatedMessage.find_first_of(" \t")));
			break;
		case RequestType::ConnectionCheck:
			clientInfos[client_port].lastCheck = Clock::now();
			break;
		case RequestType::Disconnect:
			std::cout << clientInfos[client_port].getAddress() << " has disconnected." << std::endl;
			duelsExtention.terminateDuel(client_port, false);
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
	for (const auto& [prefix, type] : simpleRequests)
	{
		if (request_buffer == prefix)
		{
			return type;
		}
	}

	for (const auto& [prefix, type] : compositeRequests)
	{
		if (request_buffer.starts_with(prefix))
		{
			return type;
		}
	}
	if (request_buffer.starts_with('/'))
	{
		std::cout << Log::msg(Log::Type::Warning) << "Unknown command encountered. Treaing it as regular message."
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
		duelsExtention.terminateDuel(port, false);
		clientInfos.erase(port);
	}
}

void Server::broadcast(const std::string& message, uint32_t exclude_port)
{
	for (const auto& [port, client] : clientInfos)
	{
		if (port != exclude_port)
		{
			sendMessage(message, client.socketInfo);
		}
	}
}

void Server::directMessage(const std::string& message, const std::string& receiver_port, uint32_t exclude_port)
{
	uint32_t port = 0;
	try
	{
		port = std::stoul(receiver_port);
	}
	catch (...)
	{
		std::cout << Log::msg(Log::Type::Error) << "Unvalid port in direct message." << std::endl;
		return;
	}

	if (port == 0 || port > 65536)
	{
		std::cout << Log::msg(Log::Type::Error) << "Port is out of possible range." << std::endl;
		return;
	}

	if (std::to_string(port).size() != receiver_port.size())
	{
		std::cout << Log::msg(Log::Type::Error) << "Port contains incorrect characters, only digits are possible."
				  << std::endl;
		return;
	}

	if (port == exclude_port)
	{
		std::cout << Log::msg(Log::Type::Warning)
				  << "Sending message to self is prohibited, so no action will be taken." << std::endl;
		return;
	}

	if (!clientInfos.contains(port))
	{
		static const std::string errorString = Log::msg(Log::Type::Error) + "No user is connected to this port.";
		sendMessage(errorString, clientInfos[exclude_port].socketInfo);
		return;
	}

	sendMessage(message, clientInfos[port].socketInfo);
}

void Server::sendMessage(const std::string& message, const sockaddr_in& address_info)
{
	sendto((SOCKET)fd, message.c_str(), static_cast<int>(message.size()), 0, (sockaddr*)&address_info,
		sizeof(address_info));
}

void Server::processDuelStart(uint32_t client_port)
{
	auto [equation, firstPlayerPort, secondPlayerPort] = duelsExtention.initiateDuel(client_port);
	if (equation != "")
	{
		sendMessage(equation, clientInfos[firstPlayerPort].socketInfo);
		sendMessage(equation, clientInfos[secondPlayerPort].socketInfo);
	}
}

void Server::processDuelAnswer(uint32_t client_port, const std::string& client_answer)
{
	int32_t answer = 0;
	try
	{
		answer = std::stoi(client_answer);
	}
	catch (...)
	{
		std::cout << Log::msg(Log::Type::Error) << "Unvalid answer message in duel." << std::endl;
		return;
	}

	if (std::to_string(answer).size() != client_answer.size())
	{
		std::cout << Log::msg(Log::Type::Error)
				  << "Answer message contains incorrect characters, only digits are possible." << std::endl;
		return;
	}

	if (duelsExtention.isAnswerCorrect(client_port, answer))
	{
		static const std::string winner = " is the winner!";
		broadcast(std::to_string(client_port) + winner, 0);
	}
}
