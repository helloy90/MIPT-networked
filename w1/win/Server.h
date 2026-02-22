#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>

#include "socket_tools.h"



class Server
{
public:
	struct ClientInfo
	{
		sockaddr_in socketInfo;
		std::string ip;
		uint32_t port;

		std::string getAddress() const { return ip + ":" + std::to_string(port); }
	};

	enum class RequestType : uint8_t
	{
		None,
		Broadcast,
		DirectMessage,
		Quitting,
	};

public:
	Server();
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	bool isValid() const { return valid; }

	void run();

private:
	void processRequest(uint32_t client_port, std::string& request_buffer);
	RequestType getRequestTypeFromBuffer(const std::string& request_buffer);
	std::string getMessageWithoutRequest(const std::string& request_buffer, RequestType request_type);

	void broadcast(const std::string& message);
	void directMessage(const std::string& message, const std::string& receiver_port);
	void sendMessage(const std::string& message, const sockaddr_in& address_info);

private:
	int fd;
	std::unique_ptr<WSA> wsa = nullptr;

	std::unordered_map<uint32_t, ClientInfo> clientInfos;
	bool valid;

	static inline const std::unordered_map<std::string, RequestType> requestPrefixes = {
		{"/all", RequestType::Broadcast},
		{"/w", RequestType::DirectMessage},
		{"/quit", RequestType::Quitting},
	};

	enum class DisplayLog : uint8_t
	{
		Info,
		Warning,
		Error,
	};

	static inline const std::unordered_map<DisplayLog, const std::string> display = {
		{DisplayLog::Info, "[[INFO]]\n"}, {DisplayLog::Warning, "[[WARNING]]\n"}, {DisplayLog::Error, "[[ERROR]]\n"}};
};