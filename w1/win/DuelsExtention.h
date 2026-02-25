#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "NumberGenerator.h"


class DuelsExtention
{
public:
	struct DuelInfo
	{
		uint32_t firstPlayerPort = 0;
		uint32_t secondPlayerPort = 0;
		std::string equation = "";
		int32_t answer = 0;

		bool hasPlayer(uint32_t port) { return port == firstPlayerPort || port == secondPlayerPort; }
	};

public:
	DuelsExtention();

	DuelsExtention(const DuelsExtention&) = delete;
	DuelsExtention& operator=(const DuelsExtention&) = delete;

	std::tuple<std::string, uint32_t, uint32_t> initiateDuel(uint32_t client_port);
	bool isAnswerCorrect(uint32_t client_port, int32_t client_answer);

	void terminateDuel(uint32_t client_port, bool after_win);

private:
	std::pair<std::string, int> generateEquation();

private:
	NumberGenerator generator;

	std::unordered_map<uint32_t, DuelInfo> duels;

	DuelInfo awaitingDuel;
};