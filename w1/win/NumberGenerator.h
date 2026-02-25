#pragma once

#include <random>


class NumberGenerator
{
public:
	NumberGenerator() = delete;
	NumberGenerator(int left, int right)
		: generator(std::random_device{}())
		, distribution(left, right)
	{
	}

	NumberGenerator(const NumberGenerator&) = delete;
	NumberGenerator& operator=(const NumberGenerator&) = delete;

	int get() { return distribution(generator); };

private:
	std::mt19937 generator;
	std::uniform_int_distribution<int> distribution;
};
