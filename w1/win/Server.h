#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>

#include "DuelsExtention.h"
#include "socket_tools.h"


class Server
{
public:
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using TimeDuration = std::chrono::seconds;

	struct ClientInfo
	{
		sockaddr_in socketInfo;
		std::string ip;
		uint32_t port;
		TimePoint lastCheck;

		std::string getAddress() const { return ip + ":" + std::to_string(port); }
	};

	enum class RequestType : uint8_t
	{
		None,
		Connect,
		Broadcast,
		DirectMessage,
		DuelStart,
		DuelAnswer,
		ConnectionCheck,
		Disconnect,
	};

public:
	Server();
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	bool isValid() const { return valid; }

	void run();

private:
	void processRequest(const ClientInfo& client, const std::string& request_buffer, uint32_t client_port);
	RequestType getRequestTypeFromBuffer(const std::string& request_buffer);

	void sendConnectionChecks();
	void checkConnections();

	void broadcast(const std::string& message, uint32_t exclude_port); // exclude_port == 0 -> send to everyone
	void directMessage(const std::string& message, const std::string& receiver_port, uint32_t exclude_port);
	void sendMessage(const std::string& message, const sockaddr_in& address_info);

	void processDuelStart(uint32_t client_port);
	void processDuelAnswer(uint32_t client_port, const std::string& client_answer);

private:
	int fd;
	std::unique_ptr<WSA> wsa = nullptr;

	std::unordered_map<uint32_t, ClientInfo> clientInfos;
	bool valid;

	DuelsExtention duelsExtention;
};