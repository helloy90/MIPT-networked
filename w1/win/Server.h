#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>

#include "ConnectionCheckMsg.h"
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

	void broadcast(const std::string& message, uint32_t client_self_port);
	void directMessage(const std::string& message, const std::string& receiver_port, uint32_t client_self_port);
	void sendMessage(const std::string& message, const sockaddr_in& address_info);

private:
	int fd;
	std::unique_ptr<WSA> wsa = nullptr;

	std::unordered_map<uint32_t, ClientInfo> clientInfos;
	bool valid;

	static inline const std::unordered_map<std::string, RequestType> requestPrefixes = {
		{"/___autoconnect", RequestType::Connect},
		{"/all ", RequestType::Broadcast},
		{"/w ", RequestType::DirectMessage},
		{ConnectionCheck::checkAnswerMsg, RequestType::ConnectionCheck},
		{"/quit", RequestType::Disconnect},
	};

	static inline const TimeDuration timeBeforeDisconnect = TimeDuration(5);
	static inline const TimeDuration timeBetweenChecks = TimeDuration(1);
	static inline TimePoint lastGlobalCheck;
};