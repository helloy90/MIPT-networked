#include "DuelsExtention.h"

#include <iostream>

#include "DisplayLog.h"


DuelsExtention::DuelsExtention()
	: generator(0, 100)
{
}

std::tuple<std::string, uint32_t, uint32_t> DuelsExtention::initiateDuel(uint32_t client_port)
{
	if (awaitingDuel.hasPlayer(client_port))
	{
		std::cout << Log::msg(Log::Type::Info) << "Player with port " << client_port << "is already awaiting duel."
				  << std::endl;
		return {"", 0, 0};
	}

	if (awaitingDuel.firstPlayerPort != 0)
	{
		awaitingDuel.secondPlayerPort = client_port;
		duels[awaitingDuel.firstPlayerPort] = awaitingDuel;
		duels[awaitingDuel.secondPlayerPort] = awaitingDuel;
		std::tuple<std::string, uint32_t, uint32_t> res = {
			awaitingDuel.equation, awaitingDuel.firstPlayerPort, awaitingDuel.secondPlayerPort};

		awaitingDuel = {
			.firstPlayerPort = 0,
			.secondPlayerPort = 0,
			.equation = "",
			.answer = 0,
		};

		return res;
	}

	auto [equation, answer] = generateEquation();

	awaitingDuel = DuelInfo{
		.firstPlayerPort = client_port,
		.secondPlayerPort = 0,
		.equation = equation,
		.answer = answer,
	};

	return {"", 0, 0};
}

bool DuelsExtention::isAnswerCorrect(uint32_t client_port, int32_t client_answer)
{
	if (!duels.contains(client_port))
	{
		return false;
	}

	if (duels[client_port].answer == client_answer)
	{
		terminateDuel(client_port, true);
		return true;
	}

	return false;
}

void DuelsExtention::terminateDuel(uint32_t client_port, bool after_win)
{
	if (!duels.contains(client_port))
	{
		return;
	}

	DuelInfo currentDuel = duels[client_port];

	if (!after_win)
	{
		std::cout << Log::msg(Log::Type::Info) << "Duel between " << currentDuel.firstPlayerPort << " and "
				  << currentDuel.secondPlayerPort << " is cancelled." << std::endl;
	}

	duels.erase(currentDuel.firstPlayerPort);
	duels.erase(currentDuel.secondPlayerPort);
}

std::pair<std::string, int32_t> DuelsExtention::generateEquation()
{
	static const std::vector<std::string> signs = {"+", "-"};

	int32_t signsAmount = generator.get() % 3 + 1; // [1; 3] range

	int32_t answer = generator.get(); // init-ed by first number
	std::string equation = std::to_string(answer);

	// doing simple equations as in example
	int32_t mul = generator.get();
	equation += " * " + std::to_string(mul);
	answer *= mul;

	for (int32_t i = 0; i < signsAmount; i++)
	{
		int32_t number = generator.get();
		int32_t signIndex = generator.get() % 2;
		equation += " " + signs[signIndex] + " " + std::to_string(number);
		switch (signIndex)
		{
			case 0:
				answer += number;
				break;
			case 1:
				answer -= number;
				break;
			default:
				std::cout << Log::msg(Log::Type::Error)
						  << "Something went wrong when constructing equation. (sign index out of range)" << std::endl;
				break;
		}
	}

	equation += " = ?";

	std::cout << Log::msg(Log::Type::Info) << "generated equation " << equation << " with answer " << answer
			  << std::endl;

	return std::make_pair(equation, answer);
}